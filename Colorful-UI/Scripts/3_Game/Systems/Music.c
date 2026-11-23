// DynamicMusicPlayerRegistry — replaces the vanilla menu music list.
// Vanilla source: P:\scripts\3_game\systems\dynamicmusicplayer\dynamicmusicplayerregistry.c

modded class DynamicMusicPlayerRegistry
{
	// CUI fully replaces the vanilla menu-music array. Each audio file has its
	// own SoundSet so the UI can identify the exact track selected by DayZ.
	protected override void RegisterTracksMenu()
	{
		if (!MenuMusic)
		{
			super.RegisterTracksMenu();
			return;
		}

		m_TracksMenu = new array<ref DynamicMusicTrackData>();

		RegisterTrackMenu("CUI_Music_Menu_WinterWarfare_SoundSet");
		RegisterTrackMenu("CUI_Music_Menu_SandAndBone_SoundSet");
		RegisterTrackMenu("CUI_Music_Menu_AgeOfArchon_SoundSet");
		RegisterTrackMenu("CUI_Music_Menu_GoingDark_SoundSet");
		RegisterTrackMenu("CUI_Music_Menu_TheResearchLab_SoundSet");
		RegisterTrackMenu("CUI_Music_Menu_TheFringe_SoundSet");
		RegisterTrackMenu("CUI_Music_Menu_ShootToKill_SoundSet");
		RegisterTrackMenu("CUI_Music_Menu_Revelation_SoundSet");
		RegisterTrackMenu("CUI_Music_Menu_NoMoreHeroes_SoundSet");
		RegisterTrackMenu("CUI_Music_Menu_OfSteelAndSorrow_SoundSet");
		RegisterTrackMenu("CUI_Music_Menu_Infiltration_SoundSet");
		RegisterTrackMenu("CUI_Music_Menu_Fallen_SoundSet");
		RegisterTrackMenu("CUI_Music_Menu_Brothers_SoundSet");
		RegisterTrackMenu("CUI_Music_Menu_Dive_SoundSet");
		RegisterTrackMenu("CUI_Music_Menu_AHerosDeath_SoundSet");
		RegisterTrackMenu("CUI_Music_Menu_TheChimera_SoundSet");
		RegisterTrackMenu("CUI_Music_Menu_Haunted_SoundSet");
		RegisterTrackMenu("CUI_Music_Menu_TheReaper_SoundSet");
		RegisterTrackMenu("CUI_Music_Menu_ThePlunge_SoundSet");
		RegisterTrackMenu("CUI_Music_Menu_ACleansingFire_SoundSet");
		RegisterTrackMenu("CUI_Music_Menu_LastStand_SoundSet");
		RegisterTrackMenu("CUI_Music_Menu_TheGreatDeepSlowedReverb_SoundSet");
		RegisterTrackMenu("CUI_Music_Menu_TheHorrorOfNuclearWinter_SoundSet");
		RegisterTrackMenu("CUI_Music_Menu_BleedingOutInTheSnow_SoundSet");
		RegisterTrackMenu("CUI_Music_Menu_MenLongForgotten_SoundSet");
		RegisterTrackMenu("CUI_Music_Menu_LongLongAgo_SoundSet");
		RegisterTrackMenu("CUI_Music_Menu_OnceMoreUntoTheBreach_SoundSet");
		RegisterTrackMenu("CUI_Music_Menu_NoTurningBack_SoundSet");
		RegisterTrackMenu("CUI_Music_Menu_TheChasm_SoundSet");
		RegisterTrackMenu("CUI_Music_Menu_SmokeBreak_SoundSet");
		RegisterTrackMenu("CUI_Music_Menu_TheDawn_SoundSet");
		RegisterTrackMenu("CUI_Music_Menu_TheComingStorm_SoundSet");
		RegisterTrackMenu("CUI_Music_Menu_ForgottenBrothers_SoundSet");
		RegisterTrackMenu("CUI_Music_Menu_TheSpirit_SoundSet");
		RegisterTrackMenu("CUI_Music_Menu_PostRockStalker_SoundSet");
	}
}

