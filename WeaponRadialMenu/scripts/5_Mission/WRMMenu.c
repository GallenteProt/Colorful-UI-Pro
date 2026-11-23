// Weapon radial menu. A UIScriptedMenu driven by the vanilla RadialMenu widget handler (vanilla gestures root layout,
// own card and centre layouts, own menu class and id, none of the gesture / emote code).
// Cards carry no text: a 3D preview of the item (or an icon), a green tint for the item in hands / the installed
// attachment and a small marker for "colour variants inside". The name of the hovered cell, the level title and short
// status messages are shown in the centre of the wheel.

const int WRM_LV_ROOT = 0;		// bottom: attachments of the weapon in hands, around: weapon categories
const int WRM_LV_CAT = 1;		// weapons of one category
const int WRM_LV_SLOTS = 2;		// magazines + attachment slots of the weapon in hands
const int WRM_LV_ITEMS = 3;		// items compatible with one slot
const int WRM_LV_VARIANTS = 4;	// colour variants of one cell

const int WRM_CELL_ATTACH = 0;
const int WRM_CELL_CAT = 1;
const int WRM_CELL_WEAPON = 2;
const int WRM_CELL_BACK = 3;
// 4 was the removed "previous page" cell: Back does that job now
const int WRM_CELL_NEXT = 5;
const int WRM_CELL_SLOT = 6;
const int WRM_CELL_ITEM = 7;
const int WRM_CELL_INFO = 8;	// placeholder (loading / empty), always disabled
const int WRM_CELL_VARIANT = 9;

class WRMCell
{
	int m_Kind;
	string m_Id;
	string m_Label;			// shown in the centre of the wheel while the cell is hovered
	bool m_Enabled;
	bool m_Highlight;		// green tint: item in hands / installed attachment
	int m_Count;			// classes behind the cell: > 1 opens the variants wheel
	string m_Preview;		// class drawn as a local 3D preview entity
	EntityAI m_Entity;		// existing world entity drawn directly (item in hands, installed item): never created or deleted here
	string m_Image;			// full image path, used when there is no preview (vanilla slot ghost icon)
	string m_Icon;			// our icon key, used when there is neither preview nor image
	Widget m_Widget;
	int m_Number;			// 1..N clockwise from the top cell as drawn on screen, 0 = none

	void WRMCell(int kind, string id, string label, bool enabled, string icon)
	{
		m_Kind = kind;
		m_Id = id;
		m_Label = label;
		m_Enabled = enabled;
		m_Icon = icon;
	}
}

class WRMLevel
{
	int m_Type;
	string m_Key;		// category id / slot name
	string m_Title;
	string m_Kind;		// variants level: "w" weapon, "i" attachment slot, "m" magazines
	string m_Rep;		// variants level: the class shown by the parent cell
	int m_Page;

	void WRMLevel(int type, string key, string title)
	{
		m_Type = type;
		m_Key = key;
		m_Title = title;
	}
}

class WRMMenu extends UIScriptedMenu
{
	static const string LAYOUT_ROOT = "gui/layouts/radial_menu/radial_gestures/day_z_gestures.layout";
	static const string LAYOUT_CARD = "WeaponRadialMenu/gui/layouts/wrm_card.layout";
	static const string LAYOUT_CENTER = "WeaponRadialMenu/gui/layouts/wrm_center.layout";
	static const string LAYOUT_DELIMITER = "gui/layouts/radial_menu/radial_gestures/day_z_gesture_delimiter.layout";
	static const string ICON_DIR = "WeaponRadialMenu/gui/textures/";
	static const string CATEGORY_ICONS = ",assault,sniper,smg,shotgun,rifle,pistol,launcher,other,";
	static const float REFRESH_INTERVAL = 0.5;	// re-evaluates the level, which also repeats unanswered requests
	static const int KEY_ESC = -1;				// pseudo "numbers" of the polled keys that are not number keys
	static const int KEY_BACK = -2;
	static const int MAX_NUMBER = 12;			// cells are numbered 1..12 (keys 1-9, 0, minus, equals)
	static const float PENDING_TIMEOUT = 5.0;		// RMB take + attachments: give up waiting after this many seconds
	static const float MOUSE_SAFE_ZONE = 120;		// px around the wheel centre where nothing is selected (as in RadialMenu)
	static const float KEY_DEDUP = 0.2;			// the same key seen by polling and by the raw key event counts once

	protected Widget m_CardPanel;
	protected RichTextWidget m_CenterText;
	protected ref array<ref WRMCell> m_Cells = new array<ref WRMCell>;
	protected ref array<ref WRMLevel> m_Stack = new array<ref WRMLevel>;
	protected ref WRMPreviewSet m_Previews = new WRMPreviewSet;		// owned by this instance, never shared
	protected string m_Signature;
	protected string m_CenterShown;
	protected string m_HoverLabel;
	protected int m_Gen = -1;
	protected float m_RefreshTimer;
	protected bool m_ForceRebuild;
	protected bool m_HandsInit;
	protected EntityAI m_LastHands;
	protected WRMCell m_HoverCell;		// cell under the cursor (set by the RadialMenu select callbacks)
	protected bool m_MmbHeld;			// middle button polled (the vanilla RadialMenu has no callback for it)
	protected bool m_PendingAttach;		// RMB on a weapon: waiting for the server reply, then for the weapon in hands
	protected int m_PendingSeq;
	protected float m_PendingUntil;
	protected EntityAI m_PendingHands;	// what was in hands when the weapon was requested
	protected string m_HandsType;
	protected bool m_HandsWeapon;
	protected bool m_Applied;		// HUD hidden and input excludes added, must be undone on every close path
	protected ref array<WRMCell> m_NumCells = new array<WRMCell>;	// index n-1 = cell with number n
	protected bool m_NumbersReady;		// numbers computed from valid on-screen positions
	protected ref array<int> m_KeyCodes = new array<int>;		// polled keys
	protected ref array<int> m_KeyAction = new array<int>;		// per key: number 1..12, KEY_ESC or KEY_BACK
	protected ref array<bool> m_KeyHeld = new array<bool>;
	protected ref array<float> m_KeyTime = new array<float>;

