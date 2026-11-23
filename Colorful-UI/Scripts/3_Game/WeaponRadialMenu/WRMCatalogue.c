// Server-side catalogue, built once from CfgWeapons / CfgVehicles / CfgMagazines in small chunks per frame.
//  - weapons:      CfgWeapons, scope 2, inherits Weapon_Base, not *_Base / *dummy*, not Archery_Base (non-firearm)
//  - attachments:  CfgVehicles, scope 2, inventorySlot (string or array) -> index slot -> classes
//  - magazines:    CfgMagazines, resolved on demand from the magazines[] of a weapon (scope 2 only)
// One cell of the wheel: a class and its colour variants ("AK101" + "AK101_Black" + "AK101_Green").
// m_Rep is the class shown in the cell, m_Variants holds every class of the group (rep first).
class WRMGroup
{
	string m_Rep;
	ref TStringArray m_Variants = new TStringArray;

	int Count() { return m_Variants.Count(); }

	bool Contains(string cls)
	{
		string lower = cls;
		lower.ToLower();
		for (int i = 0; i < m_Variants.Count(); i++)
		{
			string have = m_Variants.Get(i);
			have.ToLower();
			if (have == lower)
				return true;
		}
		return false;
	}
}

class WRMCatalogue
{
	static const int STATE_IDLE = 0;
	static const int STATE_WEAPONS = 1;
	static const int STATE_VEHICLES = 2;
	static const int STATE_GROUPS = 3;
	static const int STATE_READY = 4;
	static const int CHUNK = 250;			// classes inspected per Step()

	// vanilla classes that are unfinished in 1.30: no weapon FSM, broken muzzle states, or no Weapon_Base script.
	// They throw script exceptions when held. JSON weaponInclude brings one back.
	static const string BROKEN_VANILLA = ",akm_testbed,dartgun,law,m249,rpg7,shockpistol,shockpistol_black,groza,gp25,gp25_standalone,m203,m203_standalone,trumpet,undersluggrenadem4,";

	static int s_State;
	static int s_Index;
	static int s_Count;
	static float s_StartTime;

	static ref map<string, ref TStringArray> s_CatWeapons;	// category id (lowercase) -> canonical class names
	static ref map<string, string> s_WeaponNames;			// lowercase class -> canonical class
	static ref map<string, ref TStringArray> s_SlotItems;	// slot (lowercase) -> canonical class names
	static ref map<string, string> s_SlotItemSet;			// "slot|class" (lowercase) -> canonical class
	static ref map<string, ref TStringArray> s_MagCache;	// "weapon|muzzle" (lowercase) -> canonical magazine names

	// the same lists grouped into one cell per item with its colour variants
	static ref map<string, ref array<ref WRMGroup>> s_CatGroups;	// category id (lowercase) -> groups
	static ref map<string, ref array<ref WRMGroup>> s_SlotGroups;	// slot (lowercase) -> groups
	static ref map<string, ref array<ref WRMGroup>> s_MagGroups;	// "weapon|muzzle" (lowercase) -> groups

	// grouping work state (chunked like the scan)
	static ref map<string, string> s_Models;		// "root|class" (lowercase) -> normalised model path, "#class" when it has none
	static ref TStringArray s_GKeys;				// lists to group: "w|category" and "i|slot"
	static int s_GPos;
	static ref TStringArray s_GCls;					// classes of the list being grouped, null between lists
	static string s_GKey;
	static string s_GRoot;
	static int s_GClsPos;
	static ref map<string, ref TStringArray> s_GBuckets;	// model -> classes
	static ref TStringArray s_GBucketKeys;
	static int s_GBucketPos;
	static ref array<ref WRMGroup> s_GOut;

	static bool IsReady() { return s_State == STATE_READY; }
	static bool IsStarted() { return s_State != STATE_IDLE; }

