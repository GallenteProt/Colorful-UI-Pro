// NOTE TO ALL:  THE CUI ELEMENETS AND WRAPPER CONCEPTS ARE OBSOLETE. THIS CODE IS HERE FOR REFERENCE ONLY.
//  I have begun to think my concept for this wrapper was a bad idea. 4.0 has removed it as it just causes issues here and there and I dont want to keep fixing it.  
// However, I have a much better and more stable solution already in place for 4.0. 
class CUIButtonHandler : ScriptedWidgetEventHandler
{
    Class                m_Owner;          // Menu instance that registered this handler; used by cuiElmnt.CleanupForOwner()
    private ButtonWidget m_Button;
    private TextWidget   m_TextWidget;
    private ImageWidget  m_ImageWidget;
    private Widget       m_PanelWidget;
    private Widget       m_AccentWidget;
    private int          m_TextColor;
    private int          m_HoverColor;
    private string       m_ClickAction;
    private Class        m_TargetClass;
    private string       m_CallbackMethod;
    private string       m_ServerIP;
    private int          m_ServerPort;
    private string       m_IdleImagePath;
    private string       m_HoverImagePath;
    private TextWidget   m_TooltipWidget;
    private string       m_TooltipText;
    private bool         m_ImageStateButton = false;
    private float        m_BaseX;
    private float        m_BaseY;
    private float        m_BaseW;
    private float        m_BaseH;
    private bool         m_HasBaseMetrics = false;

    private bool         m_IconOnly = false;
    private int          m_IconImageIndex = -1;
    private bool         m_SolidBg = false;
    private bool         m_Disposed = false;

    private int ResolveRegularHoverColor()
    {
        if (m_HoverColor == colorScheme.PrimaryText())
            return UIColor.ZoneLoadingBlue();

        return m_HoverColor;
    }

    private bool IsPlayButton()
    {
        if (!m_Button) return false;
        string buttonName = m_Button.GetName();
        if (buttonName == "PlayBtn") return true;
        if (buttonName == "respawn_button") return true;
        return false;
    }

    private bool IsTextMenuButton()
    {
        if (!m_Button) return false;
        return m_Button.GetName().IndexOf("TextMenu") == 0;
    }

    private string GetTooltipText()
    {
        if (!m_Button) return "";

        string buttonName = m_Button.GetName();
        if (buttonName == "BackBtn") return "Вернуться";
        if (buttonName == "ContinueBtn") return "Вернуться";
        if (buttonName == "OptionsBtn") return "Настройки";
        if (buttonName == "TutorialBtn") return "Правила";
        if (buttonName == "RestartBtn") return "Правила";
        if (buttonName == "WebsiteBtn") return "Правила";
        if (buttonName == "QueueBtn") return "Магазин";
        if (buttonName == "DiscordBtn") return "Дискорд";
        if (buttonName == "ViewToggleBtn") return "Сменить вид меню";
        return "";
    }

    private void SetPlayButtonBorderColor(int color)
    {
        if (!m_Button) return;

        string buttonName = m_Button.GetName();
        Widget borderTop = m_Button.FindAnyWidget(buttonName + "_border_top");
        Widget borderBottom = m_Button.FindAnyWidget(buttonName + "_border_bottom");

        if (borderTop) borderTop.SetColor(color);
        if (borderBottom) borderBottom.SetColor(color);
    }

    private void SetPlayButtonSecondaryIconColor(int color)
    {
        if (!m_Button) return;

        string buttonName = m_Button.GetName();
        Widget secondaryIcon = m_Button.FindAnyWidget(buttonName + "_img_right");
        if (secondaryIcon) secondaryIcon.SetColor(color);
    }