	void ~WRMMenu()
	{
		RestoreWorld();
		ReleaseCards();
	}

	// ----- lifecycle -----

	override Widget Init()
	{
		layoutRoot = g_Game.GetWorkspace().CreateWidgets(LAYOUT_ROOT);
		m_CardPanel = layoutRoot.FindAnyWidget(RadialMenu.RADIAL_ITEM_CARD_CONTAINER);

		// vanilla title is a tiny non-wrapping text: replaced by our own wrapping RichText in the centre
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
		m_Stack.Insert(new WRMLevel(WRM_LV_ROOT, "", ""));
		WRMClient.InvalidateWeapon();
		RebuildCells(true);
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

		// force-suppress: the key that closed the menu (Esc, the open key) must not act in the next frame
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

	// The open key is a mod input outside of the "ui" input context: keyboard focus stays with the game so a second press
	// of it (read in MissionGameplay) closes the menu. The "radialmenu" input exclude blocks fire, aim, stances, gestures,
	// the "hotkey" group (UAItem0-9 = quickbar slots on keys 1-0), "optics" (UAZeroingUp/Down on equals/minus) and
	// "actions" (which contains UAUIQuickbarToggle). The number keys are read by polling KeyState in Update().
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

		CheckHands(player);
		PollKeys();
		PollMouse();
		UpdatePending();

		m_RefreshTimer += timeslice;
		bool dirty = (m_Gen != WRMClient.s_Gen);
		if (dirty || m_ForceRebuild || m_RefreshTimer >= REFRESH_INTERVAL)
		{
			m_RefreshTimer = 0;
			m_Gen = WRMClient.s_Gen;
			bool force = m_ForceRebuild;
			m_ForceRebuild = false;
			RebuildCells(force);
		}

		if (!m_NumbersReady)
			AssignNumbers();		// positions were not laid out yet when the cards were created

		UpdateCenterText();
	}

	// ----- keyboard -----

	protected void AddKey(int key, int action)
	{
		m_KeyCodes.Insert(key);
		m_KeyAction.Insert(action);
		m_KeyHeld.Insert(false);
		m_KeyTime.Insert(-1000);
	}

	protected void InitKeys()
	{
		m_KeyCodes.Clear();
		m_KeyAction.Clear();
		m_KeyHeld.Clear();
		m_KeyTime.Clear();

		AddKey(KeyCode.KC_1, 1);
		AddKey(KeyCode.KC_2, 2);
		AddKey(KeyCode.KC_3, 3);
		AddKey(KeyCode.KC_4, 4);
		AddKey(KeyCode.KC_5, 5);
		AddKey(KeyCode.KC_6, 6);
		AddKey(KeyCode.KC_7, 7);
		AddKey(KeyCode.KC_8, 8);
		AddKey(KeyCode.KC_9, 9);
		AddKey(KeyCode.KC_0, 10);
		AddKey(KeyCode.KC_MINUS, 11);
		AddKey(KeyCode.KC_EQUALS, 12);
		AddKey(KeyCode.KC_NUMPAD1, 1);
		AddKey(KeyCode.KC_NUMPAD2, 2);
		AddKey(KeyCode.KC_NUMPAD3, 3);
		AddKey(KeyCode.KC_NUMPAD4, 4);
		AddKey(KeyCode.KC_NUMPAD5, 5);
		AddKey(KeyCode.KC_NUMPAD6, 6);
		AddKey(KeyCode.KC_NUMPAD7, 7);
		AddKey(KeyCode.KC_NUMPAD8, 8);
		AddKey(KeyCode.KC_NUMPAD9, 9);
		AddKey(KeyCode.KC_NUMPAD0, 10);
		AddKey(KeyCode.KC_ESCAPE, KEY_ESC);
		AddKey(KeyCode.KC_BACK, KEY_BACK);
	}

	// Polls the raw key state (independent of input excludes): a key counts once per press, until it was seen released.
	protected void PollKeys()
	{
		for (int i = 0; i < m_KeyCodes.Count(); i++)
		{
			if (KeyState(m_KeyCodes.Get(i)) != 0)
			{
				if (!m_KeyHeld.Get(i))
				{
					m_KeyHeld.Set(i, true);
					m_KeyTime.Set(i, g_Game.GetTickTime());
					int action = m_KeyAction.Get(i);
					RunKeyAction(action);
					if (action == KEY_ESC)
						return;		// closed
				}
			}
			else
			{
				m_KeyHeld.Set(i, false);
			}
		}
	}

	// Raw key event from MissionGameplay.OnKeyPress: used when polling has not seen the key yet.
	void OnWRMKeyEvent(int key)
	{
		int i = m_KeyCodes.Find(key);
		if (i < 0 || m_KeyHeld.Get(i))
			return;
		float now = g_Game.GetTickTime();
		if (now - m_KeyTime.Get(i) < KEY_DEDUP)
			return;
		m_KeyHeld.Set(i, true);
		m_KeyTime.Set(i, now);
		RunKeyAction(m_KeyAction.Get(i));
	}

	protected void RunKeyAction(int action)
	{
		if (action == KEY_ESC)
			Close();
		else if (action == KEY_BACK)
			BackOneLevel();
		else if (action > 0)
			SelectNumber(action);
	}

	// Same as a left click on the cell drawn with this number. False when no cell has the number.
	bool SelectNumber(int number)
	{
		if (!m_NumbersReady)
			AssignNumbers();
		if (number < 1 || number > m_NumCells.Count())
			return false;
		WRMCell cell = m_NumCells.Get(number - 1);
		if (!cell || !cell.m_Widget)
			return false;
		ExecuteCell(cell.m_Widget);
		return true;
	}

