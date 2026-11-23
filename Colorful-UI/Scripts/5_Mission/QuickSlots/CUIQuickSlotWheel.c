// Quick slot wheel: the menu.
//
// Built on the same vanilla radial base as the weapon menu (WRMMenu): the vanilla
// gui/layouts/radial_menu/radial_gestures/day_z_gestures.layout supplies the ring, the sector
// delimiters and the selection, and RadialMenu picks the cell under the cursor by the angle from the
// centre. That is why the wheel looks and behaves like the weapon menu instead of a hand drawn canvas
// that has to position and hit test every card itself.
//
// Keys stay bound to positions, not to drawing order: the bottom cell is key 1 and the top cell is
// key 2 whatever the wheel size is, because those two are the weapon cells. The cells are inserted into
// the radial in screen order (slot 0 is the bottom, the slots run clockwise), so a position lands on the
// slot the player expects and a key keeps its place on screen.

class CUIQuickSlotWheel extends UIScriptedMenu
{
	static const int MENU_ID = 5718606;

	// Same vanilla root and delimiter as WRMMenu: that is what draws the ring and the sector dividers.
	static const string LAYOUT_ROOT = "gui/layouts/radial_menu/radial_gestures/day_z_gestures.layout";
	static const string LAYOUT_DELIMITER = "gui/layouts/radial_menu/radial_gestures/day_z_gesture_delimiter.layout";
	static const string LAYOUT_CARD = "Colorful-UI/GUI/layouts/quickslots/qs_card.layout";
	static const string LAYOUT_CENTER = "Colorful-UI/GUI/layouts/weapon_radial/wrm_center.layout";

	static const float REFRESH_INTERVAL = 0.25;	// re-reads the inventory this often, so a used bandage disappears
	static const float KEY_DEDUP = 0.2;
	static const int KEY_ESC = -1;
	static const int MIN_CARDS = 2;				// a one card wheel breaks the vanilla hit testing and selector

	// Atlas entries that always exist in dayz_inventory.imageset, used when a cell has no item picture of
	// its own: the back arrow, and a neutral placeholder so a cell is never blank.
	// The back cell picture. It has to be an arrow, not the vanilla inventory "back" entry: that one is
	// the BACKPACK slot icon, which is why the way out used to look like a rucksack. The mod's own icon
	// atlas has a chevron for exactly this.
	static const string ICON_BACK = "set:cui image:ChevronLeft";
	static const string ICON_EMPTY = "missing";

	protected Widget m_CardPanel;
	protected RichTextWidget m_CenterText;
	protected ref array<ref CUIQSCell> m_Cells = new array<ref CUIQSCell>;
	protected ref WRMPreviewSet m_Previews = new WRMPreviewSet;		// owned by this instance, never shared
	protected string m_Signature;
	protected string m_CenterShown;
	protected string m_HoverLabel;
	protected CUIQSCell m_Hover;
#ifdef DIAG_DEVELOPER
	protected string m_HoverLogged;		// last hovered cell written to the log, so it logs once per change
#endif
	protected float m_Timer;
	protected bool m_Applied;

	protected bool m_Category;			// second level: the items of one category
	protected string m_CatTarget;
	protected string m_CatTitle;

	protected bool m_Actions;			// second level: the actions of one equipped cell (filter, repair, back)
	protected string m_ActionSlot;
	protected string m_ActionTarget;

	protected ref array<int> m_KeyCodes = new array<int>;
	protected ref array<bool> m_KeyHeld = new array<bool>;
	protected ref array<float> m_KeyTime = new array<float>;
	protected bool m_MmbHeld;			// middle button polled: the vanilla RadialMenu has no callback for it
	protected bool m_FHeld;

	void ~CUIQuickSlotWheel()
	{
		RestoreWorld();
		ReleaseCards();
	}

	// ----- lifecycle -----

