// Quick slot wheel: the player's profile.
//
// Client side, per player: $profile:ColorfulUI/quick_slots.json
//
// Why a client file is safe: the server never creates an item on request. Every action works only
// with items that are already in the player's inventory, so editing this file can change how the
// player calls an action, but cannot give access to an item the player does not own. The server
// validates state, ownership, compatibility and rate on every request regardless of what the
// client claims.
//
// A file that exists but cannot be parsed is never overwritten: the error is logged and defaults
// are used in memory, the same rule the weapon radial menu already follows.

class CUIQuickSlotProfileData
{
	int schemaVersion = 1;
	int wheelSize = 8;	// 4, 6 or 8; anything else is clamped to the nearest supported size
	ref array<ref CUIQuickSlot> slots = new array<ref CUIQuickSlot>;
}

class CUIQuickSlotProfile
{
	static const int SCHEMA_VERSION = 1;
	static const string DIR = "$profile:ColorfulUI";
	static const string FILE = "$profile:ColorfulUI/quick_slots.json";
	static const string BACKUP = "$profile:ColorfulUI/quick_slots.json.bak";

	protected static ref CUIQuickSlotProfileData s_Data;

	static CUIQuickSlotProfileData Get()
	{
		if (!s_Data)
			Load();
		return s_Data;
	}

	static void Load()
	{
		// The base is the shipped default, not an empty profile: a fresh player must get a wheel with
		// real cells, and an unreadable file must fall back to that same default in memory.
		CUIQuickSlotProfileData data = BuildDefault();
		bool loaded = false;
		bool existed = FileExist(FILE);

		MakeDirectory(DIR);
		if (existed)
		{
			CUIQuickSlotProfileData read = new CUIQuickSlotProfileData();
			string err;
			if (JsonFileLoader<CUIQuickSlotProfileData>.LoadFile(FILE, read, err))
			{
				data = read;
				loaded = true;
			}
			else
			{
				Print("[CUI-QuickSlots] quick_slots.json is invalid, file left untouched, using defaults in memory: " + err);
			}
		}

		bool migrated = loaded && data.schemaVersion < SCHEMA_VERSION;
		if (migrated)
		{
			// Keep the old file around before the shape changes under it.
			if (!CopyFile(FILE, BACKUP))
				Print("[CUI-QuickSlots] could not back up quick_slots.json before migrating");
			data.schemaVersion = SCHEMA_VERSION;
			Print("[CUI-QuickSlots] migrated quick_slots.json to schema " + SCHEMA_VERSION);
		}

		s_Data = data;
		Sanitize(s_Data);

		// Write when there was no file at all (first run), and when an old file was migrated. A file that
		// exists but failed to parse stays exactly as it is: the defaults above are used in memory only.
		if (!existed)
		{
			string saveErr;
			if (!JsonFileLoader<CUIQuickSlotProfileData>.SaveFile(FILE, s_Data, saveErr))
				Print("[CUI-QuickSlots] could not write default quick_slots.json: " + saveErr);
		}
		else if (migrated)
		{
			string migErr;
			if (!JsonFileLoader<CUIQuickSlotProfileData>.SaveFile(FILE, s_Data, migErr))
				Print("[CUI-QuickSlots] could not write migrated quick_slots.json: " + migErr);
		}
	}

	static void Save()
	{
		if (!s_Data)
			return;

		MakeDirectory(DIR);
		string err;
		if (!JsonFileLoader<CUIQuickSlotProfileData>.SaveFile(FILE, s_Data, err))
			Print("[CUI-QuickSlots] could not write quick_slots.json: " + err);
	}

	// Replace the profile with the shipped defaults and keep the previous one as .bak.
	static void ResetToDefault()
	{
		if (FileExist(FILE) && !CopyFile(FILE, BACKUP))
			Print("[CUI-QuickSlots] could not back up quick_slots.json before reset");

		s_Data = BuildDefault();
		Save();
	}

	static CUIQuickSlotProfileData BuildDefault()
	{
		CUIQuickSlotProfileData data = new CUIQuickSlotProfileData();
		data.schemaVersion = SCHEMA_VERSION;
		data.wheelSize = 8;

		// Bottom and top are the two weapon cells, always: key 1 is the first weapon, key 2 the second.
		// The other six follow the requested layout, clockwise from the left:
		//   3 left gas mask, 4 right PDA, 5 bottom-right medkit, 6 bottom-left knife,
		//   7 top-left anomaly scanner, 8 top-right survey meter.
		// The last three are MOONLIGHT ModPack items, so they use that pack's real class names; with the
		// pack loaded they resolve and show their own model, without it they read as empty cells.
		Add(data, CUIQuickSlotPosition.BOTTOM, "weapon", "", "");
		Add(data, CUIQuickSlotPosition.TOP, "weapon", "", "");
		Add(data, CUIQuickSlotPosition.LEFT, "equip", "GP5GasMask", "Mask");
		Add(data, CUIQuickSlotPosition.RIGHT, "hands", "KPK_PDA_Item", "");
		Add(data, CUIQuickSlotPosition.LOW_RIGHT, "category", "@med", "");
		Add(data, CUIQuickSlotPosition.LOW_LEFT, "hands", "HuntingKnife", "");
		Add(data, CUIQuickSlotPosition.TOP_LEFT, "hands", "C13_AnomalyScanner", "");
		Add(data, CUIQuickSlotPosition.TOP_RIGHT, "hands", "BMM_Items_Hil_SurveyMeter", "");

		return data;
	}

