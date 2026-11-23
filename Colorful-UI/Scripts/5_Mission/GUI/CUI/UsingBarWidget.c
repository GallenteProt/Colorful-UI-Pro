class ColorfulUIUsingBarWidget extends ScriptedWidgetEventHandler
{
    protected PlayerBase m_Player;
    protected ActionBase m_Action;
    protected int m_ActionState;
    protected ActionManagerBase m_AM;

    protected Widget m_Root;
    protected ProgressBarWidget m_ProgressBar;
    protected TextWidget m_ActionNameText; 

    void ColorfulUIUsingBarWidget()
    {
        m_Action = null;
        m_ActionState = -1;
        m_AM = null;

        GetGame().GetUpdateQueue(CALL_CATEGORY_GUI).Insert(Update);
    }

    void ~ColorfulUIUsingBarWidget() 
    {
        GetGame().GetUpdateQueue(CALL_CATEGORY_GUI).Remove(Update);
    }

    protected void OnWidgetScriptInit(Widget w) 
    {
        m_Root = w;
        m_Root.SetHandler(this);
        m_Root.Show(false); 

        m_ProgressBar = ProgressBarWidget.Cast(m_Root.FindAnyWidget("ProgressBarWidgetName"));
        m_ActionNameText = TextWidget.Cast(m_Root.FindAnyWidget("ActionNameTextWidgetName")); 
        m_Root.Update();
    }

    protected void Update(float timeslice) 
    {
        if (!m_Root)
            return;

        if (!m_Player) GetPlayer();
        if (!m_AM) GetActionManager();

        GetActions();

        if (m_Action && m_Action.HasProgress() && m_ActionState != UA_NONE && GetGame().GetUIManager().GetMenu() == null) 
        {
            if (m_ActionState == UA_PROCESSING) 
            {
                if (m_AM) 
                { 
                    float progress = Math.AbsFloat(m_AM.GetActionComponentProgress() * 100);
                    SetProgress(progress);
                    m_Root.Show(true);

                    if (m_Action) 
                    { 
                        string actionName = m_Action.GetText();
                        if (m_ActionNameText) 
                        { 
                            m_ActionNameText.SetText(actionName);
                        }
                    }
                }
            }
        } else 
        {
            m_Root.Show(false);
        }
    }

    protected void GetPlayer() 
    {
        if (GetGame().IsDedicatedServer())
            return;

        Class.CastTo(m_Player, GetGame().GetPlayer());
    }

    protected void GetActionManager() 
    {
        if (m_Player && m_Player.IsPlayerSelected()) 
        {
            Class.CastTo(m_AM, m_Player.GetActionManager());
        } else {
            m_AM = null;
        }
    }

    protected void GetActions() 
    {
        if (!m_AM) return;

        m_Action = null;
        m_ActionState = -1;
        m_Action = m_AM.GetRunningAction();

        if (m_Action && m_Action.GetInput().GetInputType() == ActionInputType.AIT_CONTINUOUS) 
        {
            m_ActionState = m_AM.GetActionState(m_Action);
        } else 
        {
            m_Action = null;
        }
    }

    protected void SetProgress(float progress) 
    {
        if (m_ProgressBar) {
            m_ProgressBar.SetCurrent(progress);
        }
    }
}