	override Widget Init()
	{
		layoutRoot = g_Game.GetWorkspace().CreateWidgets(LAYOUT_ROOT);
		m_CardPanel = layoutRoot.FindAnyWidget(RadialMenu.RADIAL_ITEM_CARD_CONTAINER);

		// the vanilla title is a tiny non-wrapping text: our centre text replaces it
		Widget vanillaTitle = layoutRoot.FindAnyWidget("CategoryName");
		if (vanillaTitle)
			vanillaTitle.Show(false);

		Widget wheel = layoutRoot.FindAnyWidget("GesturesPanel");
		if (wheel)
			m_CenterText = RichTextWidget.Cast(g_Game.GetWorkspace().CreateWidgets(LAYOUT_CENTER, wheel));

		// the layout creates a fresh RadialMenu handler; it is a singleton driven by method name on this class
		RadialMenu radial = RadialMenu.GetInstance();
		radial.RegisterClass(this);
		radial.SetWidgetInitialized(false);
		radial.SetWidgetProperties(LAYOUT_DELIMITER);

		Widget toolbar = layoutRoot.FindAnyWidget("toolbar_bg");
		if (toolbar)
			toolbar.Show(false);		// its captions are text and it covered the bottom cell

		InitKeys();
		m_MmbHeld = IsMiddleDown();		// a button held while opening is not a click
		CUIQuickSlotLocal.Hello();		// asks the server once whether this player may open the arsenal
		Rebuild(true);
#ifdef DIAG_DEVELOPER
		bool hasRoot = (layoutRoot != null);
		bool hasCards = (m_CardPanel != null);
		bool hasCentre = (m_CenterText != null);
		Print("[CUI-QS] wheel opened: root=" + hasRoot + " cards=" + hasCards + " centre=" + hasCentre + " cells=" + m_Cells.Count());
#endif
		return layoutRoot;
	}

	override void OnShow()
	{
		super.OnShow();

		Mission mission = g_Game.GetMission();
		if (mission)
		{
			mission.AddActiveInputExcludes({"radialmenu"});
			IngameHud hud = IngameHud.Cast(mission.GetHud());
			if (hud)
			{
				hud.ShowQuickbarUI(false);
				hud.ShowHudUI(false);
			}
			m_Applied = true;
		}

		SetFocus(layoutRoot);
	}

	override void OnHide()
	{
		super.OnHide();
		RestoreWorld();
		ReleaseCards();
	}

	// Safe to call repeatedly; called from OnHide and from the destructor (disconnect, mission end, death).
	protected void RestoreWorld()
	{
		if (!m_Applied)
			return;
		m_Applied = false;

		if (!g_Game)
			return;
		Mission mission = g_Game.GetMission();
		if (!mission)
			return;

		// force-suppress: the key that closed the menu must not act in the next frame
		mission.RemoveActiveInputExcludes({"radialmenu"}, true);
		IngameHud hud = IngameHud.Cast(mission.GetHud());
		if (hud)
		{
			hud.ShowQuickbarUI(true);
			hud.ShowHudUI(true);
		}
	}

	override bool UseMouse()
	{
		return true;
	}

	// The open key is a mod input outside the "ui" context, so keyboard focus stays with the game and a
	// second press of it (read in MissionGameplay) closes the wheel. Keys are polled instead.
	override bool UseKeyboard()
	{
		return false;
	}

	override bool UseGamepad()
	{
		return true;
	}

	override bool OnController(Widget w, int control, int value)
	{
		super.OnController(w, control, value);
		RadialMenu.GetInstance().SetControlType(RadialMenuControlType.CONTROLLER);
		return false;
	}

	override bool OnMouseEnter(Widget w, int x, int y)
	{
		super.OnMouseEnter(w, x, y);
		RadialMenu.GetInstance().SetControlType(RadialMenuControlType.MOUSE);
		return false;
	}

	override void Update(float timeslice)
	{
		super.Update(timeslice);

		PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
		if (!player || !player.IsAlive() || player.IsUnconscious() || player.IsRestrained())
		{
			Close();
			return;
		}

		PollKeys();
		PollMiddle();
		PollDeep();

		m_Timer += timeslice;
		if (m_Timer >= REFRESH_INTERVAL)
		{
			m_Timer = 0;
			CUIQuickSlotLocal.Invalidate();
			Rebuild(false);
		}

		UpdateCenterText();
	}

	// ----- keys -----

	protected void AddKey(int key)
	{
		m_KeyCodes.Insert(key);
		m_KeyHeld.Insert(false);
		m_KeyTime.Insert(-1000);
	}

