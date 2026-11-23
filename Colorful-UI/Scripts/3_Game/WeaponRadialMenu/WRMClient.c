// Client side data layer: sends requests, keeps caches of the server replies, resolves display names from the
// local config (the server never sends item names, only class names, so every player sees his own language).

// Single player (offline) has no network: WRMServer registers an implementation here and requests go straight in.
class WRMLocalServer
{
	void HandleLocal(Man player, WRMMsg msg) {}
}

class WRMCatInfo
{
	string m_Id;
	string m_Name;
	int m_Count;		// number of cells (items with their variants folded), not shown
}

// One page of cells: m_Items holds the class shown in each cell, m_Counts the variants behind it,
// m_Flags is 1 when the cell holds the item in hands / the installed attachment.
class WRMPageData
{
	int m_Total;
	ref TStringArray m_Items = new TStringArray;
	ref TIntArray m_Counts = new TIntArray;
	ref TIntArray m_Flags = new TIntArray;
}

class WRMSlotInfo
{
	string m_Name;
	string m_Installed;
	int m_Count;
}

class WRMVariantData
{
	ref TStringArray m_Items = new TStringArray;
	int m_Installed = -1;
}

class WRMClient
{
	static const float REQUEST_TTL = 1.5;		// a request is not repeated before this many seconds
	static const float STATUS_TIME = 3.0;
	static const float ISSUE_GAP = 0.3;			// client side anti double click, the server enforces the real cooldown
	static const float SILENT_TIMEOUT = 6.0;	// no reply at all for this long -> "server has no mod"

	static ref WRMLocalServer s_Local;
	static int s_Gen;							// bumped on every data change, the menu rebuilds when it changes

	static ref array<ref WRMCatInfo> s_Cats = new array<ref WRMCatInfo>;
	static bool s_CatsReady;
	static ref map<string, ref WRMPageData> s_Pages = new map<string, ref WRMPageData>;
	static ref map<string, float> s_Sent = new map<string, float>;
	static ref map<string, string> s_Names = new map<string, string>;
	static ref map<string, ref WRMVariantData> s_Variants = new map<string, ref WRMVariantData>;

	static ref array<ref WRMSlotInfo> s_Slots = new array<ref WRMSlotInfo>;
	static string s_SlotsWeapon;
	static bool s_SlotsReady;

	static string s_StatusText;
	static float s_StatusUntil;
	static bool s_GotReply;
	static float s_FirstRequest;
	static float s_LastIssue;
	static int s_ResultSeq;						// bumped on every server result (REP_RESULT)
	static int s_LastResult = -1;				// WRMStatus of the last result

	static void Reset()
	{
		s_Cats.Clear();
		s_CatsReady = false;
		s_Pages.Clear();
		s_Sent.Clear();
		s_Variants.Clear();
		s_Slots.Clear();
		s_SlotsWeapon = "";
		s_SlotsReady = false;
		s_StatusText = "";
		s_StatusUntil = 0;
		s_GotReply = false;
		s_FirstRequest = 0;
		s_Gen++;
	}

	// ----- weapon related caches -----

	protected static void RemoveWithPrefix(string prefix)
	{
		TStringArray doomed = new TStringArray;
		for (int i = 0; i < s_Pages.Count(); i++)
		{
			string pageKey = s_Pages.GetKey(i);
			if (pageKey.IndexOf(prefix) == 0)
				doomed.Insert(pageKey);
		}
		for (int j = 0; j < doomed.Count(); j++)
			s_Pages.Remove(doomed.Get(j));

		doomed.Clear();
		for (int k = 0; k < s_Sent.Count(); k++)
		{
			string sentKey = s_Sent.GetKey(k);
			if (sentKey.IndexOf(prefix) == 0)
				doomed.Insert(sentKey);
		}
		for (int m = 0; m < doomed.Count(); m++)
			s_Sent.Remove(doomed.Get(m));
	}

	// The item in hands changed or the installed attachments changed: forget slots, slot item pages and variants.
	static void InvalidateWeapon()
	{
		s_Slots.Clear();
		s_SlotsWeapon = "";
		s_SlotsReady = false;
		s_Sent.Remove("s");
		RemoveWithPrefix("i|");
		RemoveWithPrefix("w|");	// weapon pages carry the in-hands flag
		RemoveWithPrefix("v|");	// allow an immediate new request for variants
		s_Variants.Clear();
		s_Gen++;
	}

	// ----- sending -----

