// Deterministic weapon category heuristic (documented in README). Input is the lowercase inheritance chain of
// the CfgWeapons class ("m4a1_red", "m4a1_base", "rifleboltlock_base", ..., "weapon_base"), the weapon's
// magazines[] and modes[]. Rules are evaluated in this fixed order, first hit wins:
//   0. JSON categoryOverrides            (handled by the caller)
//   1. launcher   2. pistol   3. shotgun   4. smg   5. sniper/marksman   6. assault   7. rifle   8. other
class WRMClassifier
{
	static const string LAUNCHER_CHAIN = "launcher_base,m79,rpg,panzerfaust";
	static const string PISTOL_CHAIN = "pistol_base,pistolalt_base,magnum_base,longhorn_base,singleshotpistol_base,derringer";
	static const string SHOTGUN_CHAIN = "shotgun,izh43,mp133,saiga,r12_base";
	static const string SHOTGUN_MAGS = "12gauge,shotgun,slug,20gauge,12ga";
	static const string SMG_CHAIN = "mp5k,ump45,pp19,cz61,mp18,pm73,vikhr,asval,mp7,bizon,kriss,uzi,p90";
	static const string SNIPER_CHAIN = "svd_base,sv98_base,scout_base,ssg82_base,winchester70_base,vss_base,m70_base,awm,l96,m24_base,m82,m107,barrett";
	static const string SNIPER_MAGS = "338,50bmg,127x99,127x108,408cheytac,300wm,300win";
	static const string ASSAULT_CHAIN = "akm_base,ak74_base,ak101_base,m4a1_base,m16a2_base,scarh_base,scarl_base,fal_base,aug_base,famas_base,m14_base,m249";

	protected static bool HasAny(string haystack, string csv)
	{
		TStringArray tokens = new TStringArray;
		csv.Split(",", tokens);
		for (int i = 0; i < tokens.Count(); i++)
		{
			if (haystack.Contains(tokens.Get(i)))
				return true;
		}
		return false;
	}

	// path: "CfgWeapons <Class>", chain: lowercase class names from the class itself up to the root.
	static string Classify(string path, TStringArray chain)
	{
		string ch = string.Join("|", chain);

		TStringArray mags = new TStringArray;
		g_Game.ConfigGetTextArray(path + " magazines", mags);
		string mg = string.Join("|", mags);
		mg.ToLower();

		TStringArray modes = new TStringArray;
		g_Game.ConfigGetTextArray(path + " modes", modes);
		bool fullAuto = false;
		for (int i = 0; i < modes.Count(); i++)
		{
			string mode = modes.Get(i);
			mode.ToLower();
			if (mode == "fullauto")
				fullAuto = true;
		}

		if (HasAny(ch, LAUNCHER_CHAIN))
			return "launcher";
		if (HasAny(ch, PISTOL_CHAIN))
			return "pistol";
		if (HasAny(ch, SHOTGUN_CHAIN) || HasAny(mg, SHOTGUN_MAGS))
			return "shotgun";
		if (HasAny(ch, SMG_CHAIN))
			return "smg";
		if (HasAny(ch, SNIPER_CHAIN) || (!fullAuto && HasAny(mg, SNIPER_MAGS)))
			return "sniper";
		if (HasAny(ch, ASSAULT_CHAIN) || (fullAuto && ch.Contains("rifle_base")))
			return "assault";
		if (ch.Contains("rifle_base"))
			return "rifle";
		return "other";
	}
}
