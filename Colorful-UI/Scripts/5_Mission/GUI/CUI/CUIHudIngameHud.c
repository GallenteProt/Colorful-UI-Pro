modded class IngameHud
{
    override void InitBadgesAndNotifiers()
    {
        m_StatesWidgetNames.Clear();
        m_StatesWidgets.Clear();
        m_Notifiers.Show(true);
        m_Badges.Show(true);

        m_StatesWidgetNames.Set(NTFKEY_THIRSTY, "Thirsty");
        m_StatesWidgetNames.Set(NTFKEY_HUNGRY, "Hungry");
        m_StatesWidgetNames.Set(NTFKEY_HEALTHY, "Health");
        m_StatesWidgetNames.Set(NTFKEY_BLEEDISH, "Blood");
        m_StatesWidgetNames.Set(NTFKEY_FEVERISH, "Temperature");

        m_Notifiers.Show(true);
        m_Badges.Show(true);

        int i = 0;
        int key = 0;
        for (i = 0; i < m_StatesWidgetNames.Count(); i++)
        {
            string widget_name = m_StatesWidgetNames.GetElement(i);
            key = m_StatesWidgetNames.GetKey(i);
            ImageWidget w;
            if (Class.CastTo(w, m_Notifiers.FindAnyWidget(String("Icon" + widget_name))))
            {
                m_StatesWidgets.Set(key, w);
                w.Show(true);

                string notifierImageSet = "HudStalkerSystem";
                if (widget_name == "Health" || widget_name == "Blood")
                    notifierImageSet = "dayz_gui";

                for (int y = 0; y < 5; y++)
                {
                    w.LoadImageFile(y, "set:" + notifierImageSet + " image:icon" + widget_name + y);
                }
                w.SetImage(0);
                float alpha = 0;
                m_TendencyStatusCritical.Remove(w);
                for (int x = 1; x < 4; x++)
                {
                    ImageWidget arrowUp;
                    if (Class.CastTo(arrowUp, m_Notifiers.FindAnyWidget(String(widget_name + "ArrowUp" + x.ToString()))))
                    {
                        arrowUp.Show(false);
                    }
                    ImageWidget arrowDown;
                    if (Class.CastTo(arrowDown, m_Notifiers.FindAnyWidget(String(widget_name + "ArrowDown" + x.ToString()))))
                    {
                        arrowDown.Show(false);
                    }
                }
            }
        }

        m_BadgesWidgetNames.Clear();
        m_BadgesWidgets.Clear();
        m_BadgesWidgetDisplay.Clear();
		m_BadgesSupportedLevels.Clear();
		
        m_BadgesWidgetNames.Set(NTFKEY_FRACTURE, "Fracture");
        m_BadgesWidgetNames.Set(NTFKEY_STUFFED, "Stomach");
        m_BadgesWidgetNames.Set(NTFKEY_SICK, "Sick");
        m_BadgesWidgetNames.Set(NTFKEY_WETNESS, "Wetness");
        m_BadgesWidgetNames.Set(NTFKEY_POISONED, "Poisoned");
        m_BadgesWidgetNames.Set(NTFKEY_BLEEDISH, "Bleeding");
        m_BadgesWidgetNames.Set(NTFKEY_LIVES, "Shock");
        m_BadgesWidgetNames.Set(NTFKEY_PILLS, "Pills");
		m_BadgesWidgetNames.Set(NTFKEY_LEGS, "InjuredLegs" );
		
		m_BadgesSupportedLevels.Insert(NTFKEY_LEGS,3);

        for (i = 0; i < m_BadgesWidgetNames.Count(); i++)
        {
            string badge_name = m_BadgesWidgetNames.GetElement(i);
            key = m_BadgesWidgetNames.GetKey(i);
            ImageWidget badge_widget;
    
            if (Class.CastTo(badge_widget, m_Badges.FindAnyWidget(badge_name)))
            {
                m_BadgesWidgets.Set(key, badge_widget);
                badge_widget.Show(false);
                m_BadgesWidgetDisplay.Set(key, false);
        
            if (m_BadgesSupportedLevels.Contains(key))
            {
                int badgeCountMax = m_BadgesSupportedLevels.Get(key);
            
                for (int q = 0; q < badgeCountMax; q++)
                {
                    badge_widget.LoadImageFile(q, "set:HudStalkerSystem image:icon" + badge_name + q);
                }
            }
        }

        m_AnyBadgeVisible = false;
        }
	}

    override void SetStomachState(int state)
    {
        super.SetStomachState(state);
        
        ImageWidget stomach = ImageWidget.Cast(m_Badges.FindAnyWidget("Stomach"));
        if (stomach)
        {
            stomach.LoadImageFile(0, "set:HudStalkerSystem image:iconStomach" + state);
        }
        else
        {
            Print("Failed to cast widget: Stomach");
        }
    }

    override void DisplayTendencyTemp(int key, int tendency, int status)
    {
        super.DisplayTendencyTemp(key, tendency, status);
        
        ImageWidget w = ImageWidget.Cast(m_Notifiers.FindAnyWidget(String("Icon" + m_StatesWidgetNames.Get(key))));
        TextWidget temp_top = TextWidget.Cast(m_Notifiers.FindAnyWidget("TemperatureValueTop"));
        TextWidget temp_bot = TextWidget.Cast(m_Notifiers.FindAnyWidget("TemperatureValueBottom"));
        float alpha = w.GetAlpha();

        if (tendency < 0)
        {
            temp_top.Show(true);
            temp_bot.Show(false);
        }
        else
        {
            temp_top.Show(false);
            temp_bot.Show(true);
        }
        switch (status)
        {
            case 2:
                w.SetColor( ARGB( alpha * 255, 220, 220, 220 ) );
                m_TendencyStatusCritical.Remove(w);
                w.SetImage( 2 );
                break;
            case 3:
                w.SetColor( ARGB( alpha * 255, 220, 220, 220 ) );
                m_TendencyStatusCritical.Remove(w);
                w.SetImage( 3 );
                break;
            case 4:
                w.SetColor( ARGB( alpha * 255, 220, 220, 220 ) );
                m_TendencyStatusCritical.Remove(w);
                w.SetImage( 3 );
                break;
            case 5:
                w.SetColor( ARGB( alpha * 255, 220, 220, 220 ) );
                m_TendencyStatusCritical.Remove(w);
                w.SetImage( 0 );
                break;
            case 6:
                w.SetColor( ARGB( alpha * 255, 220, 220, 220 ) );
                m_TendencyStatusCritical.Remove(w);
                w.SetImage( 4 );
                break;
            case 7:
                if (!m_TendencyStatusCritical.Contains(w))
                {
                    m_TendencyStatusCritical.Insert(w, ARGB(alpha * 255, 220, 0, 0));
                }
                w.SetImage( 4 );
                break;
            default:
                w.SetColor( ARGB( alpha * 255, 220, 220, 220 ) );
                m_TendencyStatusCritical.Remove(w);
                w.SetImage( 1 );
                break;
        }
    }

    override void SetStaminaBarVisibility( bool show )
	{
        if ( show )
		{
			m_Stamina.SetAlpha(1);
		}
		else
		{
			m_Stamina.SetAlpha(0.3);
		}

		m_IsStaminaVisible = show;
		m_StaminaTimer = 0;
    }

    override void SetStamina(int value, int range)
    {
        float sx, sy;
		float max = CfgGameplayHandler.GetStaminaMax();
		float percentage = 1;
		if (max != 0)
		{
			percentage =  range / max;
		}
		
		if (range != 0)
		{
			m_Stamina.SetCurrent(( value / range ) * 100);
		}
		else
		{
			m_Stamina.SetCurrent(0);
		}
		
		m_Stamina.GetSize(sx, sy);
		m_Stamina.SetSize(percentage, sy);
 		m_StaminaBackground.SetSize(1 - percentage, sy);

        
        if (!GetGame().IsMultiplayer())
        {
            if (GetGame().GetPlayer())
            {
                PlayerBase player;
                Class.CastTo(player, GetGame().GetPlayer());

                if (player)
                {
                    float h1 = player.GetHealth("", "");
                    float b1 = player.GetHealth("", "Blood");

                    GetDayZGame().GetBacklit().SetHealth(h1);
                    GetDayZGame().GetBacklit().SetBlood(b1);
                }
            }
        }
        GetDayZGame().GetBacklit().UpdatePlayer(false);
    }

    void MR_TNRJ_Update()
    {
        if (!m_HudPanelWidget)
            return;
        
        Widget notifiersPanel = m_HudPanelWidget.FindAnyWidget("NotifiersPanel");
        if (!notifiersPanel)
            return;
        
        Widget sleepingWidget = notifiersPanel.FindAnyWidget("TerjeSleeping");
        Widget mindWidget = notifiersPanel.FindAnyWidget("TerjeMind");
        
        float screenW, screenH;
        GetGame().GetWorkspace().GetScreenSize(screenW, screenH);
        
        float yPos = (screenH * -2.35) / 100;
        
        if (sleepingWidget)
        {
            sleepingWidget.SetPos((screenW * 10.4) / 100, yPos);
        }
        
        if (mindWidget)
        {
            mindWidget.SetPos((screenW * 12.4) / 100, yPos);
        }
    }
}