	protected static void Send(WRMMsg msg)
	{
		Man player = g_Game.GetPlayer();
		if (!player)
			return;

		if (g_Game.IsMultiplayer())
		{
			ScriptRPC rpc = new ScriptRPC();
			msg.Write(rpc);
			rpc.Send(player, WRM_RPC_ID, true, null);
		}
		else if (s_Local)
		{
			s_Local.HandleLocal(player, msg);
		}
	}

	protected static void Request(string key, WRMMsg msg)
	{
		float now = g_Game.GetTickTime();
		float last;
		if (s_Sent.Find(key, last) && now - last < REQUEST_TTL)
			return;

		s_Sent.Set(key, now);
		if (s_FirstRequest == 0)
			s_FirstRequest = now;
		Send(msg);
	}

	static void EnsureCats()
	{
		if (s_CatsReady)
			return;
		Request("c", new WRMMsg(WRMOp.REQ_CATS));
	}

	static string PageKey(string kind, string name, int page)
	{
		string lowerName = name;
		lowerName.ToLower();
		return kind + "|" + lowerName + "|" + page;
	}

	static WRMPageData GetWeaponPage(string category, int page)
	{
		string key = PageKey("w", category, page);
		WRMPageData data;
		if (s_Pages.Find(key, data))
			return data;

		WRMMsg msg = new WRMMsg(WRMOp.REQ_WEAPONS);
		msg.m_Strs.Insert(category);
		msg.m_Ints.Insert(page);
		Request(key, msg);
		return null;
	}

	static bool EnsureSlots()
	{
		if (s_SlotsReady)
			return true;
		Request("s", new WRMMsg(WRMOp.REQ_SLOTS));
		return false;
	}

	static WRMPageData GetSlotItemPage(string slot, int page, string weaponType)
	{
		string key = PageKey("i", weaponType + "|" + slot, page);
		WRMPageData data;
		if (s_Pages.Find(key, data))
			return data;

		WRMMsg msg = new WRMMsg(WRMOp.REQ_SLOT_ITEMS);
		msg.m_Strs.Insert(slot);
		msg.m_Strs.Insert(weaponType);
		msg.m_Ints.Insert(page);
		Request(key, msg);
		return null;
	}

	protected static string VariantKey(string kind, string weaponType, string scope, string rep)
	{
		string key = "v|" + kind + "|" + weaponType + "|" + scope + "|" + rep;
		key.ToLower();
		return key;
	}

	// Variants of one cell: kind "w" (category), "i" (attachment slot) or "m" (magazines of the muzzle in hands).
	static WRMVariantData GetVariants(string kind, string scope, string rep, string weaponType)
	{
		string key = VariantKey(kind, weaponType, scope, rep);
		WRMVariantData data;
		if (s_Variants.Find(key, data))
			return data;

		WRMMsg msg = new WRMMsg(WRMOp.REQ_VARIANTS);
		msg.m_Strs.Insert(kind);
		msg.m_Strs.Insert(scope);
		msg.m_Strs.Insert(rep);
		Request(key, msg);
		return null;
	}

	protected static bool CanIssueNow()
	{
		float now = g_Game.GetTickTime();
		if (now - s_LastIssue < ISSUE_GAP)
			return false;
		s_LastIssue = now;
		return true;
	}

	// false when the request was not sent (client side anti double click): no reply will come
	static bool IssueWeapon(string className)
	{
		if (!CanIssueNow())
			return false;
		WRMMsg msg = new WRMMsg(WRMOp.ISSUE_WEAPON);
		msg.m_Strs.Insert(className);
		Send(msg);
		return true;
	}

	static void IssueAttachment(string slot, string className, string weaponType)
	{
		if (!CanIssueNow())
			return;
		WRMMsg msg = new WRMMsg(WRMOp.ISSUE_ATTACH);
		msg.m_Strs.Insert(slot);
		msg.m_Strs.Insert(className);
		msg.m_Strs.Insert(weaponType);
		Send(msg);
	}

	// type of the item in the local hands, "" when empty
	protected static string GetHandsType()
	{
		Man player = g_Game.GetPlayer();
		if (!player)
			return "";
		EntityAI hands = player.GetHumanInventory().GetEntityInHands();
		if (!hands)
			return "";
		return hands.GetType();
	}

	static bool IsServerSilent()
	{
		if (s_GotReply || s_FirstRequest == 0)
			return false;
		return g_Game.GetTickTime() - s_FirstRequest > SILENT_TIMEOUT;
	}

	// ----- receiving -----

