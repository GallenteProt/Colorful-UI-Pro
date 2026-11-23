// MainMenu, stats and credits — CUI main menu (MOONLIGHT design).
// Vanilla source: P:\scripts\5_mission\gui\newui\mainmenu\mainmenu.c

// Click on the SteamID line in the bottom corner copies it to the clipboard.
class MoonlightSteamIdCopyHandler : ScriptedWidgetEventHandler
{
	protected ButtonWidget m_ClickTarget;
	protected TextWidget m_LabelText;
	protected TextWidget m_ValueText;
	protected string m_SteamId;

	void Bind(ButtonWidget clickTarget, TextWidget labelText, TextWidget valueText)
	{
		m_ClickTarget = clickTarget;
		m_LabelText = labelText;
		m_ValueText = valueText;

		if (m_ClickTarget)
		{
			m_ClickTarget.ClearFlags(WidgetFlags.IGNOREPOINTER);
			m_ClickTarget.SetHandler(this);
		}
	}

	void SetSteamId(string steamId)
	{
		m_SteamId = steamId;
	}

	void Dispose()
	{
		if (m_ClickTarget) m_ClickTarget.SetHandler(null);
		m_ClickTarget = null;
		m_LabelText = null;
		m_ValueText = null;
	}

	override bool OnMouseEnter(Widget w, int x, int y)
	{
		if (w != m_ClickTarget) return false;
		if (m_LabelText) m_LabelText.SetColor(UIColor.ZoneLoadingBlue());
		if (m_ValueText) m_ValueText.SetColor(UIColor.ZoneLoadingBlue());
		return true;
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		if (w != m_ClickTarget) return false;
		if (m_LabelText) m_LabelText.SetColor(colorScheme.PrimaryText());
		if (m_ValueText) m_ValueText.SetColor(colorScheme.SecondaryText());
		return true;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (w != m_ClickTarget) return false;
		if (m_SteamId != "") g_Game.CopyToClipboard(m_SteamId);
		return true;
	}
}

// Click on the music credit opens the composer's page (CustomURL.MusicAuthor).
class MoonlightMusicCreditHandler : ScriptedWidgetEventHandler
{
	protected ButtonWidget m_ClickTarget;
	protected TextWidget m_ArtistText;
	protected TextWidget m_TrackText;

	void Bind(ButtonWidget clickTarget, TextWidget artistText, TextWidget trackText)
	{
		m_ClickTarget = clickTarget;
		m_ArtistText = artistText;
		m_TrackText = trackText;

		if (m_ClickTarget)
		{
			m_ClickTarget.ClearFlags(WidgetFlags.IGNOREPOINTER);
			m_ClickTarget.SetHandler(this);
		}
	}

	void Dispose()
	{
		if (m_ClickTarget) m_ClickTarget.SetHandler(null);
		m_ClickTarget = null;
		m_ArtistText = null;
		m_TrackText = null;
	}

	override bool OnMouseEnter(Widget w, int x, int y)
	{
		if (w != m_ClickTarget) return false;
		if (m_ArtistText) m_ArtistText.SetColor(UIColor.ZoneLoadingBlue());
		if (m_TrackText) m_TrackText.SetColor(UIColor.ZoneLoadingBlue());
		return true;
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		if (w != m_ClickTarget) return false;
		if (m_ArtistText) m_ArtistText.SetColor(colorScheme.PrimaryText());
		if (m_TrackText) m_TrackText.SetColor(colorScheme.SecondaryText());
		return true;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (w != m_ClickTarget) return false;
		string url = CustomURL.MusicAuthor;
		if (url != "" && url != "#") GetGame().OpenURL(url);
		return true;
	}
}