    private void ApplyPlayButtonBaseStyles()
    {
        if (m_HasBaseMetrics)
        {
            m_Button.SetPos(m_BaseX, m_BaseY);
            m_Button.SetSize(m_BaseW, m_BaseH);
        }

        if (m_TextWidget) m_TextWidget.SetColor(colorScheme.PrimaryText());
        else m_Button.SetTextColor(colorScheme.PrimaryText());

        if (m_ImageWidget) m_ImageWidget.SetColor(colorScheme.PrimaryText());
        SetPlayButtonSecondaryIconColor(colorScheme.PrimaryText());
        if (m_PanelWidget) m_PanelWidget.SetColor(UIColor.Transparent());
        if (m_Button.GetName() == "respawn_button")
            SetPlayButtonBorderColor(colorScheme.PrimaryText());
        else
            SetPlayButtonBorderColor(UIColor.Transparent());
        m_Button.SetColor(UIColor.Transparent());
    }

    private void ApplyPlayButtonHoverStyles()
    {
        if (m_HasBaseMetrics)
        {
            m_Button.SetPos(m_BaseX, m_BaseY);
            m_Button.SetSize(m_BaseW, m_BaseH);
        }

        if (m_Button.GetName() == "respawn_button")
        {
            if (m_TextWidget) m_TextWidget.SetColor(UIColor.ZoneLoadingBlue());
            else m_Button.SetTextColor(UIColor.ZoneLoadingBlue());
        }
        else
        {
            if (m_TextWidget) m_TextWidget.SetColor(UIColor.ZoneLoadingBlue());
            else m_Button.SetTextColor(UIColor.ZoneLoadingBlue());
        }

        if (m_ImageWidget) m_ImageWidget.SetColor(UIColor.ZoneLoadingBlue());
        SetPlayButtonSecondaryIconColor(UIColor.ZoneLoadingBlue());
        if (m_PanelWidget) m_PanelWidget.SetColor(UIColor.Transparent());
        if (m_Button.GetName() == "respawn_button")
            SetPlayButtonBorderColor(UIColor.ZoneLoadingBlue());
        else
            SetPlayButtonBorderColor(UIColor.Transparent());
        m_Button.SetColor(UIColor.Transparent());
    }

    void CUIButtonHandler(ButtonWidget button, TextWidget textWidget, ImageWidget imageWidget, int textColor, int hoverColor, string clickAction, Class targetClass, string callbackMethod, string serverIP, int serverPort)
    {
        m_Button         = button;
        m_TextWidget     = textWidget;
        m_ImageWidget    = imageWidget;
        m_TextColor      = textColor;
        m_HoverColor     = hoverColor;
        m_ClickAction    = clickAction;
        m_TargetClass    = targetClass;
        m_CallbackMethod = callbackMethod;
        m_ServerIP       = serverIP;
        m_ServerPort     = serverPort;

        if (m_Button) m_PanelWidget = m_Button.FindAnyWidget(m_Button.GetName() + "_panel");
        if (m_Button) m_AccentWidget = m_Button.FindAnyWidget(m_Button.GetName() + "_accent");
        if (m_Button && m_Button.GetParent())
            m_TooltipWidget = TextWidget.Cast(m_Button.GetParent().FindAnyWidget("TopButtonTooltip"));
        m_TooltipText = GetTooltipText();
        if (m_Button)
        {
            m_Button.GetPos(m_BaseX, m_BaseY);
            m_Button.GetSize(m_BaseW, m_BaseH);
            m_HasBaseMetrics = true;
        }
        if (m_Button) m_Button.SetHandler(this);
        ApplyBaseStyles();
    }

    void Dispose()
    {
        HideTooltip();
        if (m_Button) m_Button.SetHandler(null);
        m_Button = null;
        m_TextWidget = null;
        m_ImageWidget = null;
        m_PanelWidget = null;
        m_AccentWidget = null;
        m_TooltipWidget = null;

        if (GetGame())
        {
            ScriptCallQueue queue = GetGame().GetCallQueue(CALL_CATEGORY_GUI);
            queue.Remove(this.InvokeCallback);
            queue.Remove(this.DoDirectConnect);
        }

        m_Disposed = true;
    }

    bool IsDisposed()
    {
        return m_Disposed;
    }