	protected void InitKeys()
	{
		m_KeyCodes.Clear();
		m_KeyHeld.Clear();
		m_KeyTime.Clear();

		AddKey(KeyCode.KC_1);
		AddKey(KeyCode.KC_2);
		AddKey(KeyCode.KC_3);
		AddKey(KeyCode.KC_4);
		AddKey(KeyCode.KC_5);
		AddKey(KeyCode.KC_6);
		AddKey(KeyCode.KC_7);
		AddKey(KeyCode.KC_8);
		AddKey(KeyCode.KC_NUMPAD1);
		AddKey(KeyCode.KC_NUMPAD2);
		AddKey(KeyCode.KC_NUMPAD3);
		AddKey(KeyCode.KC_NUMPAD4);
		AddKey(KeyCode.KC_NUMPAD5);
		AddKey(KeyCode.KC_NUMPAD6);
		AddKey(KeyCode.KC_NUMPAD7);
		AddKey(KeyCode.KC_NUMPAD8);
		AddKey(KeyCode.KC_ESCAPE);
	}

	protected void PollKeys()
	{
		for (int i = 0; i < m_KeyCodes.Count(); i++)
		{
			int code = m_KeyCodes.Get(i);
			if (KeyState(code) == 0)
			{
				m_KeyHeld.Set(i, false);
				continue;
			}
			if (m_KeyHeld.Get(i))
				continue;

			m_KeyHeld.Set(i, true);
			m_KeyTime.Set(i, g_Game.GetTickTime());

			if (code == KeyCode.KC_ESCAPE)
			{
				Back();
				return;
			}
			int number = KeyNumber(code);
			if (number > 0)
				SelectKey(number);
		}
	}

	protected int KeyNumber(int code)
	{
		switch (code)
		{
			case KeyCode.KC_1: case KeyCode.KC_NUMPAD1: return 1;
			case KeyCode.KC_2: case KeyCode.KC_NUMPAD2: return 2;
			case KeyCode.KC_3: case KeyCode.KC_NUMPAD3: return 3;
			case KeyCode.KC_4: case KeyCode.KC_NUMPAD4: return 4;
			case KeyCode.KC_5: case KeyCode.KC_NUMPAD5: return 5;
			case KeyCode.KC_6: case KeyCode.KC_NUMPAD6: return 6;
			case KeyCode.KC_7: case KeyCode.KC_NUMPAD7: return 7;
			case KeyCode.KC_8: case KeyCode.KC_NUMPAD8: return 8;
		}
		return 0;
	}

	protected CUIQSCell CellForKey(int number)
	{
		for (int i = 0; i < m_Cells.Count(); i++)
		{
			CUIQSCell cell = m_Cells.Get(i);
			if (cell && cell.m_Key == number)
				return cell;
		}
		return null;
	}

	protected void SelectKey(int number)
	{
		CUIQSCell cell = CellForKey(number);
		if (cell)
			Execute(cell);
	}

	// F on a hovered cell: the deep menu of the weapon in hands (attachments, magazines, repair).
	// Read through the mod input so the player can rebind it; the raw key is the fallback.
	protected void PollDeep()
	{
		bool down = false;
		UAInput input = GetUApi().GetInputByName(CUIQS_INPUT_DEEP);
		if (input)
			down = input.LocalPress() || input.LocalHold();
		else
			down = KeyState(CUIQuickSlotLayout.KEY_FOR_DEEP_ACTION) != 0;

		if (down && !m_FHeld)
			ExecuteDeep();
		m_FHeld = down;
	}

	// ----- mouse -----

	protected bool IsMiddleDown()
	{
		return (GetMouseState(MouseState.MIDDLE) & 0x80000000) != 0;
	}

	// Middle button on a weapon cell: the player's own weapon, also for an admin. The vanilla RadialMenu
	// has no middle-button callback, so it is polled like WRMMenu does.
	protected void PollMiddle()
	{
		bool middle = IsMiddleDown();
		if (middle && !m_MmbHeld)
		{
			CUIQSCell cell = m_Hover;
			if (cell && cell.m_Enabled && CUIQuickSlotActions.FromString(cell.m_Action) == CUIQuickSlotAction.QSA_WEAPON)
				CUIQuickSlotLocal.Act(cell);
		}
		m_MmbHeld = middle;
	}

	// ----- actions -----

