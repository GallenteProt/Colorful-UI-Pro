// Quick slot wheel: server authority.
//
// The client sends an intent only - which cell, which action, which class it thinks is there. The
// server looks the item up again in the player's own inventory, checks the player state and then
// moves the item with the engine's server-side inventory calls. Nothing is ever created here: every
// action works on an item the player already owns, which is why a tampered client gains nothing.

class CUIQuickSlotRate
{
	float m_Next;
}

// Single player: the client sends through this instead of the network.
class CUIQuickSlotLocalServerImpl : CUIQuickSlotLocalServer
{
	override void HandleLocal(Man player, WRMMsg msg)
	{
		CUIQuickSlotServer.Handle(PlayerBase.Cast(player), null, msg);
	}
}

class CUIQuickSlotServer
{
	static const float ACTION_GAP = 0.25;		// one action per player per this many seconds
	static const string ADMINS_FILE = "$profile:ColorfulUI/admins.txt";

	static bool s_Inited;
	static ref map<string, ref CUIQuickSlotRate> s_Rates = new map<string, ref CUIQuickSlotRate>;
	static ref TStringArray s_Admins = new TStringArray;

	static void Init()
	{
		if (s_Inited)
			return;
		s_Inited = true;
		LoadAdmins();
		if (!g_Game.IsMultiplayer())
			CUIQuickSlotLocal.s_Local = new CUIQuickSlotLocalServerImpl();
	}

	static void Tick(float dt)
	{
	}

	// ----- admins -----

	// One id per line: the Steam64 id or the 44 character identity id. "#" starts a comment.
	// Without the file nobody is an admin, except the local player in single player.
	protected static void LoadAdmins()
	{
		s_Admins.Clear();
		if (!FileExist(ADMINS_FILE))
		{
			MakeDirectory("$profile:ColorfulUI");
			return;
		}

		FileHandle handle = OpenFile(ADMINS_FILE, FileMode.READ);
		if (!handle)
			return;

		string line;
		while (FGets(handle, line) >= 0)
		{
			string id = line;
			int hash = id.IndexOf("#");
			if (hash == 0)
				continue;
			if (hash > 0)
				id = id.Substring(0, hash);
			id.Trim();
			if (id != "")
				s_Admins.Insert(id);
		}
		CloseFile(handle);
	}

	static bool IsAdmin(PlayerIdentity identity)
	{
		if (!g_Game.IsMultiplayer())
			return true;		// the local player owns the machine

		if (!identity)
			return false;

		if (s_Admins.Find(identity.GetId()) >= 0)
			return true;
		if (s_Admins.Find(identity.GetPlainId()) >= 0)
			return true;
		return false;
	}

	// ----- transport -----

	static void SendMsg(PlayerBase player, PlayerIdentity identity, WRMMsg rep)
	{
		if (!g_Game.IsMultiplayer())
		{
			CUIQuickSlotLocal.OnMessage(rep);
			return;
		}
		if (!player || !identity)
			return;

		ScriptRPC rpc = new ScriptRPC();
		rep.Write(rpc);
		rpc.Send(player, CUIQS_RPC_ID, true, identity);
	}

	static void SendResult(PlayerBase player, PlayerIdentity identity, int status, string cls = "")
	{
#ifdef DIAG_DEVELOPER
		Print("[CUI-QS] result=" + typename.EnumToString(CUIQSStatus, status) + " " + cls);
#endif
		WRMMsg rep = CUIQSMsg.New(CUIQSOp.QS_RESULT, status);
		rep.m_Strs.Insert(cls);
		SendMsg(player, identity, rep);
	}

	// ----- entry point -----