	static void OnMessage(WRMMsg msg)
	{
		s_GotReply = true;
		switch (msg.m_Op)
		{
			case WRMOp.REP_CATS:
				ReadCats(msg);
				break;
			case WRMOp.REP_WEAPONS:
				ReadPage(msg, "w", 1, "");
				break;
			case WRMOp.REP_SLOTS:
				ReadSlots(msg);
				break;
			case WRMOp.REP_SLOT_ITEMS:
				ReadPage(msg, "i", 2, msg.GetStr(1) + "|");
				break;
			case WRMOp.REP_VARIANTS:
				ReadVariants(msg);
				break;
			case WRMOp.REP_RESULT:
				ReadResult(msg);
				break;
		}
	}

	protected static void ReadCats(WRMMsg msg)
	{
		if (msg.m_Status != WRMStatus.OK)
			return;

		s_Cats.Clear();
		int n = msg.m_Strs.Count() / 2;
		for (int i = 0; i < n; i++)
		{
			WRMCatInfo info = new WRMCatInfo();
			info.m_Id = msg.GetStr(i * 2);
			info.m_Name = msg.GetStr(i * 2 + 1);
			info.m_Count = msg.GetInt(i);
			s_Cats.Insert(info);
		}
		s_CatsReady = true;
		s_Gen++;
	}

	// strs: [name, (weapon), reps...] with firstItem = index of the first cell;
	// ints: [page, total, requestedPage, (count, flag)...] with the per-cell pairs from index 3
	protected static void ReadPage(WRMMsg msg, string kind, int firstItem, string keyPrefix)
	{
		if (msg.m_Status != WRMStatus.OK)
			return;
		// slot item pages belong to one weapon: a late reply for a weapon no longer in hands is dropped
		if (kind == "i" && msg.GetStr(1) != GetHandsType())
			return;

		WRMPageData data = new WRMPageData();
		data.m_Total = msg.GetInt(1);
		for (int i = firstItem; i < msg.m_Strs.Count(); i++)
		{
			int cell = i - firstItem;
			data.m_Items.Insert(msg.m_Strs.Get(i));
			data.m_Counts.Insert(msg.GetInt(3 + cell * 2, 1));
			data.m_Flags.Insert(msg.GetInt(3 + cell * 2 + 1, 0));
		}

		// cached under the page that was asked for (ints[2]); the server may have clamped ints[0]
		s_Pages.Set(PageKey(kind, keyPrefix + msg.GetStr(0), msg.GetInt(2, msg.GetInt(0))), data);
		s_Gen++;
	}

	protected static void ReadVariants(WRMMsg msg)
	{
		if (msg.m_Status != WRMStatus.OK)
			return;

		string kind = msg.GetStr(0);
		string weaponType = msg.GetStr(3);
		// attachment and magazine variants belong to one weapon: a late reply for another weapon is dropped
		if (kind != "w" && weaponType != GetHandsType())
			return;
		string key = VariantKey(kind, weaponType, msg.GetStr(1), msg.GetStr(2));

		WRMVariantData data = new WRMVariantData();
		data.m_Installed = msg.GetInt(0, -1);
		for (int i = 4; i < msg.m_Strs.Count(); i++)
			data.m_Items.Insert(msg.m_Strs.Get(i));
		s_Variants.Set(key, data);
		s_Gen++;
	}

	protected static void ReadSlots(WRMMsg msg)
	{
		if (msg.m_Status != WRMStatus.OK)
			return;

		if (msg.GetStr(0) != GetHandsType())
			return;		// reply for a weapon that is no longer in hands

		s_Slots.Clear();
		s_SlotsWeapon = msg.GetStr(0);
		int n = (msg.m_Strs.Count() - 1) / 2;
		for (int i = 0; i < n; i++)
		{
			WRMSlotInfo info = new WRMSlotInfo();
			info.m_Name = msg.GetStr(1 + i * 2);
			info.m_Installed = msg.GetStr(2 + i * 2);
			info.m_Count = msg.GetInt(i);
			s_Slots.Insert(info);
		}
		s_SlotsReady = true;
		s_Gen++;
	}