modded class MainMenu extends UIScriptedMenu
{
	protected ref MainMenuStats m_Stats;
	protected ref MoonlightSteamIdCopyHandler m_SteamIdCopyHandler;
	protected ref MoonlightMusicCreditHandler m_MusicCreditHandler;
	protected TextWidget m_MusicArtistText, m_MusicTrackText;
	protected string m_CurrentMusicTrackTitle;
	protected ref CUIRulesOverlay m_RulesOverlay;
	protected TextWidget m_PlayerName;
	protected TextWidget m_WelcomeBack;
	protected TextWidget m_StatsHeader;
	protected TextWidget m_ZoneTitle, m_ZoneSubtitle;
	protected TextWidget m_DateTimeText, m_SteamIdLabel, m_SteamIdText;
	protected ButtonWidget m_SteamCredit, m_MusicCredit;
	protected ButtonWidget m_PrevCharacter, m_NextCharacter;
	protected ImageWidget m_TopShader, m_BottomShader, m_MenuDivider, m_StatisticsBoxBG, m_SurvivorBox, m_Logo, m_Background;
	protected Widget m_ZoneTint;
	protected Widget m_MenuDivider0;
	protected ButtonWidget m_Play, m_Exit, m_QuitBtn, m_SettingsBtn, m_MessageBtn, m_PrioQ, m_Website, m_Discord, m_CharacterBtn;
	protected ButtonWidget m_TestBtn0, m_TestBtn1, m_TestBtn2, m_TestBtn3, m_TestBtn4, m_TestBtn5;
	protected Widget m_TopSpacer, m_BottomSpacer;
	protected ProgressBarWidget m_LoadingBar;
	protected bool m_FirstShowDone;

	protected ref CUI_ErrorTestScreen m_ErrorTestScreen;

	override Widget Init()
	{
		if (ErrorTestScreen)
		{
			m_ErrorTestScreen = new CUI_ErrorTestScreen();
			layoutRoot = m_ErrorTestScreen.Build(this);
			return layoutRoot;
		}

		layoutRoot = GetGame().GetWorkspace().CreateWidgets("Colorful-UI/GUI/layouts/menus/cui.mainMenu.layout");
		m_RulesOverlay = new CUIRulesOverlay(layoutRoot);

		// Vanilla Init sets these; OpenCredits() and Play() -> ConnectLastSession() dereference them.
		m_Mission = MissionMainMenu.Cast(GetGame().GetMission());
		if (m_Mission) m_ScenePC = m_Mission.GetIntroScenePC();

		m_Play              = ButtonWidget.Cast(layoutRoot.FindAnyWidget("PlayBtn"));
		m_Exit              = ButtonWidget.Cast(layoutRoot.FindAnyWidget("ExitBtn"));
		m_QuitBtn           = ButtonWidget.Cast(layoutRoot.FindAnyWidget("QuitBtn"));
		m_SettingsBtn       = ButtonWidget.Cast(layoutRoot.FindAnyWidget("SettingsBtn"));
		m_MessageBtn        = ButtonWidget.Cast(layoutRoot.FindAnyWidget("MessageBtn"));
		m_CharacterBtn      = ButtonWidget.Cast(layoutRoot.FindAnyWidget("CharacterBtn"));

		// Right-hand icon column: Rules (WebsiteBtn), Shop/site (QueueBtn), Discord.
		m_PrioQ             = ButtonWidget.Cast(layoutRoot.FindAnyWidget("QueueBtn"));
		m_Website           = ButtonWidget.Cast(layoutRoot.FindAnyWidget("WebsiteBtn"));
		m_Discord           = ButtonWidget.Cast(layoutRoot.FindAnyWidget("DiscordBtn"));

		m_Background        = ImageWidget.Cast(layoutRoot.FindAnyWidget("ImageBackground"));
		m_ZoneTint          = layoutRoot.FindAnyWidget("ZoneTint");
		ApplyRandomMainMenuBackground();

		m_TopShader         = ImageWidget.Cast(layoutRoot.FindAnyWidget("TopShader"));
		m_BottomShader      = ImageWidget.Cast(layoutRoot.FindAnyWidget("BottomShader"));
		m_StatisticsBoxBG 	= ImageWidget.Cast(layoutRoot.FindAnyWidget("StatisticsBoxBG"));
		m_SurvivorBox 		= ImageWidget.Cast(layoutRoot.FindAnyWidget("SurvivorBox"));
		m_TopSpacer         = layoutRoot.FindAnyWidget("TopSpacer");
		m_MenuDivider       = ImageWidget.Cast(layoutRoot.FindAnyWidget("MenuDivider"));
		m_MenuDivider0      = layoutRoot.FindAnyWidget("MenuDivider0");
		m_BottomSpacer      = layoutRoot.FindAnyWidget("BottomSpacer");

		m_LoadingBar        = ProgressBarWidget.Cast(layoutRoot.FindAnyWidget("LoadingBar"));
		m_Logo              = ImageWidget.Cast(layoutRoot.FindAnyWidget("Logo"));
		m_ZoneTitle         = TextWidget.Cast(layoutRoot.FindAnyWidget("ZoneTitle"));
		m_ZoneSubtitle      = TextWidget.Cast(layoutRoot.FindAnyWidget("ZoneSubtitle"));
		// Vanilla MainMenu.Refresh() calls m_Version.SetText(); keep the widget bound.
		m_Version           = TextWidget.Cast(layoutRoot.FindAnyWidget("version"));
		m_PlayerName        = TextWidget.Cast(layoutRoot.FindAnyWidget("character_name_text"));
		m_WelcomeBack       = TextWidget.Cast(layoutRoot.FindAnyWidget("WelcomeBack"));
		m_DateTimeText      = TextWidget.Cast(layoutRoot.FindAnyWidget("DateTimeText"));
		m_SteamIdLabel      = TextWidget.Cast(layoutRoot.FindAnyWidget("SteamIDLabelText"));
		m_SteamIdText       = TextWidget.Cast(layoutRoot.FindAnyWidget("SteamIDText"));
		m_SteamCredit       = ButtonWidget.Cast(layoutRoot.FindAnyWidget("SteamCredit"));
		m_MusicCredit       = ButtonWidget.Cast(layoutRoot.FindAnyWidget("MusicCredit"));
		m_MusicArtistText   = TextWidget.Cast(layoutRoot.FindAnyWidget("MusicArtistText"));
		m_MusicTrackText    = TextWidget.Cast(layoutRoot.FindAnyWidget("MusicTrackText"));

		if (m_ZoneTitle) m_ZoneTitle.SetColor(colorScheme.PrimaryText());
		if (m_ZoneSubtitle) m_ZoneSubtitle.SetColor(colorScheme.SecondaryText());
		if (m_WelcomeBack) m_WelcomeBack.SetColor(colorScheme.BrandColor());
		if (m_DateTimeText) m_DateTimeText.SetColor(colorScheme.PrimaryText());
		if (m_SteamIdLabel) m_SteamIdLabel.SetColor(colorScheme.PrimaryText());
		if (m_SteamIdText) m_SteamIdText.SetColor(colorScheme.SecondaryText());

		if (m_MusicArtistText) m_MusicArtistText.SetColor(colorScheme.PrimaryText());
		if (m_MusicTrackText) m_MusicTrackText.SetColor(colorScheme.SecondaryText());

		// Track credit only makes sense with the Colorful UI playlist.
		if (m_MusicCredit) m_MusicCredit.Show(MenuMusic);
		if (MenuMusic && m_MusicCredit && m_MusicArtistText && m_MusicTrackText)
		{
			m_MusicCreditHandler = new MoonlightMusicCreditHandler();
			m_MusicCreditHandler.Bind(m_MusicCredit, m_MusicArtistText, m_MusicTrackText);
		}

		if (m_SteamCredit && m_SteamIdLabel && m_SteamIdText)
		{
			m_SteamIdCopyHandler = new MoonlightSteamIdCopyHandler();
			m_SteamIdCopyHandler.Bind(m_SteamCredit, m_SteamIdLabel, m_SteamIdText);
		}

		m_PrevCharacter     = ButtonWidget.Cast(layoutRoot.FindAnyWidget("prev_character"));
		m_NextCharacter     = ButtonWidget.Cast(layoutRoot.FindAnyWidget("next_character"));

		m_TestBtn0          = ButtonWidget.Cast(layoutRoot.FindAnyWidget("testBtn0"));
		m_TestBtn1          = ButtonWidget.Cast(layoutRoot.FindAnyWidget("testBtn1"));
		m_TestBtn2          = ButtonWidget.Cast(layoutRoot.FindAnyWidget("testBtn2"));
		m_TestBtn3          = ButtonWidget.Cast(layoutRoot.FindAnyWidget("testBtn3"));
		m_TestBtn4          = ButtonWidget.Cast(layoutRoot.FindAnyWidget("testBtn4"));
		m_TestBtn5          = ButtonWidget.Cast(layoutRoot.FindAnyWidget("testBtn5"));
		
		Widget stats_root = layoutRoot.FindAnyWidget("StatsBox");
		if (stats_root)
			m_Stats = new MainMenuStats(stats_root);

		if (m_StatisticsBoxBG) m_StatisticsBoxBG.SetColor(colorScheme.StatsBox());
		if (m_SurvivorBox) m_SurvivorBox.SetColor(colorScheme.SurvivorBox());
		if (m_ZoneTint) m_ZoneTint.SetColor(colorScheme.MenuTint());
		if (m_TopShader) m_TopShader.SetColor(colorScheme.TopShader());
		if (m_BottomShader) m_BottomShader.SetColor(colorScheme.BottomShader());
		if (m_MenuDivider) m_MenuDivider.SetColor(colorScheme.Separator());
		if (m_MenuDivider0) m_MenuDivider0.SetColor(colorScheme.Separator());
		if (m_LoadingBar) m_LoadingBar.SetColor(colorScheme.Loadingbar());

		m_StatsHeader = TextWidget.Cast(layoutRoot.FindAnyWidget("character_stats_textImg"));
		if (m_StatsHeader) m_StatsHeader.SetColor(colorScheme.BrandColor());

		// Direct-connect only when a real server is configured; the 127.0.0.1 default falls
		// back to vanilla Play() (last played server, else the server browser).
		if (SERVER_IP != "" && SERVER_IP != "127.0.0.1" && SERVER_PORT > 0)
			cuiElmnt.proSolidBtnDC(this, ButtonWidget.Cast(m_Play), "В ЗОНУ", UIColor.Transparent(), colorScheme.BrandColor(), SERVER_IP, SERVER_PORT);
		else
			cuiElmnt.proSolidBtnCB(this, ButtonWidget.Cast(m_Play), "В ЗОНУ", UIColor.Transparent(), colorScheme.BrandColor(), this, "Play");

		cuiElmnt.proBtnCB(this, ButtonWidget.Cast(m_SettingsBtn), "", colorScheme.PrimaryText(), colorScheme.BrandColor(), this, "OpenSettings");
		cuiElmnt.proBtnCB(this, ButtonWidget.Cast(m_Website), "", colorScheme.PrimaryText(), colorScheme.BrandColor(), this, "OpenRules");
		cuiElmnt.proBtnCB(this, ButtonWidget.Cast(m_PrioQ), "", colorScheme.PrimaryText(), colorScheme.BrandColor(), this, "OpenWebsite");
		cuiElmnt.proBtnCB(this, ButtonWidget.Cast(m_Discord), "", colorScheme.PrimaryText(), colorScheme.BrandColor(), this, "OpenDiscord");
		cuiElmnt.proBtnCB(this, ButtonWidget.Cast(m_QuitBtn), "", UIColor.ZoneDanger(), UIColor.ZoneDanger(), this, "OpenExitDialog");

		cuiElmnt.proBtnCB(this, ButtonWidget.Cast(m_CharacterBtn), "", colorScheme.PrimaryText(), colorScheme.ButtonHover(), this, "OpenMenuCustomizeCharacter");
		
		cuiElmnt.proBtnCB(this, ButtonWidget.Cast(m_PrevCharacter), "", colorScheme.PrimaryText(), colorScheme.ButtonHover(), this, "PreviousCharacter");
		cuiElmnt.proBtnCB(this, ButtonWidget.Cast(m_NextCharacter), "", colorScheme.PrimaryText(), colorScheme.ButtonHover(), this, "NextCharacter");

		UpdateCornerText();
		UpdateMusicCredit();
		
		Branding.ApplyLogo(m_Logo);	

		#ifndef WORKBENCH
		if (EnableMenuVideo)
		{
			if (!FileExist("$saves:" + m_MainMenuVideo))
				CopyFile("Colorful-UI/GUI/video/" + m_MainMenuVideo, "$saves:" + m_MainMenuVideo);
			CuiBackgroundVideo.Ensure("$saves:" + m_MainMenuVideo, true);
		}
		#endif

		RefreshCuiText();

		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.CheckPendingCuiError, 0, false);

		return layoutRoot;
	}

	// Re-applies every CUI_-keyed label from CuiLoc (see CuiLoc.c). Safe to call repeatedly.
	protected void RefreshCuiText()
	{
		if (m_WelcomeBack) m_WelcomeBack.SetText(CuiLoc.Get("CUI_welcome_back"));
		if (m_StatsHeader) m_StatsHeader.SetText(CuiLoc.Get("CUI_survivor_statistics"));
		if (m_Stats) m_Stats.RefreshCuiLabels();
	}

	protected void CheckPendingCuiError()
	{
		MissionBase mission = MissionBase.Cast(GetGame().GetMission());
		if (mission) mission.CuiFlushPendingError();
	}
	
	override void OnShow()
	{
		if (layoutRoot)
		{
			if (!m_LoadingBar) m_LoadingBar = ProgressBarWidget.Cast(layoutRoot.FindAnyWidget("LoadingBar"));
			if (!m_Logo) m_Logo = ImageWidget.Cast(layoutRoot.FindAnyWidget("Logo"));
			if (!m_TopShader) m_TopShader = ImageWidget.Cast(layoutRoot.FindAnyWidget("TopShader"));
			if (!m_BottomShader) m_BottomShader = ImageWidget.Cast(layoutRoot.FindAnyWidget("BottomShader"));
		}

		// New wallpaper every time the player comes back to the main menu.
		if (!m_FirstShowDone)
			m_FirstShowDone = true;
		else
			ApplyRandomMainMenuBackground();

		if (m_Stats) m_Stats.UpdateStats();
		UpdateCornerText();
		OnChangeCharacter(false);

		RefreshCuiText();
	}

	void ApplyRandomMainMenuBackground()
	{
		if (!m_Background) return;

		if (EnableMenuVideo)
		{
			m_Background.Show(false);
			return;
		}

		m_Background.Show(true);
		m_Background.LoadImageFile(0, GetMainMenuBackground());
		m_Background.SetColor(colorScheme.BackgroundTone());
	}
	
	override void Refresh()
	{
		OnChangeCharacter(false);

		if (m_Version)
		{
			string version;
			g_Game.GetVersion(version);
			m_Version.SetText("#main_menu_version" + " " + version);
		}

		if (m_DisplayedDlcHandler)
			m_DisplayedDlcHandler.UpdateAllPromotionInfo();
	}
	
	override void OnChangeCharacter(bool create_character = true)
	{
		MissionMainMenu mission = MissionMainMenu.Cast(GetGame().GetMission());
		if (mission && mission.GetIntroScenePC() && mission.GetIntroScenePC().GetIntroCharacter())
		{
			int charID = mission.GetIntroScenePC().GetIntroCharacter().GetCharacterID();
			if (create_character)
			{
				mission.GetIntroScenePC().GetIntroCharacter().CreateNewCharacterById(charID);
			}

			string name = mission.GetIntroScenePC().GetIntroCharacter().GetCharacterNameById(charID);

			if (m_PlayerName)
				m_PlayerName.SetText(name);

			if (m_Stats) m_Stats.UpdateStats();
		}
	}
	
	override void NextCharacter()
	{
		MissionMainMenu mission = MissionMainMenu.Cast(GetGame().GetMission());
		if (mission && mission.GetIntroScenePC() && mission.GetIntroScenePC().GetIntroCharacter())
		{
			int charID = mission.GetIntroScenePC().GetIntroCharacter().GetNextCharacterID();
			if (charID != mission.GetIntroScenePC().GetIntroCharacter().GetCharacterID())
			{
				mission.GetIntroScenePC().GetIntroCharacter().SetCharacterID(charID);
				OnChangeCharacter();
			}
		}
	}
	
	override void PreviousCharacter()
	{
		MissionMainMenu mission = MissionMainMenu.Cast(GetGame().GetMission());
		if (mission && mission.GetIntroScenePC() && mission.GetIntroScenePC().GetIntroCharacter())
		{
			int charID = mission.GetIntroScenePC().GetIntroCharacter().GetPrevCharacterID();
			if (charID != mission.GetIntroScenePC().GetIntroCharacter().GetCharacterID())
			{
				mission.GetIntroScenePC().GetIntroCharacter().SetCharacterID(charID);
				OnChangeCharacter();
			}
		}
	}

	// Vanilla handlers compare w against vanilla members this Init leaves null (m_Play,
	// m_CharacterRotationFrame, tooltip widgets), so a null w matches them: OnClick would
	// fire Play(), the others would reach null tooltip widgets. Our buttons have their own handlers.
	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (!w) return false;
		return super.OnClick(w, x, y, button);
	}

	override bool OnMouseEnter(Widget w, int x, int y)
	{
		if (!w) return false;
		return super.OnMouseEnter(w, x, y);
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		if (!w) return false;
		return super.OnMouseLeave(w, enterW, x, y);
	}

	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		if (!w) return false;
		return super.OnMouseButtonDown(w, x, y, button);
	}

	void OpenExitDialog()
	{
		CuiDialog.Show("#main_menu_exit", "#main_menu_exit_desc", true, this, "DoExit", "");
	}

	void OpenRules()
	{
		if (!m_RulesOverlay)
			m_RulesOverlay = new CUIRulesOverlay(layoutRoot);

		m_RulesOverlay.Open(true);
	}

	void OpenWebsite()
	{
		if (!IsConfiguredURL(CustomURL.Website))
			return;

		CuiDialog.Show("САЙТ ПРОЕКТА", "Вы хотите перейти на сайт проекта?", true, this, "ConfirmWebsite", "");
	}

	void ConfirmWebsite()
	{
		OpenConfiguredURL(CustomURL.Website);
	}

	void OpenDiscord()
	{
		if (!IsConfiguredURL(SocialURL.Discord))
			return;

		CuiDialog.Show("DISCORD-СЕРВЕР", "Вы хотите посетить Discord-сервер?", true, this, "ConfirmDiscord", "");
	}

	void ConfirmDiscord()
	{
		OpenConfiguredURL(SocialURL.Discord);
	}

	bool IsConfiguredURL(string url)
	{
		return url != "" && url != "#" && url != "test" && url != "/test";
	}

	void OpenConfiguredURL(string url)
	{
		if (!IsConfiguredURL(url))
			return;

		GetGame().OpenURL(url);
	}

	// Date/time and SteamID in the bottom-left corner.
	void UpdateCornerText()
	{
		if (m_DateTimeText)
			m_DateTimeText.SetText(GetMenuDateTime());

		string steamId = GetPlayerSteamId();
		if (m_SteamIdText)
			m_SteamIdText.SetText(steamId);

		if (m_SteamIdCopyHandler)
			m_SteamIdCopyHandler.SetSteamId(steamId);
	}

	string GetMenuDateTime()
	{
		int year, month, day;
		int hour, minute, second;

		GetYearMonthDay(year, month, day);
		GetHourMinuteSecond(hour, minute, second);

		return day.ToStringLen(2) + "." + month.ToStringLen(2) + "." + year.ToString() + " " + hour.ToStringLen(2) + ":" + minute.ToStringLen(2);
	}

	string GetPlayerSteamId()
	{
		BiosUserManager userManager = g_Game.GetUserManager();
		if (!userManager) return "";

		BiosUser selectedUser = userManager.GetSelectedUser();
		if (!selectedUser) return "";

		return selectedUser.GetUid();
	}

	// Shows "<track> - Joe Mathews" for the menu track DayZ is playing now.
	void UpdateMusicCredit()
	{
		if (!MenuMusic || !m_MusicTrackText) return;

		string trackTitle = "";
		Mission mission = GetGame().GetMission();
		if (mission)
		{
			DynamicMusicPlayer musicPlayer = mission.GetDynamicMusicPlayer();
			if (musicPlayer)
				trackTitle = musicPlayer.GetCUITrackTitle();
		}

		if (trackTitle == "")
			trackTitle = "...";

		if (m_CurrentMusicTrackTitle != trackTitle)
		{
			m_CurrentMusicTrackTitle = trackTitle;
			m_MusicTrackText.SetText(trackTitle + " - Joe Mathews");
		}
	}

	override void Update(float timeslice)
	{
		UpdateMusicCredit();

		if (m_RulesOverlay && m_RulesOverlay.IsOpen())
		{
			if (GetUApi().GetInputByID(UAUIBack).LocalPress())
			{
				m_RulesOverlay.Close();
				return;
			}
		}

		super.Update(timeslice);
	}

	override void Exit()
	{
		if (m_RulesOverlay && m_RulesOverlay.IsOpen())
		{
			m_RulesOverlay.Close();
			return;
		}

		if (CuiDialog.CancelTop())
			return;
		OpenExitDialog();
	}

	void DoExit()
	{
		MissionMainMenu.s_CuiQuitRequested = true;
	}

	void CUI_ErrorTest_FlushPendingError()
	{
		CheckPendingCuiError();
	}

	void CUI_BackToMainMenu()
	{
		ErrorTestScreen = false;
		Close();
		GetGame().GetUIManager().EnterScriptedMenu(MENU_MAIN, null);
	}

	void ~MainMenu()
	{
		if (GetGame())
			GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(this.CheckPendingCuiError);

		if (m_RulesOverlay)
		{
			m_RulesOverlay.Dispose();
			m_RulesOverlay = null;
		}

		if (m_SteamIdCopyHandler)
		{
			m_SteamIdCopyHandler.Dispose();
			m_SteamIdCopyHandler = null;
		}

		if (m_MusicCreditHandler)
		{
			m_MusicCreditHandler.Dispose();
			m_MusicCreditHandler = null;
		}

		cuiElmnt.CleanupForOwner(this);
		if (m_ErrorTestScreen) m_ErrorTestScreen.Cleanup();
	}
}