	protected void Execute(CUIQSCell cell)
	{
		if (!cell || !cell.m_Enabled)
			return;

		if (cell.m_IsBack)
		{
			Back();
			return;
		}

		int action = CUIQuickSlotActions.FromString(cell.m_Action);
		if (action == CUIQuickSlotAction.QSA_WEAPON)
		{
			if (RadialArsenal && CUIQuickSlotLocal.Admin())
			{
				CUIQuickSlotLocal.s_ArsenalRequested = true;
				Close();
				return;
			}
			// Taking a weapon in hands is terminal: there is no level after it, so the wheel closes.
			CUIQuickSlotLocal.Act(cell);
			Close();
			return;
		}

		if (action == CUIQuickSlotAction.QSA_CATEGORY)
		{
			OpenCategory(cell);
			return;
		}

		// Everything below is a terminal action: it acts on the world and nothing follows it, so the
		// wheel closes itself instead of leaving the player in a menu with nothing left to do.
		if (action == CUIQuickSlotAction.QSA_USE)
		{
			UseLocal(cell);
			Close();
			return;
		}

		CUIQuickSlotLocal.Act(cell);
		Close();
	}

	protected void ExecuteDeep()
	{
		PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
		if (!player)
			return;
		EntityAI hands = player.GetItemInHands();
		if (!hands || !hands.IsWeapon())
		{
			CUIQuickSlotLocal.SetStatus(Widget.TranslateString("#STR_CUI_QS_ST_NO_WEAPON"));
			return;
		}
		CUIQuickSlotLocal.s_DeepRequested = true;
		Close();
	}

	// Use runs through the vanilla action system on the client: the same call the vanilla quickbar
	// makes on a held slot. Actions are client driven in DayZ and validated by the action system, so
	// this is the vanilla path and not a shortcut around it.
	protected void UseLocal(CUIQSCell cell)
	{
		PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
		if (!player)
			return;

		EntityAI item = CUIQuickSlotLocal.FindEntity(player, cell.Resolved());
		if (!item)
		{
			CUIQuickSlotLocal.SetStatus(Widget.TranslateString("#STR_CUI_QS_ST_NOT_FOUND"));
			return;
		}

		ActionManagerClient manager = ActionManagerClient.Cast(player.GetActionManager());
		if (!manager)
			return;

		ItemBase main = player.GetItemInHands();
		ItemBase target = ItemBase.Cast(item);
		if (manager.CanPerformActionFromQuickbar(main, target))
			manager.PerformActionFromQuickbar(main, target);
		else
			CUIQuickSlotLocal.Act(cell);		// nothing to apply it to: take it in hands instead
	}

	protected void OpenCategory(CUIQSCell cell)
	{
		m_Category = true;
		m_CatTarget = cell.m_Target;
		m_CatTitle = cell.m_Label;
		m_Hover = null;
		m_HoverLabel = "";
		Rebuild(true);
	}

	// RMB on an equipped cell: the actions of that one item (replace the filter, repair) and a way back.
	protected void OpenActions(CUIQSCell cell)
	{
		if (!cell)
			return;
		m_Actions = true;
		m_ActionSlot = cell.m_Slot;
		m_ActionTarget = cell.m_Class;
		m_Hover = null;
		m_HoverLabel = "";
		Rebuild(true);
	}

	protected void Back()
	{
		if (m_Actions)
		{
			m_Actions = false;
			m_ActionSlot = "";
			m_ActionTarget = "";
			m_Hover = null;
			m_HoverLabel = "";
			Rebuild(true);
			return;
		}
		if (m_Category)
		{
			m_Category = false;
			m_CatTarget = "";
			m_CatTitle = "";
			m_Hover = null;
			m_HoverLabel = "";
			Rebuild(true);
			return;
		}
		Close();
	}

	// ----- RadialMenu callbacks (invoked by method name) -----

	void OnControlsChanged(RadialMenuControlType type)
	{
	}

	void OnMouseSelect(Widget w)
	{
		HoverCell(w);
	}

	void OnMouseDeselect(Widget w)
	{
		ClearHover();
	}

	void OnMouseExecute(Widget w)
	{
	}

	//! LMB
	void OnMousePressLeft(Widget w)
	{
		ExecuteWidget(w);
	}