	static void Begin()
	{
		if (s_State != STATE_IDLE)
			return;

		WRMConfig.Get();
		s_CatWeapons = new map<string, ref TStringArray>;
		s_WeaponNames = new map<string, string>;
		s_SlotItems = new map<string, ref TStringArray>;
		s_SlotItemSet = new map<string, string>;
		s_MagCache = new map<string, ref TStringArray>;
		s_CatGroups = new map<string, ref array<ref WRMGroup>>;
		s_SlotGroups = new map<string, ref array<ref WRMGroup>>;
		s_MagGroups = new map<string, ref array<ref WRMGroup>>;
		s_Models = new map<string, string>;
		s_StartTime = g_Game.GetTickTime();
		s_Index = 0;
		s_Count = g_Game.ConfigGetChildrenCount("CfgWeapons");
		s_State = STATE_WEAPONS;
	}

	// Call every frame until IsReady().
	static void Step()
	{
		int budget = CHUNK;
		if (s_State == STATE_WEAPONS)
		{
			while (budget > 0 && s_Index < s_Count)
			{
				ScanWeapon(s_Index);
				s_Index++;
				budget--;
			}
			if (s_Index >= s_Count)
			{
				s_Index = 0;
				s_Count = g_Game.ConfigGetChildrenCount("CfgVehicles");
				s_State = STATE_VEHICLES;
			}
		}
		else if (s_State == STATE_VEHICLES)
		{
			while (budget > 0 && s_Index < s_Count)
			{
				ScanVehicle(s_Index);
				s_Index++;
				budget--;
			}
			if (s_Index >= s_Count)
				BeginGroups();
		}
		else if (s_State == STATE_GROUPS)
		{
			StepGroups();
		}
	}

	protected static bool IsBaseOrDummyName(string lowerName)
	{
		int len = lowerName.Length();
		if (len > 5 && lowerName.Substring(len - 5, 5) == "_base")
			return true;
		return lowerName.Contains("dummy");
	}

	static void BuildChain(string root, string name, TStringArray chain)
	{
		string lowerName = name;
		lowerName.ToLower();
		chain.Insert(lowerName);

		string current = root + " " + name;
		string parent;
		int guard = 0;
		while (guard < 24 && g_Game.ConfigGetBaseName(current, parent) && parent != "")
		{
			string lowerParent = parent;
			lowerParent.ToLower();
			chain.Insert(lowerParent);
			current = root + " " + parent;
			guard++;
		}
	}

	protected static void ScanWeapon(int index)
	{
		string name;
		if (!g_Game.ConfigGetChildName("CfgWeapons", index, name) || name == "")
			return;

		string lowerName = name;
		lowerName.ToLower();
		if (WRMConfig.IsWeaponExcluded(lowerName))
			return;

		bool forced = WRMConfig.IsWeaponIncluded(lowerName);
		if (!forced && IsBaseOrDummyName(lowerName))
			return;
		string broken = BROKEN_VANILLA;
		if (!forced && broken.Contains("," + lowerName + ","))
			return;

		string path = "CfgWeapons " + name;
		if (g_Game.ConfigGetInt(path + " scope") != 2)
			return;

		TStringArray chain = new TStringArray;
		BuildChain("CfgWeapons", name, chain);
		if (!forced)
		{
			if (chain.Find("weapon_base") == -1 || chain.Find("archery_base") != -1)
				return;
		}

		string cat = WRMConfig.GetOverride(lowerName);
		if (cat == "")
			cat = WRMClassifier.Classify(path, chain);

		TStringArray list;
		if (!s_CatWeapons.Find(cat, list))
		{
			list = new TStringArray;
			s_CatWeapons.Insert(cat, list);
		}
		list.Insert(name);
		s_WeaponNames.Set(lowerName, name);
	}

	protected static void ScanVehicle(int index)
	{
		string name;
		if (!g_Game.ConfigGetChildName("CfgVehicles", index, name) || name == "")
			return;

		string path = "CfgVehicles " + name;
		TStringArray slots = new TStringArray;
		int slotType = g_Game.ConfigGetType(path + " inventorySlot");
		if (slotType == CT_STRING)
		{
			string single;
			g_Game.ConfigGetText(path + " inventorySlot", single);
			if (single != "")
				slots.Insert(single);
		}
		else if (slotType == CT_ARRAY)
		{
			g_Game.ConfigGetTextArray(path + " inventorySlot", slots);
		}
		if (slots.Count() == 0)
			return;

		string lowerName = name;
		lowerName.ToLower();
		if (WRMConfig.IsAttachmentExcluded(lowerName))
			return;
		if (!WRMConfig.IsAttachmentIncluded(lowerName) && IsBaseOrDummyName(lowerName))
			return;
		if (g_Game.ConfigGetInt(path + " scope") != 2)
			return;

		for (int i = 0; i < slots.Count(); i++)
		{
			string slotLower = slots.Get(i);
			slotLower.ToLower();
			if (slotLower == "")
				continue;

			string setKey = slotLower + "|" + lowerName;
			if (s_SlotItemSet.Contains(setKey))
				continue;
			s_SlotItemSet.Insert(setKey, name);

			TStringArray items;
			if (!s_SlotItems.Find(slotLower, items))
			{
				items = new TStringArray;
				s_SlotItems.Insert(slotLower, items);
			}
			items.Insert(name);
		}
	}

