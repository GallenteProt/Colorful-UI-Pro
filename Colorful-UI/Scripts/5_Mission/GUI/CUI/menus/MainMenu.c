class LonelinessSteamIdCopyHandler : ScriptedWidgetEventHandler
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

class LonelinessMusicCreditHandler : ScriptedWidgetEventHandler
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
		GetGame().OpenURL(CustomURL.MusicAuthor);
		return true;
	}
}

modded class MainMenu extends UIScriptedMenu
{
	protected ref MainMenuStats m_Stats;
	protected ref LonelinessSteamIdCopyHandler m_SteamIdCopyHandler;
	protected ref LonelinessMusicCreditHandler m_MusicCreditHandler;
	protected ref CUIRulesOverlay m_RulesOverlay;
	protected TextWidget m_PlayerName;
	protected TextWidget m_WelcomeBack;
	protected TextWidget m_ZoneTitle, m_ZoneSubtitle;
	protected TextWidget m_LonelinessDateTime, m_LonelinessSteamIdLabel, m_LonelinessSteamId;
	protected TextWidget m_MusicArtistText, m_MusicTrackText;
	protected string m_CurrentMusicTrackTitle;
	protected ButtonWidget m_SteamCredit, m_MusicCredit;
	protected ButtonWidget m_PrevCharacter, m_NextCharacter;
	protected ImageWidget m_TopShader, m_BottomShader, m_MenuDivider, m_StatisticsBoxBG, m_SurvivorBox, m_Logo, m_Background;
	protected Widget m_ZoneTint;
	protected bool m_LonelinessFirstShowDone;
	// MenuDivider0 was switched to PanelWidget (solid bg) in the layout, so it's
	// typed as plain Widget here — SetColor lives on the base Widget class.
	protected Widget m_MenuDivider0;
	protected ButtonWidget m_Play, m_Exit, m_QuitBtn, m_SettingsBtn, m_MessageBtn, m_PrioQ, m_Website, m_Discord, m_CharacterBtn;
	protected ButtonWidget m_TestBtn0, m_TestBtn1, m_TestBtn2, m_TestBtn3, m_TestBtn4, m_TestBtn5;
	protected Widget m_TopSpacer, m_BottomSpacer;
	protected ProgressBarWidget m_LoadingBar;

	override Widget Init()
	{
		layoutRoot = GetGame().GetWorkspace().CreateWidgets("Colorful-UI/GUI/layouts/menus/cui.mainMenu.layout");
		m_RulesOverlay = new CUIRulesOverlay(layoutRoot);

		m_Play              = ButtonWidget.Cast(layoutRoot.FindAnyWidget("PlayBtn"));
		m_Exit              = ButtonWidget.Cast(layoutRoot.FindAnyWidget("ExitBtn"));
		m_QuitBtn           = ButtonWidget.Cast(layoutRoot.FindAnyWidget("QuitBtn"));
		m_SettingsBtn       = ButtonWidget.Cast(layoutRoot.FindAnyWidget("SettingsBtn"));
		m_MessageBtn        = ButtonWidget.Cast(layoutRoot.FindAnyWidget("MessageBtn"));
		m_CharacterBtn      = ButtonWidget.Cast(layoutRoot.FindAnyWidget("CharacterBtn"));

		m_PrioQ             = ButtonWidget.Cast(layoutRoot.FindAnyWidget("QueueBtn"));
		m_Website           = ButtonWidget.Cast(layoutRoot.FindAnyWidget("WebsiteBtn"));
		m_Discord           = ButtonWidget.Cast(layoutRoot.FindAnyWidget("DiscordBtn"));
		m_Background        = ImageWidget.Cast(layoutRoot.FindAnyWidget("ImageBackground"));
		ApplyRandomMainMenuBackground();

		m_ZoneTint          = layoutRoot.FindAnyWidget("ZoneTint");
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
		// Vanilla MainMenu.Refresh() (and Dabs Framework's override) call
		// m_Version.SetText() unguarded. The engine invokes Refresh() from
		// DayZGame.SetDatabaseID() during connect, so without this widget the
		// connect sequence dies with a VM NPE -> permanent black screen.
		m_Version           = TextWidget.Cast(layoutRoot.FindAnyWidget("version"));
		m_PlayerName        = TextWidget.Cast(layoutRoot.FindAnyWidget("character_name_text"));
		m_WelcomeBack       = TextWidget.Cast(layoutRoot.FindAnyWidget("WelcomeBack"));
		m_LonelinessDateTime = TextWidget.Cast(layoutRoot.FindAnyWidget("DateTimeText"));
		m_LonelinessSteamIdLabel = TextWidget.Cast(layoutRoot.FindAnyWidget("SteamIDLabelText"));
		m_LonelinessSteamId = TextWidget.Cast(layoutRoot.FindAnyWidget("SteamIDText"));
		m_MusicArtistText   = TextWidget.Cast(layoutRoot.FindAnyWidget("MusicArtistText"));
		m_MusicTrackText    = TextWidget.Cast(layoutRoot.FindAnyWidget("MusicTrackText"));
		m_SteamCredit       = ButtonWidget.Cast(layoutRoot.FindAnyWidget("SteamCredit"));
		m_MusicCredit       = ButtonWidget.Cast(layoutRoot.FindAnyWidget("MusicCredit"));
		if (m_ZoneTitle) m_ZoneTitle.SetColor(colorScheme.PrimaryText());
		if (m_ZoneSubtitle) m_ZoneSubtitle.SetColor(colorScheme.SecondaryText());
		if (m_WelcomeBack) m_WelcomeBack.SetColor(colorScheme.BrandColor());
		if (m_LonelinessDateTime) m_LonelinessDateTime.SetColor(colorScheme.PrimaryText());
		if (m_LonelinessSteamIdLabel) m_LonelinessSteamIdLabel.SetColor(colorScheme.PrimaryText());
		if (m_LonelinessSteamId) m_LonelinessSteamId.SetColor(colorScheme.SecondaryText());
		if (m_MusicArtistText) m_MusicArtistText.SetColor(colorScheme.PrimaryText());
		if (m_MusicTrackText) m_MusicTrackText.SetColor(colorScheme.SecondaryText());
		if (m_SteamCredit && m_LonelinessSteamIdLabel && m_LonelinessSteamId)
		{
			m_SteamIdCopyHandler = new LonelinessSteamIdCopyHandler();
			m_SteamIdCopyHandler.Bind(m_SteamCredit, m_LonelinessSteamIdLabel, m_LonelinessSteamId);
		}
		if (m_MusicCredit && m_MusicArtistText && m_MusicTrackText)
		{
			m_MusicCreditHandler = new LonelinessMusicCreditHandler();
			m_MusicCreditHandler.Bind(m_MusicCredit, m_MusicArtistText, m_MusicTrackText);
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

		TextWidget statsHeader = TextWidget.Cast(layoutRoot.FindAnyWidget("character_stats_textImg"));
		if (statsHeader) statsHeader.SetColor(colorScheme.BrandColor());

		cuiElmnt.proSolidBtnDC(this, ButtonWidget.Cast(m_Play), "В ЗОНУ", UIColor.Transparent(), colorScheme.BrandColor(), SERVER_IP, SERVER_PORT);
		cuiElmnt.proBtnCB(this, ButtonWidget.Cast(m_SettingsBtn), "", colorScheme.PrimaryText(), colorScheme.BrandColor(), this, "OpenSettings");
		cuiElmnt.proBtnCB(this, ButtonWidget.Cast(m_Website), "", colorScheme.PrimaryText(), colorScheme.BrandColor(), this, "OpenRules");
		cuiElmnt.proBtnCB(this, ButtonWidget.Cast(m_PrioQ), "", colorScheme.PrimaryText(), colorScheme.BrandColor(), this, "OpenWebsite");
		cuiElmnt.proBtnCB(this, ButtonWidget.Cast(m_Discord), "", colorScheme.PrimaryText(), colorScheme.BrandColor(), this, "OpenDiscord");
		cuiElmnt.proBtnCB(this, ButtonWidget.Cast(m_QuitBtn), "", UIColor.ZoneDanger(), UIColor.ZoneDanger(), this, "OpenExitDialog");

		cuiElmnt.proBtnCB(this, ButtonWidget.Cast(m_CharacterBtn), "", colorScheme.PrimaryText(), colorScheme.ButtonHover(), this, "OpenMenuCustomizeCharacter");
		
		cuiElmnt.proBtnCB(this, ButtonWidget.Cast(m_PrevCharacter), "", colorScheme.PrimaryText(), colorScheme.ButtonHover(), this, "PreviousCharacter");
		cuiElmnt.proBtnCB(this, ButtonWidget.Cast(m_NextCharacter), "", colorScheme.PrimaryText(), colorScheme.ButtonHover(), this, "NextCharacter");

		UpdateLonelinessCornerText();
		UpdateMusicCredit();
		
		Branding.ApplyLogo(m_Logo);	

		#ifndef WORKBENCH
		// Singleton workspace-rooted video — persists across menu transitions
		// so MainMenu → Credits → Options never blackscreens between teardowns.
		// Ensure() is a no-op if already running.
		if (EnableMenuVideo && m_MainMenuVideo != "")
		{
			if (!FileExist("$saves:" + m_MainMenuVideo))
				CopyFile("Colorful-UI/GUI/video/" + m_MainMenuVideo, "$saves:" + m_MainMenuVideo);
			CuiBackgroundVideo.Ensure("$saves:" + m_MainMenuVideo, true);
		}
		#endif		
		return layoutRoot;
	}
	
	override void OnShow()
	{
		if (!m_LoadingBar) m_LoadingBar = ProgressBarWidget.Cast(layoutRoot.FindAnyWidget("LoadingBar"));
		if (!m_Logo) m_Logo = ImageWidget.Cast(layoutRoot.FindAnyWidget("Logo"));
		if (!m_TopShader) m_TopShader = ImageWidget.Cast(layoutRoot.FindAnyWidget("TopShader"));
		if (!m_BottomShader) m_BottomShader = ImageWidget.Cast(layoutRoot.FindAnyWidget("BottomShader"));
		
		if (!m_LonelinessFirstShowDone)
		{
			m_LonelinessFirstShowDone = true;
		}
		else
		{
			ApplyRandomMainMenuBackground();
		}
		if (m_Stats) m_Stats.UpdateStats();
		UpdateLonelinessCornerText();
		OnChangeCharacter(false);
	}

	void ApplyRandomMainMenuBackground()
	{
		if (!m_Background) return;

		if (EnableMenuVideo && m_MainMenuVideo != "")
		{
			m_Background.Show(false);
			return;
		}

		m_Background.Show(true);
		m_Background.LoadImageFile(0, GetRandomMainMenuBackground());
		m_Background.SetColor(colorScheme.BackgroundTone());
	}

	string GetRandomMainMenuBackground()
	{
		return GetMainMenuBackground();
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
		if (!mission) return;

		DayZIntroScenePC introScene = mission.GetIntroScenePC();
		if (!introScene) return;

		IntroSceneCharacter introCharacter = introScene.GetIntroCharacter();
		if (!introCharacter) return;

		int charID = introCharacter.GetCharacterID();
		if (create_character)
		{
			introCharacter.CreateNewCharacterById(charID);
		}

		string name = introCharacter.GetCharacterNameById(charID);

		if (m_PlayerName)
			m_PlayerName.SetText(name);

		if (m_Stats) m_Stats.UpdateStats();
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

	// Replaces the vanilla colorfulExitDialog flow. Show our CuiDialog with
	// a Confirm callback that fires the same RequestExit(IDC_MAIN_QUIT) the
	// old dialog used. Cancel just closes (no callback needed).
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
		if (CustomURL.Website == "" || CustomURL.Website == "#" || CustomURL.Website == "test" || CustomURL.Website == "/test")
			return;

		CuiDialog.Show("САЙТ ПРОЕКТА", "Вы хотите перейти на сайт проекта?", true, this, "ConfirmWebsite", "", false, "ДА", "НЕТ");
	}

	void ConfirmWebsite()
	{
		OpenConfiguredURL(CustomURL.Website);
	}

	void OpenDiscord()
	{
		if (SocialURL.Discord == "" || SocialURL.Discord == "#" || SocialURL.Discord == "test" || SocialURL.Discord == "/test")
			return;

		CuiDialog.Show("DISCORD-СЕРВЕР", "Вы хотите посетить Discord-сервер?", true, this, "ConfirmDiscord", "", false, "ДА", "НЕТ");
	}

	void ConfirmDiscord()
	{
		OpenConfiguredURL(SocialURL.Discord);
	}

	void OpenConfiguredURL(string url)
	{
		if (url == "" || url == "#" || url == "test" || url == "/test")
			return;

		GetGame().OpenURL(url);
	}

	void UpdateLonelinessCornerText()
	{
		if (m_LonelinessDateTime)
			m_LonelinessDateTime.SetText(GetLonelinessDateTime());

		if (m_LonelinessSteamId)
			m_LonelinessSteamId.SetText(GetLonelinessSteamId());

		if (m_SteamIdCopyHandler)
			m_SteamIdCopyHandler.SetSteamId(GetLonelinessRawSteamId());
	}

	void UpdateMusicCredit()
	{
		if (!m_MusicTrackText) return;

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

	string GetLonelinessDateTime()
	{
		int year;
		int month;
		int day;
		int hour;
		int minute;
		int second;

		GetYearMonthDay(year, month, day);
		GetHourMinuteSecond(hour, minute, second);

		return day.ToStringLen(2) + "." + month.ToStringLen(2) + "." + year.ToString() + " " + hour.ToStringLen(2) + ":" + minute.ToStringLen(2);
	}

	string GetLonelinessSteamId()
	{
		string steamId = GetLonelinessRawSteamId();
		return steamId;
	}

	string GetLonelinessRawSteamId()
	{
		string steamId = "";
		BiosUserManager userManager = g_Game.GetUserManager();
		if (userManager)
		{
			BiosUser selectedUser = userManager.GetSelectedUser();
			if (selectedUser)
			{
				string uid = selectedUser.GetUid();
				if (uid != "")
					steamId = uid;
			}
		}

		return steamId;
	}

	override void Update(float timeslice)
	{
		UpdateMusicCredit();

		if (GetGame().GetInput().LocalPress("UAColorfulUIDiagConnect", false))
		{
			DayZGame game = DayZGame.Cast(GetGame());
			if (game)
			{
				GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(game.ConnectFromJoin, 0, false, DIAG_SERVER_IP, DIAG_SERVER_PORT);
				return;
			}
		}

		if ((m_RulesOverlay != null) && m_RulesOverlay.IsOpen())
		{
			if (GetUApi().GetInputByID(UAUIBack).LocalPress())
			{
				m_RulesOverlay.Close();
				return;
			}
		}

		super.Update(timeslice);
	}

	// Vanilla Update() calls Exit() on UAUIBack (Escape) and Exit() routes
	// to GetUIManager().ShowDialog — bypassing our CuiDialog. Override so
	// Escape and the Exit button take the same path. If a CuiDialog is
	// already open (e.g. our exit confirm), treat Escape as Cancel on the
	// top dialog instead of stacking another one.
	override void Exit()
	{
		int n = CuiDialog.s_OpenDialogs.Count();
		if (n > 0)
		{
			CuiDialog top = CuiDialog.s_OpenDialogs.Get(n - 1);
			if (top) top.OnCancel();
			return;
		}
		OpenExitDialog();
	}

	void DoExit()
	{
		MissionMainMenu.s_CuiQuitRequested = true;
	}

	void ~MainMenu()
	{
		// Singleton bg video persists — do NOT touch CuiBackgroundVideo.s_Instance.
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

// Brand-color the unit suffixes (h/m/s/d in time, m/km in distance + long-range hit).
// The 3 value widgets are RichTextWidgetClass in the layout so SetText parses
// the <color rgba="R, G, B, A"> markup. rgba MUST be comma-separated decimal —
// hex like #FFCC66FF is not recognized by the rich-text parser. Cast as
// RichTextWidget explicitly (don't rely on TextWidget polymorphism).
modded class MainMenuStats
{
	protected RichTextWidget m_RichTime;
	protected RichTextWidget m_RichDistance;
	protected RichTextWidget m_RichLongRange;

	// Build "R, G, B, A" string from colorScheme.BrandColor() at call time so
	// rich-text markup follows theme swaps instead of a baked literal.
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
	}

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

// Vanilla's department_element.layout and department_section.layout bake
// SeparatorPanel as DayZ red (color 0.7569 0.0941 0.0941 0.7843). Modded
// constructors chain after the original, so by the time these run m_Root
// is built and SeparatorPanel exists.
modded class CreditsDepartmentElement
{
	void CreditsDepartmentElement(int index, Widget parent, JsonDataCreditsDepartment department_data)
	{
		if (m_Root)
		{
			Widget sep = m_Root.FindAnyWidget("SeparatorPanel");
			if (sep) sep.SetColor(colorScheme.Separator());
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

// Vanilla credits_menu.layout has a fully transparent root, so the main
// menu's character preview rendered behind it leaks through. Inject our
// fullscreen background panel (ImageBackground + Top/BottomShader,
// mirroring the main-menu structure) with priority -1 so credits content
// stays on top. The injected panel is a child of layoutRoot so vanilla's
// SetAlpha-driven fade in Update() cascades to it for free.
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

			// Static bg only when video is off — otherwise the injected
			// ImageBackground covers the workspace-rooted video below.
			if (m_CuiBackground)
			{
				if (EnableMenuVideo && m_MainMenuVideo != "") m_CuiBackground.Show(false);
				else                 m_CuiBackground.LoadImageFile(0, GetMainMenuBackground());
			}
			if (m_CuiTopShader)    m_CuiTopShader.SetColor(colorScheme.TopShader());
			if (m_CuiBottomShader) m_CuiBottomShader.SetColor(colorScheme.BottomShader());
		}

		#ifndef WORKBENCH
		// Same singleton CuiBackgroundVideo MainMenu uses; just Ensure() it
		// (no-op if already running). Persists across MainMenu→Credits so
		// there's no black flash during the transition.
		if (EnableMenuVideo && m_MainMenuVideo != "")
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
		// Singleton bg video persists — do nothing here.
	}
}
