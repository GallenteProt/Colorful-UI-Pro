// Quick slot wheel: action types and the per-cell descriptor.
//
// A cell is not an item. It is an action, and the same item can behave differently in two cells
// (a dosimeter can be "toggle" in one cell and "take in hands" in another). That is why the action
// type is part of the profile instead of being derived from the class name.

enum CUIQuickSlotAction
{
	QSA_NONE = 0,		// empty cell, drawn as a dimmed placeholder
	QSA_WEAPON = 1,		// take own weapon in hands; the deep action (key F) opens attachments / magazines / repair
	QSA_EQUIP = 2,		// equip into a clothing slot and swap with whatever was there
	QSA_CATEGORY = 3,	// open a category built from the player's own inventory (e.g. "apteka")
	QSA_USE = 4,		// apply immediately (bandage, syringe, tablet)
	QSA_TOGGLE = 5,		// switch a device on or off (dosimeter, flashlight)
	QSA_HANDS = 6,		// take the item in hands (PDA, radio, artifact analyzer)
	QSA_REPAIR = 7,		// repair, enabled only while the required material is present
	QSA_FILTER = 8		// swap the gas mask filter
}

// One cell of the wheel. Kept a plain serializable class so JsonFileLoader can read it directly.
class CUIQuickSlot
{
	int index;			// 0..7, position on the wheel, see CUIQuickSlotLayout
	string action = "";	// one of the CUIQuickSlotAction names, stored as text so a profile stays readable and forwards compatible
	string target = "";	// item class name, or category id for QSA_CATEGORY
	string slot = "";	// clothing slot name for QSA_EQUIP, e.g. "Mask"

	void CUIQuickSlot()
	{
	}
}

// Text <-> enum. The profile stores names, not numbers: a number would silently change meaning if
// the enum is ever reordered, a name either matches or fails loudly.
class CUIQuickSlotActions
{
	static const string NAMES = "none,weapon,equip,category,use,toggle,hands,repair,filter";

	static int FromString(string name)
	{
		string wanted = name;
		wanted = wanted.Trim();
		wanted.ToLower();

		switch (wanted)
		{
			case "none":		return CUIQuickSlotAction.QSA_NONE;
			case "weapon":		return CUIQuickSlotAction.QSA_WEAPON;
			case "equip":		return CUIQuickSlotAction.QSA_EQUIP;
			case "category":	return CUIQuickSlotAction.QSA_CATEGORY;
			case "use":			return CUIQuickSlotAction.QSA_USE;
			case "toggle":		return CUIQuickSlotAction.QSA_TOGGLE;
			case "hands":		return CUIQuickSlotAction.QSA_HANDS;
			case "repair":		return CUIQuickSlotAction.QSA_REPAIR;
			case "filter":		return CUIQuickSlotAction.QSA_FILTER;
		}
		return CUIQuickSlotAction.QSA_NONE;
	}

	static string ToString(int action)
	{
		switch (action)
		{
			case CUIQuickSlotAction.QSA_NONE:		return "none";
			case CUIQuickSlotAction.QSA_WEAPON:		return "weapon";
			case CUIQuickSlotAction.QSA_EQUIP:		return "equip";
			case CUIQuickSlotAction.QSA_CATEGORY:	return "category";
			case CUIQuickSlotAction.QSA_USE:		return "use";
			case CUIQuickSlotAction.QSA_TOGGLE:		return "toggle";
			case CUIQuickSlotAction.QSA_HANDS:		return "hands";
			case CUIQuickSlotAction.QSA_REPAIR:		return "repair";
			case CUIQuickSlotAction.QSA_FILTER:		return "filter";
		}
		return "none";
	}

	// Does this action need a target class or category id to mean anything?
	static bool NeedsTarget(int action)
	{
		if (action == CUIQuickSlotAction.QSA_NONE)
			return false;
		if (action == CUIQuickSlotAction.QSA_WEAPON)
			return false;	// the weapon comes from the player's own inventory, not from the cell
		return true;
	}
}
