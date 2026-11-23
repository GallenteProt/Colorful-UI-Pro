// Self test for the quick slot wheel profile and layout.
//
// Runs once on the client at mission start when `Settings.QuickSlotsSelfTest` is true. It exists
// because the profile logic (defaults, sanitising, key assignment, JSON round trip) is the part
// that decides what every cell does, and a mistake there is invisible until a player opens the
// wheel. Each check prints a single PASS/FAIL line so the result can be read straight out of
// script.log and diffed between runs.

class CUIQuickSlotSelfTest
{
	static const string TAG = "[CUI-QuickSlots-test]";

	protected static bool s_Done;
	protected static int s_Passed;

	// Called from a path that always runs once the engine is up (localisation). Guarded so it runs
	// at most once per session, and does nothing at all while the flag is off.
	//
	// It deliberately does not filter on IsClient(): the profile and layout logic is shared by both
	// layers, and running the checks on a server as well is extra evidence for free.
	static void RunOnce()
	{
		if (s_Done)
			return;
		if (!QuickSlotsSelfTest)
			return;

		s_Done = true;
		MakeDirectory(CUIQuickSlotProfile.DIR);
		Run();
	}

	static void Run()
	{
		int failed = 0;
		s_Passed = 0;

		failed += CheckLayoutKeys();
		failed += CheckClampSize();
		failed += CheckDefaultProfile();
		failed += CheckSanitize();
		failed += CheckRoundTrip();

		if (failed == 0)
			Print(TAG + " ALL PASS (" + s_Passed + " checks)");
		else
			Print(TAG + " FAILURES: " + failed + " (passed " + s_Passed + ")");
	}

	// Bottom is key 1 and top is key 2 at every size, because those two cells are the weapon slots.
	protected static int CheckLayoutKeys()
	{
		int failed = 0;

		if (CUIQuickSlotLayout.KeyForCell(4, CUIQuickSlotPosition.BOTTOM) != 1) failed += Fail("4 cells: bottom must be key 1");
		if (CUIQuickSlotLayout.KeyForCell(4, CUIQuickSlotPosition.TOP) != 2) failed += Fail("4 cells: top must be key 2");
		if (CUIQuickSlotLayout.KeyForCell(4, CUIQuickSlotPosition.LEFT) != 3) failed += Fail("4 cells: left must be key 3");
		if (CUIQuickSlotLayout.KeyForCell(4, CUIQuickSlotPosition.RIGHT) != 4) failed += Fail("4 cells: right must be key 4");

		if (CUIQuickSlotLayout.KeyForCell(8, CUIQuickSlotPosition.BOTTOM) != 1) failed += Fail("8 cells: bottom must be key 1");
		if (CUIQuickSlotLayout.KeyForCell(8, CUIQuickSlotPosition.TOP) != 2) failed += Fail("8 cells: top must be key 2");

		// A cell outside the wheel has no key.
		if (CUIQuickSlotLayout.KeyForCell(4, CUIQuickSlotPosition.TOP_LEFT) != 0) failed += Fail("4 cells: top-left must have no key");

		// Inverse mapping must agree.
		if (CUIQuickSlotLayout.CellForKey(6, 1) != CUIQuickSlotPosition.BOTTOM) failed += Fail("6 cells: key 1 must be bottom");
		if (CUIQuickSlotLayout.CellForKey(6, 2) != CUIQuickSlotPosition.TOP) failed += Fail("6 cells: key 2 must be top");
		if (CUIQuickSlotLayout.CellForKey(6, 9) != -1) failed += Fail("6 cells: key 9 must be unused");

		if (failed == 0) { s_Passed++; Print(TAG + " PASS layout keys"); }
		return failed;
	}

	protected static int CheckClampSize()
	{
		int failed = 0;
		if (CUIQuickSlotLayout.ClampSize(3) != 4) failed += Fail("clamp 3 -> 4");
		if (CUIQuickSlotLayout.ClampSize(4) != 4) failed += Fail("clamp 4 -> 4");
		if (CUIQuickSlotLayout.ClampSize(5) != 6) failed += Fail("clamp 5 -> 6");
		if (CUIQuickSlotLayout.ClampSize(6) != 6) failed += Fail("clamp 6 -> 6");
		if (CUIQuickSlotLayout.ClampSize(7) != 8) failed += Fail("clamp 7 -> 8");
		if (CUIQuickSlotLayout.ClampSize(9) != 8) failed += Fail("clamp 9 -> 8");
		if (failed == 0) { s_Passed++; Print(TAG + " PASS clamp size"); }
		return failed;
	}