	// ----- mouse -----

	protected bool IsMiddleDown()
	{
		return (GetMouseState(MouseState.MIDDLE) & 0x80000000) != 0;
	}

	// The vanilla RadialMenu reports only LMB and RMB, so the middle button is polled (state edge = one click).
	protected void PollMouse()
	{
		bool down = IsMiddleDown();
		if (down && !m_MmbHeld)
			HandleMiddleClick();
		m_MmbHeld = down;
	}

	// The cell under the cursor: the cell the RadialMenu selected, else found from the cursor angle (the selection is lost
	// when the cards are rebuilt under a motionless mouse). Null in the dead zone in the middle.
	protected WRMCell CellAtMouse()
	{
		if (m_HoverCell && m_HoverCell.m_Widget)
			return m_HoverCell;

		int count = m_Cells.Count();
		if (count < 2)
			return null;

		float meanX = 0;
		float meanY = 0;
		array<float> xs = new array<float>;
		array<float> ys = new array<float>;
		for (int i = 0; i < count; i++)
		{
			Widget card = m_Cells.Get(i).m_Widget;
			if (!card)
				return null;
			float x;
			float y;
			float w;
			float h;
			card.GetScreenPos(x, y);
			card.GetScreenSize(w, h);
			xs.Insert(x + w * 0.5);
			ys.Insert(y + h * 0.5);
			meanX += xs.Get(i);
			meanY += ys.Get(i);
		}
		meanX = meanX / count;
		meanY = meanY / count;

		int mx;
		int my;
		GetMousePos(mx, my);
		float mdx = mx - meanX;
		float mdy = my - meanY;
		if (Math.Sqrt(mdx * mdx + mdy * mdy) < MOUSE_SAFE_ZONE)
			return null;

		float mouseAngle = Math.Atan2(mdx, -mdy);
		float best = 1000;
		WRMCell found = null;
		for (int k = 0; k < count; k++)
		{
			float diff = mouseAngle - Math.Atan2(xs.Get(k) - meanX, -(ys.Get(k) - meanY));
			while (diff > Math.PI)
				diff -= 2 * Math.PI;
			while (diff < -Math.PI)
				diff += 2 * Math.PI;
			if (Math.AbsFloat(diff) < best)
			{
				best = Math.AbsFloat(diff);
				found = m_Cells.Get(k);
			}
		}
		return found;
	}

	// a cell that stands for a weapon to take: a weapon cell, or a variant of a weapon
	protected bool IsWeaponCell(WRMCell cell)
	{
		if (!cell || !cell.m_Enabled)
			return false;
		if (cell.m_Kind == WRM_CELL_WEAPON)
			return true;
		return cell.m_Kind == WRM_CELL_VARIANT && CurrentLevel().m_Kind == "w";
	}

	//! MMB: colour variants of a group cell, nothing on a single class
	protected void HandleMiddleClick()
	{
		WRMCell cell = CellAtMouse();
		if (!cell || !cell.m_Enabled || cell.m_Count < 2)
			return;
		WRMLevel level = CurrentLevel();
		if (cell.m_Kind == WRM_CELL_WEAPON)
			PushVariants("w", level.m_Key, cell.m_Id, cell.m_Label);
		else if (cell.m_Kind == WRM_CELL_ITEM)
			PushVariants(ItemKind(level), level.m_Key, cell.m_Id, cell.m_Label);
	}

	//! RMB on a weapon cell: take it, then go to the attachments of the weapon in hands; anywhere else: back
	protected void HandleRightClick()
	{
		WRMCell cell = CellAtMouse();
		if (!IsWeaponCell(cell))
		{
			BackOneLevel();
			return;
		}

		m_PendingSeq = WRMClient.s_ResultSeq;
		if (!WRMClient.IssueWeapon(cell.m_Id))
			return;		// refused by the client side double click guard: no reply will come
		m_PendingAttach = true;
		m_PendingUntil = g_Game.GetTickTime() + PENDING_TIMEOUT;
		m_PendingHands = m_LastHands;
	}

	// Server reply first (OK: wait for the weapon to show up in hands; ALREADY: it is there; anything else: stay,
	// the status is already shown), then the attachments level.
	protected void UpdatePending()
	{
		if (!m_PendingAttach)
			return;
		if (g_Game.GetTickTime() > m_PendingUntil)
		{
			m_PendingAttach = false;
			return;
		}
		if (WRMClient.s_ResultSeq == m_PendingSeq)
			return;		// no reply yet

		int status = WRMClient.s_LastResult;
		if (status == WRMStatus.ALREADY)
		{
			m_PendingAttach = false;
			if (m_HandsWeapon)
				ShowAttachments();
		}
		else if (status == WRMStatus.OK)
		{
			if (m_HandsWeapon && m_LastHands != m_PendingHands)
			{
				m_PendingAttach = false;
				ShowAttachments();
			}
		}
		else
		{
			m_PendingAttach = false;
		}
	}

	protected void ShowAttachments()
	{
		TruncateStack(1);
		PushLevel(WRM_LV_SLOTS, "", Widget.TranslateString("#STR_WRM_ATTACHMENTS"));
	}

	protected string ItemKind(WRMLevel level)
	{
		if (level.m_Key == WRM_MAG_KEY)
			return "m";
		return "i";
	}

	// ----- numbers -----