class CUITrackAttribution
{
	static string GetTitle(string soundSet)
	{
		if (soundSet == "CUI_Music_Menu_WinterWarfare_SoundSet") return "Winter Warfare";
		if (soundSet == "CUI_Music_Menu_SandAndBone_SoundSet") return "Sand and Bone";
		if (soundSet == "CUI_Music_Menu_AgeOfArchon_SoundSet") return "Age of Archon";
		if (soundSet == "CUI_Music_Menu_GoingDark_SoundSet") return "Going Dark";
		if (soundSet == "CUI_Music_Menu_TheResearchLab_SoundSet") return "The Research Lab";
		if (soundSet == "CUI_Music_Menu_TheFringe_SoundSet") return "The Fringe";
		if (soundSet == "CUI_Music_Menu_ShootToKill_SoundSet") return "Shoot to Kill";
		if (soundSet == "CUI_Music_Menu_Revelation_SoundSet") return "Revelation";
		if (soundSet == "CUI_Music_Menu_NoMoreHeroes_SoundSet") return "No More Heroes";
		if (soundSet == "CUI_Music_Menu_OfSteelAndSorrow_SoundSet") return "Of Steel and Sorrow";
		if (soundSet == "CUI_Music_Menu_Infiltration_SoundSet") return "Infiltration";
		if (soundSet == "CUI_Music_Menu_Fallen_SoundSet") return "Fallen";
		if (soundSet == "CUI_Music_Menu_Brothers_SoundSet") return "Brothers";
		if (soundSet == "CUI_Music_Menu_Dive_SoundSet") return "Dive";
		if (soundSet == "CUI_Music_Menu_AHerosDeath_SoundSet") return "A Hero's Death";
		if (soundSet == "CUI_Music_Menu_TheChimera_SoundSet") return "The Chimera";
		if (soundSet == "CUI_Music_Menu_Haunted_SoundSet") return "Haunted";
		if (soundSet == "CUI_Music_Menu_TheReaper_SoundSet") return "The Reaper";
		if (soundSet == "CUI_Music_Menu_ThePlunge_SoundSet") return "The Plunge";
		if (soundSet == "CUI_Music_Menu_ACleansingFire_SoundSet") return "A Cleansing Fire";
		if (soundSet == "CUI_Music_Menu_LastStand_SoundSet") return "Last Stand";
		if (soundSet == "CUI_Music_Menu_TheGreatDeepSlowedReverb_SoundSet") return "The Great Deep (Slowed + Reverb)";
		if (soundSet == "CUI_Music_Menu_TheHorrorOfNuclearWinter_SoundSet") return "The Horror of Nuclear Winter";
		if (soundSet == "CUI_Music_Menu_BleedingOutInTheSnow_SoundSet") return "Bleeding Out in the Snow";
		if (soundSet == "CUI_Music_Menu_MenLongForgotten_SoundSet") return "Men Long Forgotten";
		if (soundSet == "CUI_Music_Menu_LongLongAgo_SoundSet") return "Long, Long Ago";
		if (soundSet == "CUI_Music_Menu_OnceMoreUntoTheBreach_SoundSet") return "Once More Unto the Breach";
		if (soundSet == "CUI_Music_Menu_NoTurningBack_SoundSet") return "No Turning Back";
		if (soundSet == "CUI_Music_Menu_TheChasm_SoundSet") return "The Chasm";
		if (soundSet == "CUI_Music_Menu_SmokeBreak_SoundSet") return "Smoke Break";
		if (soundSet == "CUI_Music_Menu_TheDawn_SoundSet") return "The Dawn";
		if (soundSet == "CUI_Music_Menu_TheComingStorm_SoundSet") return "The Coming Storm";
		if (soundSet == "CUI_Music_Menu_ForgottenBrothers_SoundSet") return "Forgotten Brothers";
		if (soundSet == "CUI_Music_Menu_TheSpirit_SoundSet") return "The Spirit";
		if (soundSet == "CUI_Music_Menu_PostRockStalker_SoundSet") return "Post Rock Stalker";
		return "";
	}
}

modded class DynamicMusicPlayer
{
	string GetCUITrackTitle()
	{
		if (!m_CurrentTrack) return "";
		return CUITrackAttribution.GetTitle(m_CurrentTrack.m_SoundSet);
	}
}
