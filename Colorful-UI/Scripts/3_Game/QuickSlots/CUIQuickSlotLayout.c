// Quick slot wheel: geometry and key assignment.
//
// Keys are bound to positions, not to clockwise order. The bottom cell is always key 1 and the top
// cell is always key 2, whatever the wheel size is, because those two cells are the weapon slots.
// The remaining cells take 3, 4, 5... clockwise starting from the right.

class CUIQuickSlotPosition
{
	static const int TOP = 0;
	static const int TOP_RIGHT = 1;
	static const int RIGHT = 2;
	static const int LOW_RIGHT = 3;
	static const int BOTTOM = 4;
	static const int LOW_LEFT = 5;
	static const int LEFT = 6;
	static const int TOP_LEFT = 7;
}

class CUIQuickSlotLayout
{
	static const int SIZE_MIN = 4;
	static const int SIZE_MAX = 8;
	static const int KEY_FOR_DEEP_ACTION = KeyCode.KC_F;	// F on a hovered weapon cell opens attachments / magazines / repair

	// Angle in degrees, 0 = top, clockwise. The menu turns this into a screen position.
	protected static const string ANGLES = "0,45,90,135,180,225,270,315";
	// Cell names, index = CUIQuickSlotPosition, for logging and config editing.
	protected static const string POS_NAMES = "top,top_right,right,low_right,bottom,low_left,left,top_left";

	// The cells of a wheel of the given size, in the order they receive keys.
	// 4: bottom, top, left, right.
	// 6: those four plus the two lower cells.
	// 8: all eight.
	static void GetCells(int wheelSize, out TIntArray cells)
	{
		cells = new TIntArray;

		if (wheelSize < SIZE_MIN)
			wheelSize = SIZE_MIN;
		if (wheelSize > SIZE_MAX)
			wheelSize = SIZE_MAX;

		cells.Insert(CUIQuickSlotPosition.BOTTOM);
		cells.Insert(CUIQuickSlotPosition.TOP);
		cells.Insert(CUIQuickSlotPosition.LEFT);
		cells.Insert(CUIQuickSlotPosition.RIGHT);

		if (wheelSize >= 6)
		{
			cells.Insert(CUIQuickSlotPosition.LOW_RIGHT);
			cells.Insert(CUIQuickSlotPosition.LOW_LEFT);
		}
		if (wheelSize >= 8)
		{
			cells.Insert(CUIQuickSlotPosition.TOP_LEFT);
			cells.Insert(CUIQuickSlotPosition.TOP_RIGHT);
		}
	}

	// Key number (1..8) for a cell of a wheel of the given size, 0 if the cell is not part of it.
	static int KeyForCell(int wheelSize, int cell)
	{
		TIntArray cells;
		GetCells(wheelSize, cells);
		for (int i = 0; i < cells.Count(); i++)
		{
			if (cells.Get(i) == cell)
				return i + 1;
		}
		return 0;
	}

	// Inverse of KeyForCell. Returns -1 for a key the wheel does not use.
	static int CellForKey(int wheelSize, int key)
	{
		TIntArray cells;
		GetCells(wheelSize, cells);
		int index = key - 1;
		if (index < 0 || index >= cells.Count())
			return -1;
		return cells.Get(index);
	}

	static float AngleForCell(int cell)
	{
		TStringArray parts = new TStringArray;
		string angles = ANGLES;
		angles.Split(",", parts);
		if (cell < 0 || cell >= parts.Count())
			return 0;
		return parts.Get(cell).ToFloat();
	}

	static string NameForCell(int cell)
	{
		TStringArray parts = new TStringArray;
		string names = POS_NAMES;
		names.Split(",", parts);
		if (cell < 0 || cell >= parts.Count())
			return "unknown";
		return parts.Get(cell);
	}

	// Is this wheel size one we actually support?
	static bool IsSupportedSize(int wheelSize)
	{
		return wheelSize == 4 || wheelSize == 6 || wheelSize == 8;
	}

	// Normalize any number to the nearest supported size, so a hand-edited profile cannot produce
	// a wheel that half-renders.
	static int ClampSize(int wheelSize)
	{
		if (wheelSize <= 4)
			return 4;
		if (wheelSize <= 6)
			return 6;
		return 8;
	}
}
