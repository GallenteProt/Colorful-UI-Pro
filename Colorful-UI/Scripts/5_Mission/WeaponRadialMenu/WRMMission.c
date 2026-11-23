modded class MissionBase
{
	override UIScriptedMenu CreateScriptedMenu(int id)
	{
		UIScriptedMenu menu = super.CreateScriptedMenu(id);
		if (menu)
			return menu;

		if (id == WRM_MENU_ID && WeaponRadialMenu)
		{
			menu = new WRMMenu();
			menu.SetID(id);
		}
		else if (id == CUIQuickSlotWheel.MENU_ID && RadialQuickSlots)
		{
			menu = new CUIQuickSlotWheel();
			menu.SetID(id);
		}
		return menu;
	}
}

modded class MissionServer
{
	override void OnMissionStart()
	{
		super.OnMissionStart();
		if (WeaponRadialMenu)
			WRMServer.Init();
		CUIQuickSlotServer.Init();
	}

	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		if (WeaponRadialMenu)
			WRMServer.Tick(timeslice);
	}
}

// The vanilla quickbar / HUD toggle (UAUIQuickbarToggle, default key ~) calls these two methods from
// MissionGameplay.OnUpdate (tap: ShowQuickbarPlayer on release, hold: ShowHudPlayer). Our open key defaults to the same
// key, so they are ignored while WRMInputGuard says the key belongs to our menu. The "Show quickbar" game option does
// not go through these methods and keeps working.
modded class IngameHud
{
	override void ShowQuickbarPlayer(bool show)
	{
		if (WRMInputGuard.s_BlockToggle)
			return;
		super.ShowQuickbarPlayer(show);
	}

	override void ShowHudPlayer(bool show)
	{
		if (WRMInputGuard.s_BlockToggle)
			return;
		super.ShowHudPlayer(show);
	}
}