    private void ShowTooltip()
    {
        if (!m_TooltipWidget || m_TooltipText == "") return;

        m_TooltipWidget.SetText(m_TooltipText);
        m_TooltipWidget.SetColor(UIColor.ZoneLoadingBlue());
        m_TooltipWidget.Show(true);
    }

    private void HideTooltip()
    {
        if (m_TooltipWidget) m_TooltipWidget.Show(false);
    }

    void SetIconOnly(bool isIconOnly, int imageIdx)
    {
        m_IconOnly = isIconOnly;
        m_IconImageIndex = imageIdx;
        ApplyBaseStyles();
    }

    void SetSolidBg(bool solidOn)
    {
        m_SolidBg = solidOn;
        ApplyBaseStyles();
    }

    void SetImageStates(string idleImagePath, string hoverImagePath)
    {
        m_IdleImagePath = idleImagePath;
        m_HoverImagePath = hoverImagePath;
        m_ImageStateButton = true;
        ApplyBaseStyles();
    }

    private void ApplyBaseStyles()
    {
        if (!m_Button) return;

        if ((m_Button.GetFlags() & WidgetFlags.IGNOREPOINTER) == WidgetFlags.IGNOREPOINTER) return;

        if (IsTextMenuButton())
        {
            if (m_TextWidget)
            {
                m_TextWidget.Show(true);
                m_TextWidget.SetAlpha(1);
                m_TextWidget.SetColor(m_TextColor);
            }
            else
            {
                m_Button.SetTextColor(m_TextColor);
            }

            if (m_ImageWidget) m_ImageWidget.Show(false);
            if (m_PanelWidget) m_PanelWidget.SetColor(UIColor.Transparent());
            if (m_AccentWidget) m_AccentWidget.SetColor(colorScheme.Separator());
            m_Button.SetColor(UIColor.Transparent());
            return;
        }

        if (m_ImageStateButton)
        {
            m_Button.SetText("");
            if (m_TextWidget) m_TextWidget.Show(false);
            if (m_ImageWidget)
            {
                m_ImageWidget.LoadImageFile(0, m_IdleImagePath);
                m_ImageWidget.SetImage(0);
                m_ImageWidget.SetColor(UIColor.White());
                m_ImageWidget.SetAlpha(1);
                m_ImageWidget.Show(true);
            }
            m_Button.SetColor(UIColor.Transparent());
            return;
        }

        if (m_IconOnly)
        {
            if (m_TextWidget) m_TextWidget.Show(false);
            m_Button.SetText("");

            if (m_ImageWidget)
            {
                if (m_IconImageIndex >= 0) m_ImageWidget.SetImage(m_IconImageIndex);
                m_ImageWidget.Show(true);
                m_ImageWidget.SetColor(m_TextColor);
            }

            if (m_PanelWidget) m_PanelWidget.SetColor(UIColor.Transparent());
            m_Button.SetColor(UIColor.Transparent());
            return;
        }

        if (m_SolidBg)
        {
            if (IsPlayButton())
            {
                ApplyPlayButtonBaseStyles();
                return;
            }

            if (m_HasBaseMetrics)
            {
                m_Button.SetPos(m_BaseX, m_BaseY);
                m_Button.SetSize(m_BaseW, m_BaseH);
            }

            if (m_TextColor == colorScheme.BrandColor())
            {
                if (m_TextWidget) m_TextWidget.SetColor(colorScheme.BtnHoverText());
                else m_Button.SetTextColor(colorScheme.BtnHoverText());
            }
            else
            {
                if (m_TextWidget) m_TextWidget.SetColor(colorScheme.PrimaryText());
                else m_Button.SetTextColor(colorScheme.PrimaryText());
            }

            if (m_ImageWidget)
            {
                if (m_IconImageIndex >= 0) m_ImageWidget.SetImage(m_IconImageIndex);
                if (m_TextColor == colorScheme.BrandColor())
                    m_ImageWidget.SetColor(colorScheme.BtnHoverText());
                else
                    m_ImageWidget.SetColor(colorScheme.PrimaryText());
            }

            if (m_PanelWidget) m_PanelWidget.SetColor(m_TextColor);
            m_Button.SetColor(UIColor.Transparent());
            return;
        }

        if (!m_TextWidget && !m_ImageWidget)
        {
            m_Button.SetTextColor(m_TextColor);
            m_Button.SetColor(UIColor.Transparent());
            return;
        }

        if (m_TextWidget)
        {
            m_TextWidget.Show(true);
            m_TextWidget.SetAlpha(1);
            m_TextWidget.SetColor(m_TextColor);
        }
        else m_Button.SetTextColor(m_TextColor);

        if (m_ImageWidget)
        {
            m_ImageWidget.SetColor(m_TextColor);
            if (m_IconImageIndex >= 0) m_ImageWidget.SetImage(m_IconImageIndex);
        }

        m_Button.SetColor(UIColor.Transparent());
    }