modded class MissionMainMenu
{
	static bool s_CuiQuitRequested;

	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		if (s_CuiQuitRequested)
		{
			s_CuiQuitRequested = false;
			g_Game.RequestExit(IDC_MAIN_QUIT);
		}
	}
}

modded class MissionBase
{
	void CuiFlushPendingError()
	{
		if (!CuiPendingError.s_Pending) return;

		string caption   = CuiPendingError.s_Caption;
		string message   = CuiPendingError.s_Message;
		int    errorCode = CuiPendingError.s_ErrorCode;
		CuiPendingError.Clear();

		Print(string.Format("[CUI ErrorRouter] MissionBase.CuiFlushPendingError showing caption='%1' message='%2'", caption, message));

		CuiDialog dlg = CuiDialog.Show(caption, message);
		if (!dlg)
		{
			Print("[CUI ErrorRouter] CuiDialog.Show returned null - falling back to native ShowDialog");

			UIManager ui = g_Game.GetUIManager();
			if (ui)
				ui.ShowDialog(caption, message, errorCode, DBT_OK, DBB_OK, DMT_EXCLAMATION, null);
		}
	}
}

modded class MainMenuStats
{
	protected RichTextWidget m_RichTime;
	protected RichTextWidget m_RichDistance;
	protected RichTextWidget m_RichLongRange;
	protected TextWidget m_TimeLabel;
	protected TextWidget m_LongRangeLabel;

	protected string BrandRgba()
	{
		int c = colorScheme.BrandColor();
		int a = (c >> 24) & 0xFF;
		int r = (c >> 16) & 0xFF;
		int g = (c >> 8)  & 0xFF;
		int b =  c        & 0xFF;
		return string.Format("%1, %2, %3, %4", r, g, b, a);
	}

	void MainMenuStats(Widget root)
	{
		m_RichTime      = RichTextWidget.Cast(root.FindAnyWidget("TimeSurvivedValue"));
		m_RichDistance  = RichTextWidget.Cast(root.FindAnyWidget("DistanceTraveledValue"));
		m_RichLongRange = RichTextWidget.Cast(root.FindAnyWidget("LongRangeShotValue"));
		m_TimeLabel      = TextWidget.Cast(root.FindAnyWidget("TimeSurvivedLabel"));
		m_LongRangeLabel = TextWidget.Cast(root.FindAnyWidget("LongRangeShotLabel"));

		RefreshCuiLabels();
	}

	// See MainMenu.RefreshCuiText() — same reasoning, these two labels are otherwise only
	// ever resolved once from the static "#CUI_..." text baked into cui.mainMenu.layout.
	void RefreshCuiLabels()
	{
		if (m_TimeLabel)      m_TimeLabel.SetText(CuiLoc.Get("CUI_time_alive"));
		if (m_LongRangeLabel) m_LongRangeLabel.SetText(CuiLoc.Get("CUI_best_long_range_hit"));
	}

	// Does not call super: vanilla UpdateStats() writes to the stat widgets
	// unguarded, and the MOONLIGHT main menu layout has no stats block.
	override void UpdateStats()
	{
		MissionMainMenu missionMainMenu = MissionMainMenu.Cast(g_Game.GetMission());
		if (!missionMainMenu) return;

		DayZIntroScenePC introScene = missionMainMenu.GetIntroScenePC();
		if (!introScene) return;

		IntroSceneCharacter introCharacter = introScene.GetIntroCharacter();
		if (!introCharacter) return;

		PlayerBase player = introCharacter.GetCharacterObj();
		if (!player) return;

		FullTimeData ft = new FullTimeData();
		TimeConversions.ConvertSecondsToFullTime(player.StatGet(AnalyticsManagerServer.STAT_PLAYTIME), ft);

		if (m_TimeSurvivedValue) m_TimeSurvivedValue.SetText(ft.FormatedNonZero());
		if (m_PlayersKilledValue) m_PlayersKilledValue.SetText(GetValueString(player.StatGet(AnalyticsManagerServer.STAT_PLAYERS_KILLED)));
		if (m_InfectedKilledValue) m_InfectedKilledValue.SetText(GetValueString(player.StatGet(AnalyticsManagerServer.STAT_INFECTED_KILLED)));
		if (m_DistanceTraveledValue) m_DistanceTraveledValue.SetText(GetDistanceString(player.StatGet(AnalyticsManagerServer.STAT_DISTANCE)));
		if (m_LongRangeShotValue) m_LongRangeShotValue.SetText(GetDistanceString(player.StatGet(AnalyticsManagerServer.STAT_LONGEST_SURVIVOR_HIT), true));

		if (m_RichTime) m_RichTime.SetText(BuildColoredTime(ft));

		if (m_RichDistance)  m_RichDistance.SetText(BuildColoredDistance(player.StatGet(AnalyticsManagerServer.STAT_DISTANCE), false));
		if (m_RichLongRange) m_RichLongRange.SetText(BuildColoredDistance(player.StatGet(AnalyticsManagerServer.STAT_LONGEST_SURVIVOR_HIT), true));
	}

	protected string ColorTag(string locKey)
	{
		return string.Format("<color rgba=\"%1\">%2</color>", BrandRgba(), locKey);
	}

	protected string BuildColoredTime(FullTimeData ft)
	{
		string msg;
		if (ft.m_Days > 0)    msg += string.Format("%1%2 ", ft.m_Days,    ColorTag("#STR_time_unit_abbrev_day_0"));
		if (ft.m_Hours > 0)   msg += string.Format("%1%2 ", ft.m_Hours,   ColorTag("#STR_time_unit_abbrev_hour_0"));
		if (ft.m_Minutes > 0) msg += string.Format("%1%2 ", ft.m_Minutes, ColorTag("#STR_time_unit_abbrev_minute_0"));
		msg += string.Format("%1%2", ft.m_Seconds, ColorTag("#STR_time_unit_abbrev_second_0"));
		return msg;
	}

	protected string BuildColoredDistance(float total_distance, bool meters_only)
	{
		if (total_distance > 0)
		{
			float kilometers = Math.Round(total_distance * 0.001);
			if (kilometers >= 10 && !meters_only)
				return GetValueString(kilometers, true) + " " + ColorTag("#STR_distance_unit_abbrev_kilometer_0");
			return GetValueString(total_distance) + " " + ColorTag("#STR_distance_unit_abbrev_meter_0");
		}
		return "0 " + ColorTag("#STR_distance_unit_abbrev_meter_0");
	}
}