modded class MissionGameplay
{
	override void OnMissionStart()
	{
		super.OnMissionStart();
		WRMClient.Reset();
		CUIQuickSlotLocal.Reset();
		CUIQuickSlotServer.Init();
		if (WeaponRadialMenu && !g_Game.IsMultiplayer())
			WRMServer.Init();		// offline: this process is the server too
	}

	override void OnMissionFinish()
	{
		CloseWRMMenu();
		CloseQuickSlotWheel();
		WRMInputGuard.s_BlockToggle = false;
		super.OnMissionFinish();
	}

	override void OnUpdate(float timeslice)
	{
		// the vanilla toggle is handled inside super.OnUpdate: decide before it runs whether the key is ours this frame
		WRMInputGuard.s_BlockToggle = IsWRMKeyActive();
		super.OnUpdate(timeslice);
		WRMInputGuard.s_BlockToggle = false;

		if (WeaponRadialMenu && !g_Game.IsMultiplayer())
			WRMServer.Tick(timeslice);

		HandleWheelRequests();

		if (!WeaponRadialMenu && !RadialQuickSlots)
			return;
		if (!g_Game.IsMultiplayer())
			WRMServer.Tick(timeslice);

		UpdateWRMMenu();
	}

	// raw key events (number keys, Esc, Backspace) also arrive here; the menu ignores what it already got by polling
	override void OnKeyPress(int key)
	{
		super.OnKeyPress(key);

		UIManager uiManager = g_Game.GetUIManager();
		if (!uiManager || !uiManager.IsMenuOpen(WRM_MENU_ID))
			return;
		WRMMenu menu = WRMMenu.Cast(uiManager.FindMenu(WRM_MENU_ID));
		if (menu)
			menu.OnWRMKeyEvent(key);
	}

	// our key is pressed, held or was released this frame AND the vanilla toggle sees the same key
	protected bool IsWRMKeyActive()
	{
		if (!WeaponRadialMenu && !RadialQuickSlots)
			return false;
		UAInput ours = GetUApi().GetInputByName(WRM_INPUT_OPEN);
		if (!ours)
			return false;
		if (!ours.LocalPress() && !ours.LocalHold() && !ours.LocalRelease() && ours.LocalValue() == 0)
			return false;

		UAInput toggle = GetUApi().GetInputByID(UAUIQuickbarToggle);
		if (!toggle)
			return false;
		return toggle.LocalPress() || toggle.LocalHold() || toggle.LocalRelease() || toggle.LocalValue() > 0;
	}

	protected void CloseWRMMenu()
	{
		UIManager uiManager = g_Game.GetUIManager();
		if (uiManager && uiManager.IsMenuOpen(WRM_MENU_ID))
			uiManager.CloseMenu(WRM_MENU_ID);
	}

	protected void CloseQuickSlotWheel()
	{
		UIManager uiManager = g_Game.GetUIManager();
		if (uiManager && uiManager.IsMenuOpen(CUIQuickSlotWheel.MENU_ID))
			uiManager.CloseMenu(CUIQuickSlotWheel.MENU_ID);
	}

	// The wheel cannot open the arsenal itself: a menu may not enter another menu while it is closing.
	// It leaves a request instead and this runs on the next frame, once no menu is open any more.
	protected void HandleWheelRequests()
	{
		if (!CUIQuickSlotLocal.s_ArsenalRequested && !CUIQuickSlotLocal.s_DeepRequested)
			return;
		if (!WeaponRadialMenu)
		{
			CUIQuickSlotLocal.s_ArsenalRequested = false;
			CUIQuickSlotLocal.s_DeepRequested = false;
			return;
		}

		UIManager uiManager = g_Game.GetUIManager();
		if (!uiManager || uiManager.GetMenu())
			return;		// the wheel is still closing, try again next frame

		// RMB on a weapon cell of the quick slot wheel leaves s_DeepRequested set; WRMMenu.Init() sees
		// it and opens the attachments level itself, so the menu is entered with no extra work here.
		uiManager.EnterScriptedMenu(WRM_MENU_ID, null);
		CUIQuickSlotLocal.s_ArsenalRequested = false;		// WRMMenu clears the deep flag itself
	}

	protected bool CanOpenWRMMenu(PlayerBase player)
	{
		if (!player || !player.IsAlive() || player.IsUnconscious() || player.IsRestrained() || player.IsSurrendered())
			return false;
		if (player.IsInVehicle() || player.IsRaised() || player.IsSwimming() || player.IsClimbing() || player.IsClimbingLadder())
			return false;

		UIManager uiManager = g_Game.GetUIManager();
		if (!uiManager || uiManager.GetMenu() || uiManager.IsDialogVisible())
			return false;		// never over another menu (inventory, map, chat, gestures, pause, ...)

		return !IsPaused();
	}

	// tap toggles: the first press opens, the next press closes (Esc closes from inside the menu)
	protected void UpdateWRMMenu()
	{
		UAInput input = GetUApi().GetInputByName(WRM_INPUT_OPEN);
		if (!input || !input.LocalPress())
			return;

		UIManager uiManager = g_Game.GetUIManager();
		if (uiManager.IsMenuOpen(WRM_MENU_ID))
		{
			CloseWRMMenu();
			return;
		}
		if (uiManager.IsMenuOpen(CUIQuickSlotWheel.MENU_ID))
		{
			CloseQuickSlotWheel();
			return;
		}

		PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
		if (!CanOpenWRMMenu(player))
			return;

		// One key, one wheel: the quick slots when the feature is on, the arsenal alone when it is off.
#ifdef DIAG_DEVELOPER
		Print("[CUI-QS] open key: RadialQuickSlots=" + RadialQuickSlots + " WeaponRadialMenu=" + WeaponRadialMenu);
#endif
		if (RadialQuickSlots)
			uiManager.EnterScriptedMenu(CUIQuickSlotWheel.MENU_ID, null);
		else
			uiManager.EnterScriptedMenu(WRM_MENU_ID, null);
	}
}
