// Quick slot wheel: client side data.
//
// The wheel shows only what the player already owns, so nothing here talks to the server to build a
// page. The client enumerates its own inventory, resolves every profile cell to a concrete item, and
// asks the server only to run the action. The server re-checks everything it is asked to do, so a
// tampered client can change how it calls an action but cannot reach an item it does not have.

// One resolved cell of the wheel, ready to be drawn.
class CUIQSCell
{
	int m_Index;			// wheel position, see CUIQuickSlotPosition
	int m_Key;				// key number 1..8, 0 = this position is not on the wheel
	string m_Action;		// profile action name
	string m_Target;
	string m_Slot;
	string m_Class;			// resolved class, "" when the player has nothing for this cell
	string m_Label;			// the item name: the cell caption and the centre text on hover
	string m_Icon;			// "set:dayz_inventory image:..." when the game has a raster icon for it, else ""
	bool m_Enabled;			// false = grey cell, the click does nothing
	bool m_Highlight;		// weapon in hands, or the item already equipped
	bool m_IsBack;			// the "back one level" cell, always drawn at the bottom of a sub level
	int m_Count;			// items behind a category cell
	ref array<string> m_Items = new array<string>;	// category level: the classes to list
	Widget m_Widget;		// the card this cell is drawn on, owned by the wheel

	string Resolved()
	{
		if (m_Class != "")
			return m_Class;
		return m_Target;
	}
}

class CUIQuickSlotLocal
{
	static const float HELLO_TTL = 30.0;
	static const float STATUS_TIME = 3.0;
	static const float ACTION_GAP = 0.25;		// client side anti double click, the server has the real cooldown

	// Built-in category targets. A profile cell may also carry a plain comma separated list of class
	// prefixes, with "*" as the wildcard, so a player can point a cell at anything without a code change.
	static const string MED_PREFIXES = "Bandage*,BloodBag*,BloodTestKit,SalineBag*,IVStartKit,Epinephrine,Morphine,Promethazine,PainkillerTablets,CharcoalTablets,TetracyclineAntibiotics,Antibiotic*,Vitamin*,Syringe,FirstAid*,HeatPack,ColdPack,Disinfectant*,Iodine*,Alcohol*,Chelating*";

	static ref array<EntityAI> s_Scan = new array<EntityAI>;
	static float s_ScanAt = -1;
	static EntityAI s_ScanHands;

	static ref CUIQuickSlotLocalServer s_Local;

	static bool s_Admin;
	static bool s_AdminKnown;
	static float s_HelloAt = -1;

	static string s_Status;
	static float s_StatusUntil;
	static int s_ResultSeq;
	static int s_LastResult = -1;
	static float s_LastAction;
	static bool s_ArsenalRequested;		// set by the wheel, consumed by the mission hook
	static bool s_DeepRequested;		// set by the wheel, consumed by WRMMenu (attachments level)

	// ----- inventory -----

