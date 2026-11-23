// Quick slot wheel: the wire format.
//
// The wheel runs on the client and works only with items the player already owns, so the display
// needs no round trip at all: the client enumerates its own inventory. The server is contacted for
// two things only - to run an action (it re-checks the item and the player state) and to say whether
// this player may open the admin arsenal.

const int CUIQS_RPC_ID = 1464815923;	// second ScriptRPC id, next to WRM_RPC_ID
const int CUIQS_MAGIC = 1464815924;		// first int of every quick slot payload
const string CUIQS_INPUT_DEEP = "CUIQuickSlotsDeep";	// F by default, declared in Scripts/Inputs.xml

enum CUIQSOp
{
	// client -> server
	QS_HELLO = 1,		// ask whether the arsenal is allowed for this player
	QS_ACTION = 2,		// run one cell of the wheel
	// server -> client
	QS_ADMIN = 101,		// int 0: 1 = arsenal allowed, 0 = not
	QS_RESULT = 102		// status of the last action, plus the class it acted on
}

enum CUIQSStatus
{
	QSS_OK = 0,
	QSS_DENIED = 1,		// player state forbids the action (dead, unconscious, in a vehicle, ...)
	QSS_BUSY = 2,		// hands are busy with another action
	QSS_NOT_FOUND = 3,	// the item is not in the player's inventory any more
	QSS_NO_ROOM = 4,	// nowhere to put the item that was replaced
	QSS_RATE = 5,		// too many requests
	QSS_FAILED = 6,		// the engine refused the move
	QSS_UNSUPPORTED = 7	// this action is not implemented yet
}

// Single player (offline) has no network. The server implementation lives in 4_World and cannot be
// named from 3_Game, so it registers itself here and the client calls through this base class.
class CUIQuickSlotLocalServer
{
	void HandleLocal(Man player, WRMMsg msg) {}
}

class CUIQSMsg
{
	// The quick slot wheel shares WRMMsg: same envelope, different magic, so the two handlers can
	// never read each other's payload by mistake.
	static WRMMsg New(int op, int status = 0)
	{
		WRMMsg msg = new WRMMsg(op, status);
		msg.m_Magic = CUIQS_MAGIC;
		return msg;
	}

	static WRMMsg Read(ParamsReadContext ctx)
	{
		WRMMsg msg = new WRMMsg();
		msg.m_Magic = CUIQS_MAGIC;
		if (!msg.Read(ctx))
			return null;
		return msg;
	}
}