	// ===== grouping: one cell per item, colour variants folded =====
	// Two classes of one list are the same cell when they have the same normalised `model` path (non-empty, not a
	// dummy) AND one of: the same raw displayName, one inherits the other, or they share the immediate base class.
	// Magazines must also have the same `count`. Relations are merged transitively (union-find inside a model bucket).
	// The face of a cell is the ancestor of all other members, else the shortest name, then the alphabetically first.

	// normalised model path of a class ("root|class" cached); "#class" (a private key) when there is none or a dummy
	protected static string ModelKey(string root, string name)
	{
		string lowerName = name;
		lowerName.ToLower();
		string cacheKey = root + "|" + lowerName;
		string cached;
		if (s_Models.Find(cacheKey, cached))
			return cached;

		string model;
		g_Game.ConfigGetText(root + " " + name + " model", model);
		model.ToLower();
		model.Replace("/", "\\");
		model = model.Trim();

		string result = model;
		if (model == "" || model.Contains("dummy"))
			result = "#" + lowerName;
		s_Models.Insert(cacheKey, result);
		return result;
	}

	// true when 'a' sorts before 'b'
	protected static bool IsLess(string a, string b)
	{
		TStringArray pair = new TStringArray;
		pair.Insert(a);
		pair.Insert(b);
		pair.Sort();
		return pair.Get(0) == a && a != b;
	}

	protected static int FindRoot(TIntArray parent, int index)
	{
		int current = index;
		while (parent.Get(current) != current)
			current = parent.Get(current);
		return current;
	}

	// chains: lowercase inheritance chains, [0] is the class itself, [1] its immediate base
	protected static bool Related(TStringArray chainA, TStringArray chainB, string rawA, string rawB)
	{
		if (rawA != "" && rawA == rawB)
			return true;
		if (chainB.Find(chainA.Get(0)) != -1 || chainA.Find(chainB.Get(0)) != -1)
			return true;
		return chainA.Count() > 1 && chainB.Count() > 1 && chainA.Get(1) == chainB.Get(1);
	}

