class CfgPatches
{
	class WeaponRadialMenu
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DZ_Data", "DZ_Scripts"};
	};
};

class CfgMods
{
	class WeaponRadialMenu
	{
		dir = "WeaponRadialMenu";
		name = "Weapon Radial Menu";
		author = "";
		type = "mod";
		inputs = "WeaponRadialMenu/data/inputs.xml";
		dependencies[] = {"Game", "World", "Mission"};
		class defs
		{
			class gameScriptModule
			{
				value = "";
				files[] = {"WeaponRadialMenu/scripts/3_Game"};
			};
			class worldScriptModule
			{
				value = "";
				files[] = {"WeaponRadialMenu/scripts/4_World"};
			};
			class missionScriptModule
			{
				value = "";
				files[] = {"WeaponRadialMenu/scripts/5_Mission"};
			};
		};
	};
};