    private void ApplyHoverStyles()
    {
        if (!m_Button) return;

        if ((m_Button.GetFlags() & WidgetFlags.IGNOREPOINTER) == WidgetFlags.IGNOREPOINTER) return;

        int hoverTextColor = ResolveRegularHoverColor();

        if (IsTextMenuButton())
        {
            if (m_TextWidget)
            {
                m_TextWidget.Show(true);
                m_TextWidget.SetAlpha(1);
                m_TextWidget.SetColor(hoverTextColor);
            }
            else
            {
                m_Button.SetTextColor(hoverTextColor);
            }

            if (m_PanelWidget) m_PanelWidget.SetColor(UIColor.ZonePanelSoft());
            if (m_AccentWidget) m_AccentWidget.SetColor(UIColor.ZoneLoadingBlue());
            m_Button.SetColor(UIColor.Transparent());
            return;
        }

        if (m_ImageStateButton)
        {
            if (m_ImageWidget)
            {
                m_ImageWidget.LoadImageFile(0, m_HoverImagePath);
                m_ImageWidget.SetImage(0);
                m_ImageWidget.SetColor(UIColor.White());
                m_ImageWidget.SetAlpha(1);
                m_ImageWidget.Show(true);
            }
            m_Button.SetColor(UIColor.Transparent());
            return;
        }

        if (m_IconOnly)
        {
            if (m_ImageWidget) m_ImageWidget.SetColor(hoverTextColor);
            if (m_PanelWidget)
            {
                if (m_HoverColor == UIColor.ZoneDanger())
                    m_PanelWidget.SetColor(UIColor.ZoneDangerDark());
                else
                    m_PanelWidget.SetColor(UIColor.ZonePanelSoft());

                m_Button.SetColor(UIColor.Transparent());
            }
            else
            {
                m_Button.SetColor(UIColor.Transparent());
            }
            return;
        }

        if (m_SolidBg)
        {
            if (IsPlayButton())
            {
                ApplyPlayButtonHoverStyles();
                return;
            }

            if (m_HasBaseMetrics)
            {
                float hoverX = m_BaseX + 0.018;
                float hoverW = m_BaseW + 0.024;
                m_Button.SetPos(hoverX, m_BaseY);
                m_Button.SetSize(hoverW, m_BaseH);
            }

            if (m_TextWidget)
            {
                if (m_HoverColor == UIColor.ZoneDanger())
                    m_TextWidget.SetColor(colorScheme.PrimaryText());
                else
                    m_TextWidget.SetColor(colorScheme.BtnHoverText());
            }
            else
            {
                if (m_HoverColor == UIColor.ZoneDanger())
                    m_Button.SetTextColor(colorScheme.PrimaryText());
                else
                    m_Button.SetTextColor(colorScheme.BtnHoverText());
            }
            if (m_ImageWidget)
            {
                if (m_HoverColor == UIColor.ZoneDanger())
                    m_ImageWidget.SetColor(colorScheme.PrimaryText());
                else
                    m_ImageWidget.SetColor(colorScheme.BtnHoverText());
            }
            if (m_PanelWidget) m_PanelWidget.SetColor(m_HoverColor);
            m_Button.SetColor(UIColor.Transparent());
            return;
        }

        if (!m_TextWidget && !m_ImageWidget)
        {
            if (m_HoverColor == UIColor.ZoneDanger())
            {
                m_Button.SetTextColor(colorScheme.PrimaryText());
                m_Button.SetColor(UIColor.ZoneDanger());
            }
            else
            {
                m_Button.SetTextColor(hoverTextColor);
                m_Button.SetColor(UIColor.Transparent());
            }

            return;
        }

        if (m_TextWidget)
        {
            // Keep the label fully opaque above the hover brush. Some SDF
            // labels preserve alpha from prior widget state unless reset.
            m_TextWidget.Show(true);
            m_TextWidget.SetAlpha(1);
            if (m_HoverColor == UIColor.ZoneDanger())
                m_TextWidget.SetColor(colorScheme.PrimaryText());
            else
                m_TextWidget.SetColor(hoverTextColor);
        }

        if (m_ImageWidget)
        {
            if (m_HoverColor == UIColor.ZoneDanger())
                m_ImageWidget.SetColor(colorScheme.PrimaryText());
            else
                m_ImageWidget.SetColor(hoverTextColor);
        }

        if (m_HoverColor == UIColor.ZoneDanger())
            m_Button.SetColor(UIColor.ZoneDanger());
        else
            m_Button.SetColor(UIColor.Transparent());
    }

