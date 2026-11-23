
class CfgPatches
{
	class ColorfulUI_GUI
	{
        requiredVersion = 0.1;
		requiredAddons[] = {"IsolationMap"};
	};

	class ColorfulUI_Scripts
	{
        requiredVersion = 0.1;
		requiredAddons[] = {"DZ_Data","DZ_Scripts","DZ_Sounds_Effects"};
	};
};

// IsolationZone does not link its character scene to a main-menu cutscene.
// Patch the world after IsolationMap loads so the engine creates and renders
// a fresh intro mission when returning from the server to the main menu.
class CfgMissions
{
	class Cutscenes
	{
		class IsolationZoneMenu
		{
			directory = "\intro.IsolationZone";
		};
	};
};

class CfgWorlds
{
	class CAWorld;
	class IsolationZone: CAWorld
	{
		cutscenes[] = {"IsolationZoneMenu"};
	};
};

class CfgAddons
{
	class PreloadBanks{};
	class PreloadAddons
	{
		class DayZ
		{
			list[] = {"DZ_Data","DZ_Scripts","DZ_UI","DZ_UIFonts","DZ_Sounds_Effects","ColorfulUI_GUI","ColorfulUI_Scripts"};
		};
	};
};

class CfgMods 
{
	class ColorfulUI
	{
		name = "Colorful-UI";
		dir = "Colorful-UI";
		creditsJson = "Colorful-UI/Scripts/Credits.json";
		inputs = "Colorful-UI/Scripts/Inputs.xml";
		type = "mod";

		dependencies[] =
		{
			"Game", "World", "Mission"
		};
		class defs
		{
			class imageSets			  {files[]= {"Colorful-UI/GUI/imagesets/CUI.imageset","Colorful-UI/GUI/imagesets/dayz_hud.imageset","Colorful-UI/GUI/imagesets/cui_symbols.imageset"};};
			class gameScriptModule 	  {files[]= {"Colorful-UI/Scripts/3_Game"};};
			class worldScriptModule	  {files[]= {"Colorful-UI/Scripts/4_World"};};
			class missionScriptModule {files[]= {"Colorful-UI/Scripts/5_Mission"};};
		};
	};	
};

class CfgSoundSets
{
	class CUI_Music_Menu_Base_SoundSet
	{
		volumeFactor = 1;
		frequencyFactor = 1;
		spatial = 0;
	};

