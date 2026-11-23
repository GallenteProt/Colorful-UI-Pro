// NOTE: You cannot use CUI Elements in this file, as it is loaded before the CUI is initialized.
//       To control elements of this file, edit the at the Colorful-UI/Scripts/3_Game/UIConfig/Settings.c

string CuiLoadingDefaultText()
{
    return "ЗАГРУЗКА...";
}

string CuiTranslateLoadingStatus(string status)
{
    if (status == "")
        return CuiLoadingDefaultText();

    if (status == "Loading")
        return "ЗАГРУЗКА";

    if (status == "Loading...")
        return "ЗАГРУЗКА...";

    if (status == "Connecting")
        return "ПОДКЛЮЧЕНИЕ";

    if (status == "Connecting...")
        return "ПОДКЛЮЧЕНИЕ...";

    if (status == "Wait for host")
        return "ОЖИДАНИЕ СЕРВЕРА";

    return status;
}

// Phase 1: Loading  -------------------------------------------------------------
modded class LoadingScreen
{
    protected ImageWidget m_Background, m_TopShader, m_BottomShader, m_Mask, m_Logo;
    protected TextWidget m_LoadingMsg, m_Title;
    protected ProgressBarWidget m_ProgressLoading;
    protected Widget m_LoadingDimmer;

    void LoadingScreen(DayZGame game)
    {
        m_DayZGame = game;
        m_WidgetRoot = game.GetLoadingWorkspace().CreateWidgets("Colorful-UI/GUI/layouts/loading/cui.loading.layout");

        Class.CastTo(m_Background, m_WidgetRoot.FindAnyWidget("ImageBackground"));
        Class.CastTo(m_Logo, m_WidgetRoot.FindAnyWidget("Logo"));
        Class.CastTo(m_TopShader, m_WidgetRoot.FindAnyWidget("TopShader"));
        Class.CastTo(m_BottomShader, m_WidgetRoot.FindAnyWidget("BottomShader"));
        Class.CastTo(m_LoadingMsg, m_WidgetRoot.FindAnyWidget("LoadingMsg"));
        Class.CastTo(m_ProgressLoading, m_WidgetRoot.FindAnyWidget("LoadingBar"));
        Class.CastTo(m_Title, m_WidgetRoot.FindAnyWidget("Title"));
        m_LoadingDimmer = m_WidgetRoot.FindAnyWidget("LoadingBlueTint");

        if (m_TopShader) m_TopShader.SetColor(colorScheme.TopShader());
        if (m_BottomShader) m_BottomShader.SetColor(colorScheme.BottomShader());
        if (m_LoadingMsg) m_LoadingMsg.SetColor(colorScheme.LoadingMsg());
        if (m_LoadingMsg) m_LoadingMsg.SetText(CuiLoadingDefaultText());
        if (m_ProgressLoading) m_ProgressLoading.SetColor(colorScheme.Loadingbar());

        Branding.ApplyLogo(m_Logo);
        ApplyLoadingScreenTint();
        ProgressAsync.SetProgressData(m_ProgressLoading);
        ProgressAsync.SetUserData(m_Background);
    }

    // Override vanilla LoadingScreen.Show to avoid the NPE on m_ProgressText
    // (vanilla expects widgets our layout doesn't have). Routes background
    // through GetLoadingBackground() so loading phases use the same custom
    // Loneliness screen pool as hints instead of vanilla assets.
    override void Show()
    {
        if (m_Background)
            m_Background.LoadImageFile(0, GetLoadingBackground());

        ApplyLoadingScreenTint();
    }

    protected void ApplyLoadingScreenTint()
    {
        if (m_Background)
            m_Background.SetColor(ARGB(255, 255, 255, 255));
        if (m_LoadingDimmer)
            m_LoadingDimmer.SetColor(colorScheme.LoadingDim());
    }

    override void SetTitle(string title)
    {
        if (!m_Title)
        {
            m_Title = TextWidget.Cast(m_WidgetRoot.FindAnyWidget("Title"));
        }

        if (m_Title)
        {
            m_Title.SetText(title);
        }
    }

    // Vanilla SetStatus(string) writes to its m_TextWidgetStatus, which on our
    // layout is m_LoadingMsg. Engine calls SetStatus("") moments after the
    // constructor runs, wiping our "GAME IS LOADING!" default and exposing the
    // layout's baked placeholder ("Load screen message"). Override to ignore
    // empty strings — keep the constructor default visible until a real
    // engine status (e.g. "Connecting...") replaces it.
    override void SetStatus(string status)
    {
        if (!m_LoadingMsg) return;
        m_LoadingMsg.SetText(CuiTranslateLoadingStatus(status));
    }
}