	static void Handle(PlayerBase player, PlayerIdentity identity, WRMMsg msg)
	{
		if (!player || !msg)
			return;

		if (msg.m_Op == CUIQSOp.QS_HELLO)
		{
			WRMMsg rep = CUIQSMsg.New(CUIQSOp.QS_ADMIN);
			int allowed = 0;
			if (IsAdmin(identity))
				allowed = 1;
			rep.m_Ints.Insert(allowed);
			SendMsg(player, identity, rep);
			return;
		}

		if (msg.m_Op != CUIQSOp.QS_ACTION)
			return;

		string key = "local";
		if (identity)
			key = identity.GetId();
		if (!TakeSlot(key))
		{
			SendResult(player, identity, CUIQSStatus.QSS_RATE);
			return;
		}

		int state = CheckPlayer(player);
		if (state != CUIQSStatus.QSS_OK)
		{
			SendResult(player, identity, state);
			return;
		}

		string action = msg.GetStr(0);
		string target = msg.GetStr(1);
		string slot = msg.GetStr(2);
		string hint = msg.GetStr(3);

		int act = CUIQuickSlotActions.FromString(action);
		switch (act)
		{
			case CUIQuickSlotAction.QSA_WEAPON:
			case CUIQuickSlotAction.QSA_HANDS:
				DoHands(player, identity, hint, target);
				break;
			case CUIQuickSlotAction.QSA_EQUIP:
				DoEquip(player, identity, target, slot);
				break;
			case CUIQuickSlotAction.QSA_TOGGLE:
				DoToggle(player, identity, target);
				break;
			case CUIQuickSlotAction.QSA_FILTER:
				DoFilter(player, identity, target);
				break;
			case CUIQuickSlotAction.QSA_USE:
			case CUIQuickSlotAction.QSA_REPAIR:
			case CUIQuickSlotAction.QSA_CATEGORY:
				// use runs through the vanilla action system on the client; repair and the category
				// wheel are not part of this stage
				SendResult(player, identity, CUIQSStatus.QSS_UNSUPPORTED);
				break;
			default:
				SendResult(player, identity, CUIQSStatus.QSS_UNSUPPORTED);
				break;
		}
	}

	protected static bool TakeSlot(string key)
	{
		float now = g_Game.GetTickTime();
		CUIQuickSlotRate rate;
		if (!s_Rates.Find(key, rate))
		{
			rate = new CUIQuickSlotRate();
			rate.m_Next = 0;
			s_Rates.Set(key, rate);
		}
		if (now < rate.m_Next)
			return false;
		rate.m_Next = now + ACTION_GAP;
		return true;
	}

	protected static int CheckPlayer(PlayerBase player)
	{
		if (!player.IsAlive() || player.IsPlayerDisconnected())
			return CUIQSStatus.QSS_DENIED;
		if (player.IsUnconscious() || player.IsRestrained() || player.IsSurrendered())
			return CUIQSStatus.QSS_DENIED;
		if (player.IsInVehicle() || player.IsSwimming() || player.IsClimbing() || player.IsClimbingLadder() || player.IsFalling())
			return CUIQSStatus.QSS_BUSY;
		if (player.IsRaised() || player.IsEmotePlaying())
			return CUIQSStatus.QSS_BUSY;

		DayZPlayerInventory inventory = player.GetDayZPlayerInventory();
		if (inventory && inventory.IsProcessing())
			return CUIQSStatus.QSS_BUSY;

		ActionManagerBase actionManager = player.GetActionManager();
		if (actionManager && actionManager.GetRunningAction())
			return CUIQSStatus.QSS_BUSY;

		return CUIQSStatus.QSS_OK;
	}

	// ----- inventory helpers -----