    override bool OnMouseEnter(Widget w, int x, int y)
    {
        if (w != m_Button) return false;
        ApplyHoverStyles();
        ShowTooltip();
        return true;
    }

    override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
    {
        if (w != m_Button) return false;
        HideTooltip();
        ApplyBaseStyles();
        return true;
    }

    override bool OnFocus(Widget w, int x, int y)
    {
        if (w != m_Button) return false;
        return false;
    }

    override bool OnFocusLost(Widget w, int x, int y)
    {
        if (w != m_Button) return false;
        HideTooltip();
        ApplyBaseStyles();
        return true;
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w != m_Button) return false;
        if (IsPlayButton()) ApplyPlayButtonBaseStyles();

        if (m_ClickAction != "")
        {
            GetGame().OpenURL(m_ClickAction);
            return true;
        }

        // Defer the callback to the next GUI tick. Calling m_CallbackMethod
        // synchronously here is a use-after-free hazard: methods like
        // MainMenu.OpenSettings or OptionsMenu.Back destroy the current
        // menu, which fires CleanupForOwner, which deletes THIS handler
        // mid-OnClick. When OnClick returns, the engine's event dispatcher
        // does a vtable touch on the freed handler -> ACCESS_VIOLATION at
        // call qword [rax+0x88]. Posting via CallLater(0) lets the engine
        // finish its event loop on a still-live handler before menu
        // teardown begins.
        if (m_TargetClass && m_CallbackMethod != "")
        {
            GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.InvokeCallback, 0, false);
            return true;
        }

        if (m_ServerIP != "" && m_ServerPort > 0)
        {
            GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.DoDirectConnect, 100, false);
            return true;
        }

        return true;
    }

    void InvokeCallback()
    {
        if (m_TargetClass && m_CallbackMethod != "")
            GetGame().GameScript.CallFunction(m_TargetClass, m_CallbackMethod, null, 0);
    }

    override bool OnMouseButtonUp(Widget w, int x, int y, int button)
    {
        return false;
    }

    void DoDirectConnect()
    {
        if (m_ServerIP != "" && m_ServerPort > 0)
        {
            DayZGame game = DayZGame.Cast(GetGame());
            if (game) {
                game.ConnectFromJoin(m_ServerIP, m_ServerPort);
            }
        }
    }
}
// NOTE: THese are not the final elements.  This was just a quick shit way of getting thing together for a release candadite. 
// I schlocked it together and will be putting something more appropriate after the release candadite.  
class cuiElmnt
{
    static ref array<ref CUIButtonHandler> s_Handlers = new array<ref CUIButtonHandler>();

