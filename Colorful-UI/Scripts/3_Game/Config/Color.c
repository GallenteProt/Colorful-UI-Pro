// COLOR PALETTES ---------------------------------------------------------
class UIColor
{
	// Cold Zone palette (MOONLIGHT main menu, loading screens, rules overlay).
	// Kept separate so the rest of the UI (inventory etc.) keeps its own colors.
	static int ZoneBlack()        { return ARGB(255, 2, 5, 7); };       // #020507
	static int ZoneDeepBlack()    { return ARGB(255, 7, 16, 24); };     // #071018
	static int ZoneNavy()         { return ARGB(255, 13, 23, 33); };    // #0D1721
	static int ZoneGraphite()     { return ARGB(255, 28, 39, 48); };    // #1C2730
	static int ZoneText()         { return ARGB(255, 236, 242, 246); }; // #ECF2F6
	static int ZoneTextMuted()    { return ARGB(220, 236, 242, 246); };
	static int ZoneTextDim()      { return ARGB(165, 236, 242, 246); };
	static int ZoneDanger()       { return ARGB(255, 127, 47, 47); };   // #7F2F2F
	static int ZoneDangerDark()   { return ARGB(255, 72, 22, 22); };
	static int ZoneLoadingBlue()  { return ARGB(255, 82, 156, 210); };  // #529CD2
	static int ZonePanel()        { return ARGB(196, 2, 5, 7); };
	static int ZonePanelSoft()    { return ARGB(132, 7, 16, 24); };
	static int ZoneSurface()      { return ARGB(184, 13, 23, 33); };
	static int ZoneBorder()       { return ARGB(150, 236, 242, 246); };
	static int ZoneBorderDim()    { return ARGB(82, 236, 242, 246); };
	static int ZoneAgedPaper()    { return ARGB(255, 154, 150, 136); }; // #9A9688
	static int ZoneActivePaper()  { return ARGB(255, 236, 242, 246); };
	static int ZoneActiveText()   { return ARGB(255, 2, 5, 7); };

	// GreyScale
	static int White()            { return ARGB(255, 255, 255, 255); };
	static int Grey()             { return ARGB(255, 130, 130, 130); };
	static int Black()            { return ARGB(255, 0, 0, 0); };
	static int Transparent()      { return ARGB(1, 0, 0, 0); };
	static int SemiTransWhite()   { return ARGB(120, 255, 255, 255); };
	static int SemiTransBlack()   { return ARGB(120, 0, 0, 0); };
	
	// R.O.Y.G.B.I.V
	static int Red()              { return ARGB(255, 173, 35, 35); };
	static int Orange()           { return ARGB(255, 215, 153, 19); };
	static int Yellow()           { return ARGB(255, 255, 204, 102); };
	static int Green()            { return ARGB(255, 51, 204, 51); };
	static int Blue()             { return ARGB(255, 51, 102, 102); };
	static int Indigo()           { return ARGB(255, 102, 51, 204); };
	static int Violet()           { return ARGB(255, 204, 51, 204); };
	
	// Social Network Colors 
	static int Discord()          { return ARGB(255, 88, 101, 242); };
	static int Twitter()          { return ARGB(255, 29, 161, 242); };
	static int Facebook()         { return ARGB(255, 59, 89, 152); };
	static int YouTube()          { return ARGB(255, 205, 32, 31); };
	static int Instagram()        { return ARGB(255, 193, 42, 163); };
	static int Meta()             { return ARGB(255, 193, 42, 163); };
	static int Reddit()           { return ARGB(255, 102, 51, 204); };
	static int LinkedIn()         { return ARGB(255, 0, 119, 181); };
	static int Pinterest()        { return ARGB(255, 189, 8, 28); };
	static int Snapchat()         { return ARGB(255, 255, 252, 0); };
	static int TikTok()           { return ARGB(255, 1, 1, 1); };
	static int WhatsApp()         { return ARGB(255, 37, 211, 102); };
	static int Tumblr()           { return ARGB(255, 54, 70, 93); };
	static int Flickr()           { return ARGB(255, 244, 0, 131); };
	static int Vimeo()            { return ARGB(255, 26, 183, 234); };
	static int Skype()            { return ARGB(255, 0, 175, 240); };
	static int Slack()            { return ARGB(255, 74, 21, 75); };
	static int Medium()           { return ARGB(255, 2, 184, 117); };
	static int Quora()            { return ARGB(255, 170, 34, 0); };
	static int Dribbble()         { return ARGB(255, 234, 76, 137); };
	static int SoundCloud()       { return ARGB(255, 255, 85, 0); };
	static int Spotify()          { return ARGB(255, 30, 215, 96); };
	static int Twitch()           { return ARGB(255, 145, 70, 255); };
	static int Telegram()         { return ARGB(255, 0, 136, 204); };
	static int Mastodon()         { return ARGB(255, 99, 100, 255); };
 	
	//Custom Colors 
	static int cuiTeal()          { return ARGB(255, 102, 153, 153); };
	static int cuiBlue()          { return ARGB(255, 1, 5, 74); };
	static int cuiDarkBlue()      { return ARGB(155, 0, 0, 32); };
	static int cuiSubtleRed()     { return ARGB(255, 100, 35, 35); };
	static int cuiBrightRed()     { return ARGB(255, 152, 0, 0); };
}