	// groups the classes of one model bucket and appends the groups to output
	protected static void AddBucket(string root, TStringArray members, bool byCount, array<ref WRMGroup> output)
	{
		int n = members.Count();
		if (n == 1)
		{
			WRMGroup single = new WRMGroup();
			single.m_Rep = members.Get(0);
			single.m_Variants.Insert(members.Get(0));
			output.Insert(single);
			return;
		}

		array<ref TStringArray> chains = new array<ref TStringArray>;
		TStringArray raws = new TStringArray;
		TIntArray counts = new TIntArray;
		TIntArray parent = new TIntArray;
		for (int i = 0; i < n; i++)
		{
			TStringArray chain = new TStringArray;
			BuildChain(root, members.Get(i), chain);
			chains.Insert(chain);

			string raw;
			g_Game.ConfigGetTextRaw(root + " " + members.Get(i) + " displayName", raw);
			raws.Insert(raw);

			int count = 0;
			if (byCount)
				count = g_Game.ConfigGetInt(root + " " + members.Get(i) + " count");
			counts.Insert(count);
			parent.Insert(i);
		}

		for (int a = 0; a < n; a++)
		{
			for (int b = a + 1; b < n; b++)
			{
				if (byCount && counts.Get(a) != counts.Get(b))
					continue;
				if (!Related(chains.Get(a), chains.Get(b), raws.Get(a), raws.Get(b)))
					continue;
				int rootA = FindRoot(parent, a);
				int rootB = FindRoot(parent, b);
				if (rootA != rootB)
					parent.Set(rootB, rootA);
			}
		}

		map<int, ref TIntArray> clusters = new map<int, ref TIntArray>;
		TIntArray clusterKeys = new TIntArray;
		for (int m = 0; m < n; m++)
		{
			int clusterRoot = FindRoot(parent, m);
			TIntArray cluster;
			if (!clusters.Find(clusterRoot, cluster))
			{
				cluster = new TIntArray;
				clusters.Insert(clusterRoot, cluster);
				clusterKeys.Insert(clusterRoot);
			}
			cluster.Insert(m);
		}

		for (int k = 0; k < clusterKeys.Count(); k++)
		{
			TIntArray idxs = clusters.Get(clusterKeys.Get(k));
			int face = -1;

			// the ancestor of every other member
			for (int p = 0; p < idxs.Count() && face < 0; p++)
			{
				bool ancestor = true;
				for (int q = 0; q < idxs.Count(); q++)
				{
					if (q != p && chains.Get(idxs.Get(q)).Find(chains.Get(idxs.Get(p)).Get(0)) == -1)
						ancestor = false;
				}
				if (ancestor)
					face = idxs.Get(p);
			}

			// else the shortest name, then alphabetical
			if (face < 0)
			{
				for (int r = 0; r < idxs.Count(); r++)
				{
					string candidate = members.Get(idxs.Get(r));
					if (face < 0)
					{
						face = idxs.Get(r);
						continue;
					}
					string best = members.Get(face);
					if (candidate.Length() < best.Length() || (candidate.Length() == best.Length() && IsLess(candidate, best)))
						face = idxs.Get(r);
				}
			}

			WRMGroup group = new WRMGroup();
			group.m_Rep = members.Get(face);
			group.m_Variants.Insert(group.m_Rep);
			TStringArray others = new TStringArray;
			for (int s = 0; s < idxs.Count(); s++)
			{
				if (idxs.Get(s) != face)
					others.Insert(members.Get(idxs.Get(s)));
			}
			others.Sort();
			group.m_Variants.InsertAll(others);
			output.Insert(group);
		}
	}

	// groups ordered by the class name of their face
	protected static void SortGroups(array<ref WRMGroup> groups)
	{
		map<string, ref WRMGroup> byRep = new map<string, ref WRMGroup>;
		TStringArray reps = new TStringArray;
		for (int i = 0; i < groups.Count(); i++)
		{
			byRep.Set(groups.Get(i).m_Rep, groups.Get(i));
			reps.Insert(groups.Get(i).m_Rep);
		}
		reps.Sort();
		groups.Clear();
		for (int j = 0; j < reps.Count(); j++)
			groups.Insert(byRep.Get(reps.Get(j)));
	}

	protected static void BeginGroups()
	{
		s_GKeys = new TStringArray;
		for (int i = 0; i < s_CatWeapons.Count(); i++)
		{
			s_CatWeapons.GetElement(i).Sort();
			s_GKeys.Insert("w|" + s_CatWeapons.GetKey(i));
		}
		for (int j = 0; j < s_SlotItems.Count(); j++)
		{
			s_SlotItems.GetElement(j).Sort();
			s_GKeys.Insert("i|" + s_SlotItems.GetKey(j));
		}
		s_GPos = 0;
		s_GCls = null;
		s_State = STATE_GROUPS;
	}

	protected static void OpenGroupList(string key)
	{
		s_GKey = key;
		string listKey = key.Substring(2, key.Length() - 2);
		if (key.Substring(0, 1) == "w")
		{
			s_GRoot = "CfgWeapons";
			s_GCls = s_CatWeapons.Get(listKey);
		}
		else
		{
			s_GRoot = "CfgVehicles";
			s_GCls = s_SlotItems.Get(listKey);
		}
		s_GClsPos = 0;
		s_GBuckets = new map<string, ref TStringArray>;
		s_GBucketKeys = new TStringArray;
		s_GBucketPos = 0;
		s_GOut = new array<ref WRMGroup>;
	}