    protected static void RegisterHandler(CUIButtonHandler handler, Class owner)
    {
        if (!handler) return;
        if (!s_Handlers)
            s_Handlers = new array<ref CUIButtonHandler>();

        handler.m_Owner = owner;
        s_Handlers.Insert(handler);
    }

    // Cleanup ALL handlers. Last-resort full purge — prefer CleanupForOwner.
    static void Cleanup()
    {
        if (!s_Handlers) return;

        for (int i = 0; i < s_Handlers.Count(); i++)
        {
            if (s_Handlers[i]) s_Handlers[i].Dispose();
            delete s_Handlers[i];
        }
        s_Handlers.Clear();
    }

    // Dispose every handler tagged with `owner`. Call from each menu's destructor:
    //   void ~MyMenu() { cuiElmnt.CleanupForOwner(this); }
    //
    // Removing synchronously can free the handler while the engine is still
    // dispatching its callback. Dispose now, then purge after the GUI tick.
    static void CleanupForOwner(Class owner)
    {
        if (!owner) return;
        if (!s_Handlers) return;

        for (int i = 0; i < s_Handlers.Count(); i++)
        {
            if (s_Handlers[i] && s_Handlers[i].m_Owner == owner)
                s_Handlers[i].Dispose();
        }

        if (GetGame())
            GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(PurgeDisposed, 1000, false);
    }

    static void PurgeDisposed()
    {
        if (!s_Handlers) return;

        for (int i = s_Handlers.Count() - 1; i >= 0; i--)
        {
            if (s_Handlers[i] == null || s_Handlers[i].IsDisposed())
                s_Handlers.RemoveOrdered(i);
        }
    }

    protected static void GetParts(ButtonWidget button, out TextWidget label, out ImageWidget icon)
    {
        label = null; icon = null;
        if (!button) return;
        label = TextWidget.Cast(button.FindAnyWidget(button.GetName() + "_label"));
        icon  = ImageWidget.Cast(button.FindAnyWidget(button.GetName() + "_img"));
        if (!icon) icon = ImageWidget.Cast(button.FindAnyWidget(button.GetName() + "_image"));
    }

    static void proSolidBtn(Class owner, ButtonWidget button, string text, int bgColor, int hoverBgColor, string clickAction)
    {
        if (!button) return;

        button.SetText(text);
        TextWidget label; ImageWidget icon; GetParts(button, label, icon);
        if (label) { label.SetText(text); button.SetText(""); }

        CUIButtonHandler h = new CUIButtonHandler(button, label, icon, bgColor, hoverBgColor, clickAction, null, "", "", 0);
        h.SetSolidBg(true);
        RegisterHandler(h, owner);
    }

    static void proSolidBtnDC(Class owner, ButtonWidget button, string text, int bgColor, int hoverBgColor, string serverIP, int serverPort)
    {
        if (!button) return;

        button.SetText(text);
        TextWidget label; ImageWidget icon; GetParts(button, label, icon);
        if (label) { label.SetText(text); button.SetText(""); }

        CUIButtonHandler h = new CUIButtonHandler(button, label, icon, bgColor, hoverBgColor, "", null, "", serverIP, serverPort);
        h.SetSolidBg(true);
        RegisterHandler(h, owner);
    }

    static void proSolidBtnCB(Class owner, ButtonWidget button, string text, int bgColor, int hoverBgColor, Class targetClass, string callbackMethod)
    {
        if (!button) return;

        button.SetText(text);
        TextWidget label; ImageWidget icon; GetParts(button, label, icon);
        if (label) { label.SetText(text); button.SetText(""); }

        CUIButtonHandler h = new CUIButtonHandler(button, label, icon, bgColor, hoverBgColor, "", targetClass, callbackMethod, "", 0);
        h.SetSolidBg(true);
        RegisterHandler(h, owner);
    }