	// Numbers the cells 1..N clockwise starting at the cell on top, from where the cards really are on screen
	// (the RadialMenu puts card 0 at the bottom, so the array index says nothing). With an odd count no card sits exactly
	// on top: the first one clockwise from the top gets 1.
	protected void AssignNumbers()
	{
		m_NumbersReady = false;
		m_NumCells.Clear();

		int count = m_Cells.Count();
		if (count < 1)
			return;

		array<float> xs = new array<float>;
		array<float> ys = new array<float>;
		float meanX = 0;
		float meanY = 0;
		for (int i = 0; i < count; i++)
		{
			Widget card = m_Cells.Get(i).m_Widget;
			if (!card)
				return;
			float x;
			float y;
			float w;
			float h;
			card.GetScreenPos(x, y);
			card.GetScreenSize(w, h);
			xs.Insert(x + w * 0.5);
			ys.Insert(y + h * 0.5);
			meanX += xs.Get(i);
			meanY += ys.Get(i);
		}
		meanX = meanX / count;
		meanY = meanY / count;

		float twoPi = 2 * Math.PI;
		float tolerance = 0.25 * twoPi / count;
		float maxDist = 0;
		array<float> angles = new array<float>;
		for (int a = 0; a < count; a++)
		{
			float dx = xs.Get(a) - meanX;
			float dy = ys.Get(a) - meanY;
			float dist = Math.Sqrt(dx * dx + dy * dy);
			if (dist > maxDist)
				maxDist = dist;
			float ang = Math.Atan2(dx, -dy);		// 0 = straight up, grows clockwise
			if (ang < 0)
				ang += twoPi;
			if (ang > twoPi - tolerance)
				ang -= twoPi;		// a card exactly on top (float noise) stays first
			angles.Insert(ang);
		}
		if (maxDist < 1)
			return;		// all cards at one point: not positioned yet

		array<int> order = new array<int>;
		for (int k = 0; k < count; k++)
		{
			int pos = order.Count();
			while (pos > 0 && angles.Get(order.Get(pos - 1)) > angles.Get(k))
				pos--;
			order.InsertAt(k, pos);
		}

		for (int n = 0; n < count; n++)
		{
			WRMCell cell = m_Cells.Get(order.Get(n));
			cell.m_Number = n + 1;
			if (cell.m_Number > MAX_NUMBER)
				cell.m_Number = 0;
			m_NumCells.Insert(cell);
			ShowNumber(cell);
		}
		m_NumbersReady = true;
	}

	protected void ShowNumber(WRMCell cell)
	{
		TextWidget num = TextWidget.Cast(cell.m_Widget.FindAnyWidget("Num"));
		if (!num)
			return;
		if (cell.m_Number < 1)
		{
			num.Show(false);
			return;
		}
		num.SetText(cell.m_Number.ToString());
		num.SetOutline(2, ARGB(255, 0, 0, 0));
		num.SetColor(ARGB(255, 255, 255, 255));
		num.Show(true);
	}

	// ----- item in hands -----

	protected void CheckHands(PlayerBase player)
	{
		EntityAI hands = player.GetItemInHands();
		if (m_HandsInit && hands == m_LastHands)
			return;

		m_HandsInit = true;
		m_LastHands = hands;
		m_HandsWeapon = hands && hands.IsWeapon();
		m_HandsType = "";
		if (hands)
			m_HandsType = hands.GetType();

		// whatever was known about the old weapon is void
		WRMClient.InvalidateWeapon();

		// rebuild the attachment levels: back to the slots of the new weapon, or out of the branch when it is gone
		int branch = FindLevel(WRM_LV_SLOTS);
		if (branch >= 0)
		{
			TruncateStack(branch);
			if (m_HandsWeapon)
				PushLevel(WRM_LV_SLOTS, "", Widget.TranslateString("#STR_WRM_ATTACHMENTS"));
		}
		m_ForceRebuild = true;		// cards may point at the old entity in hands
	}

	// ----- navigation stack -----

	protected WRMLevel CurrentLevel()
	{
		return m_Stack.Get(m_Stack.Count() - 1);
	}

	protected int FindLevel(int type)
	{
		for (int i = 0; i < m_Stack.Count(); i++)
		{
			if (m_Stack.Get(i).m_Type == type)
				return i;
		}
		return -1;
	}

	protected void PushLevel(int type, string key, string title)
	{
		m_Stack.Insert(new WRMLevel(type, key, title));
		RebuildCells(true);
	}

	protected void PushVariants(string kind, string scope, string rep, string title)
	{
		WRMLevel level = new WRMLevel(WRM_LV_VARIANTS, scope, title);
		level.m_Kind = kind;
		level.m_Rep = rep;
		m_Stack.Insert(level);
		RebuildCells(true);
	}

	// keep the first 'count' levels (at least the root)
	protected void TruncateStack(int count)
	{
		if (count < 1)
			count = 1;
		while (m_Stack.Count() > count)
			m_Stack.RemoveOrdered(m_Stack.Count() - 1);
	}

	// On a page after the first: previous page of the same level. Otherwise one level up; the level below keeps its page,
	// so the wheel returns exactly where it was.
	protected void BackOneLevel()
	{
		WRMLevel level = CurrentLevel();
		if (level.m_Page > 0)
		{
			level.m_Page = level.m_Page - 1;
			RebuildCells(true);
			return;
		}
		if (m_Stack.Count() <= 1)
			return;
		m_Stack.RemoveOrdered(m_Stack.Count() - 1);
		RebuildCells(true);
	}

	// ----- cells -----

	protected WRMCell InfoCell(string text)
	{
		return new WRMCell(WRM_CELL_INFO, "", text, false, "other");
	}

	// Back cell (bottom of the wheel). Its hover text says what it does: previous page after the first page, else back.
	protected WRMCell BackCell(WRMLevel level)
	{
		string label = Widget.TranslateString("#STR_WRM_BACK");
		if (level.m_Page > 0)
			label = Widget.TranslateString("#STR_WRM_PREV");
		return new WRMCell(WRM_CELL_BACK, "", label, true, "back");
	}