	//! RMB: the actions of an equipped cell, the admin arsenal on a weapon cell, otherwise back one level
	void OnMousePressRight(Widget w)
	{
		if (!m_Hover || !m_Hover.m_Enabled)
		{
			Back();
			return;
		}

		int action = CUIQuickSlotActions.FromString(m_Hover.m_Action);

		// A weapon cell opens its attachments level: magazines on top, the attachment cells below, the
		// same nested level the F deep action opens. Left click still takes the weapon in hands.
		if (action == CUIQuickSlotAction.QSA_WEAPON)
		{
			if (RadialArsenal && CUIQuickSlotLocal.Admin())
			{
				CUIQuickSlotLocal.Act(m_Hover);		// admin: the arsenal takes RMB
				return;
			}
			CUIQuickSlotLocal.s_DeepRequested = true;
			Close();
			return;
		}

		// An equipped cell opens its own action wheel: filter, repair, back. Only on the root level, so
		// the sub levels themselves keep plain back on RMB.
		if (!m_Category && !m_Actions && action == CUIQuickSlotAction.QSA_EQUIP && m_Hover.m_Slot != "")
		{
			OpenActions(m_Hover);
			return;
		}

		Back();
	}

	void OnControllerSelect(Widget w)
	{
		HoverCell(w);
	}

	void OnControllerDeselect(Widget w)
	{
		ClearHover();
	}

	void OnControllerExecute(Widget w)
	{
	}

	void OnControllerPressSelect(Widget w)
	{
		ExecuteWidget(w);
	}

	void OnControllerPressBack(Widget w)
	{
		Back();
	}

	protected void HoverCell(Widget w)
	{
		ClearHover();
		if (!w)
			return;
		CUIQSCell cell;
		w.GetUserData(cell);
		if (cell)
		{
			m_HoverLabel = cell.m_Label;
			m_Hover = cell;
			PaintCard(cell, true);
#ifdef DIAG_DEVELOPER
			// The vanilla radial re-fires the select callback every frame, so this prints only when the
			// hovered cell actually changes, not once per frame. A logical && must never sit inside a
			// string concatenation: it silently breaks the Enforce code generator, so every boolean is
			// computed into a local first.
			string stamp = cell.m_Key.ToString() + "|" + cell.m_Label;
			if (stamp != m_HoverLogged)
			{
				m_HoverLogged = stamp;
				bool hasLabel = (cell.m_Label != "");
				bool enabled = cell.m_Enabled;
				bool hasIcon = (cell.m_Icon != "");
				Print("[CUI-QS] hover key=" + cell.m_Key + " label='" + cell.m_Label + "' hasLabel=" + hasLabel + " enabled=" + enabled + " icon=" + hasIcon);
			}
#endif
		}
	}

	protected void ClearHover()
	{
		CUIQSCell old = m_Hover;
		m_HoverLabel = "";
		m_Hover = null;
		if (old)
			PaintCard(old, false);
	}

	protected void ExecuteWidget(Widget w)
	{
		if (!w)
			return;
		CUIQSCell cell;
		w.GetUserData(cell);
		if (cell)
			Execute(cell);
	}

	// ----- cells -----

	protected void Rebuild(bool force)
	{
		PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
		if (!player)
			return;

		array<ref CUIQSCell> cells;
		if (m_Actions)
			cells = BuildActionCells(player);
		else if (m_Category)
			cells = BuildCategoryCells(player);
		else
			cells = OrderForSlots(CUIQuickSlotLocal.BuildWheel(player));

		// a one card wheel breaks the vanilla hit testing and selector: always at least two
		while (cells.Count() < MIN_CARDS)
			cells.Insert(EmptyCell());

		string sig = Signature(cells);
		if (!force && sig == m_Signature)
			return;

		m_Signature = sig;
		m_Cells = cells;
		m_Hover = null;
		m_HoverLabel = "";
		BuildWidgets();
	}

	// A category level: the items of the category, with the back cell always in front of them.
	protected array<ref CUIQSCell> BuildCategoryCells(PlayerBase player)
	{
		array<ref CUIQSCell> content = CUIQuickSlotLocal.BuildCategory(player, m_CatTarget);
		array<ref CUIQSCell> cells = new array<ref CUIQSCell>;
		cells.Insert(BackCell());
		for (int i = 0; i < content.Count(); i++)
			cells.Insert(content.Get(i));
		return cells;
	}