// Phase 2: Logging In ------------------------------------------------------------
// NOTE: modded class declarations MUST NOT have an `extends` clause — vanilla's
// parent is implicit, and adding `extends X` silently breaks the modded chain
// (the override never runs, vanilla's body runs and crashes on null widget refs).
modded class LoginTimeBase
{
    protected ImageWidget m_Background, m_TopShader, m_BottomShader, m_ExitIcon, m_Logo;
    protected TextWidget m_LoadingMsg, m_ExitText;
    protected ProgressBarWidget m_ProgressLoading;
    protected Widget m_LoadingDimmer;

    override Widget Init()
    {
        layoutRoot = GetGame().GetWorkspace().CreateWidgets("Colorful-UI/GUI/layouts/loading/cui.loggingIn.layout");

        m_Background = ImageWidget.Cast(layoutRoot.FindAnyWidget("ImageBackground"));
        m_Logo = ImageWidget.Cast(layoutRoot.FindAnyWidget("Logo"));
        m_TopShader = ImageWidget.Cast(layoutRoot.FindAnyWidget("TopShader"));
        m_BottomShader = ImageWidget.Cast(layoutRoot.FindAnyWidget("BottomShader"));
        m_LoadingMsg = TextWidget.Cast(layoutRoot.FindAnyWidget("LoadingMsg"));
        // Point vanilla's m_txtLabel field at our LoadingMsg widget too — any
        // vanilla code path that still touches m_txtLabel won't NPE.
        m_txtLabel = m_LoadingMsg;
        m_ProgressLoading = ProgressBarWidget.Cast(layoutRoot.FindAnyWidget("LoadingBar"));

        m_btnLeave = ButtonWidget.Cast(layoutRoot.FindAnyWidget("btnLeave"));
        m_ExitText = TextWidget.Cast(layoutRoot.FindAnyWidget("ExitText"));
        m_ExitIcon = ImageWidget.Cast(layoutRoot.FindAnyWidget("Exit"));
        m_LoadingDimmer = layoutRoot.FindAnyWidget("LoadingBlueTint");

        if (m_Background)
            m_Background.LoadImageFile(0, GetLoadingBackground());

        ApplyLoginTimeBackgroundTint();

        if (m_TopShader) m_TopShader.SetColor(colorScheme.TopShader());
        if (m_BottomShader) m_BottomShader.SetColor(colorScheme.BottomShader());
        if (m_LoadingMsg) m_LoadingMsg.SetColor(colorScheme.LoadingMsg());
        if (m_ProgressLoading) m_ProgressLoading.SetColor(colorScheme.Loadingbar());
        if (m_ExitIcon) m_ExitIcon.SetColor(colorScheme.Icons());

        Branding.ApplyLogo(m_Logo);

        return layoutRoot;
    }

    override void Show()
    {
        super.Show();
        ApplyLoginTimeBackgroundTint();
    }

    protected void ApplyLoginTimeBackgroundTint()
    {
        if (m_Background)
            m_Background.SetColor(ARGB(255, 255, 255, 255));
        if (m_LoadingDimmer)
            m_LoadingDimmer.SetColor(colorScheme.LoadingDim());
    }

    override bool OnMouseEnter(Widget w, int x, int y)
    {
        if (w == m_btnLeave)
        {
            if (m_ExitText) m_ExitText.SetColor(colorScheme.ButtonHover());
            if (m_btnLeave) m_btnLeave.SetColor(UIColor.Transparent());
            return true;
        }
        return false;
    }

    override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
    {
        if (w == m_btnLeave)
        {
            if (m_ExitText) m_ExitText.SetColor(colorScheme.PrimaryText());
            return true;
        }
        return false;
    }

    // Override here on LoginTimeBase (not LoginTimeStatic) — vanilla's SetTime
    // body lives on LoginTimeBase and dereferences m_txtLabel (a widget our
    // layout doesn't have). LoginTimeStatic inherits, so engine dispatch
    // resolves to whichever class defines SetTime closest to the instance.
    override void SetTime(int time)
    {
        if (!layoutRoot) return;
        if (!m_LoadingMsg)
            m_LoadingMsg = TextWidget.Cast(layoutRoot.FindAnyWidget("LoadingMsg"));

        if (m_LoadingMsg)
        {
            if (IsRespawn())
                m_LoadingMsg.SetText("ВОЗРОЖДЕНИЕ ЧЕРЕЗ " + time.ToString());
            else
                m_LoadingMsg.SetText("ПОДКЛЮЧЕНИЕ К СЕРВЕРУ ЧЕРЕЗ " + time.ToString());
        }

        if (m_IsRespawn && time <= 1)
            GetGame().SetLoginTimerFinished();
    }
}