	class CUI_Music_Menu_WinterWarfare_SoundSet: CUI_Music_Menu_Base_SoundSet {soundShaders[] = {"CUI_Music_Menu_WinterWarfare_SoundShader"};};
	class CUI_Music_Menu_SandAndBone_SoundSet: CUI_Music_Menu_Base_SoundSet {soundShaders[] = {"CUI_Music_Menu_SandAndBone_SoundShader"};};
	class CUI_Music_Menu_AgeOfArchon_SoundSet: CUI_Music_Menu_Base_SoundSet {soundShaders[] = {"CUI_Music_Menu_AgeOfArchon_SoundShader"};};
	class CUI_Music_Menu_GoingDark_SoundSet: CUI_Music_Menu_Base_SoundSet {soundShaders[] = {"CUI_Music_Menu_GoingDark_SoundShader"};};
	class CUI_Music_Menu_TheResearchLab_SoundSet: CUI_Music_Menu_Base_SoundSet {soundShaders[] = {"CUI_Music_Menu_TheResearchLab_SoundShader"};};
	class CUI_Music_Menu_TheFringe_SoundSet: CUI_Music_Menu_Base_SoundSet {soundShaders[] = {"CUI_Music_Menu_TheFringe_SoundShader"};};
	class CUI_Music_Menu_ShootToKill_SoundSet: CUI_Music_Menu_Base_SoundSet {soundShaders[] = {"CUI_Music_Menu_ShootToKill_SoundShader"};};
	class CUI_Music_Menu_Revelation_SoundSet: CUI_Music_Menu_Base_SoundSet {soundShaders[] = {"CUI_Music_Menu_Revelation_SoundShader"};};
	class CUI_Music_Menu_NoMoreHeroes_SoundSet: CUI_Music_Menu_Base_SoundSet {soundShaders[] = {"CUI_Music_Menu_NoMoreHeroes_SoundShader"};};
	class CUI_Music_Menu_OfSteelAndSorrow_SoundSet: CUI_Music_Menu_Base_SoundSet {soundShaders[] = {"CUI_Music_Menu_OfSteelAndSorrow_SoundShader"};};
	class CUI_Music_Menu_Infiltration_SoundSet: CUI_Music_Menu_Base_SoundSet {soundShaders[] = {"CUI_Music_Menu_Infiltration_SoundShader"};};
	class CUI_Music_Menu_Fallen_SoundSet: CUI_Music_Menu_Base_SoundSet {soundShaders[] = {"CUI_Music_Menu_Fallen_SoundShader"};};
	class CUI_Music_Menu_Brothers_SoundSet: CUI_Music_Menu_Base_SoundSet {soundShaders[] = {"CUI_Music_Menu_Brothers_SoundShader"};};
	class CUI_Music_Menu_Dive_SoundSet: CUI_Music_Menu_Base_SoundSet {soundShaders[] = {"CUI_Music_Menu_Dive_SoundShader"};};
	class CUI_Music_Menu_AHerosDeath_SoundSet: CUI_Music_Menu_Base_SoundSet {soundShaders[] = {"CUI_Music_Menu_AHerosDeath_SoundShader"};};
	class CUI_Music_Menu_TheChimera_SoundSet: CUI_Music_Menu_Base_SoundSet {soundShaders[] = {"CUI_Music_Menu_TheChimera_SoundShader"};};
	class CUI_Music_Menu_Haunted_SoundSet: CUI_Music_Menu_Base_SoundSet {soundShaders[] = {"CUI_Music_Menu_Haunted_SoundShader"};};
	class CUI_Music_Menu_TheReaper_SoundSet: CUI_Music_Menu_Base_SoundSet {soundShaders[] = {"CUI_Music_Menu_TheReaper_SoundShader"};};
	class CUI_Music_Menu_ThePlunge_SoundSet: CUI_Music_Menu_Base_SoundSet {soundShaders[] = {"CUI_Music_Menu_ThePlunge_SoundShader"};};
	class CUI_Music_Menu_ACleansingFire_SoundSet: CUI_Music_Menu_Base_SoundSet {soundShaders[] = {"CUI_Music_Menu_ACleansingFire_SoundShader"};};
	class CUI_Music_Menu_LastStand_SoundSet: CUI_Music_Menu_Base_SoundSet {soundShaders[] = {"CUI_Music_Menu_LastStand_SoundShader"};};
	class CUI_Music_Menu_TheGreatDeepSlowedReverb_SoundSet: CUI_Music_Menu_Base_SoundSet {soundShaders[] = {"CUI_Music_Menu_TheGreatDeepSlowedReverb_SoundShader"};};
	class CUI_Music_Menu_TheHorrorOfNuclearWinter_SoundSet: CUI_Music_Menu_Base_SoundSet {soundShaders[] = {"CUI_Music_Menu_TheHorrorOfNuclearWinter_SoundShader"};};
	class CUI_Music_Menu_BleedingOutInTheSnow_SoundSet: CUI_Music_Menu_Base_SoundSet {soundShaders[] = {"CUI_Music_Menu_BleedingOutInTheSnow_SoundShader"};};
	class CUI_Music_Menu_MenLongForgotten_SoundSet: CUI_Music_Menu_Base_SoundSet {soundShaders[] = {"CUI_Music_Menu_MenLongForgotten_SoundShader"};};
	class CUI_Music_Menu_LongLongAgo_SoundSet: CUI_Music_Menu_Base_SoundSet {soundShaders[] = {"CUI_Music_Menu_LongLongAgo_SoundShader"};};
	class CUI_Music_Menu_OnceMoreUntoTheBreach_SoundSet: CUI_Music_Menu_Base_SoundSet {soundShaders[] = {"CUI_Music_Menu_OnceMoreUntoTheBreach_SoundShader"};};
	class CUI_Music_Menu_NoTurningBack_SoundSet: CUI_Music_Menu_Base_SoundSet {soundShaders[] = {"CUI_Music_Menu_NoTurningBack_SoundShader"};};
	class CUI_Music_Menu_TheChasm_SoundSet: CUI_Music_Menu_Base_SoundSet {soundShaders[] = {"CUI_Music_Menu_TheChasm_SoundShader"};};
	class CUI_Music_Menu_SmokeBreak_SoundSet: CUI_Music_Menu_Base_SoundSet {soundShaders[] = {"CUI_Music_Menu_SmokeBreak_SoundShader"};};
	class CUI_Music_Menu_TheDawn_SoundSet: CUI_Music_Menu_Base_SoundSet {soundShaders[] = {"CUI_Music_Menu_TheDawn_SoundShader"};};
	class CUI_Music_Menu_TheComingStorm_SoundSet: CUI_Music_Menu_Base_SoundSet {soundShaders[] = {"CUI_Music_Menu_TheComingStorm_SoundShader"};};
	class CUI_Music_Menu_ForgottenBrothers_SoundSet: CUI_Music_Menu_Base_SoundSet {soundShaders[] = {"CUI_Music_Menu_ForgottenBrothers_SoundShader"};};
	class CUI_Music_Menu_TheSpirit_SoundSet: CUI_Music_Menu_Base_SoundSet {soundShaders[] = {"CUI_Music_Menu_TheSpirit_SoundShader"};};
	class CUI_Music_Menu_PostRockStalker_SoundSet: CUI_Music_Menu_Base_SoundSet {soundShaders[] = {"CUI_Music_Menu_PostRockStalker_SoundShader"};};
};	