	// The actions of one equipped item: replace its filter when a spare is carried, repair, and back.
	// The back cell comes first, which is the bottom of the wheel.
	protected array<ref CUIQSCell> BuildActionCells(PlayerBase player)
	{
		array<ref CUIQSCell> cells = new array<ref CUIQSCell>;
		cells.Insert(BackCell());

		string filter = FindSpareFilter(player);
		if (filter != "")
		{
			CUIQSCell cell = new CUIQSCell;
			cell.m_Index = 1;
			cell.m_Key = 0;
			cell.m_Action = "filter";
			cell.m_Target = filter;
			cell.m_Slot = m_ActionSlot;
			cell.m_Class = filter;
			cell.m_Enabled = true;
			cell.m_Label = WRMClient.GetItemName(filter);
			cell.m_Icon = CUIQuickSlotLocal.IconFor("filter", filter, filter);
			cells.Insert(cell);
		}

		return cells;
	}

	// Any gas mask filter the player carries, else "". Scans the player's own inventory only.
	protected string FindSpareFilter(PlayerBase player)
	{
		array<EntityAI> items;
		CUIQuickSlotLocal.Scan(player, items);
		for (int i = 0; i < items.Count(); i++)
		{
			EntityAI item = items.Get(i);
			if (!item)
				continue;
			string lower = item.GetType();
			lower.ToLower();
			if (lower.Contains("gasmask_filter") || lower.Contains("gasmaskfilter"))
				return item.GetType();
		}
		return "";
	}

	// The back cell: always the bottom cell of a sub level, and always key 1, so the way out sits under
	// the same key in every sub level. It is not a profile cell, so it never collides with a slot key.
	protected CUIQSCell BackCell()
	{
		CUIQSCell cell = new CUIQSCell;
		cell.m_Index = -1;
		cell.m_Key = 1;
		cell.m_Action = "none";
		cell.m_Label = Widget.TranslateString("#STR_WRM_BACK");
		cell.m_Enabled = true;
		cell.m_IsBack = true;
		cell.m_Icon = ICON_BACK;
		return cell;
	}

	protected CUIQSCell EmptyCell()
	{
		CUIQSCell cell = new CUIQSCell;
		cell.m_Index = -1;
		cell.m_Key = 0;
		cell.m_Action = "none";
		cell.m_Label = "";
		cell.m_Enabled = false;
		return cell;
	}

	// The vanilla radial places card k at a fixed angle, so the cells are inserted in screen order: slot
	// 0 is the bottom cell and the slots run clockwise. Each slot takes the position whose own angle is
	// nearest, so bottom, top, left and right land where the player expects, and the key of a position
	// comes from CUIQuickSlotLayout rather than from this order.
	protected array<ref CUIQSCell> OrderForSlots(array<ref CUIQSCell> cells)
	{
		array<ref CUIQSCell> ordered = new array<ref CUIQSCell>;
		int count = cells.Count();
		if (count < 1)
			return ordered;

		array<bool> used = new array<bool>;
		for (int u = 0; u < count; u++)
			used.Insert(false);

		for (int k = 0; k < count; k++)
		{
			float slotAngle = 180.0 + k * (360.0 / count);
			if (slotAngle >= 360.0)
				slotAngle = slotAngle - 360.0;

			int bestIndex = -1;
			float bestDiff = 1000.0;
			for (int i = 0; i < count; i++)
			{
				if (used.Get(i))
					continue;
				CUIQSCell cell = cells.Get(i);
				float diff = AngleDiff(slotAngle, CUIQuickSlotLayout.AngleForCell(cell.m_Index));
				if (diff < bestDiff)
				{
					bestDiff = diff;
					bestIndex = i;
				}
			}

			if (bestIndex < 0)
				continue;
			used.Set(bestIndex, true);
			ordered.Insert(cells.Get(bestIndex));
		}
		return ordered;
	}

	// Shortest distance between two angles in degrees, 0..180.
	protected float AngleDiff(float a, float b)
	{
		float d = a - b;
		while (d < 0)
			d = d + 360.0;
		while (d >= 360.0)
			d = d - 360.0;
		if (d > 180.0)
			d = 360.0 - d;
		return d;
	}

	protected string Signature(array<ref CUIQSCell> cells)
	{
		string sig = "";
		if (m_Category)
			sig = "c:" + m_CatTarget + ";";
		if (m_Actions)
			sig = "a:" + m_ActionSlot + ";";

		for (int i = 0; i < cells.Count(); i++)
		{
			CUIQSCell cell = cells.Get(i);
			// One member per statement: a long concatenation is where the Enforce code generator can
			// break silently (no SCRIPT (E), then an engine crash), so it is kept in the same shape as
			// WRMMenu.Signature.
			sig = sig + cell.m_Index.ToString();
			sig = sig + "," + cell.m_Key.ToString();
			sig = sig + "," + cell.m_Action;
			sig = sig + "," + cell.m_Class;
			sig = sig + "," + cell.m_Enabled.ToString();
			sig = sig + "," + cell.m_Highlight.ToString();
			sig = sig + "," + cell.m_Count.ToString();
			sig = sig + "," + cell.m_IsBack.ToString();
			sig = sig + "," + cell.m_Label;
			sig = sig + ";";
		}
		return sig;
	}

