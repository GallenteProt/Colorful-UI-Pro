modded class LogoutMenu extends UIScriptedMenu
{	
	protected ImageWidget m_TopShader, m_BottomShader, m_Logo;
	protected ButtonWidget m_Cancel;
	private	Widget m_timerText;
	protected TextWidget m_LogoutTimeText;

        override Widget Init()
        {
                layoutRoot = GetGame().GetWorkspace().CreateWidgets("Colorful-UI/GUI/layouts/menus/inGame/cui.logout.layout");

		m_Logo 				= ImageWidget.Cast(layoutRoot.FindAnyWidget("Logo"));
		m_LogoutTimeText 	= TextWidget.Cast(layoutRoot.FindAnyWidget("txtLogoutTime"));

		m_Cancel = ButtonWidget.Cast(layoutRoot.FindAnyWidget("BackBtn"));

		m_TopShader = ImageWidget.Cast(layoutRoot.FindAnyWidget("TopShader"));
		m_BottomShader = ImageWidget.Cast(layoutRoot.FindAnyWidget("BottomShader"));
		
		// Set the colors of Shader and Logout Timer
        m_TopShader.SetColor(colorScheme.TopShader());
        m_BottomShader.SetColor(colorScheme.BottomShader());
		m_LogoutTimeText.SetColor(colorScheme.LogOutTimer());

		// Set Button color, and type. 
		// COLOR  CONTROLS:: Colorful-UI-Pro\Colorful-UI\Scripts\3_Game\UIConfig\Color.c
		// GLOBAL CONTROLS:: Colorful-UI-Pro\Colorful-UI\Scripts\3_Game\UIConfig\Scheme.c
		// You can even be specific and set the color directly. ie. argb(255, 255, 255, 255);
		
		// Example of a button with a callback
		cuiElmnt.proBtnCB(this, ButtonWidget.Cast(m_Cancel),"",colorScheme.SecondaryText(),colorScheme.BrandColor(),this,"canelExit");

		// Make the player lay down. Change with what ever emote you want to use. 
		// Emotes:: scripts\5_mission\gui\gesturesmenu.c
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		if (player && player.GetEmoteManager() && !player.IsRestrained() && !player.IsUnconscious())
		{
			player.GetEmoteManager().CreateEmoteCBFromMenu(EmoteConstants.ID_EMOTE_LYINGDOWN);
			player.GetEmoteManager().GetEmoteLauncher().SetForced(EmoteLauncher.FORCE_DIFFERENT);
		}
		
		Branding.ApplyLogo(m_Logo);
		return layoutRoot;
	}

        void canelExit()
        {
                Hide();
                Cancel();
        }

	void ~LogoutMenu()
	{
		cuiElmnt.CleanupForOwner(this);
	}
}
