modded class InGameMenu
{
    protected ImageWidget m_TopShader, m_BottomShader, m_MenuDivider, m_Logo;
    protected ButtonWidget m_PrioQ, m_Website, m_Discord;
    protected ButtonWidget m_ViewToggleButton;
    protected ButtonWidget m_TextMenuContinueButton, m_TextMenuOptionsButton, m_TextMenuRulesButton;
    protected ButtonWidget m_TextMenuWebsiteButton, m_TextMenuDiscordButton, m_TextMenuExitButton;
    protected TextWidget m_RespawnButtonLabel;
    protected Widget m_TopSpacer, m_BottomSpacer, m_TextMenuRoot, m_ViewToggleIndicator;
    protected ref CUIRulesOverlay m_RulesOverlay;
    protected TextWidget m_LonelinessDateTime;
    protected static bool s_UseTextMenu;

    override Widget Init()
    {
        layoutRoot = GetGame().GetWorkspace().CreateWidgets("Colorful-UI/GUI/layouts/menus/inGame/cui.ingame.layout");
        m_RulesOverlay = new CUIRulesOverlay(layoutRoot);
        m_Logo                  = ImageWidget.Cast(layoutRoot.FindAnyWidget("Logo"));
        m_HintPanel             = new UiHintPanel(layoutRoot.FindAnyWidget("hint_frame"));

        m_ExitButton = layoutRoot.FindAnyWidget("ExitBtn");
        m_ContinueButton = layoutRoot.FindAnyWidget("ContinueBtn");
        m_OptionsButton = layoutRoot.FindAnyWidget("OptionsBtn");
        m_RestartButton = layoutRoot.FindAnyWidget("RestartBtn");
        m_RespawnButton = layoutRoot.FindAnyWidget("respawn_button");
        if (m_RespawnButton)
        {
            m_RespawnButtonLabel = TextWidget.Cast(m_RespawnButton.FindAnyWidget("respawn_button_label"));
        }
                
		m_PrioQ = ButtonWidget.Cast(layoutRoot.FindAnyWidget("QueueBtn"));
        m_Website = ButtonWidget.Cast(layoutRoot.FindAnyWidget("WebsiteBtn"));
        m_Discord = ButtonWidget.Cast(layoutRoot.FindAnyWidget("DiscordBtn"));
        m_ViewToggleButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("ViewToggleBtn"));
        m_LonelinessDateTime = TextWidget.Cast(layoutRoot.FindAnyWidget("DateTimeText"));

        m_TextMenuRoot = layoutRoot.FindAnyWidget("TextMenuRoot");
        m_ViewToggleIndicator = layoutRoot.FindAnyWidget("ViewToggleIndicator");
        m_TextMenuContinueButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("TextMenuContinueBtn"));
        m_TextMenuOptionsButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("TextMenuOptionsBtn"));
        m_TextMenuRulesButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("TextMenuRulesBtn"));
        m_TextMenuWebsiteButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("TextMenuWebsiteBtn"));
        m_TextMenuDiscordButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("TextMenuDiscordBtn"));
        m_TextMenuExitButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("TextMenuExitBtn"));

        m_TopShader         = ImageWidget.Cast(layoutRoot.FindAnyWidget("TopShader"));
        m_BottomShader      = ImageWidget.Cast(layoutRoot.FindAnyWidget("BottomShader"));
        
        m_TopSpacer         = layoutRoot.FindAnyWidget("TopSpacer");
        m_MenuDivider       = ImageWidget.Cast(layoutRoot.FindAnyWidget("MenuDivider"));
        m_SeparatorPanel    = m_MenuDivider;
        m_BottomSpacer      = layoutRoot.FindAnyWidget("BottomSpacer");

        // Set the colors of Shader, Divider
        if (m_TopShader)    m_TopShader.SetColor(colorScheme.TopShader());
        if (m_BottomShader) m_BottomShader.SetColor(colorScheme.BottomShader());
        if (m_MenuDivider)  m_MenuDivider.SetColor(colorScheme.Separator());
        if (m_LonelinessDateTime) m_LonelinessDateTime.SetColor(colorScheme.PrimaryText());

        cuiElmnt.proBtnCB(this, ButtonWidget.Cast(m_ContinueButton), "", colorScheme.PrimaryText(), colorScheme.BrandColor(), this, "OnClick_Continue");
        cuiElmnt.proBtnCB(this, ButtonWidget.Cast(m_OptionsButton), "", colorScheme.PrimaryText(), colorScheme.BrandColor(), this, "OnClick_Options");
        cuiElmnt.proBtnCB(this, ButtonWidget.Cast(m_Website), "", colorScheme.PrimaryText(), colorScheme.BrandColor(), this, "OpenRules");
        cuiElmnt.proBtnCB(this, ButtonWidget.Cast(m_PrioQ), "", colorScheme.PrimaryText(), colorScheme.BrandColor(), this, "OpenWebsite");
        cuiElmnt.proBtnCB(this, ButtonWidget.Cast(m_Discord), "", colorScheme.PrimaryText(), colorScheme.BrandColor(), this, "OpenDiscord");
        cuiElmnt.proBtnCB(this, ButtonWidget.Cast(m_ExitButton), "", colorScheme.PrimaryText(), colorScheme.BrandColor(), this, "OnClick_Exit");
        cuiElmnt.proBtnCB(this, m_ViewToggleButton, "", colorScheme.PrimaryText(), colorScheme.BrandColor(), this, "ToggleMenuView");
        cuiElmnt.proBtnCB(this, m_TextMenuContinueButton, "ПРОДОЛЖИТЬ", colorScheme.PrimaryText(), colorScheme.BrandColor(), this, "OnClick_Continue");
        cuiElmnt.proBtnCB(this, m_TextMenuOptionsButton, "НАСТРОЙКИ", colorScheme.PrimaryText(), colorScheme.BrandColor(), this, "OnClick_Options");
        cuiElmnt.proBtnCB(this, m_TextMenuRulesButton, "ПРАВИЛА", colorScheme.PrimaryText(), colorScheme.BrandColor(), this, "OpenRules");
        cuiElmnt.proBtnCB(this, m_TextMenuWebsiteButton, "МАГАЗИН", colorScheme.PrimaryText(), colorScheme.BrandColor(), this, "OpenWebsite");
        cuiElmnt.proBtnCB(this, m_TextMenuDiscordButton, "DISCORD", colorScheme.PrimaryText(), colorScheme.BrandColor(), this, "OpenDiscord");
        cuiElmnt.proBtnCB(this, m_TextMenuExitButton, "ВЫЙТИ В МЕНЮ", colorScheme.PrimaryText(), colorScheme.BrandColor(), this, "OnClick_Exit");
        cuiElmnt.proSolidBtnCB(this, ButtonWidget.Cast(m_RespawnButton), "ВОЗРОЖДЕНИЕ", colorScheme.PrimaryText(), colorScheme.BrandColor(), this, "OpenRespawnConfirm");

        // These checks show/hide what buttons that are invalid or null
        ApplyRespawnButtonBaseStyles();
        if (m_RestartButton) m_RestartButton.Show(false);
        UpdateRespawnButtonVisibility();
        ApplyMenuView();
		UpdateLonelinessDateTime();

		HudShow(false);       
        
        Branding.ApplyLogo(m_Logo);

		return layoutRoot;
    }

	override void OnShow()
	{
		super.OnShow();
		UpdateLonelinessDateTime();
	}

	void UpdateLonelinessDateTime()
	{
		if (m_LonelinessDateTime)
			m_LonelinessDateTime.SetText(GetLonelinessDateTime());
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

   	override void Update(float timeslice)
	{
		if ((m_RulesOverlay != null) && m_RulesOverlay.IsOpen())
		{
			if (GetUApi().GetInputByID(UAUIBack).LocalPress())
			{
				m_RulesOverlay.Close();
				return;
			}
		}

		super.Update(timeslice);
		if (m_RestartButton) m_RestartButton.Show(false);
		UpdateRespawnButtonVisibility();
		ApplyMenuView();
	};

	void ToggleMenuView()
	{
		s_UseTextMenu = !s_UseTextMenu;
		ApplyMenuView();
	}

	void ApplyMenuView()
	{
		bool showIconMenu = !s_UseTextMenu;
		bool showContinue = false;
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		if (player && player.IsAlive())
			showContinue = true;

		if (m_TextMenuRoot) m_TextMenuRoot.Show(s_UseTextMenu);
		if (m_ViewToggleIndicator) m_ViewToggleIndicator.Show(s_UseTextMenu);

		if (m_ContinueButton) m_ContinueButton.Show(showIconMenu && showContinue);
		if (m_OptionsButton) m_OptionsButton.Show(showIconMenu);
		if (m_Website) m_Website.Show(showIconMenu);
		if (m_PrioQ) m_PrioQ.Show(showIconMenu);
		if (m_Discord) m_Discord.Show(showIconMenu);
		if (m_ExitButton) m_ExitButton.Show(showIconMenu);

		if (m_TextMenuContinueButton) m_TextMenuContinueButton.Show(s_UseTextMenu && showContinue);
	}

	void UpdateRespawnButtonVisibility()
	{
		if (!m_RespawnButton)
			return;

		bool showRespawn = false;
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		if (player && !player.IsAlive())
			showRespawn = true;

		m_RespawnButton.Show(showRespawn);
	}

	void ~InGameMenu()
	{
		if (m_RulesOverlay)
		{
			m_RulesOverlay.Dispose();
			m_RulesOverlay = null;
		}

		cuiElmnt.CleanupForOwner(this);
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

	void OpenRespawnConfirm()
	{
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		if (!player || player.IsAlive())
			return;

		CuiDialog.Show("ВОЗРОЖДЕНИЕ", "Вы действительно хотите возродиться?", true, this, "ConfirmRespawn", "", false, "ДА", "НЕТ");
	}

	void ConfirmRespawn()
	{
		if (GetGame().IsMultiplayer())
		{
			GameRespawn(true);
		}
	}

	void ApplyRespawnButtonBaseStyles()
	{
		if (!m_RespawnButton) return;

		Widget respawnPanel = m_RespawnButton.FindAnyWidget("respawn_button_panel");
		if (respawnPanel) respawnPanel.SetColor(UIColor.Transparent());
		if (m_RespawnButtonLabel) m_RespawnButtonLabel.SetColor(colorScheme.PrimaryText());
		m_RespawnButton.SetColor(UIColor.Transparent());
	}

	void ApplyRespawnButtonHoverStyles()
	{
		if (!m_RespawnButton) return;

		Widget respawnPanel = m_RespawnButton.FindAnyWidget("respawn_button_panel");
		if (respawnPanel) respawnPanel.SetColor(UIColor.Transparent());
		if (m_RespawnButtonLabel) m_RespawnButtonLabel.SetColor(UIColor.ZoneLoadingBlue());
		m_RespawnButton.SetColor(UIColor.Transparent());
	}

	override bool OnMouseEnter(Widget w, int x, int y)
	{
		if (w == m_RespawnButton)
		{
			ApplyRespawnButtonHoverStyles();
			return true;
		}

		return super.OnMouseEnter(w, x, y);
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		if (w == m_RespawnButton)
		{
			ApplyRespawnButtonBaseStyles();
			return true;
		}

		return super.OnMouseLeave(w, enterW, x, y);
	}

	void OpenConfiguredURL(string url)
	{
		if (url == "" || url == "#" || url == "test" || url == "/test")
			return;

		GetGame().OpenURL(url);
	}
}