	// A fresh list of everything the player carries, cached for a fraction of a second: the wheel
	// rebuilds on every hover change and must not walk the whole inventory each time.
	static void Scan(PlayerBase player, out array<EntityAI> items)
	{
		items = s_Scan;
		float now = g_Game.GetTickTime();
		EntityAI hands = null;
		if (player)
			hands = player.GetItemInHands();

		if (s_ScanAt >= 0 && now - s_ScanAt < 0.25 && hands == s_ScanHands)
			return;

		s_ScanAt = now;
		s_ScanHands = hands;
		s_Scan.Clear();
		if (player)
			player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, s_Scan);
	}

	static void Invalidate()
	{
		s_ScanAt = -1;
		s_Scan.Clear();
	}

	// "Bandage*,Morphine" -> does this class match any of the prefixes?
	static bool Matches(string type, string prefixes)
	{
		if (type == "" || prefixes == "")
			return false;

		string lowerType = type;
		lowerType.ToLower();

		TStringArray parts = new TStringArray;
		string list = prefixes;
		list.Split(",", parts);
		for (int i = 0; i < parts.Count(); i++)
		{
			string token = parts.Get(i);
			token.Trim();
			if (token == "")
				continue;
			token.ToLower();

			int len = token.Length();
			if (len > 0 && token.Get(len - 1) == "*")
			{
				string head = token.Substring(0, len - 1);
				if (head != "" && lowerType.IndexOf(head) == 0)
					return true;
			}
			else if (lowerType == token)
			{
				return true;
			}
		}
		return false;
	}

	// The prefixes a cell target stands for. "@med" and "@mags" are built in; anything else is a list.
	static string PrefixesFor(string target)
	{
		string lower = target;
		lower.ToLower();
		if (lower == "@med")
			return MED_PREFIXES;
		if (lower == "@mags")
			return "";		// handled by IsMagazineTarget, a magazine is not a class prefix
		return target;
	}

	static bool IsMagazineTarget(string target)
	{
		string lower = target;
		lower.ToLower();
		return lower == "@mags";
	}

	static bool IsFoodTarget(string target)
	{
		string lower = target;
		lower.ToLower();
		return lower == "@food";
	}

	static bool ItemMatches(EntityAI item, string target)
	{
		if (!item)
			return false;
		if (IsMagazineTarget(target))
			return item.IsKindOf("Magazine");
		if (IsFoodTarget(target))
			return item.IsKindOf("Edible_Base");
		return Matches(item.GetType(), PrefixesFor(target));
	}

	// Sorted by class name: the server enumerates the same inventory, and both sides have to agree on
	// which weapon is "the first one" or the cell would promise one weapon and hand another.
	protected static void SortByType(array<string> types)
	{
		types.Sort();
	}

	static void CollectWeapons(PlayerBase player, out array<string> types)
	{
		types = new array<string>;
		array<EntityAI> items;
		Scan(player, items);
		for (int i = 0; i < items.Count(); i++)
		{
			EntityAI item = items.Get(i);
			if (!item || !item.IsWeapon())
				continue;
			string type = item.GetType();
			if (types.Find(type) < 0)
				types.Insert(type);
		}
		SortByType(types);
	}

	// The entity itself, for the paths that need the object rather than its class name (the vanilla
	// action system does).
	static EntityAI FindEntity(PlayerBase player, string type)
	{
		if (!player || type == "")
			return null;
		array<EntityAI> items;
		Scan(player, items);
		for (int i = 0; i < items.Count(); i++)
		{
			EntityAI item = items.Get(i);
			if (item && item != player && item.GetType() == type)
				return item;
		}
		return null;
	}

	static string FindItem(PlayerBase player, string target, bool skipEquipped, string slot)
	{
		array<EntityAI> items;
		Scan(player, items);

		EntityAI equipped = null;
		if (slot != "")
			equipped = player.GetInventory().FindAttachment(InventorySlots.GetSlotIdFromString(slot));

		for (int i = 0; i < items.Count(); i++)
		{
			EntityAI item = items.Get(i);
			if (!item || item == player)
				continue;
			if (!ItemMatches(item, target))
				continue;
			if (skipEquipped && item == equipped)
				continue;
			return item.GetType();
		}
		return "";
	}

	static int CountItems(PlayerBase player, string target)
	{
		array<EntityAI> items;
		Scan(player, items);
		int count = 0;
		for (int i = 0; i < items.Count(); i++)
		{
			EntityAI item = items.Get(i);
			if (item && item != player && ItemMatches(item, target))
				count++;
		}
		return count;
	}

	static void CollectItems(PlayerBase player, string target, out array<string> types)
	{
		types = new array<string>;
		array<EntityAI> items;
		Scan(player, items);
		for (int i = 0; i < items.Count(); i++)
		{
			EntityAI item = items.Get(i);
			if (!item || item == player || !ItemMatches(item, target))
				continue;
			string type = item.GetType();
			if (types.Find(type) < 0)
				types.Insert(type);
		}
		SortByType(types);
	}

	// ----- cells -----

	// The caption of a cell is the name of the thing it holds, and nothing else: no action verb, no
	// slot name, no "hands". An empty cell has no caption at all, which is what the grey cell says.
	static string LabelFor(string action, string target, string cls)
	{
		if (cls != "")
			return WRMClient.GetItemName(cls);

		int act = CUIQuickSlotActions.FromString(action);
		if (act == CUIQuickSlotAction.QSA_CATEGORY)
			return CategoryName(target);
		return "";
	}

	// Raster icon of a cell, taken from the game's own inventory icon atlas (gui/imagesets/
	// dayz_inventory.imageset). The atlas carries the slot and category icons the vanilla inventory
	// uses - hands, headgear, magazine, medicalbandage, food - and not one icon per item, so an item
	// without a mapping falls back to the 3D preview the card already draws.
	static string IconFor(string action, string target, string cls)
	{
		if (cls != "")
		{
			string fromClass = IconForClass(cls);
			if (fromClass != "")
				return IconName(fromClass);
			return "";
		}

		int act = CUIQuickSlotActions.FromString(action);
		if (act == CUIQuickSlotAction.QSA_CATEGORY)
		{
			string lower = target;
			lower.ToLower();
			if (lower == "@med")
				return IconName("medicalbandage");
			if (lower == "@mags")
				return IconName("magazine");
			if (lower == "@food")
				return IconName("food");
		}

		// Nothing in the slot: still show the icon of what the cell is for, from the target's own class
		// family. This is what keeps a wheel of empty cells readable instead of a row of blanks.
		if (target != "")
		{
			string fromTarget = IconForClass(target);
			if (fromTarget != "")
				return IconName(fromTarget);
		}

		// A weapon cell with no weapon yet: the pistol/rifle family icon, the most common case.
		if (act == CUIQuickSlotAction.QSA_WEAPON)
			return IconName("pistol");

		return "";
	}

	// Slot icons: the icon of the slot a cell equips into, or of the class family an item belongs to.
	// Every name here exists in gui/imagesets/dayz_inventory.imageset; an item with no mapping returns ""
	// and the cell falls back to the engine's 3D item preview.
	static string IconForClass(string cls)
	{
		string lower = cls;
		lower.ToLower();

		if (lower.Contains("bandage") || lower.Contains("bloodbag") || lower.Contains("saline") || lower.Contains("morphine") || lower.Contains("painkiller") || lower.Contains("tetracycline") || lower.Contains("charcoal") || lower.Contains("vitamin") || lower.Contains("epinephrine") || lower.Contains("syringe") || lower.Contains("disinfectant") || lower.Contains("iodine"))
			return "medicalbandage";
		if (lower.Contains("magazine") || lower.Contains("mag_") || lower.Contains("ammo") || lower.Contains("rounds"))
			return "magazine";
		if (lower.Contains("gasmask_filter") || lower.Contains("gasmaskfilter"))
			return "gasmaskfilter";
		if (lower.Contains("gasmask") || lower.Contains("gas_mask") || lower.Contains("mask"))
			return "headgear";
		if (lower.Contains("nvg") || lower.Contains("nightvision"))
			return "nvggoggles";
		if (lower.Contains("canteen") || lower.Contains("waterbottle"))
			return "canteen";
		if (lower.Contains("battery"))
			return "carbattery";
		if (lower.Contains("walkietalkie") || lower.Contains("radio"))
			return "walkietalkie";
		if (lower.Contains("chemlight"))
			return "chemlight";
		if (lower.Contains("grenade") || lower.Contains("frag"))
			return "grenade";
		if (lower.Contains("suppressor") || lower.Contains("supressor"))
			return "supressor";
		if (lower.Contains("flashlight") || lower.Contains("tlr"))
			return "flashlight";
		if (lower.Contains("optic") || lower.Contains("scope") || lower.Contains("pso") || lower.Contains("acog"))
			return "weaponoptics";
		if (lower.Contains("knife") || lower.Contains("bayonet") || lower.Contains("machete") || lower.Contains("cleaver"))
			return "knife";
		if (lower.Contains("apple") || lower.Contains("pear") || lower.Contains("plum") || lower.Contains("berry") || lower.Contains("food") || lower.Contains("meat") || lower.Contains("can"))
			return "food";
		if (lower.Contains("tincan") || lower.Contains("pot"))
			return "tincan";
		if (lower.Contains("rags") || lower.Contains("rag"))
			return "rags";
		if (lower.Contains("pistol") || lower.Contains("cz75") || lower.Contains("glock") || lower.Contains("mkii") || lower.Contains("pmm") || lower.Contains("engraved") || lower.Contains("deagle") || lower.Contains("revolver") || lower.Contains("longhorn") || lower.Contains("magnum"))
			return "pistol";
		return "";
	}

	static string IconName(string name)
	{
		return "set:dayz_inventory image:" + name;
	}

	static string CategoryName(string target)
	{
		string lower = target;
		lower.ToLower();
		if (lower == "@med")
			return Widget.TranslateString("#STR_CUI_QS_CAT_MED");
		if (lower == "@mags")
			return Widget.TranslateString("#STR_CUI_QS_CAT_MAGS");
		if (lower == "@food")
			return Widget.TranslateString("#STR_CUI_QS_CAT_FOOD");
		return target;
	}

	// Builds one cell from the profile entry and what the player actually carries.
	static void ResolveCell(CUIQSCell cell, CUIQuickSlot entry, PlayerBase player, int wheelSize)
	{
		cell.m_Index = entry.index;
		cell.m_Action = entry.action;
		cell.m_Target = entry.target;
		cell.m_Slot = entry.slot;
		cell.m_Key = CUIQuickSlotLayout.KeyForCell(wheelSize, entry.index);
		cell.m_Enabled = false;
		cell.m_Highlight = false;
		cell.m_Count = 0;
		cell.m_Class = "";

		int action = CUIQuickSlotActions.FromString(entry.action);
		switch (action)
		{
			case CUIQuickSlotAction.QSA_WEAPON:
			{
				array<string> weapons;
				CollectWeapons(player, weapons);
				// Bottom is key 1 and takes the first weapon, top is key 2 and takes the second.
				int wanted = cell.m_Key - 1;
				if (wanted >= 0 && wanted < weapons.Count())
				{
					cell.m_Class = weapons.Get(wanted);
					cell.m_Enabled = true;
				}
				EntityAI hands = null;
				if (player)
					hands = player.GetItemInHands();
				cell.m_Highlight = hands && cell.m_Class != "" && hands.GetType() == cell.m_Class;
				break;
			}

			case CUIQuickSlotAction.QSA_EQUIP:
			{
				string inSlot = "";
				if (entry.slot != "")
				{
					EntityAI equipped = player.GetInventory().FindAttachment(InventorySlots.GetSlotIdFromString(entry.slot));
					if (equipped)
						inSlot = equipped.GetType();
				}
				string spare = FindItem(player, entry.target, true, entry.slot);
				if (spare != "")
				{
					cell.m_Class = spare;
					cell.m_Enabled = true;
				}
				else if (inSlot != "")
				{
					cell.m_Class = inSlot;
					cell.m_Enabled = true;		// clicking takes it off again
					cell.m_Highlight = true;
				}
				break;
			}

			case CUIQuickSlotAction.QSA_CATEGORY:
				cell.m_Count = CountItems(player, entry.target);
				cell.m_Enabled = cell.m_Count > 0;
				break;

			// "The item in hands": with no target the cell follows whatever the player is holding, so the
			// cell is always live and always shows the current item instead of a fixed class.
			case CUIQuickSlotAction.QSA_HANDS:
				if (entry.target == "")
				{
					EntityAI held = null;
					if (player)
						held = player.GetItemInHands();
					if (held)
					{
						cell.m_Class = held.GetType();
						cell.m_Enabled = true;
						cell.m_Highlight = true;
					}
				}
				else
				{
					cell.m_Class = FindItem(player, entry.target, false, "");
					cell.m_Enabled = cell.m_Class != "";
				}
				break;

			case CUIQuickSlotAction.QSA_NONE:
				break;

			default:
				cell.m_Class = FindItem(player, entry.target, false, "");
				cell.m_Enabled = cell.m_Class != "";
				break;
		}

		cell.m_Label = LabelFor(entry.action, entry.target, cell.m_Class);
		cell.m_Icon = IconFor(entry.action, entry.target, cell.m_Class);

		// A cell that has nothing behind it still says what it is for, so the wheel is never a row of
		// blank tiles: the weapon cells read "Weapon", and any cell with a target shows that item's
		// name. This is the caption only; an empty cell stays grey and still does nothing when clicked.
		if (cell.m_Label == "")
		{
			if (action == CUIQuickSlotAction.QSA_WEAPON)
				cell.m_Label = Widget.TranslateString("#STR_CUI_QS_WEAPON");
			else if (entry.target != "")
				cell.m_Label = WRMClient.GetItemName(entry.target);
		}
	}

	// The wheel itself: every position of the current profile, in the order the keys are handed out
	// (bottom, top, left, right, then clockwise).
	static array<ref CUIQSCell> BuildWheel(PlayerBase player)
	{
		array<ref CUIQSCell> cells = new array<ref CUIQSCell>;

		CUIQuickSlotProfileData data = CUIQuickSlotProfile.Get();
		int size = CUIQuickSlotLayout.ClampSize(data.wheelSize);

		TIntArray order;
		CUIQuickSlotLayout.GetCells(size, order);

		for (int i = 0; i < order.Count(); i++)
		{
			int position = order.Get(i);
			CUIQuickSlot entry = CUIQuickSlotProfile.GetCell(position);
			CUIQSCell cell = new CUIQSCell;
			if (entry)
				ResolveCell(cell, entry, player, size);
			else
			{
				cell.m_Index = position;
				cell.m_Key = CUIQuickSlotLayout.KeyForCell(size, position);
				cell.m_Action = "none";
				cell.m_Label = "";
			}
			cells.Insert(cell);
		}
		return cells;
	}

	// The items behind a category cell, as a flat list of cells with key numbers 1..N.
	static array<ref CUIQSCell> BuildCategory(PlayerBase player, string target)
	{
		array<ref CUIQSCell> cells = new array<ref CUIQSCell>;

		array<string> types;
		CollectItems(player, target, types);

		for (int i = 0; i < types.Count(); i++)
		{
			CUIQSCell cell = new CUIQSCell;
			cell.m_Index = i;
			cell.m_Key = i + 1;
			cell.m_Action = "hands";
			cell.m_Target = types.Get(i);
			cell.m_Class = types.Get(i);
			cell.m_Enabled = true;
			cell.m_Label = WRMClient.GetItemName(types.Get(i));
			cell.m_Icon = IconFor("hands", "", types.Get(i));
			cells.Insert(cell);
		}
		return cells;
	}

	// ----- server -----

	static void Hello()
	{
		float now = g_Game.GetTickTime();
		if (s_HelloAt >= 0 && now - s_HelloAt < HELLO_TTL)
			return;
		s_HelloAt = now;
		Send(CUIQSMsg.New(CUIQSOp.QS_HELLO));
	}

	static void Send(WRMMsg msg)
	{
		Man player = g_Game.GetPlayer();
		if (!player)
			return;

		if (g_Game.IsMultiplayer())
		{
			ScriptRPC rpc = new ScriptRPC();
			msg.Write(rpc);
			rpc.Send(player, CUIQS_RPC_ID, true, null);
		}
		else if (s_Local)
		{
			s_Local.HandleLocal(player, msg);
		}
	}

	// Runs one cell. The server looks the item up again; the class sent here is only a hint.
	static bool Act(CUIQSCell cell)
	{
		if (!cell || !cell.m_Enabled)
			return false;

		float now = g_Game.GetTickTime();
		if (now - s_LastAction < ACTION_GAP)
			return false;
		s_LastAction = now;

		WRMMsg msg = CUIQSMsg.New(CUIQSOp.QS_ACTION);
		msg.m_Ints.Insert(cell.m_Index);
		msg.m_Strs.Insert(cell.m_Action);
		msg.m_Strs.Insert(cell.m_Target);
		msg.m_Strs.Insert(cell.m_Slot);
		msg.m_Strs.Insert(cell.Resolved());
		Send(msg);
		return true;
	}

	static void OnMessage(WRMMsg msg)
	{
		if (!msg)
			return;

		switch (msg.m_Op)
		{
			case CUIQSOp.QS_ADMIN:
				s_Admin = msg.GetInt(0) != 0;
				s_AdminKnown = true;
				break;

			case CUIQSOp.QS_RESULT:
				s_LastResult = msg.m_Status;
				s_ResultSeq++;
				SetStatus(StatusText(msg.m_Status, msg.GetStr(0)));
				Invalidate();
				break;
		}
	}

	static void SetStatus(string text)
	{
		s_Status = text;
		s_StatusUntil = g_Game.GetTickTime() + STATUS_TIME;
	}

	static string GetStatus()
	{
		if (g_Game.GetTickTime() > s_StatusUntil)
			return "";
		return s_Status;
	}

	static string StatusText(int status, string cls)
	{
		string name = cls;
		if (cls != "")
			name = WRMClient.GetItemName(cls);

		switch (status)
		{
			case CUIQSStatus.QSS_OK:
				return string.Format(Widget.TranslateString("#STR_CUI_QS_ST_OK"), name);
			case CUIQSStatus.QSS_DENIED:
				return Widget.TranslateString("#STR_CUI_QS_ST_DENIED");
			case CUIQSStatus.QSS_BUSY:
				return Widget.TranslateString("#STR_CUI_QS_ST_BUSY");
			case CUIQSStatus.QSS_NOT_FOUND:
				return Widget.TranslateString("#STR_CUI_QS_ST_NOT_FOUND");
			case CUIQSStatus.QSS_NO_ROOM:
				return Widget.TranslateString("#STR_CUI_QS_ST_NO_ROOM");
			case CUIQSStatus.QSS_RATE:
				return Widget.TranslateString("#STR_CUI_QS_ST_RATE");
			case CUIQSStatus.QSS_UNSUPPORTED:
				return Widget.TranslateString("#STR_CUI_QS_ST_UNSUPPORTED");
		}
		return Widget.TranslateString("#STR_CUI_QS_ST_FAILED");
	}

	static bool Admin()
	{
		return s_Admin;
	}

	static void Reset()
	{
		Invalidate();
		s_Admin = false;
		s_AdminKnown = false;
		s_HelloAt = -1;
		s_Status = "";
		s_StatusUntil = 0;
		s_ResultSeq = 0;
		s_LastResult = -1;
		s_LastAction = -1000;
		s_ArsenalRequested = false;
		s_DeepRequested = false;
	}
}
