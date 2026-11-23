// Shared constants and enums of WeaponRadialMenu.

const int WRM_MENU_ID = 5718605;			// unique UIScriptedMenu id (vanilla uses 1..48)
const int WRM_RPC_ID = 1464815921;			// ScriptRPC id, not part of ERPCs
const int WRM_MAGIC = 1464815922;			// first int of every payload, checked on read
const string WRM_INPUT_OPEN = "UAWRMOpenMenu";
const string WRM_MAG_KEY = "@mag";			// pseudo slot name of the magazine cell
const int WRM_PAGE_SIZE = 8;				// content cells per page (+ back and next = 10 cards at most; even, so Next sits exactly opposite Back)
const int WRM_MAX_CATS = 20;

enum WRMOp
{
	// client -> server
	REQ_CATS = 1,
	REQ_WEAPONS = 2,
	REQ_SLOTS = 3,
	REQ_SLOT_ITEMS = 4,
	ISSUE_WEAPON = 5,
	ISSUE_ATTACH = 6,
	REQ_VARIANTS = 7,
	// server -> client
	REP_CATS = 101,
	REP_WEAPONS = 102,
	REP_SLOTS = 103,
	REP_SLOT_ITEMS = 104,
	REP_RESULT = 105,
	REP_VARIANTS = 106
}

enum WRMStatus
{
	OK = 0,
	BUILDING = 1,
	DENIED = 2,
	RATE = 3,
	NOT_FOUND = 4,
	MISMATCH = 5,
	NO_ROOM = 6,
	LIMIT = 7,
	FAILED = 8,
	BUSY = 9,
	BAD_REQUEST = 10,
	NO_WEAPON = 11,
	ALREADY = 12
}

// Set by MissionGameplay while the open key (also bound to the vanilla quickbar / HUD toggle) is pressed, held or
// released. IngameHud ignores ShowQuickbarPlayer / ShowHudPlayer meanwhile, so the vanilla toggle never fires with ours.
class WRMInputGuard
{
	static bool s_BlockToggle;
}