	protected static int CheckDefaultProfile()
	{
		int failed = 0;
		CUIQuickSlotProfileData data = CUIQuickSlotProfile.BuildDefault();

		if (data.wheelSize != 8) failed += Fail("default wheel size must be 8");
		if (data.slots.Count() != 8) failed += Fail("default must define 8 cells, got " + data.slots.Count());

		CUIQuickSlot bottom = FindIn(data, CUIQuickSlotPosition.BOTTOM);
		CUIQuickSlot top = FindIn(data, CUIQuickSlotPosition.TOP);
		if (!bottom || bottom.action != "weapon") failed += Fail("default bottom must be a weapon cell");
		if (!top || top.action != "weapon") failed += Fail("default top must be a weapon cell");

		// Every default action must parse back to something that is not "none".
		for (int i = 0; i < data.slots.Count(); i++)
		{
			CUIQuickSlot cell = data.slots.Get(i);
			int action = CUIQuickSlotActions.FromString(cell.action);
			if (action == CUIQuickSlotAction.QSA_NONE)
				failed += Fail("default cell " + cell.index + " has unparsable action '" + cell.action + "'");
			if (CUIQuickSlotActions.NeedsTarget(action) && cell.target == "")
				failed += Fail("default cell " + cell.index + " needs a target but has none");
		}

		if (failed == 0) { s_Passed++; Print(TAG + " PASS default profile"); }
		return failed;
	}

	protected static int CheckSanitize()
	{
		int failed = 0;
		CUIQuickSlotProfileData data = new CUIQuickSlotProfileData();
		data.wheelSize = 5;	// unsupported on purpose: must clamp to 6

		// Duplicate position: the second one must be dropped.
		data.slots.Insert(MakeCell(CUIQuickSlotPosition.BOTTOM, "weapon", "", ""));
		data.slots.Insert(MakeCell(CUIQuickSlotPosition.BOTTOM, "hands", "PDA", ""));
		// Unknown action: must become an empty cell, not a cell that does nothing.
		data.slots.Insert(MakeCell(CUIQuickSlotPosition.LEFT, "teleport", "X", ""));
		// Action that needs a target but has none: must become empty.
		data.slots.Insert(MakeCell(CUIQuickSlotPosition.RIGHT, "hands", "", ""));
		// Position outside the wheel: must be dropped.
		data.slots.Insert(MakeCell(CUIQuickSlotPosition.TOP_LEFT, "hands", "PDA", ""));

		CUIQuickSlotProfile.SanitizePublic(data);

		if (data.wheelSize != 6) failed += Fail("sanitize must clamp size 5 to 6, got " + data.wheelSize);
		if (data.slots.Count() != 6) failed += Fail("sanitize must produce one cell per position (6), got " + data.slots.Count());

		CUIQuickSlot bottom = FindIn(data, CUIQuickSlotPosition.BOTTOM);
		if (!bottom || bottom.action != "weapon") failed += Fail("sanitize must keep the first definition of a duplicated position");

		CUIQuickSlot left = FindIn(data, CUIQuickSlotPosition.LEFT);
		if (!left || left.action != "none") failed += Fail("sanitize must empty a cell with an unknown action");

		CUIQuickSlot right = FindIn(data, CUIQuickSlotPosition.RIGHT);
		if (!right || right.action != "none") failed += Fail("sanitize must empty an action that lost its target");

		if (FindIn(data, CUIQuickSlotPosition.TOP_LEFT) != null)
			failed += Fail("sanitize must drop a position outside the wheel size");

		if (failed == 0) { s_Passed++; Print(TAG + " PASS sanitize"); }
		return failed;
	}

	protected static int CheckRoundTrip()
	{
		int failed = 0;
		string path = "$profile:ColorfulUI/quickslots_selftest.json";

		CUIQuickSlotProfileData before = CUIQuickSlotProfile.BuildDefault();
		string saveErr;
		if (!JsonFileLoader<CUIQuickSlotProfileData>.SaveFile(path, before, saveErr))
		{
			failed += Fail("round trip: save failed: " + saveErr);
			return failed;
		}

		CUIQuickSlotProfileData after = new CUIQuickSlotProfileData();
		string loadErr;
		if (!JsonFileLoader<CUIQuickSlotProfileData>.LoadFile(path, after, loadErr))
		{
			failed += Fail("round trip: load failed: " + loadErr);
			return failed;
		}

		if (after.schemaVersion != before.schemaVersion) failed += Fail("round trip: schemaVersion changed");
		if (after.wheelSize != before.wheelSize) failed += Fail("round trip: wheelSize changed");
		if (after.slots.Count() != before.slots.Count()) failed += Fail("round trip: cell count changed");

		for (int i = 0; i < before.slots.Count(); i++)
		{
			CUIQuickSlot a = before.slots.Get(i);
			CUIQuickSlot b = FindIn(after, a.index);
			if (!b)
			{
				failed += Fail("round trip: lost cell " + a.index);
				continue;
			}
			if (b.action != a.action) failed += Fail("round trip: cell " + a.index + " action changed");
			if (b.target != a.target) failed += Fail("round trip: cell " + a.index + " target changed");
			if (b.slot != a.slot) failed += Fail("round trip: cell " + a.index + " slot changed");
		}

		DeleteFile(path);

		if (failed == 0) { s_Passed++; Print(TAG + " PASS round trip"); }
		return failed;
	}

	protected static CUIQuickSlot FindIn(CUIQuickSlotProfileData data, int index)
	{
		for (int i = 0; i < data.slots.Count(); i++)
		{
			CUIQuickSlot cell = data.slots.Get(i);
			if (cell && cell.index == index)
				return cell;
		}
		return null;
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

	protected static int Fail(string message)
	{
		Print(TAG + " FAIL " + message);
		return 1;
	}
}