	protected WRMCell LoadingCell()
	{
		if (WRMClient.IsServerSilent())
			return InfoCell(Widget.TranslateString("#STR_WRM_NO_SERVER"));
		return InfoCell(Widget.TranslateString("#STR_WRM_LOADING"));
	}

	// keeps the page inside [0, last page] for a list of 'total' entries
	protected void ClampPage(WRMLevel level, int total)
	{
		int maxPage = 0;
		if (total > 0)
			maxPage = (total - 1) / WRM_PAGE_SIZE;
		if (level.m_Page > maxPage)
			level.m_Page = maxPage;
		if (level.m_Page < 0)
			level.m_Page = 0;
	}

	// Adds the content cells of a page after the first cell (Back, or the attachments cell) and, when more pages follow,
	// the Next cell. The RadialMenu puts card 0 at the bottom and card k at angle k * 360 / N clockwise from there
	// (SetOffsetFromTop(-PI)), so card N / 2 is exactly on top when N is even. A page that has a Next is always full
	// (WRM_PAGE_SIZE, even), N = 1 + 8 + 1 = 10, so Next goes to index N / 2 = 5 and the content is split around it:
	// the first half of the page right after Next (clockwise from the top), the second half before it.
	protected void AddContent(array<ref WRMCell> cells, array<ref WRMCell> content, int page, int total)
	{
		bool hasNext = (page + 1) * WRM_PAGE_SIZE < total;
		if (!hasNext)
		{
			for (int a = 0; a < content.Count(); a++)
				cells.Insert(content.Get(a));
			return;
		}

		int cardCount = 1 + content.Count() + 1;		// first cell + content + Next
		int nextIndex = cardCount / 2;					// exact top for an even count
		int before = nextIndex - 1;						// content cards between the first cell and Next (counter-clockwise side)
		if (before > content.Count())
			before = content.Count();
		int firstHalf = content.Count() - before;		// content right after Next, clockwise
		for (int b = firstHalf; b < content.Count(); b++)
			cells.Insert(content.Get(b));
		cells.Insert(new WRMCell(WRM_CELL_NEXT, "", Widget.TranslateString("#STR_WRM_NEXT"), true, "next"));
		for (int c = 0; c < firstHalf; c++)
			cells.Insert(content.Get(c));
	}

	protected string InstalledFor(string slot)
	{
		for (int i = 0; i < WRMClient.s_Slots.Count(); i++)
		{
			WRMSlotInfo info = WRMClient.s_Slots.Get(i);
			if (info.m_Name == slot)
				return info.m_Installed;
		}
		return "";
	}

	// vanilla slot icon (CfgSlots ghostIcon, same lookup as Attachments.c:521), "" when the slot has none
	protected string GhostIcon(string slot)
	{
		string icon;
		if (!g_Game.ConfigGetText("CfgSlots Slot_" + slot + " ghostIcon", icon) || icon == "")
			return "";
		if (!icon.Contains("image:"))
			icon = "set:dayz_inventory image:" + icon;
		return icon;
	}

	// the item that sits in a slot of the weapon in hands, as the client sees it (null when not synced / empty)
	protected EntityAI InstalledEntity(string slot)
	{
		Weapon_Base weapon = Weapon_Base.Cast(m_LastHands);
		if (!weapon)
			return null;
		if (slot == WRM_MAG_KEY)
			return weapon.GetMagazine(weapon.GetCurrentMuzzle());
		return weapon.GetInventory().FindAttachment(InventorySlots.GetSlotIdFromString(slot));
	}

	protected void BuildRoot(array<ref WRMCell> cells, WRMLevel level)
	{
		// index 0 is drawn at the bottom of the wheel: the weapon in hands (shown as it is), else a greyed icon
		string title = Widget.TranslateString("#STR_WRM_ATTACHMENTS");
		if (m_HandsWeapon)
			title = title + ": " + WRMClient.GetItemName(m_HandsType);
		else
			title = title + ": " + Widget.TranslateString("#STR_WRM_NEED_WEAPON");
		WRMCell attach = new WRMCell(WRM_CELL_ATTACH, "", title, m_HandsWeapon, "attach");
		if (m_HandsWeapon)
			attach.m_Entity = m_LastHands;
		cells.Insert(attach);

		WRMClient.EnsureCats();
		if (!WRMClient.s_CatsReady)
		{
			cells.Insert(LoadingCell());
			return;
		}

		int total = WRMClient.s_Cats.Count();
		if (total == 0)
		{
			cells.Insert(InfoCell(Widget.TranslateString("#STR_WRM_EMPTY")));
			return;
		}

		ClampPage(level, total);
		if (level.m_Page > 0)
			cells.Set(0, BackCell(level));		// bottom cell: previous page (the attachments cell lives on the first page)
		array<ref WRMCell> content = new array<ref WRMCell>;
		int from = level.m_Page * WRM_PAGE_SIZE;
		for (int i = from; i < total && i < from + WRM_PAGE_SIZE; i++)
		{
			WRMCatInfo info = WRMClient.s_Cats.Get(i);
			string icon = "other";
			string lowerId = info.m_Id;
			lowerId.ToLower();
			if (CATEGORY_ICONS.Contains("," + lowerId + ","))
				icon = lowerId;
			content.Insert(new WRMCell(WRM_CELL_CAT, info.m_Id, WRMClient.TranslateName(info.m_Name), true, icon));
		}
		AddContent(cells, content, level.m_Page, total);
	}