	// Everything the player carries, in the same order the client sees it.
	protected static void Collect(PlayerBase player, out array<EntityAI> items)
	{
		items = new array<EntityAI>;
		player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);
	}

	protected static bool Matches(EntityAI item, string target)
	{
		if (!item || target == "")
			return false;
		if (CUIQuickSlotLocal.IsMagazineTarget(target))
			return item.IsKindOf("Magazine");
		if (CUIQuickSlotLocal.IsFoodTarget(target))
			return item.IsKindOf("Edible_Base");
		return CUIQuickSlotLocal.Matches(item.GetType(), CUIQuickSlotLocal.PrefixesFor(target));
	}

	protected static EntityAI FindByType(PlayerBase player, string type)
	{
		if (type == "")
			return null;
		array<EntityAI> items;
		Collect(player, items);
		for (int i = 0; i < items.Count(); i++)
		{
			EntityAI item = items.Get(i);
			if (item && item != player && item.GetType() == type)
				return item;
		}
		return null;
	}

	protected static EntityAI FindByTarget(PlayerBase player, string target, string skipType = "")
	{
		array<EntityAI> items;
		Collect(player, items);
		for (int i = 0; i < items.Count(); i++)
		{
			EntityAI item = items.Get(i);
			if (!item || item == player)
				continue;
			if (skipType != "" && item.GetType() == skipType)
				continue;
			if (Matches(item, target))
				return item;
		}
		return null;
	}

	// ----- actions -----

	// Take the item in hands. The client's hint is used first because the wheel promises exactly that
	// item; the target prefixes are the fallback when the inventory changed since the wheel was drawn.
	protected static void DoHands(PlayerBase player, PlayerIdentity identity, string hint, string target)
	{
		EntityAI item = FindByType(player, hint);
		if (!item && target != "")
			item = FindByTarget(player, target);
		if (!item)
		{
			SendResult(player, identity, CUIQSStatus.QSS_NOT_FOUND, hint);
			return;
		}

		if (player.GetItemInHands() == item)
		{
			SendResult(player, identity, CUIQSStatus.QSS_OK, item.GetType());
			return;
		}

		player.ServerTakeEntityToHands(item);
		if (player.GetItemInHands() != item)
		{
			SendResult(player, identity, CUIQSStatus.QSS_FAILED, item.GetType());
			return;
		}
		SendResult(player, identity, CUIQSStatus.QSS_OK, item.GetType());
	}

	// Equip into a clothing slot and swap with whatever is there. Clicking again with nothing spare in
	// the inventory takes the equipped item off into the player's own inventory.
	protected static void DoEquip(PlayerBase player, PlayerIdentity identity, string target, string slot)
	{
		if (slot == "")
		{
			SendResult(player, identity, CUIQSStatus.QSS_FAILED);
			return;
		}

		int slotId = InventorySlots.GetSlotIdFromString(slot);
		EntityAI equipped = null;
		if (slotId >= 0)
			equipped = player.GetInventory().FindAttachment(slotId);

		string equippedType = "";
		if (equipped)
			equippedType = equipped.GetType();

		EntityAI spare = FindByTarget(player, target, equippedType);
		if (spare)
		{
			bool moved = false;
			if (equipped)
				moved = player.ServerSwapEntities(spare, equipped);
			else
				moved = player.ServerTakeEntityToTargetAttachment(player, spare);

			if (!moved)
			{
				SendResult(player, identity, CUIQSStatus.QSS_FAILED, spare.GetType());
				return;
			}
			SendResult(player, identity, CUIQSStatus.QSS_OK, spare.GetType());
			return;
		}

		if (equipped)
		{
			if (player.ServerTakeEntityToTargetInventory(player, FindInventoryLocationType.CARGO | FindInventoryLocationType.ATTACHMENT, equipped))
			{
				SendResult(player, identity, CUIQSStatus.QSS_OK, equippedType);
				return;
			}
			if (player.ServerDropEntity(equipped))
			{
				SendResult(player, identity, CUIQSStatus.QSS_NO_ROOM, equippedType);
				return;
			}
			SendResult(player, identity, CUIQSStatus.QSS_FAILED, equippedType);
			return;
		}

		SendResult(player, identity, CUIQSStatus.QSS_NOT_FOUND, target);
	}

	protected static void DoToggle(PlayerBase player, PlayerIdentity identity, string target)
	{
		EntityAI item = FindByTarget(player, target);
		if (!item)
		{
			SendResult(player, identity, CUIQSStatus.QSS_NOT_FOUND, target);
			return;
		}
		if (!item.HasEnergyManager())
		{
			SendResult(player, identity, CUIQSStatus.QSS_UNSUPPORTED, item.GetType());
			return;
		}

		InventoryItem device = InventoryItem.Cast(item);
		if (!device)
		{
			SendResult(player, identity, CUIQSStatus.QSS_UNSUPPORTED, item.GetType());
			return;
		}

		bool on = !device.IsOn();
		device.SwitchOn(on);
		SendResult(player, identity, CUIQSStatus.QSS_OK, item.GetType());
	}

	// The gas mask filter is an ordinary attachment in its own slot, so this is the equip path with a
	// fixed slot name.
	protected static void DoFilter(PlayerBase player, PlayerIdentity identity, string target)
	{
		DoEquip(player, identity, target, "GasMaskFilter");
	}
}