	protected void DeleteCards()
	{
		if (!m_CardPanel)
			return;
		Widget child = m_CardPanel.GetChildren();
		while (child)
		{
			Widget doomed = child;
			child = child.GetSibling();
			delete doomed;
		}
	}

	// Cards go first, then the preview entities this menu created: no widget outlives the entity it shows.
	protected void ReleaseCards()
	{
		DeleteCards();
		m_Cells.Clear();
		m_Hover = null;
		m_Signature = "";
		m_Previews.Clear();
	}

	protected void BuildWidgets()
	{
		DeleteCards();
		m_Previews.BeginFrame();

		for (int i = 0; i < m_Cells.Count(); i++)
		{
			CUIQSCell cell = m_Cells.Get(i);
			Widget card = g_Game.GetWorkspace().CreateWidgets(LAYOUT_CARD, m_CardPanel);
			cell.m_Widget = card;
			card.SetUserData(cell);
			ApplyCell(cell);
		}

		m_Previews.EndFrame();		// previews of the previous page are deleted here

#ifdef DIAG_DEVELOPER
		// One line per rebuild proving the card really contains the widgets the code addresses. Without
		// it a layout rename would silently leave empty cards and nothing in the log would say why.
		if (m_Cells.Count() > 0 && m_Cells.Get(0).m_Widget)
		{
			Widget probe = m_Cells.Get(0).m_Widget;
			bool hasIcon = (probe.FindAnyWidget("Icon") != null);
			bool hasLabel = (probe.FindAnyWidget("Label") != null);
			bool hasRender = (probe.FindAnyWidget("Render") != null);
			bool hasNum = (probe.FindAnyWidget("Num") != null);
			Print("[CUI-QS] card widgets: icon=" + hasIcon + " label=" + hasLabel + " render=" + hasRender + " num=" + hasNum);
		}
#endif

		// same placement as WRMMenu: card 0 at the bottom, cards evenly spaced clockwise
		RadialMenu radial = RadialMenu.GetInstance();
		radial.SetRadiusOffset(0);
		radial.SetExecuteDistOffset(0.5);
		radial.SetOffsetFromTop(-Math.PI);		// -PI puts card 0 at the bottom; +PI would exceed 360 degrees and break hit testing
		radial.SetItemCardRadiusOffset(0.38);
		radial.ActivateControllerTimeout(false);
		radial.Refresh();
	}