	protected static void ReadResult(WRMMsg msg)
	{
		string cls = msg.GetStr(0);
		string text = "";
		switch (msg.m_Status)
		{
			case WRMStatus.OK:
				text = string.Format(Widget.TranslateString("#STR_WRM_ST_OK"), GetItemName(cls));
				break;
			case WRMStatus.ALREADY:
				text = string.Format(Widget.TranslateString("#STR_WRM_ST_ALREADY"), GetItemName(cls));
				break;
			case WRMStatus.BUILDING:
				text = Widget.TranslateString("#STR_WRM_ST_BUILDING");
				break;
			case WRMStatus.DENIED:
				text = Widget.TranslateString("#STR_WRM_ST_DENIED");
				break;
			case WRMStatus.RATE:
				text = Widget.TranslateString("#STR_WRM_ST_RATE");
				break;
			case WRMStatus.NOT_FOUND:
				text = Widget.TranslateString("#STR_WRM_ST_NOT_FOUND");
				break;
			case WRMStatus.MISMATCH:
				text = Widget.TranslateString("#STR_WRM_ST_MISMATCH");
				break;
			case WRMStatus.NO_ROOM:
				text = Widget.TranslateString("#STR_WRM_ST_NO_ROOM");
				break;
			case WRMStatus.LIMIT:
				text = Widget.TranslateString("#STR_WRM_ST_LIMIT");
				break;
			case WRMStatus.FAILED:
				text = Widget.TranslateString("#STR_WRM_ST_FAILED");
				break;
			case WRMStatus.BUSY:
				text = Widget.TranslateString("#STR_WRM_ST_BUSY");
				break;
			case WRMStatus.NO_WEAPON:
				text = Widget.TranslateString("#STR_WRM_ST_NO_WEAPON");
				break;
			default:
				text = Widget.TranslateString("#STR_WRM_ST_BAD");
				break;
		}
		SetStatus(text);
		s_LastResult = msg.m_Status;
		s_ResultSeq++;

		// installed state may have changed (or the weapon changed under us): refresh slot data
		if (msg.m_Status == WRMStatus.OK || msg.m_Status == WRMStatus.ALREADY || msg.m_Status == WRMStatus.MISMATCH)
			InvalidateWeapon();
	}

	static void SetStatus(string text)
	{
		s_StatusText = text;
		s_StatusUntil = g_Game.GetTickTime() + STATUS_TIME;
		s_Gen++;
	}

	// "" when no status is showing
	static string GetStatus()
	{
		if (s_StatusText != "" && g_Game.GetTickTime() > s_StatusUntil)
		{
			s_StatusText = "";
			s_Gen++;
		}
		return s_StatusText;
	}

	// ----- display names -----

	protected static string ReadDisplayName(string root, string cls)
	{
		string name;
		g_Game.ConfigGetText(root + " " + cls + " displayName", name);
		return name;
	}

	static string GetItemName(string cls)
	{
		if (cls == "")
			return "";

		string cached;
		if (s_Names.Find(cls, cached))
			return cached;

		string name = ReadDisplayName("CfgWeapons", cls);
		if (name == "")
			name = ReadDisplayName("CfgVehicles", cls);
		if (name == "")
			name = ReadDisplayName("CfgMagazines", cls);
		if (name == "")
			name = cls;
		name = TranslateName(name);
		name.Replace("<", "(");
		s_Names.Set(cls, name);
		return name;
	}

	// "#STR_KEY" / "$STR_KEY" -> localized text, anything else is returned as is
	static string TranslateName(string name)
	{
		string text = name;
		if (text.IndexOf("$UNT$") == 0)		// vanilla marker of untranslated config names
			text = text.Substring(5, text.Length() - 5);
		if (text.IndexOf("$STR") == 0)
			text.Replace("$STR", "#STR");
		if (text.IndexOf("#") == 0)
			text = Widget.TranslateString(text);
		return text;
	}

	// readable tail of a variant class, used to tell apart variants that share a displayName:
	// "AK101_Black" next to the face "AK101" gives "Black"
	static string VariantSuffix(string face, string cls)
	{
		int common = 0;
		int limit = face.Length();
		if (cls.Length() < limit)
			limit = cls.Length();
		while (common < limit && face.Substring(common, 1) == cls.Substring(common, 1))
			common++;

		string tail = cls.Substring(common, cls.Length() - common);
		while (tail.Length() > 0 && tail.Substring(0, 1) == "_")
			tail = tail.Substring(1, tail.Length() - 1);
		if (tail == "")
			return cls;
		return tail;
	}

	static string GetSlotName(string slot)
	{
		if (slot == WRM_MAG_KEY)
			return Widget.TranslateString("#STR_WRM_MAGAZINES");

		int slotId = InventorySlots.GetSlotIdFromString(slot);
		if (slotId != InventorySlots.INVALID)
		{
			string displayName = InventorySlots.GetSlotDisplayName(slotId);
			if (displayName != "")
				return TranslateName(displayName);
		}
		return slot;
	}
}