	protected void BuildCategory(array<ref WRMCell> cells, WRMLevel level)
	{
		cells.Insert(BackCell(level));

		WRMPageData data = WRMClient.GetWeaponPage(level.m_Key, level.m_Page);
		if (!data)
		{
			cells.Insert(LoadingCell());
			return;
		}

		// the server may know fewer pages than we assumed: clamp and fetch the page that really exists
		int askedPage = level.m_Page;
		ClampPage(level, data.m_Total);
		if (level.m_Page != askedPage)
		{
			data = WRMClient.GetWeaponPage(level.m_Key, level.m_Page);
			if (!data)
			{
				cells.Insert(LoadingCell());
				return;
			}
		}

		array<ref WRMCell> content = new array<ref WRMCell>;
		for (int i = 0; i < data.m_Items.Count(); i++)
		{
			string cls = data.m_Items.Get(i);
			WRMCell cell = new WRMCell(WRM_CELL_WEAPON, cls, WRMClient.GetItemName(cls), true, "other");
			cell.m_Highlight = data.m_Flags.Get(i) != 0;
			cell.m_Count = data.m_Counts.Get(i);
			cell.m_Preview = cls;
			content.Insert(cell);
		}
		AddContent(cells, content, level.m_Page, data.m_Total);
	}

	protected void BuildSlots(array<ref WRMCell> cells, WRMLevel level)
	{
		cells.Insert(BackCell(level));

		if (!m_HandsWeapon)
		{
			cells.Insert(InfoCell(Widget.TranslateString("#STR_WRM_NEED_WEAPON")));
			return;
		}
		if (WRMClient.s_SlotsReady && WRMClient.s_SlotsWeapon != m_HandsType)
			WRMClient.s_SlotsReady = false;		// stale answer for another weapon: ask again (rate limited by the request TTL)
		if (!WRMClient.EnsureSlots())
		{
			cells.Insert(LoadingCell());
			return;
		}

		int total = WRMClient.s_Slots.Count();
		if (total == 0)
		{
			cells.Insert(InfoCell(Widget.TranslateString("#STR_WRM_EMPTY")));
			return;
		}

		ClampPage(level, total);
		array<ref WRMCell> content = new array<ref WRMCell>;
		int from = level.m_Page * WRM_PAGE_SIZE;
		for (int i = from; i < total && i < from + WRM_PAGE_SIZE; i++)
		{
			WRMSlotInfo info = WRMClient.s_Slots.Get(i);
			string label = WRMClient.GetSlotName(info.m_Name);
			string fallback = "attach";
			if (info.m_Name == WRM_MAG_KEY)
				fallback = "magazine";

			WRMCell cell = new WRMCell(WRM_CELL_SLOT, info.m_Name, label, true, fallback);
			if (info.m_Installed != "")
			{
				// installed: the real item as the client has it, else a local preview of its class
				cell.m_Highlight = true;
				cell.m_Label = label + ": " + WRMClient.GetItemName(info.m_Installed);
				cell.m_Entity = InstalledEntity(info.m_Name);
				cell.m_Preview = info.m_Installed;
			}
			else
			{
				cell.m_Image = GhostIcon(info.m_Name);
			}
			content.Insert(cell);
		}
		AddContent(cells, content, level.m_Page, total);
	}

	protected void BuildItems(array<ref WRMCell> cells, WRMLevel level)
	{
		cells.Insert(BackCell(level));

		WRMPageData data = WRMClient.GetSlotItemPage(level.m_Key, level.m_Page, m_HandsType);
		if (!data)
		{
			cells.Insert(LoadingCell());
			return;
		}

		int askedPage = level.m_Page;
		ClampPage(level, data.m_Total);
		if (level.m_Page != askedPage)
		{
			data = WRMClient.GetSlotItemPage(level.m_Key, level.m_Page, m_HandsType);
			if (!data)
			{
				cells.Insert(LoadingCell());
				return;
			}
		}

		// after an issue the slot data is invalidated: refetch it so the installed tint stays correct
		WRMClient.EnsureSlots();
		string installed = InstalledFor(level.m_Key);
		string fallback = "attach";
		if (level.m_Key == WRM_MAG_KEY)
			fallback = "magazine";
		array<ref WRMCell> content = new array<ref WRMCell>;
		for (int i = 0; i < data.m_Items.Count(); i++)
		{
			string cls = data.m_Items.Get(i);
			WRMCell cell = new WRMCell(WRM_CELL_ITEM, cls, WRMClient.GetItemName(cls), true, fallback);
			cell.m_Highlight = data.m_Flags.Get(i) != 0 || cls == installed;
			cell.m_Count = data.m_Counts.Get(i);
			cell.m_Preview = cls;
			content.Insert(cell);
		}
		AddContent(cells, content, level.m_Page, data.m_Total);
	}

	protected void BuildVariants(array<ref WRMCell> cells, WRMLevel level)
	{
		cells.Insert(BackCell(level));

		string weaponType = "";
		if (level.m_Kind != "w")
			weaponType = m_HandsType;
		WRMVariantData data = WRMClient.GetVariants(level.m_Kind, level.m_Key, level.m_Rep, weaponType);
		if (!data)
		{
			cells.Insert(LoadingCell());
			return;
		}

		string fallback = "other";
		if (level.m_Kind == "i")
			fallback = "attach";
		else if (level.m_Kind == "m")
			fallback = "magazine";

		// variants are paged on the client, the whole list is small and bounded by the message size
		int total = data.m_Items.Count();
		ClampPage(level, total);
		array<ref WRMCell> content = new array<ref WRMCell>;
		int from = level.m_Page * WRM_PAGE_SIZE;
		for (int i = from; i < total && i < from + WRM_PAGE_SIZE; i++)
		{
			string cls = data.m_Items.Get(i);
			string label = WRMClient.GetItemName(cls);

			// variants that read the same get a suffix taken from the class name ("AK-101 (Black)")
			int same = 0;
			for (int j = 0; j < total; j++)
			{
				if (WRMClient.GetItemName(data.m_Items.Get(j)) == label)
					same++;
			}
			if (same > 1 && cls != level.m_Rep)
				label = label + " (" + WRMClient.VariantSuffix(level.m_Rep, cls) + ")";

			WRMCell cell = new WRMCell(WRM_CELL_VARIANT, cls, label, true, fallback);
			cell.m_Highlight = (i == data.m_Installed);
			cell.m_Preview = cls;
			content.Insert(cell);
		}
		AddContent(cells, content, level.m_Page, total);
	}