	protected void ApplyCell(CUIQSCell cell)
	{
		Widget card = cell.m_Widget;
		if (!card)
			return;

		ItemPreviewWidget render = ItemPreviewWidget.Cast(card.FindAnyWidget("Render"));
		ImageWidget icon = ImageWidget.Cast(card.FindAnyWidget("Icon"));
		TextWidget label = TextWidget.Cast(card.FindAnyWidget("Label"));
		TextWidget num = TextWidget.Cast(card.FindAnyWidget("Num"));
		TextWidget count = TextWidget.Cast(card.FindAnyWidget("Count"));

		int tint = WRMLook.C13Text();
		if (!cell.m_Enabled)
			tint = WRMLook.IconDisabled();

		if (cell.m_IsBack)
			tint = WRMLook.C13Accent();		// the way out is the same gold as the key numbers

		// A cell whose item the player actually carries shows the item's real 3D preview, exactly as the
		// game pictures an item everywhere. Only a cell with nothing behind it falls back to a grey atlas
		// icon, so a placeholder never stands in for an item the player owns.
		EntityAI entity = null;
		if (cell.m_Enabled && cell.m_Class != "" && !cell.m_IsBack)
			entity = m_Previews.Get(cell.m_Class);

		bool hasIcon = cell.m_Icon != "";
		if (icon)
		{
			if (!entity && (hasIcon || cell.m_IsBack || !cell.m_Enabled))
			{
				// Grey, like the bandage in the inventory: the placeholder must read as "nothing here",
				// never as a real item. The back arrow keeps the C13 accent so it stays findable.
				string path = cell.m_Icon;
				if (path == "")
					path = CUIQuickSlotLocal.IconName(ICON_EMPTY);
				icon.LoadImageFile(0, path);
				icon.SetImage(0);
				if (cell.m_IsBack)
					icon.SetColor(WRMLook.C13Accent());
				else
					icon.SetColor(WRMLook.C13Muted());
				icon.Show(true);
			}
			else
			{
				icon.Show(false);
			}
		}

		if (entity && render)
		{
			render.SetItem(entity);
			render.SetView(entity.GetViewIndex());
			float alpha = 0.4;
			if (cell.m_Enabled)
				alpha = 1;
			render.SetAlpha(alpha);
			render.Show(true);
		}
		else if (render)
		{
			render.Show(false);
		}

		// The cell caption is the item name and nothing else. With no plate under the cell the text sits
		// straight on the world, so it carries a dark outline: readable over snow and over a night scene
		// without bringing the plate back.
		if (label)
		{
			if (cell.m_Label != "")
			{
				label.SetText(cell.m_Label);
				label.SetColor(tint);
				label.SetOutline(1, ARGB(230, 0, 0, 0));
				label.Show(true);
			}
			else
			{
				label.Show(false);
			}
		}

		if (num)
		{
			if (cell.m_Key > 0)
			{
				num.SetText(cell.m_Key.ToString());
				num.SetOutline(1, ARGB(200, 0, 0, 0));
				num.SetColor(WRMLook.C13Accent());		// C13 gold: the key number is the one loud thing in a cell
				num.Show(true);
			}
			else
			{
				num.Show(false);
			}
		}

		if (count)
		{
			if (cell.m_Count > 1)
			{
				count.SetText("x" + cell.m_Count);
				count.SetColor(WRMLook.C13Muted());
				count.Show(true);
			}
			else
			{
				count.Show(false);
			}
		}

		PaintCard(cell, cell == m_Hover);

		// RadialMenu colours its selector by this flag and the logic is inverted: an ENABLED card has
		// DISABLED set, exactly as WRMMenu does.
		if (cell.m_Enabled)
			card.SetFlags(WidgetFlags.DISABLED);
		else
			card.ClearFlags(WidgetFlags.DISABLED);
	}

	// Inventory-card look (WRMLook): dark rounded panel, thin edge in the theme colour for the item in
	// hands and for the hovered card, theme-colour wash on hover.
	protected void PaintCard(CUIQSCell cell, bool hovered)
	{
		if (!cell || !cell.m_Widget)
			return;

		Widget card = cell.m_Widget;
		Widget back = card.FindAnyWidget("Back");
		Widget wash = card.FindAnyWidget("Wash");
		Widget edge = card.FindAnyWidget("Edge");
		bool hover = hovered && cell.m_Enabled;

		if (back)
		{
			if (cell.m_Enabled)
				back.SetColor(WRMLook.CardBack());
			else
				back.SetColor(WRMLook.CardBackDisabled());
		}
		if (wash)
		{
			if (hover)
			{
				wash.SetColor(WRMLook.Wash());
				wash.Show(true);
			}
			else if (cell.m_Highlight)
			{
				wash.SetColor(WRMLook.ActiveWash());
				wash.Show(true);
			}
			else
			{
				wash.Show(false);
			}
		}
		if (edge)
		{
			int edgeColor = WRMLook.Edge();
			if (cell.m_Highlight)
				edgeColor = WRMLook.EdgeActive();
			else if (hover)
				edgeColor = WRMLook.EdgeHover();
			else if (!cell.m_Enabled)
				edgeColor = WRMLook.EdgeDisabled();
			edge.SetColor(edgeColor);
		}
	}

	// ----- centre text -----

	// The centre shows one thing only: the name of the cell under the cursor.
	protected void UpdateCenterText()
	{
		if (!m_CenterText)
			return;

		string text = m_HoverLabel;
		if (text != m_CenterShown)
		{
			m_CenterShown = text;
			string safe = text;
			// string.Replace is in place in Enforce (proto int Replace(...) returns the count of
			// replacements, it does not return a new string), so the call is a statement, not an
			// assignment. RichText would otherwise read '<' as a tag.
			safe.Replace("<", "(");
			m_CenterText.SetText(safe);
		}
	}
}