	protected static void CloseGroupList()
	{
		SortGroups(s_GOut);
		string listKey = s_GKey.Substring(2, s_GKey.Length() - 2);
		if (s_GKey.Substring(0, 1) == "w")
			s_CatGroups.Insert(listKey, s_GOut);
		else
			s_SlotGroups.Insert(listKey, s_GOut);
		s_GCls = null;
		s_GBuckets = null;
		s_GOut = null;
	}

	// one slice of grouping work per call: bucket classes by model, then cluster the buckets
	protected static void StepGroups()
	{
		int budget = CHUNK;
		while (budget > 0)
		{
			if (!s_GCls)
			{
				if (s_GPos >= s_GKeys.Count())
				{
					Finish();
					return;
				}
				OpenGroupList(s_GKeys.Get(s_GPos));
				s_GPos++;
				continue;
			}

			if (s_GClsPos < s_GCls.Count())
			{
				string name = s_GCls.Get(s_GClsPos);
				s_GClsPos++;
				budget--;
				string model = ModelKey(s_GRoot, name);
				TStringArray bucket;
				if (!s_GBuckets.Find(model, bucket))
				{
					bucket = new TStringArray;
					s_GBuckets.Insert(model, bucket);
					s_GBucketKeys.Insert(model);
				}
				bucket.Insert(name);
				continue;
			}

			if (s_GBucketPos < s_GBucketKeys.Count())
			{
				TStringArray members = s_GBuckets.Get(s_GBucketKeys.Get(s_GBucketPos));
				s_GBucketPos++;
				budget -= members.Count();
				AddBucket(s_GRoot, members, false, s_GOut);
				continue;
			}

			CloseGroupList();
		}
	}

	protected static void Finish()
	{
		int weaponClasses = 0;
		int weaponGroups = 0;
		for (int i = 0; i < s_CatWeapons.Count(); i++)
			weaponClasses += s_CatWeapons.GetElement(i).Count();
		for (int j = 0; j < s_CatGroups.Count(); j++)
			weaponGroups += s_CatGroups.GetElement(j).Count();

		int slotClasses = 0;
		int slotGroups = 0;
		for (int k = 0; k < s_SlotItems.Count(); k++)
			slotClasses += s_SlotItems.GetElement(k).Count();
		for (int m = 0; m < s_SlotGroups.Count(); m++)
			slotGroups += s_SlotGroups.GetElement(m).Count();

		s_Models.Clear();
		s_State = STATE_READY;
		Print(string.Format("[WRM] catalogue ready: %1 weapon classes in %2 cells (%3 categories), %4 attachment entries in %5 cells (%6 slots), %7 s", weaponClasses, weaponGroups, s_CatGroups.Count(), slotClasses, slotGroups, s_SlotGroups.Count(), g_Game.GetTickTime() - s_StartTime));
	}

	// ----- queries (valid when IsReady()) -----

	// visible category ids (non-hidden, non-empty), ordered by config order then id
	static void GetVisibleCategories(TStringArray ids)
	{
		TStringArray all = new TStringArray;
		for (int i = 0; i < s_CatWeapons.Count(); i++)
		{
			string id = s_CatWeapons.GetKey(i);
			TStringArray weapons = s_CatWeapons.GetElement(i);
			if (weapons.Count() > 0 && !WRMConfig.IsCategoryHidden(id))
				all.Insert(id);
		}

		// alphabetical first so that equal orders stay deterministic, then a stable selection by order
		all.Sort();
		while (all.Count() > 0)
		{
			int best = 0;
			for (int k = 1; k < all.Count(); k++)
			{
				if (WRMConfig.GetCategoryOrder(all.Get(k)) < WRMConfig.GetCategoryOrder(all.Get(best)))
					best = k;
			}
			ids.Insert(all.Get(best));
			all.RemoveOrdered(best);
		}
	}

	static TStringArray GetWeapons(string categoryLower)
	{
		TStringArray list;
		if (s_CatWeapons.Find(categoryLower, list))
			return list;
		return null;
	}

	// canonical weapon class or "" when the class is not in the catalogue
	static string FindWeapon(string lowerName)
	{
		string canonical;
		if (s_WeaponNames.Find(lowerName, canonical))
			return canonical;
		return "";
	}

	static TStringArray GetSlotItems(string slotLower)
	{
		TStringArray list;
		if (s_SlotItems.Find(slotLower, list))
			return list;
		return null;
	}

