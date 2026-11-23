// UI THEME ---------------------------------------------------------------
// Adjust these settings to align with your desired theme and aesthetics.
//
// The palette follows the MOONLIGHT wallpapers. Measured from their own pixels, the art is
// near-black with a cold steel-blue glow: average #0B0F15 on the main menu wallpaper, mid tones
// around #1E2936, highlights around #324356 to #394D60. Every colour below is taken from that
// family, so the UI reads as part of the wallpaper instead of sitting on top of it.
class colorScheme 
{
	// Brand Specific 
	static int BrandColor()          { return ARGB(255, 138, 180, 214); }  // #8AB4D6, the wallpaper's highlight blue, lifted to stay readable on black
	static int AccentColor()         { return ARGB(255, 44, 74, 102); }    // #2C4A66, the wallpaper's deep shadow blue
	static int TertiaryColor()       { return ARGB(255, 196, 216, 232); }  // #C4D8E8, frost highlight for loading messages

	// Base Typography 
	static int PrimaryText()         { return UIColor.White(); }            // Primary text color
	static int SecondaryText()       { return UIColor.Grey(); }         	// Color for secondary text, e.g., subtitles or info
	static int TextHover()           { return BrandColor(); }           	// Text color for hover effects
	static int DisabledText()        { return ARGB(180, 120, 134, 148); };  // Color for secondary text, e.g., subtitles or info

	// Global UI Elements 
	static int ButtonHover()         { return BrandColor(); }           	// Button color on hover
	static int Icons()         		 { return BrandColor(); }           	// Primary Color for all icons
	static int LogOutTimer()         { return BrandColor(); }           	// Timer color for logout on logout Screen
	static int Separator()           { return BrandColor(); }           	// Divider color to segment UI sections
	static int Loadingbar()          { return BrandColor(); }           	// Loading bar color
	
	// Buttons
	static int BtnText()          { return PrimaryText(); }
	static int BtnHoverText()     { return TextHover(); }
	static int BtnSolidBG()       { return AccentColor(); }            	// Deep blue keeps white button text readable
	static int BtnSolidHoverBG()  { return ARGB(255, 62, 96, 128); }     	// One step lighter, still under white text
	static int BtnIcon()          { return Icons(); }

	// Tabs
	static int TabIdle()           { return PrimaryText(); }
	static int TabHoverColor()     { return BrandColor(); }
	static int TabSelectedColor()  { return BrandColor(); }
	static int TabBackground()     { return UIColor.Black(); }

	// Shader Colors 
	static int TopShader()           {return UIColor.Black();}  			// Top shader on layouts
	static int BottomShader()        {return UIColor.Black();}  			// Bottom shader on layouts

	// Loading Screen
	static int TipText()             { return PrimaryText(); }          	// Main text color for tips
	static int LoadingMsg()          { return TertiaryColor(); }        	// Color for loading messages
	static int TipHeader()           { return BrandColor(); }           	// Header color for tips
	static int TipLine()             { return BrandColor(); }           	// Divider line color in tips
	static int LoadingDim()          { return ARGB(115, 4, 8, 14); }      	// Dark overlay over loading screen wallpapers

	// Main Menu
	static int NavIcon()             { return BrandColor(); }           	// Navigation icon color
	static int SurvivorBox()         { return UIColor.cuiDarkBlue(); }		// Background color for "Survivor" box
	static int StatsBox()            { return UIColor.cuiDarkBlue(); }  	// Background color for "Stats" box
	static int MenuTint()            { return ARGB(165, 3, 7, 13); }      	// Tint over the main menu wallpaper (ZoneTint)
	static int BackgroundTone()      { return ARGB(255, 138, 162, 186); } 	// Color multiplier for the main menu wallpaper

	// Options Page 
	static int OptionHeaders()       { return BrandColor(); }           	// Header color in options
	static int OptionLine()          { return AccentColor(); }          	// Header color in options
	static int OptionInputColors()   { return BrandColor(); }           	// Input fields color
	static int OptionSliderColors()  { return BrandColor(); }           	// Slider color in options
	static int OptionSelectionText() { return BrandColor(); }           	// Caret color for dropdowns
	static int OptionCaretColors()   { return BrandColor(); }           	// Caret color for dropdowns
	static int OptionIconHover()     { return TextHover(); }
	static int OptionIconNormal()    { return PrimaryText(); } // Or Icons() if that exists

	static int OptionBGHover()       { return UIColor.cuiDarkBlue(); }  	// Background color for hovered options

	// Dialogs
	static int DialogBG()            { return ARGB(238, 11, 19, 29); }   	// #0B131D popup panel, the wallpaper's darkest tone
}