// Phase 3: Prio Queue  -------------------------------------------------------------
modded class LoginQueueBase
{
    protected ImageWidget m_Background, m_TopShader, m_BottomShader, m_ExitIcon, m_ShopIcon;
    protected TextWidget m_ExitText, m_PrioText;
    protected ProgressBarWidget m_ProgressLoading;
    protected ButtonWidget m_btnLeave, m_PrioQBtn;
    protected Widget m_LoadingDimmer;

    override Widget Init()
    {
        layoutRoot = GetGame().GetWorkspace().CreateWidgets("Colorful-UI/GUI/layouts/loading/cui.priorityQueue.layout");

        m_HintPanel = new UiHintPanelLoading(layoutRoot.FindAnyWidget("hint_frame0"));
        m_txtPosition = TextWidget.Cast(layoutRoot.FindAnyWidget("LoadingMsg"));
        m_txtNote = TextWidget.Cast(layoutRoot.FindAnyWidget("txtNote"));

        m_Background = ImageWidget.Cast(layoutRoot.FindAnyWidget("ImageBackground"));
        m_TopShader = ImageWidget.Cast(layoutRoot.FindAnyWidget("TopShader"));
        m_BottomShader = ImageWidget.Cast(layoutRoot.FindAnyWidget("BottomShader"));
        m_ProgressLoading = ProgressBarWidget.Cast(layoutRoot.FindAnyWidget("LoadingBar"));

        m_btnLeave = ButtonWidget.Cast(layoutRoot.FindAnyWidget("btnLeave"));
        m_ExitText = TextWidget.Cast(layoutRoot.FindAnyWidget("ExitText"));
        m_ExitIcon = ImageWidget.Cast(layoutRoot.FindAnyWidget("Exit"));

        m_PrioQBtn = ButtonWidget.Cast(layoutRoot.FindAnyWidget("btnPrioQ"));
        m_PrioText = TextWidget.Cast(layoutRoot.FindAnyWidget("PrioText"));
        m_ShopIcon = ImageWidget.Cast(layoutRoot.FindAnyWidget("shopIcon"));
        m_LoadingDimmer = layoutRoot.FindAnyWidget("LoadingBlueTint");

        if (m_Background) m_Background.LoadImageFile(0, GetLoadingBackground());
        ApplyLoginQueueBackgroundTint();
        if (m_ExitIcon) m_ExitIcon.SetColor(colorScheme.Icons());
        if (m_ShopIcon) m_ShopIcon.SetColor(colorScheme.Icons());
        if (m_ExitText) m_ExitText.SetText("ВЫЙТИ");
        if (m_PrioText) m_PrioText.SetText("МАГАЗИН");
        if (m_TopShader) m_TopShader.SetColor(colorScheme.TopShader());
        if (m_BottomShader) m_BottomShader.SetColor(colorScheme.BottomShader());
        if (m_ProgressLoading) m_ProgressLoading.SetColor(colorScheme.Loadingbar());

        if (CustomURL.PriorityQ == "#" || CustomURL.PriorityQ == "")
        {
            m_PrioQBtn.Show(false);
        }
        else
        {
            m_PrioQBtn.Show(true);
        }

        return layoutRoot;
    }

    override void Show()
    {
        if (!NoHints)
        {
            layoutRoot.Show(true);
            // Only allocate the hint panel once. Show() may fire multiple times
            // (reconnects, etc.); recreating leaks the previous panel + its video.
            if (!m_HintPanel)
                m_HintPanel = new UiHintPanelLoading(layoutRoot.FindAnyWidget("hint_frame0"));
        }

        ApplyLoginQueueBackgroundTint();
    }

    protected void ApplyLoginQueueBackgroundTint()
    {
        if (m_Background)
            m_Background.SetColor(ARGB(255, 255, 255, 255));
        if (m_LoadingDimmer)
            m_LoadingDimmer.SetColor(colorScheme.LoadingDim());
    }

    override void SetPosition(int position)
    {
        if (position != m_iPosition)
        {
            m_iPosition = position;
            if (m_txtPosition)
            {
                m_txtPosition.SetText("МЕСТО В ОЧЕРЕДИ: " + position.ToString());
                m_txtPosition.SetColor(colorScheme.LoadingMsg());
            }
        }
    }

    override bool OnMouseEnter(Widget w, int x, int y)
    {
        if (w == m_btnLeave)
        {
            if (m_ExitText) m_ExitText.SetColor(colorScheme.ButtonHover());
            if (m_btnLeave) m_btnLeave.SetColor(UIColor.Transparent());
            return true;
        }
        if (w == m_PrioQBtn)
        {
            if (m_PrioText) m_PrioText.SetColor(colorScheme.ButtonHover());
            if (m_PrioQBtn) m_PrioQBtn.SetColor(UIColor.Transparent());
            return true;
        }
        return false;
    }

    override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
    {
        if (w == m_btnLeave)
        {
            if (m_ExitText) m_ExitText.SetColor(colorScheme.PrimaryText());
            return true;
        }
        if (w == m_PrioQBtn)
        {
            if (m_PrioText) m_PrioText.SetColor(colorScheme.PrimaryText());
            return true;
        }
        return false;
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (button == MouseState.LEFT && w == m_PrioQBtn)
        {
            GetGame().OpenURL(CustomURL.PriorityQ);
            return false;
        }
        return super.OnClick(w, x, y, button);
    }
}

// Start at Main Menu  ----------------------------------------------------------
modded class DayZGame
{
    override void ConnectLaunch()
    {
        if (StartMainMenu) { MainMenuLaunch(); }
        else { ConnectFromCLI(); };
    };
}