	static string FindSlotItem(string slotLower, string lowerName)
	{
		string canonical;
		if (s_SlotItemSet.Find(slotLower + "|" + lowerName, canonical))
			return canonical;
		return "";
	}

	// Magazines of one muzzle of a weapon, scope 2 only. Cached.
	static TStringArray GetMagazines(string weaponType, int muzzle)
	{
		string weaponLower = weaponType;
		weaponLower.ToLower();
		string cacheKey = weaponLower + "|" + muzzle;
		TStringArray cached;
		if (s_MagCache.Find(cacheKey, cached))
			return cached;

		TStringArray result = new TStringArray;
		string path = "CfgWeapons " + weaponType;
		TStringArray muzzles = new TStringArray;
		g_Game.ConfigGetTextArray(path + " muzzles", muzzles);
		string magPath = path;
		if (muzzle >= 0 && muzzle < muzzles.Count() && muzzles.Get(muzzle) != "this")
			magPath = path + " " + muzzles.Get(muzzle);

		TStringArray raw = new TStringArray;
		g_Game.ConfigGetTextArray(magPath + " magazines", raw);
		for (int i = 0; i < raw.Count(); i++)
		{
			string magName = raw.Get(i);
			string magLower = magName;
			magLower.ToLower();
			if (magLower == "mag_akm_drum_test" && !WRMConfig.IsAttachmentIncluded(magLower))
				continue;		// vanilla test asset left with scope 2
			if (WRMConfig.IsAttachmentExcluded(magLower))
				continue;
			if (g_Game.ConfigGetInt("CfgMagazines " + magName + " scope") != 2)
				continue;
			if (result.Find(magName) == -1)
				result.Insert(magName);
		}
		s_MagCache.Insert(cacheKey, result);
		return result;
	}

	// ----- grouped queries: one cell per item with its colour variants -----

	static array<ref WRMGroup> GetWeaponGroups(string categoryLower)
	{
		array<ref WRMGroup> groups;
		if (s_CatGroups.Find(categoryLower, groups))
			return groups;
		return null;
	}

	static array<ref WRMGroup> GetSlotItemGroups(string slotLower)
	{
		array<ref WRMGroup> groups;
		if (s_SlotGroups.Find(slotLower, groups))
			return groups;
		return null;
	}

	// Magazines of one muzzle grouped by base class. Cached next to the flat list.
	static array<ref WRMGroup> GetMagazineGroups(string weaponType, int muzzle)
	{
		string weaponLower = weaponType;
		weaponLower.ToLower();
		string cacheKey = weaponLower + "|" + muzzle;
		array<ref WRMGroup> cached;
		if (s_MagGroups.Find(cacheKey, cached))
			return cached;

		TStringArray flat = GetMagazines(weaponType, muzzle);
		map<string, ref TStringArray> buckets = new map<string, ref TStringArray>;
		TStringArray bucketKeys = new TStringArray;
		for (int i = 0; i < flat.Count(); i++)
		{
			string model = ModelKey("CfgMagazines", flat.Get(i));
			TStringArray bucket;
			if (!buckets.Find(model, bucket))
			{
				bucket = new TStringArray;
				buckets.Insert(model, bucket);
				bucketKeys.Insert(model);
			}
			bucket.Insert(flat.Get(i));
		}

		array<ref WRMGroup> groups = new array<ref WRMGroup>;
		for (int j = 0; j < bucketKeys.Count(); j++)
			AddBucket("CfgMagazines", buckets.Get(bucketKeys.Get(j)), true, groups);
		SortGroups(groups);
		s_MagGroups.Insert(cacheKey, groups);
		return groups;
	}

	// variants of the cell whose face is 'rep' in a list of groups, null when the cell is gone
	static TStringArray FindGroupVariants(array<ref WRMGroup> groups, string rep)
	{
		if (!groups)
			return null;
		string wanted = rep;
		wanted.ToLower();
		for (int i = 0; i < groups.Count(); i++)
		{
			WRMGroup group = groups.Get(i);
			string have = group.m_Rep;
			have.ToLower();
			if (have == wanted)
				return group.m_Variants;
		}
		return null;
	}
}