	protected string Signature(array<ref WRMCell> cells, WRMLevel level)
	{
		string sig = level.m_Type.ToString();
		sig = sig + "|" + level.m_Key;
		sig = sig + "|" + level.m_Kind;
		sig = sig + "|" + level.m_Rep;
		sig = sig + "|" + level.m_Page.ToString();
		sig = sig + "|" + m_Stack.Count().ToString();
		for (int i = 0; i < cells.Count(); i++)
		{
			WRMCell cell = cells.Get(i);
			sig = sig + "\n" + cell.m_Kind.ToString();
			sig = sig + "|" + cell.m_Id;
			sig = sig + "|" + cell.m_Label;
			sig = sig + "|" + cell.m_Enabled.ToString();
			sig = sig + "|" + cell.m_Highlight.ToString();
			sig = sig + "|" + cell.m_Count.ToString();
			sig = sig + "|" + cell.m_Preview;
			sig = sig + "|" + cell.m_Image;
			sig = sig + "|" + cell.m_Icon;
			if (cell.m_Entity)
				sig = sig + "|e";
		}
		return sig;
	}

	// Builds the cells of the current level and recreates the widgets only when something visible changed,
	// so the selection under the cursor stays stable while data arrives.
	protected void RebuildCells(bool force)
	{
		WRMLevel level = CurrentLevel();
		array<ref WRMCell> cells = new array<ref WRMCell>;
		switch (level.m_Type)
		{
			case WRM_LV_ROOT:
				BuildRoot(cells, level);
				break;
			case WRM_LV_CAT:
				BuildCategory(cells, level);
				break;
			case WRM_LV_SLOTS:
				BuildSlots(cells, level);
				break;
			case WRM_LV_ITEMS:
				BuildItems(cells, level);
				break;
			case WRM_LV_VARIANTS:
				BuildVariants(cells, level);
				break;
		}
		// a wheel with a single card breaks hit testing and the selector: always at least two
		if (cells.Count() < 2)
			cells.Insert(InfoCell(Widget.TranslateString("#STR_WRM_EMPTY")));

		string sig = Signature(cells, level);
		if (!force && sig == m_Signature)
			return;

		m_Signature = sig;
		m_Cells = cells;
		m_HoverLabel = "";
		m_HoverCell = null;
		BuildWidgets();
		UpdateControlsElements();
	}

