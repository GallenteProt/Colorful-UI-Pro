// $profile:WeaponRadialMenu/config.json (server only). Created with defaults when missing, never overwritten
// when it exists but cannot be parsed (the error is logged and defaults are used in memory).

class WRMCategoryCfg
{
	string id;
	string name;		// "" = built-in localized name; "#STR_KEY" or plain text allowed
	int order;
	bool hidden;
}

class WRMConfigData
{
	int version = 1;
	int cooldownMs = 1500;				// minimum time between two issue requests of one player
	int maxIssuedPerPlayer = 30;
	bool collapseColorVariants = false;	// obsolete and ignored (variants are always grouped by model), kept so that old files still load
	bool groupColorVariants = true;		// obsolete and ignored, same reason
	ref array<ref WRMCategoryCfg> categories = new array<ref WRMCategoryCfg>;
	ref array<string> categoryOverrides = new array<string>;	// "ClassName=categoryId"
	ref array<string> weaponInclude = new array<string>;		// force-listed weapon classes (scope 2 still required)
	ref array<string> weaponExclude = new array<string>;		// "Class" or "Prefix*"
	ref array<string> attachmentInclude = new array<string>;
	ref array<string> attachmentExclude = new array<string>;	// also applied to magazines
}

class WRMConfig
{
	static const string DIR = "$profile:WeaponRadialMenu";
	static const string FILE = "$profile:WeaponRadialMenu/config.json";
	static const string BUILTIN_IDS = "assault,sniper,smg,shotgun,rifle,pistol,launcher,other";

	static ref WRMConfigData s_Data;
	static ref map<string, string> s_Overrides;	// lowercase class -> lowercase category id
	static ref TStringArray s_WInc;
	static ref TStringArray s_WExc;
	static ref TStringArray s_AInc;
	static ref TStringArray s_AExc;

	static WRMConfigData Get()
	{
		if (!s_Data)
			Load();
		return s_Data;
	}

	static void Load()
	{
		WRMConfigData data = new WRMConfigData();
		bool useLoaded = false;

		MakeDirectory(DIR);
		if (FileExist(FILE))
		{
			WRMConfigData loaded = new WRMConfigData();
			string err;
			if (JsonFileLoader<WRMConfigData>.LoadFile(FILE, loaded, err))
			{
				data = loaded;
				useLoaded = true;
			}
			else
			{
				Print("[WRM] config.json is invalid, file left untouched, using defaults in memory: " + err);
			}
		}

		s_Data = data;
		Sanitize();
		FillMissingCategories(s_Data);

		if (!FileExist(FILE) && !useLoaded)
		{
			string saveErr;
			if (!JsonFileLoader<WRMConfigData>.SaveFile(FILE, s_Data, saveErr))
				Print("[WRM] could not write default config.json: " + saveErr);
		}

		BuildLookups();
	}

	protected static void FillMissingCategories(WRMConfigData data)
	{
		TStringArray ids = new TStringArray;
		string csv = BUILTIN_IDS;
		csv.Split(",", ids);
		for (int i = 0; i < ids.Count(); i++)
		{
			string id = ids.Get(i);
			if (FindCategory(data, id))
				continue;
			WRMCategoryCfg cfg = new WRMCategoryCfg();
			cfg.id = id;
			cfg.name = "";
			cfg.order = (i + 1) * 10;
			cfg.hidden = false;
			data.categories.Insert(cfg);
		}
	}

	protected static WRMCategoryCfg FindCategory(WRMConfigData data, string id)
	{
		string wanted = id;
		wanted.ToLower();
		for (int i = 0; i < data.categories.Count(); i++)
		{
			WRMCategoryCfg cfg = data.categories.Get(i);
			if (!cfg)
				continue;
			string have = cfg.id;
			have.ToLower();
			if (have == wanted)
				return cfg;
		}
		return null;
	}