modded class CreditsDepartmentElement
{
	void CreditsDepartmentElement(int index, Widget parent, JsonDataCreditsDepartment department_data)
	{
		if (m_Root)
		{
			Widget sep = m_Root.FindAnyWidget("SeparatorPanel");
			if (sep) sep.SetColor(colorScheme.Separator());

			if (department_data && department_data.DepartmentName == CUI_CREDITS_DEPARTMENT)
				m_Root.SetSort(0);
		}
	}
}

modded class CreditsDepartmentSection
{
	void CreditsDepartmentSection(int index, Widget parent, JsonDataCreditsSection section_data)
	{
		if (m_Root)
		{
			Widget sep = m_Root.FindAnyWidget("SeparatorPanel");
			if (sep) sep.SetColor(colorScheme.Separator());
		}
	}
}

modded class CreditsMenu
{
	protected ImageWidget m_CuiBackground;
	protected ImageWidget m_CuiTopShader;
	protected ImageWidget m_CuiBottomShader;

	override Widget Init()
	{
		Widget root = super.Init();
		if (!root) return root;

		Widget panel = GetGame().GetWorkspace().CreateWidgets("Colorful-UI/GUI/layouts/menus/cui.credits_bg.layout", root);
		if (panel)
		{
			m_CuiBackground   = ImageWidget.Cast(panel.FindAnyWidget("ImageBackground"));
			m_CuiTopShader    = ImageWidget.Cast(panel.FindAnyWidget("TopShader"));
			m_CuiBottomShader = ImageWidget.Cast(panel.FindAnyWidget("BottomShader"));

			if (m_CuiBackground)
			{
				if (EnableMenuVideo) m_CuiBackground.Show(false);
				else                 m_CuiBackground.LoadImageFile(0, GetMainMenuBackground());
			}
			if (m_CuiTopShader)    m_CuiTopShader.SetColor(colorScheme.TopShader());
			if (m_CuiBottomShader) m_CuiBottomShader.SetColor(colorScheme.BottomShader());
		}

		#ifndef WORKBENCH
		if (EnableMenuVideo)
		{
			if (!FileExist("$saves:" + m_MainMenuVideo))
				CopyFile("Colorful-UI/GUI/video/" + m_MainMenuVideo, "$saves:" + m_MainMenuVideo);
			CuiBackgroundVideo.Ensure("$saves:" + m_MainMenuVideo, true);
		}
		#endif

		return root;
	}

	void ~CreditsMenu()
	{
	}
}