	// ----- widgets -----

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
		m_NumCells.Clear();
		m_NumbersReady = false;
		m_HoverCell = null;
		m_Signature = "";
		m_Previews.Clear();
	}

	protected void BuildWidgets()
	{
		DeleteCards();
		m_NumbersReady = false;
		m_NumCells.Clear();
		m_Previews.BeginFrame();

		for (int i = 0; i < m_Cells.Count(); i++)
		{
			WRMCell cell = m_Cells.Get(i);
			Widget card = g_Game.GetWorkspace().CreateWidgets(LAYOUT_CARD, m_CardPanel);
			cell.m_Widget = card;
			card.SetUserData(cell);
			ApplyCell(cell);
		}

		m_Previews.EndFrame();		// previews of the previous page are deleted here

		RadialMenu radial = RadialMenu.GetInstance();
		radial.SetRadiusOffset(0);
		radial.SetExecuteDistOffset(0.5);
		radial.SetOffsetFromTop(-Math.PI);		// -PI puts card 0 at the bottom; +PI would exceed 360 degrees and break hit testing
		radial.SetItemCardRadiusOffset(0.38);
		radial.ActivateControllerTimeout(false);
		radial.Refresh();

		m_NumbersReady = false;
		AssignNumbers();		// retried from Update() when the positions are not valid yet
	}

	protected void ApplyCell(WRMCell cell)
	{
		Widget card = cell.m_Widget;
		ImageWidget back = ImageWidget.Cast(card.FindAnyWidget("Back"));
		ItemPreviewWidget render = ItemPreviewWidget.Cast(card.FindAnyWidget("Render"));
		ImageWidget icon = ImageWidget.Cast(card.FindAnyWidget("Icon"));
		ImageWidget mark = ImageWidget.Cast(card.FindAnyWidget("Mark"));

		// 1. a real entity (hands, installed), 2. a local preview entity, 3. an image, 4. our icon
		EntityAI entity = cell.m_Entity;
		if (!entity && cell.m_Preview != "")
			entity = m_Previews.Get(cell.m_Preview);

		bool shown = false;
		if (entity && render)
		{
			render.SetItem(entity);
			render.SetView(entity.GetViewIndex());
			render.SetAlpha(1);
			if (!cell.m_Enabled)
				render.SetAlpha(0.4);
			render.Show(true);
			shown = true;
		}
		if (!shown && icon)
		{
			string path = cell.m_Image;
			if (path == "")
				path = ICON_DIR + "wrm_" + cell.m_Icon + "_ca.paa";
			icon.LoadImageFile(0, path);
			icon.SetImage(0);
			int iconColor = ARGB(255, 255, 255, 255);
			if (!cell.m_Enabled)
				iconColor = ARGB(110, 150, 150, 150);
			icon.SetColor(iconColor);
			icon.Show(true);
		}

		// tint: green = in hands / installed, dark grey = not available, black = normal
		if (back)
		{
			int backColor = ARGB(115, 0, 0, 0);
			if (cell.m_Highlight)
				backColor = ARGB(170, 50, 150, 55);
			else if (!cell.m_Enabled)
				backColor = ARGB(90, 60, 60, 60);
			back.SetColor(backColor);
		}

		// RadialMenu colours its selector by this flag and the logic is inverted: an ENABLED card has DISABLED set
		if (cell.m_Enabled)
			card.SetFlags(WidgetFlags.DISABLED);
		else
			card.ClearFlags(WidgetFlags.DISABLED);

		// small marker: colour variants behind this cell
		if (mark)
		{
			if (cell.m_Count > 1)
			{
				mark.LoadImageFile(0, ICON_DIR + "wrm_variants_ca.paa");
				mark.SetImage(0);
				mark.Show(true);
			}
			else
			{
				mark.Show(false);
			}
		}
	}

	// ----- centre text -----

	// status (a few seconds) > hovered cell > level title
	protected void UpdateCenterText()
	{
		if (!m_CenterText)
			return;

		string text = WRMClient.GetStatus();
		if (text == "" && m_HoverLabel != "")
			text = m_HoverLabel;
		if (text == "")
		{
			text = Widget.TranslateString("#STR_WRM_WEAPONS");
			for (int i = 1; i < m_Stack.Count(); i++)
				text = text + " > " + m_Stack.Get(i).m_Title;
		}
		if (text != m_CenterShown)
		{
			m_CenterShown = text;
			string safe = text;
			safe.Replace("<", "(");		// RichText would read it as a tag
			m_CenterText.SetText(safe);
		}
	}

	protected void UpdateControlsElements()
	{
		Widget backSpacer = layoutRoot.FindAnyWidget("BackSpacer");
		RichTextWidget selectIcon = RichTextWidget.Cast(layoutRoot.FindAnyWidget("SelectIcon"));
		RichTextWidget backIcon = RichTextWidget.Cast(layoutRoot.FindAnyWidget("BackIcon"));
		if (!backSpacer || !selectIcon || !backIcon)
			return;

		string selectAction;
		string backAction;
		int deviceId;
		if (g_Game.GetInput().IsEnabledMouseAndKeyboardEvenOnServer() && g_Game.GetInput().GetCurrentInputDevice() == EInputDeviceType.MOUSE_AND_KEYBOARD)
		{
			selectAction = "UAMenuSelect";
			backAction = "UAMenuBack";
			deviceId = EUAINPUT_DEVICE_KEYBOARDMOUSE;
		}
		else
		{
			selectAction = "UAUISelect";
			backAction = "UAUIBack";
			deviceId = EUAINPUT_DEVICE_CONTROLLER;
		}

		selectIcon.SetText(InputUtils.GetRichtextButtonIconFromInputAction(selectAction, "", deviceId, InputUtils.ICON_SCALE_TOOLBAR));
		backIcon.SetText(InputUtils.GetRichtextButtonIconFromInputAction(backAction, "", deviceId, InputUtils.ICON_SCALE_TOOLBAR));
		backSpacer.Show(m_Stack.Count() > 1 || CurrentLevel().m_Page > 0);
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
		m_HoverLabel = "";
		m_HoverCell = null;
	}

	void OnMouseExecute(Widget w)
	{
	}

	//! LMB
	void OnMousePressLeft(Widget w)
	{
		ExecuteCell(w);
	}

	//! RMB (the RadialMenu passes no widget): take the weapon under the cursor and open its attachments, else back
	void OnMousePressRight(Widget w)
	{
		HandleRightClick();
	}

	void OnControllerSelect(Widget w)
	{
		HoverCell(w);
	}

	void OnControllerDeselect(Widget w)
	{
		m_HoverLabel = "";
		m_HoverCell = null;
	}

	void OnControllerExecute(Widget w)
	{
	}

	void OnControllerPressSelect(Widget w)
	{
		ExecuteCell(w);
	}

	void OnControllerPressBack(Widget w)
	{
		BackOneLevel();
	}

	protected void HoverCell(Widget w)
	{
		m_HoverLabel = "";
		m_HoverCell = null;
		if (!w)
			return;
		WRMCell cell;
		w.GetUserData(cell);
		if (cell)
		{
			m_HoverLabel = cell.m_Label;
			m_HoverCell = cell;
		}
	}

	// ----- actions -----

	protected void ExecuteCell(Widget w)
	{
		if (!w)
			return;

		WRMCell cell;
		w.GetUserData(cell);
		if (!cell)
			return;

		if (!cell.m_Enabled)
		{
			if (cell.m_Kind == WRM_CELL_ATTACH)
				WRMClient.SetStatus(Widget.TranslateString("#STR_WRM_NEED_WEAPON"));
			return;
		}

		WRMLevel level = CurrentLevel();
		switch (cell.m_Kind)
		{
			case WRM_CELL_ATTACH:
				PushLevel(WRM_LV_SLOTS, "", Widget.TranslateString("#STR_WRM_ATTACHMENTS"));
				break;
			case WRM_CELL_CAT:
				PushLevel(WRM_LV_CAT, cell.m_Id, cell.m_Label);
				break;
			case WRM_CELL_SLOT:
				PushLevel(WRM_LV_ITEMS, cell.m_Id, WRMClient.GetSlotName(cell.m_Id));
				break;
			case WRM_CELL_WEAPON:
				WRMClient.IssueWeapon(cell.m_Id);		// a group issues its face class; MMB opens the variants
				break;
			case WRM_CELL_ITEM:
				if (cell.m_Count > 1)
				{
					PushVariants(ItemKind(level), level.m_Key, cell.m_Id, cell.m_Label);
				}
				else
				{
					WRMClient.IssueAttachment(level.m_Key, cell.m_Id, m_HandsType);
				}
				break;
			case WRM_CELL_VARIANT:
				if (level.m_Kind == "w")
					WRMClient.IssueWeapon(cell.m_Id);
				else
					WRMClient.IssueAttachment(level.m_Key, cell.m_Id, m_HandsType);
				break;
			case WRM_CELL_BACK:
				BackOneLevel();
				break;
			case WRM_CELL_NEXT:
				level.m_Page = level.m_Page + 1;
				RebuildCells(true);
				break;
		}
	}
}