class CfgSoundShaders
{
	class CUI_Music_Menu_Base_SoundShader
	{
		volume = 0.8;
	};

	class CUI_Music_Menu_WinterWarfare_SoundShader: CUI_Music_Menu_Base_SoundShader {samples[] = {{"\Colorful-UI\GUI\sounds\MainMenu\Background_sounds\Winter_Warfare", 1}};};
	class CUI_Music_Menu_SandAndBone_SoundShader: CUI_Music_Menu_Base_SoundShader {samples[] = {{"\Colorful-UI\GUI\sounds\MainMenu\Background_sounds\Sand_and_Bone", 1}};};
	class CUI_Music_Menu_AgeOfArchon_SoundShader: CUI_Music_Menu_Base_SoundShader {samples[] = {{"\Colorful-UI\GUI\sounds\MainMenu\Background_sounds\Age_of_Archon", 1}};};
	class CUI_Music_Menu_GoingDark_SoundShader: CUI_Music_Menu_Base_SoundShader {samples[] = {{"\Colorful-UI\GUI\sounds\MainMenu\Background_sounds\Going_Dark", 1}};};
	class CUI_Music_Menu_TheResearchLab_SoundShader: CUI_Music_Menu_Base_SoundShader {samples[] = {{"\Colorful-UI\GUI\sounds\MainMenu\Background_sounds\The_Research_Lab", 1}};};
	class CUI_Music_Menu_TheFringe_SoundShader: CUI_Music_Menu_Base_SoundShader {samples[] = {{"\Colorful-UI\GUI\sounds\MainMenu\Background_sounds\The_Fringe", 1}};};
	class CUI_Music_Menu_ShootToKill_SoundShader: CUI_Music_Menu_Base_SoundShader {samples[] = {{"\Colorful-UI\GUI\sounds\MainMenu\Background_sounds\Shoot_To_Kill", 1}};};
	class CUI_Music_Menu_Revelation_SoundShader: CUI_Music_Menu_Base_SoundShader {samples[] = {{"\Colorful-UI\GUI\sounds\MainMenu\Background_sounds\Revelation", 1}};};
	class CUI_Music_Menu_NoMoreHeroes_SoundShader: CUI_Music_Menu_Base_SoundShader {samples[] = {{"\Colorful-UI\GUI\sounds\MainMenu\Background_sounds\No_More_Heroes", 1}};};
	class CUI_Music_Menu_OfSteelAndSorrow_SoundShader: CUI_Music_Menu_Base_SoundShader {samples[] = {{"\Colorful-UI\GUI\sounds\MainMenu\Background_sounds\Of_Steel_and_Sorrow", 1}};};
	class CUI_Music_Menu_Infiltration_SoundShader: CUI_Music_Menu_Base_SoundShader {samples[] = {{"\Colorful-UI\GUI\sounds\MainMenu\Background_sounds\Infiltration", 1}};};
	class CUI_Music_Menu_Fallen_SoundShader: CUI_Music_Menu_Base_SoundShader {samples[] = {{"\Colorful-UI\GUI\sounds\MainMenu\Background_sounds\Fallen", 1}};};
	class CUI_Music_Menu_Brothers_SoundShader: CUI_Music_Menu_Base_SoundShader {samples[] = {{"\Colorful-UI\GUI\sounds\MainMenu\Background_sounds\Brothers", 1}};};
	class CUI_Music_Menu_Dive_SoundShader: CUI_Music_Menu_Base_SoundShader {samples[] = {{"\Colorful-UI\GUI\sounds\MainMenu\Background_sounds\Dive", 1}};};
	class CUI_Music_Menu_AHerosDeath_SoundShader: CUI_Music_Menu_Base_SoundShader {samples[] = {{"\Colorful-UI\GUI\sounds\MainMenu\Background_sounds\A_Heros_Death", 1}};};
	class CUI_Music_Menu_TheChimera_SoundShader: CUI_Music_Menu_Base_SoundShader {samples[] = {{"\Colorful-UI\GUI\sounds\MainMenu\Background_sounds\The_Chimera", 1}};};
	class CUI_Music_Menu_Haunted_SoundShader: CUI_Music_Menu_Base_SoundShader {samples[] = {{"\Colorful-UI\GUI\sounds\MainMenu\Background_sounds\Haunted", 1}};};
	class CUI_Music_Menu_TheReaper_SoundShader: CUI_Music_Menu_Base_SoundShader {samples[] = {{"\Colorful-UI\GUI\sounds\MainMenu\Background_sounds\The_Reaper", 1}};};
	class CUI_Music_Menu_ThePlunge_SoundShader: CUI_Music_Menu_Base_SoundShader {samples[] = {{"\Colorful-UI\GUI\sounds\MainMenu\Background_sounds\The_Plunge", 1}};};
	class CUI_Music_Menu_ACleansingFire_SoundShader: CUI_Music_Menu_Base_SoundShader {samples[] = {{"\Colorful-UI\GUI\sounds\MainMenu\Background_sounds\A_Cleansing_Fire", 1}};};
	class CUI_Music_Menu_LastStand_SoundShader: CUI_Music_Menu_Base_SoundShader {samples[] = {{"\Colorful-UI\GUI\sounds\MainMenu\Background_sounds\Last_Stand", 1}};};
	class CUI_Music_Menu_TheGreatDeepSlowedReverb_SoundShader: CUI_Music_Menu_Base_SoundShader {samples[] = {{"\Colorful-UI\GUI\sounds\MainMenu\Background_sounds\The_Great_Deep_Slowed_Reverb", 1}};};
	class CUI_Music_Menu_TheHorrorOfNuclearWinter_SoundShader: CUI_Music_Menu_Base_SoundShader {samples[] = {{"\Colorful-UI\GUI\sounds\MainMenu\Background_sounds\The_Horror_of_Nuclear_Winter", 1}};};
	class CUI_Music_Menu_BleedingOutInTheSnow_SoundShader: CUI_Music_Menu_Base_SoundShader {samples[] = {{"\Colorful-UI\GUI\sounds\MainMenu\Background_sounds\Bleeding_Out_in_the_Snow", 1}};};
	class CUI_Music_Menu_MenLongForgotten_SoundShader: CUI_Music_Menu_Base_SoundShader {samples[] = {{"\Colorful-UI\GUI\sounds\MainMenu\Background_sounds\Men_Long_Forgotten", 1}};};
	class CUI_Music_Menu_LongLongAgo_SoundShader: CUI_Music_Menu_Base_SoundShader {samples[] = {{"\Colorful-UI\GUI\sounds\MainMenu\Background_sounds\Long_Long_Ago", 1}};};
	class CUI_Music_Menu_OnceMoreUntoTheBreach_SoundShader: CUI_Music_Menu_Base_SoundShader {samples[] = {{"\Colorful-UI\GUI\sounds\MainMenu\Background_sounds\Once_More_Unto_The_Breach", 1}};};
	class CUI_Music_Menu_NoTurningBack_SoundShader: CUI_Music_Menu_Base_SoundShader {samples[] = {{"\Colorful-UI\GUI\sounds\MainMenu\Background_sounds\No_Turning_Back", 1}};};
	class CUI_Music_Menu_TheChasm_SoundShader: CUI_Music_Menu_Base_SoundShader {samples[] = {{"\Colorful-UI\GUI\sounds\MainMenu\Background_sounds\The_Chasm", 1}};};
	class CUI_Music_Menu_SmokeBreak_SoundShader: CUI_Music_Menu_Base_SoundShader {samples[] = {{"\Colorful-UI\GUI\sounds\MainMenu\Background_sounds\Smoke_Break", 1}};};
	class CUI_Music_Menu_TheDawn_SoundShader: CUI_Music_Menu_Base_SoundShader {samples[] = {{"\Colorful-UI\GUI\sounds\MainMenu\Background_sounds\The_Dawn", 1}};};
	class CUI_Music_Menu_TheComingStorm_SoundShader: CUI_Music_Menu_Base_SoundShader {samples[] = {{"\Colorful-UI\GUI\sounds\MainMenu\Background_sounds\The_Coming_Storm", 1}};};
	class CUI_Music_Menu_ForgottenBrothers_SoundShader: CUI_Music_Menu_Base_SoundShader {samples[] = {{"\Colorful-UI\GUI\sounds\MainMenu\Background_sounds\Forgotten_Brothers", 1}};};
	class CUI_Music_Menu_TheSpirit_SoundShader: CUI_Music_Menu_Base_SoundShader {samples[] = {{"\Colorful-UI\GUI\sounds\MainMenu\Background_sounds\The_Spirit", 1}};};
	class CUI_Music_Menu_PostRockStalker_SoundShader: CUI_Music_Menu_Base_SoundShader {samples[] = {{"\Colorful-UI\GUI\sounds\MainMenu\Background_sounds\Post_Rock_Stalker", 1}};};
};