	protected static void Sanitize()
	{
		if (!s_Data.categories)
			s_Data.categories = new array<ref WRMCategoryCfg>;
		if (!s_Data.categoryOverrides)
			s_Data.categoryOverrides = new array<string>;

		if (s_Data.cooldownMs < 0)
			s_Data.cooldownMs = 0;
		if (s_Data.cooldownMs > 60000)
			s_Data.cooldownMs = 60000;
		if (s_Data.maxIssuedPerPlayer < 1)
			s_Data.maxIssuedPerPlayer = 1;
		if (s_Data.maxIssuedPerPlayer > 500)
			s_Data.maxIssuedPerPlayer = 500;
	}

	protected static TStringArray LowerList(array<string> source)
	{
		TStringArray result = new TStringArray;
		if (!source)
			return result;
		for (int i = 0; i < source.Count(); i++)
		{
			string entry = source.Get(i).Trim();
			entry.ToLower();
			if (entry != "")
				result.Insert(entry);
		}
		return result;
	}

	protected static void BuildLookups()
	{
		s_WInc = LowerList(s_Data.weaponInclude);
		s_WExc = LowerList(s_Data.weaponExclude);
		s_AInc = LowerList(s_Data.attachmentInclude);
		s_AExc = LowerList(s_Data.attachmentExclude);

		s_Overrides = new map<string, string>;
		for (int i = 0; i < s_Data.categoryOverrides.Count(); i++)
		{
			string line = s_Data.categoryOverrides.Get(i);
			int eq = line.IndexOf("=");
			if (eq < 1)
				continue;
			string cls = line.Substring(0, eq).Trim();
			string cat = line.Substring(eq + 1, line.Length() - eq - 1).Trim();
			cls.ToLower();
			cat.ToLower();
			if (cls != "" && cat != "")
				s_Overrides.Set(cls, cat);
		}
	}

	// exact (lowercase) match, or prefix match for entries ending with '*'
	static bool Match(TStringArray list, string lowerName)
	{
		if (!list)
			return false;
		for (int i = 0; i < list.Count(); i++)
		{
			string entry = list.Get(i);
			int len = entry.Length();
			if (len > 1 && entry.Substring(len - 1, 1) == "*")
			{
				string prefix = entry.Substring(0, len - 1);
				if (lowerName.IndexOf(prefix) == 0)
					return true;
			}
			else if (entry == lowerName)
			{
				return true;
			}
		}
		return false;
	}

	static bool IsWeaponIncluded(string lowerName) { Get(); return Match(s_WInc, lowerName); }
	static bool IsWeaponExcluded(string lowerName) { Get(); return Match(s_WExc, lowerName); }
	static bool IsAttachmentIncluded(string lowerName) { Get(); return Match(s_AInc, lowerName); }
	static bool IsAttachmentExcluded(string lowerName) { Get(); return Match(s_AExc, lowerName); }

	// "" when no override exists
	static string GetOverride(string lowerName)
	{
		Get();
		string cat;
		if (s_Overrides.Find(lowerName, cat))
			return cat;
		return "";
	}

	// Display name sent to the client: "#STR_..." key or plain text.
	static string GetCategoryName(string id)
	{
		WRMCategoryCfg cfg = FindCategory(Get(), id);
		if (cfg && cfg.name != "")
			return cfg.name;

		string lowerId = id;
		lowerId.ToLower();
		switch (lowerId)
		{
			case "assault": return "#STR_WRM_CAT_ASSAULT";
			case "sniper": return "#STR_WRM_CAT_SNIPER";
			case "smg": return "#STR_WRM_CAT_SMG";
			case "shotgun": return "#STR_WRM_CAT_SHOTGUN";
			case "rifle": return "#STR_WRM_CAT_RIFLE";
			case "pistol": return "#STR_WRM_CAT_PISTOL";
			case "launcher": return "#STR_WRM_CAT_LAUNCHER";
			case "other": return "#STR_WRM_CAT_OTHER";
		}
		return id;
	}

	static bool IsCategoryHidden(string id)
	{
		WRMCategoryCfg cfg = FindCategory(Get(), id);
		return cfg && cfg.hidden;
	}

	static int GetCategoryOrder(string id)
	{
		WRMCategoryCfg cfg = FindCategory(Get(), id);
		if (cfg)
			return cfg.order;
		return 1000;
	}
}