	protected static void Add(CUIQuickSlotProfileData data, int index, string action, string target, string slot)
	{
		data.slots.Insert(MakeCell(index, action, target, slot));
	}

	protected static CUIQuickSlot MakeCell(int index, string action, string target, string slot)
	{
		CUIQuickSlot cell = new CUIQuickSlot();
		cell.index = index;
		cell.action = action;
		cell.target = target;
		cell.slot = slot;
		return cell;
	}

	// Make any loaded profile safe to render: supported size, exactly one cell per position, no
	// duplicates, no unknown actions, no action that needs a target without one.
	// Public wrapper so the self test can exercise exactly the code the loader runs.
	static void SanitizePublic(CUIQuickSlotProfileData data)
	{
		Sanitize(data);
	}

	protected static void Sanitize(CUIQuickSlotProfileData data)
	{
		if (!data.slots)
			data.slots = new array<ref CUIQuickSlot>;

		data.wheelSize = CUIQuickSlotLayout.ClampSize(data.wheelSize);

		ref array<ref CUIQuickSlot> cleaned = new array<ref CUIQuickSlot>;
		ref map<int, bool> seen = new map<int, bool>;

		for (int i = 0; i < data.slots.Count(); i++)
		{
			CUIQuickSlot cell = data.slots.Get(i);
			if (!cell)
				continue;

			// A position outside this wheel size cannot be drawn, drop it rather than render it off-wheel.
			if (cell.index < 0 || cell.index >= 8)
				continue;
			if (CUIQuickSlotLayout.KeyForCell(data.wheelSize, cell.index) == 0)
				continue;

			// First definition of a position wins; a duplicate would make the wheel ambiguous.
			if (seen.Contains(cell.index))
				continue;
			seen.Insert(cell.index, true);

			int action = CUIQuickSlotActions.FromString(cell.action);
			if (action == CUIQuickSlotAction.QSA_NONE)
			{
				// Unknown action name: keep the cell but empty it, so the player sees a hole
				// instead of a cell that does nothing when clicked.
				cell.action = "none";
				cell.target = "";
				cell.slot = "";
				cleaned.Insert(cell);
				continue;
			}

			if (CUIQuickSlotActions.NeedsTarget(action) && cell.target == "")
			{
				cell.action = "none";
				cell.slot = "";
				cleaned.Insert(cell);
				continue;
			}

			// Store the canonical name so the file stays readable and casing mistakes heal themselves.
			cell.action = CUIQuickSlotActions.ToString(action);
			cleaned.Insert(cell);
		}

		// Fill every position of the wheel that had no entry at all.
		TIntArray cells;
		CUIQuickSlotLayout.GetCells(data.wheelSize, cells);
		for (int c = 0; c < cells.Count(); c++)
		{
			int position = cells.Get(c);
			if (seen.Contains(position))
				continue;
			cleaned.Insert(MakeCell(position, "none", "", ""));
		}

		data.slots = cleaned;
	}

	// The cell configured for a wheel position, or null when the position is empty.
	static CUIQuickSlot GetCell(int position)
	{
		CUIQuickSlotProfileData data = Get();
		for (int i = 0; i < data.slots.Count(); i++)
		{
			CUIQuickSlot cell = data.slots.Get(i);
			if (cell && cell.index == position)
				return cell;
		}
		return null;
	}

	// Bind a position to an action. Used by the settings UI and by "assign from hover".
	static bool SetCell(int position, string action, string target, string slot)
	{
		CUIQuickSlotProfileData data = Get();
		int parsed = CUIQuickSlotActions.FromString(action);
		if (parsed == CUIQuickSlotAction.QSA_NONE)
			return false;
		if (CUIQuickSlotActions.NeedsTarget(parsed) && target == "")
			return false;

		CUIQuickSlot cell = GetCell(position);
		if (!cell)
		{
			cell = new CUIQuickSlot();
			cell.index = position;
			data.slots.Insert(cell);
		}

		cell.action = CUIQuickSlotActions.ToString(parsed);
		cell.target = target;
		cell.slot = slot;
		return true;
	}

	static void ClearCell(int position)
	{
		CUIQuickSlot cell = GetCell(position);
		if (!cell)
			return;
		cell.action = "none";
		cell.target = "";
		cell.slot = "";
	}

	static int GetWheelSize()
	{
		return Get().wheelSize;
	}

	static void SetWheelSize(int wheelSize)
	{
		CUIQuickSlotProfileData data = Get();
		data.wheelSize = CUIQuickSlotLayout.ClampSize(wheelSize);
		Sanitize(data);
	}
}