    static void proBtnURL(Class owner, ButtonWidget button, string text, int textColor, int hoverColor, string clickAction)
    {
        if (!button) return;

        button.SetText(text);
        TextWidget label; ImageWidget icon; GetParts(button, label, icon);
        if (label) { label.SetText(text); button.SetText(""); }

        ImageWidget handlerIcon = icon;
        if (icon && text != "")
        {
            icon.SetColor(hoverColor);
            handlerIcon = NULL;
        }

        CUIButtonHandler h = new CUIButtonHandler(button, label, handlerIcon, textColor, hoverColor, clickAction, null, "", "", 0);

        if (text == "" && icon)
        {
            h.SetIconOnly(true, -1);
        }

        RegisterHandler(h, owner);
    }

    static void proBtnDC(Class owner, ButtonWidget button, string text, int textColor, int hoverColor, string serverIP, int serverPort)
    {
        if (!button) return;

        button.SetText(text);
        TextWidget label; ImageWidget icon; GetParts(button, label, icon);
        if (label) { label.SetText(text); button.SetText(""); }
        if (icon) icon.SetColor(colorScheme.Icons());

        CUIButtonHandler h = new CUIButtonHandler(button, label, NULL, textColor, hoverColor, "", null, "", serverIP, serverPort);
        RegisterHandler(h, owner);
    }

    static void proIconBtn(Class owner, ButtonWidget button, int iconImageIndex, int iconColor, int hoverColor, string clickAction)
    {
        if (!button) return;

        button.SetText("");
        TextWidget label; ImageWidget icon; GetParts(button, label, icon);
        if (label) label.Show(false);

        CUIButtonHandler h = new CUIButtonHandler(button, NULL, icon, iconColor, hoverColor, clickAction, null, "", "", 0);
        h.SetIconOnly(true, iconImageIndex);
        if (icon) icon.SetColor(iconColor);

        RegisterHandler(h, owner);
    }

    static void proBtnCB(Class owner, ButtonWidget button, string text, int textColor, int hoverColor, Class targetClass, string callbackMethod)
    {
        if (!button) return;

        button.SetText(text);
        TextWidget label; ImageWidget icon; GetParts(button, label, icon);
        if (label) { label.SetText(text); button.SetText(""); }

        ImageWidget handlerIcon = icon;
        if (icon && text != "")
        {
            icon.SetColor(hoverColor);
            handlerIcon = NULL; // Match proBtnURL: Handler handles text, script handles icon color
        }

        CUIButtonHandler h = new CUIButtonHandler(button, label, handlerIcon, textColor, hoverColor, "", targetClass, callbackMethod, "", 0);

        if (text == "" && icon)
        {
            h.SetIconOnly(true, -1);
        }

        RegisterHandler(h, owner);
    }

    static void proImageBtnURL(Class owner, ButtonWidget button, string idleImage, string hoverImage, string clickAction)
    {
        if (!button) return;

        TextWidget label; ImageWidget icon; GetParts(button, label, icon);
        CUIButtonHandler h = new CUIButtonHandler(button, label, icon, UIColor.White(), UIColor.White(), clickAction, null, "", "", 0);
        h.SetImageStates(idleImage, hoverImage);
        RegisterHandler(h, owner);
    }

    static void proImageBtnDC(Class owner, ButtonWidget button, string idleImage, string hoverImage, string serverIP, int serverPort)
    {
        if (!button) return;

        TextWidget label; ImageWidget icon; GetParts(button, label, icon);
        CUIButtonHandler h = new CUIButtonHandler(button, label, icon, UIColor.White(), UIColor.White(), "", null, "", serverIP, serverPort);
        h.SetImageStates(idleImage, hoverImage);
        RegisterHandler(h, owner);
    }

    static void proImageBtnCB(Class owner, ButtonWidget button, string idleImage, string hoverImage, Class targetClass, string callbackMethod)
    {
        if (!button) return;

        TextWidget label; ImageWidget icon; GetParts(button, label, icon);
        CUIButtonHandler h = new CUIButtonHandler(button, label, icon, UIColor.White(), UIColor.White(), "", targetClass, callbackMethod, "", 0);
        h.SetImageStates(idleImage, hoverImage);
        RegisterHandler(h, owner);
    }
}
