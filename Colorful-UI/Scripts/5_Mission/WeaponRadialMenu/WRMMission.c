modded class MissionBase
{
	override UIScriptedMenu CreateScriptedMenu(int id)
	{
		UIScriptedMenu menu = super.CreateScriptedMenu(id);
		if (!menu && id == WRM_MENU_ID && WeaponRadialMenu)
		{
			menu = new WRMMenu();
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
		if (WeaponRadialMenu && !g_Game.IsMultiplayer())
			WRMServer.Init();		// offline: this process is the server too
	}

	override void OnMissionFinish()
	{
		CloseWRMMenu();
		WRMInputGuard.s_BlockToggle = false;
		super.OnMissionFinish();
	}

	override void OnUpdate(float timeslice)
	{
		// the vanilla toggle is handled inside super.OnUpdate: decide before it runs whether the key is ours this frame
		WRMInputGuard.s_BlockToggle = IsWRMKeyActive();
		super.OnUpdate(timeslice);
		WRMInputGuard.s_BlockToggle = false;

		if (!WeaponRadialMenu)
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
		if (!WeaponRadialMenu)
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

		PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
		if (CanOpenWRMMenu(player))
			uiManager.EnterScriptedMenu(WRM_MENU_ID, null);
	}
}
