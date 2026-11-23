// Server authority. The client only sends an intent (class names + slot name). Everything else, including the
// player, comes from the RPC sender and from the server-side state of the player and of the item in hands.

class WRMRate
{
	float m_NextIssue;		// earliest tick time of the next issue request
	float m_Tokens = 12;	// token bucket for browse requests
	float m_Last;
	float m_NextRateReply;	// RATE answers are throttled so a flood cannot be amplified into replies
}

// A weapon swap that waits for the hand state machine (hand events are deferred to the next player tick).
class WRMPending
{
	PlayerBase m_Player;
	PlayerIdentity m_Identity;
	string m_Key;
	string m_Type;
	float m_Elapsed;
	float m_NextPoll;
	EntityAI m_Stowed;
	bool m_Dropped;
}

class WRMLocalServerImpl : WRMLocalServer
{
	override void HandleLocal(Man player, WRMMsg msg)
	{
		WRMServer.Handle(PlayerBase.Cast(player), null, msg);
	}
}

// One item issued by this mod. m_Item is weak: the engine nulls it when the entity is deleted.
class WRMIssued
{
	EntityAI m_Item;
	string m_Owner;			// identity id of the issuer
	string m_Snap;			// ammo state at issue time
	bool m_Tainted;			// has been in another identity's possession: never reclaimed
}

class WRMServer
{
	static const float POLL_INTERVAL = 0.1;
	static const float PENDING_TIMEOUT = 4.0;
	static const float DROP_FALLBACK = 1.5;		// the hand move is a juncture and needs about a round trip
	static const float BROWSE_RATE = 6.0;		// tokens per second
	static const float BROWSE_BURST = 12.0;
	static const int MAX_SLOT_ENTRIES = 22;

	static bool s_Inited;
	static ref map<string, ref WRMRate> s_Rates = new map<string, ref WRMRate>;
	static ref array<ref WRMPending> s_Pending = new array<ref WRMPending>;
	static ref array<ref WRMIssued> s_Issued = new array<ref WRMIssued>;

	// ----- lifecycle -----

	static void Init()
	{
		if (s_Inited)
			return;
		s_Inited = true;

		WRMConfig.Load();
		WRMCatalogue.Begin();
		if (!g_Game.IsMultiplayer())
			WRMClient.s_Local = new WRMLocalServerImpl();
	}

	static void Tick(float dt)
	{
		if (!s_Inited)
			return;

		if (!WRMCatalogue.IsReady())
			WRMCatalogue.Step();

		if (s_Pending.Count() > 0)
			TickPending(dt);
	}

	// ----- transport -----

	static void SendMsg(PlayerBase player, PlayerIdentity identity, WRMMsg rep)
	{
		if (!g_Game.IsMultiplayer())
		{
			WRMClient.OnMessage(rep);
			return;
		}
		if (!player || !identity)
			return;

		ScriptRPC rpc = new ScriptRPC();
		rep.Write(rpc);
		rpc.Send(player, WRM_RPC_ID, true, identity);
	}

	static void SendResult(PlayerBase player, PlayerIdentity identity, int status, string className = "")
	{
#ifdef DIAG_DEVELOPER
		Print("[WRM] result=" + typename.EnumToString(WRMStatus, status) + " " + className);
#endif
		WRMMsg rep = new WRMMsg(WRMOp.REP_RESULT, status);
		if (className != "")
			rep.m_Strs.Insert(className);
		SendMsg(player, identity, rep);
	}

	// ----- request entry point -----

	static void Handle(PlayerBase player, PlayerIdentity identity, WRMMsg msg)
	{
		if (!player || !msg)
			return;

		string key = "local";
		if (identity)
			key = identity.GetId();

		int op = msg.m_Op;
		bool isIssue = (op == WRMOp.ISSUE_WEAPON || op == WRMOp.ISSUE_ATTACH);
		bool isBrowse = (op == WRMOp.REQ_CATS || op == WRMOp.REQ_WEAPONS || op == WRMOp.REQ_SLOTS || op == WRMOp.REQ_SLOT_ITEMS || op == WRMOp.REQ_VARIANTS);
		if (!isIssue && !isBrowse)
			return;

		if (isBrowse)
		{
			if (!TakeBrowseToken(key))
			{
				ReplyRate(player, identity, key);
				return;
			}
		}
		else if (!TakeIssueSlot(key))
		{
			ReplyRate(player, identity, key);
			return;
		}

		int state = CheckPlayer(player, isIssue);
		if (state != WRMStatus.OK)
		{
			SendResult(player, identity, state);
			return;
		}
		if (!WRMCatalogue.IsReady())
		{
			SendResult(player, identity, WRMStatus.BUILDING);
			return;
		}

		switch (op)
		{
			case WRMOp.REQ_CATS:
				HandleCats(player, identity);
				break;
			case WRMOp.REQ_WEAPONS:
				HandleWeapons(player, identity, msg);
				break;
			case WRMOp.REQ_SLOTS:
				HandleSlots(player, identity);
				break;
			case WRMOp.REQ_SLOT_ITEMS:
				HandleSlotItems(player, identity, msg);
				break;
			case WRMOp.REQ_VARIANTS:
				HandleVariants(player, identity, msg);
				break;
			case WRMOp.ISSUE_WEAPON:
				IssueWeapon(player, identity, key, msg.GetStr(0));
				break;
			case WRMOp.ISSUE_ATTACH:
				IssueAttachment(player, identity, key, msg.GetStr(0), msg.GetStr(1), msg.GetStr(2));
				break;
		}
	}

	// ----- limits and validation -----

	protected static WRMRate GetRate(string key)
	{
		WRMRate rate;
		if (!s_Rates.Find(key, rate))
		{
			if (s_Rates.Count() > 512)
				s_Rates.Clear();		// bounded memory; worst case everybody gets a fresh bucket
			rate = new WRMRate();
			rate.m_Last = g_Game.GetTickTime();
			s_Rates.Insert(key, rate);
		}
		return rate;
	}

	// at most one RATE reply per second per identity, silence otherwise
	protected static void ReplyRate(PlayerBase player, PlayerIdentity identity, string key)
	{
		WRMRate rate = GetRate(key);
		float now = g_Game.GetTickTime();
		if (now < rate.m_NextRateReply)
			return;
		rate.m_NextRateReply = now + 1.0;
		SendResult(player, identity, WRMStatus.RATE);
	}

	protected static bool TakeBrowseToken(string key)
	{
		WRMRate rate = GetRate(key);
		float now = g_Game.GetTickTime();
		rate.m_Tokens = rate.m_Tokens + (now - rate.m_Last) * BROWSE_RATE;
		if (rate.m_Tokens > BROWSE_BURST)
			rate.m_Tokens = BROWSE_BURST;
		rate.m_Last = now;
		if (rate.m_Tokens < 1)
			return false;
		rate.m_Tokens = rate.m_Tokens - 1;
		return true;
	}

	protected static bool TakeIssueSlot(string key)
	{
		WRMRate rate = GetRate(key);
		float now = g_Game.GetTickTime();
		if (now < rate.m_NextIssue)
			return false;
		rate.m_NextIssue = now + WRMConfig.Get().cooldownMs / 1000.0;
		return true;
	}

	// State of the requesting player. Issuing needs more than browsing.
	protected static int CheckPlayer(PlayerBase player, bool forIssue)
	{
		if (!player.IsAlive() || player.IsPlayerDisconnected())
			return WRMStatus.DENIED;
		if (player.IsUnconscious() || player.IsRestrained() || player.IsSurrendered())
			return WRMStatus.DENIED;

		if (!forIssue)
			return WRMStatus.OK;

		if (player.IsInVehicle() || player.IsSwimming() || player.IsClimbing() || player.IsClimbingLadder() || player.IsFalling() || player.IsRaised() || player.IsEmotePlaying())
			return WRMStatus.BUSY;

		ActionManagerBase actionManager = player.GetActionManager();
		if (actionManager && actionManager.GetRunningAction())
			return WRMStatus.BUSY;

		WeaponManager weaponManager = player.GetWeaponManager();
		if (weaponManager && weaponManager.IsRunning())
			return WRMStatus.BUSY;

		DayZPlayerInventory inventory = player.GetDayZPlayerInventory();
		if (inventory && inventory.IsProcessing())
			return WRMStatus.BUSY;

		return WRMStatus.OK;
	}

	// ----- browsing -----

	// the client drops a whole message when one string exceeds MAX_STR_LEN, so server sent text is cut to fit
	protected static string FitString(string text)
	{
		int chars = text.LengthUtf8();
		while (text.Length() > WRMMsg.MAX_STR_LEN && chars > 1)
		{
			chars--;
			text = text.SubstringUtf8(0, chars);
		}
		return text;
	}

	protected static void HandleCats(PlayerBase player, PlayerIdentity identity)
	{
		TStringArray ids = new TStringArray;
		WRMCatalogue.GetVisibleCategories(ids);

		WRMMsg rep = new WRMMsg(WRMOp.REP_CATS, WRMStatus.OK);
		for (int i = 0; i < ids.Count() && i < WRM_MAX_CATS; i++)
		{
			string id = ids.Get(i);
			if (id.Length() > WRMMsg.MAX_STR_LEN)
				continue;
			array<ref WRMGroup> groups = WRMCatalogue.GetWeaponGroups(id);
			int count = 0;
			if (groups)
				count = groups.Count();
			rep.m_Strs.Insert(id);
			rep.m_Strs.Insert(FitString(WRMConfig.GetCategoryName(id)));
			rep.m_Ints.Insert(count);
		}
		SendMsg(player, identity, rep);
	}

	// copies one page of cells into rep: strs gets the face class of each cell, ints the page numbers
	// (first) followed by a variant count and a "holds the current item" flag per cell
	protected static int FillGroupPage(WRMMsg rep, array<ref WRMGroup> groups, int page, string currentType)
	{
		int requestedPage = page;
		int total = groups.Count();
		int maxPage = 0;
		if (total > 0)
			maxPage = (total - 1) / WRM_PAGE_SIZE;
		if (page < 0)
			page = 0;
		if (page > maxPage)
			page = maxPage;

		string currentLower = currentType;
		currentLower.ToLower();

		rep.m_Ints.Insert(page);
		rep.m_Ints.Insert(total);
		rep.m_Ints.Insert(requestedPage);	// the client caches under the page it asked for, even when clamped

		int from = page * WRM_PAGE_SIZE;
		for (int i = from; i < total && i < from + WRM_PAGE_SIZE; i++)
		{
			WRMGroup group = groups.Get(i);
			if (group.m_Rep.Length() > WRMMsg.MAX_STR_LEN)
				continue;
			rep.m_Strs.Insert(group.m_Rep);
			rep.m_Ints.Insert(group.Count());
			int holds = 0;
			if (group.Contains(currentLower))
				holds = 1;
			rep.m_Ints.Insert(holds);
		}
		return page;
	}

	protected static void HandleWeapons(PlayerBase player, PlayerIdentity identity, WRMMsg msg)
	{
		string category = msg.GetStr(0);
		category.ToLower();
		array<ref WRMGroup> groups = WRMCatalogue.GetWeaponGroups(category);
		if (!groups || groups.Count() == 0 || WRMConfig.IsCategoryHidden(category))
		{
			SendResult(player, identity, WRMStatus.NOT_FOUND);
			return;
		}

		string hands = "";
		EntityAI inHands = player.GetItemInHands();
		if (inHands)
			hands = inHands.GetType();

		WRMMsg rep = new WRMMsg(WRMOp.REP_WEAPONS, WRMStatus.OK);
		rep.m_Strs.Insert(category);
		FillGroupPage(rep, groups, msg.GetInt(0), hands);
		SendMsg(player, identity, rep);
	}

	protected static Weapon_Base GetWeaponInHands(PlayerBase player)
	{
		return Weapon_Base.Cast(player.GetItemInHands());
	}

	// canonical slot name from the attachments[] of the weapon, "" when the weapon does not have it
	protected static string ResolveSlot(Weapon_Base weapon, string requested)
	{
		string wanted = requested;
		wanted.ToLower();
		if (wanted == "" || wanted.IndexOf("magazine") == 0)
			return "";

		TStringArray slots = new TStringArray;
		g_Game.ConfigGetTextArray("CfgWeapons " + weapon.GetType() + " attachments", slots);
		for (int i = 0; i < slots.Count(); i++)
		{
			string have = slots.Get(i);
			string haveLower = have;
			haveLower.ToLower();
			if (haveLower != wanted)
				continue;

			int slotId = InventorySlots.GetSlotIdFromString(have);
			if (slotId == InventorySlots.INVALID || !weapon.GetInventory().HasAttachmentSlot(slotId))
				return "";
			return have;
		}
		return "";
	}

	protected static void HandleSlots(PlayerBase player, PlayerIdentity identity)
	{
		Weapon_Base weapon = GetWeaponInHands(player);
		if (!weapon)
		{
			SendResult(player, identity, WRMStatus.NO_WEAPON);
			return;
		}

		WRMMsg rep = new WRMMsg(WRMOp.REP_SLOTS, WRMStatus.OK);
		rep.m_Strs.Insert(weapon.GetType());

		// magazines first
		int muzzle = weapon.GetCurrentMuzzle();
		int magSlot = weapon.GetSlotFromMuzzleIndex(muzzle);
		if (magSlot != InventorySlots.INVALID)		// slot ids are hashes and may be negative
		{
			array<ref WRMGroup> magGroups = WRMCatalogue.GetMagazineGroups(weapon.GetType(), muzzle);
			if (magGroups.Count() > 0)
			{
				string installedMag = "";
				Magazine mag = weapon.GetMagazine(muzzle);
				if (mag)
					installedMag = mag.GetType();
				rep.m_Strs.Insert(WRM_MAG_KEY);
				rep.m_Strs.Insert(installedMag);
				rep.m_Ints.Insert(magGroups.Count());
			}
		}

		TStringArray slots = new TStringArray;
		g_Game.ConfigGetTextArray("CfgWeapons " + weapon.GetType() + " attachments", slots);
		for (int i = 0; i < slots.Count() && rep.m_Ints.Count() < MAX_SLOT_ENTRIES; i++)
		{
			string slotName = ResolveSlot(weapon, slots.Get(i));
			if (slotName == "")
				continue;

			string slotLower = slotName;
			slotLower.ToLower();
			array<ref WRMGroup> groups = WRMCatalogue.GetSlotItemGroups(slotLower);
			if (!groups || groups.Count() == 0)
				continue;

			string installed = "";
			EntityAI attached = weapon.GetInventory().FindAttachment(InventorySlots.GetSlotIdFromString(slotName));
			if (attached)
				installed = attached.GetType();

			rep.m_Strs.Insert(slotName);
			rep.m_Strs.Insert(installed);
			rep.m_Ints.Insert(groups.Count());
		}
		SendMsg(player, identity, rep);
	}

	protected static void HandleSlotItems(PlayerBase player, PlayerIdentity identity, WRMMsg msg)
	{
		Weapon_Base weapon = GetWeaponInHands(player);
		if (!weapon)
		{
			SendResult(player, identity, WRMStatus.NO_WEAPON);
			return;
		}

		string requested = msg.GetStr(0);
		array<ref WRMGroup> groups;
		string slotKey;
		string installed = "";
		if (requested == WRM_MAG_KEY)
		{
			slotKey = WRM_MAG_KEY;
			int muzzle = weapon.GetCurrentMuzzle();
			groups = WRMCatalogue.GetMagazineGroups(weapon.GetType(), muzzle);
			Magazine mag = weapon.GetMagazine(muzzle);
			if (mag)
				installed = mag.GetType();
		}
		else
		{
			slotKey = ResolveSlot(weapon, requested);
			if (slotKey != "")
			{
				string slotLower = slotKey;
				slotLower.ToLower();
				groups = WRMCatalogue.GetSlotItemGroups(slotLower);
				EntityAI attached = weapon.GetInventory().FindAttachment(InventorySlots.GetSlotIdFromString(slotKey));
				if (attached)
					installed = attached.GetType();
			}
		}
		if (!groups || groups.Count() == 0)
		{
			SendResult(player, identity, WRMStatus.NOT_FOUND);
			return;
		}

		// the page key on the client is built from the slot the client asked for
		WRMMsg rep = new WRMMsg(WRMOp.REP_SLOT_ITEMS, WRMStatus.OK);
		rep.m_Strs.Insert(requested);
		rep.m_Strs.Insert(weapon.GetType());
		FillGroupPage(rep, groups, msg.GetInt(0), installed);
		SendMsg(player, identity, rep);
	}

	// variants of one cell: kind "w" (weapon category), "i" (attachment slot) or "m" (magazines of the muzzle in hands)
	protected static void HandleVariants(PlayerBase player, PlayerIdentity identity, WRMMsg msg)
	{
		string kind = msg.GetStr(0);
		kind.ToLower();
		string scope = msg.GetStr(1);
		string repClass = msg.GetStr(2);
		if (repClass == "")
		{
			SendResult(player, identity, WRMStatus.BAD_REQUEST);
			return;
		}

		array<ref WRMGroup> groups;
		string current = "";
		Weapon_Base weapon;
		if (kind == "w")
		{
			scope.ToLower();
			if (WRMConfig.IsCategoryHidden(scope))
			{
				SendResult(player, identity, WRMStatus.NOT_FOUND);
				return;
			}
			groups = WRMCatalogue.GetWeaponGroups(scope);
			EntityAI inHands = player.GetItemInHands();
			if (inHands)
				current = inHands.GetType();
		}
		else if (kind == "i")
		{
			weapon = GetWeaponInHands(player);
			if (!weapon)
			{
				SendResult(player, identity, WRMStatus.NO_WEAPON);
				return;
			}
			string slotName = ResolveSlot(weapon, scope);
			if (slotName == "")
			{
				SendResult(player, identity, WRMStatus.NOT_FOUND);
				return;
			}
			string slotLower = slotName;
			slotLower.ToLower();
			groups = WRMCatalogue.GetSlotItemGroups(slotLower);
			EntityAI attached = weapon.GetInventory().FindAttachment(InventorySlots.GetSlotIdFromString(slotName));
			if (attached)
				current = attached.GetType();
		}
		else if (kind == "m")
		{
			weapon = GetWeaponInHands(player);
			if (!weapon)
			{
				SendResult(player, identity, WRMStatus.NO_WEAPON);
				return;
			}
			int muzzle = weapon.GetCurrentMuzzle();
			groups = WRMCatalogue.GetMagazineGroups(weapon.GetType(), muzzle);
			Magazine mag = weapon.GetMagazine(muzzle);
			if (mag)
				current = mag.GetType();
		}
		else
		{
			SendResult(player, identity, WRMStatus.BAD_REQUEST);
			return;
		}

		TStringArray variants = WRMCatalogue.FindGroupVariants(groups, repClass);
		if (!variants || variants.Count() == 0)
		{
			SendResult(player, identity, WRMStatus.NOT_FOUND);
			return;
		}

		string currentLower = current;
		currentLower.ToLower();
		int installedIndex = -1;
		int slot = 0;
		WRMMsg rep = new WRMMsg(WRMOp.REP_VARIANTS, WRMStatus.OK);
		rep.m_Strs.Insert(kind);
		rep.m_Strs.Insert(scope);
		rep.m_Strs.Insert(repClass);
		string weaponType = "";
		if (weapon)
			weaponType = weapon.GetType();
		rep.m_Strs.Insert(weaponType);		// attachment variants belong to one weapon, the client drops late replies
		for (int i = 0; i < variants.Count() && rep.m_Strs.Count() < WRMMsg.MAX_STRS - 1; i++)
		{
			string cls = variants.Get(i);
			if (cls.Length() > WRMMsg.MAX_STR_LEN)
				continue;
			string lower = cls;
			lower.ToLower();
			if (lower == currentLower)
				installedIndex = slot;
			rep.m_Strs.Insert(cls);
			slot++;
		}
		rep.m_Ints.Insert(installedIndex);
		SendMsg(player, identity, rep);
	}

	// ----- issued item tracking and anti-flood -----
	// Rule of thumb: when in doubt, never delete. An issued item is only ever reclaimed while it is untouched:
	// untainted (never held by another identity), on the ground, and with the same ammo state as when it was issued.

	// identity key of a player, the same string that Handle() derives from the RPC sender
	static string OwnerKey(Man man)
	{
		if (!g_Game.IsMultiplayer())
			return "local";
		if (!man)
			return "";
		PlayerIdentity id = man.GetIdentity();
		if (!id)
			return "";
		return id.GetId();
	}

	// Cartridges (chamber, internal magazine, magazine contents) are not child entities, so they are
	// part of a text snapshot taken at issue time and compared again before anything is deleted.
	protected static string Snapshot(EntityAI item)
	{
		string snap = item.GetType();

		Magazine mag = Magazine.Cast(item);
		if (mag)
		{
			int count = mag.GetAmmoCount();
			snap = snap + "|m" + count;
			string lastType = "";
			for (int i = 0; i < count; i++)
			{
				float dmg;
				string cart;
				if (mag.GetCartridgeAtIndex(i, dmg, cart) && cart != lastType)
				{
					snap = snap + "," + i + ":" + cart;
					lastType = cart;
				}
			}
		}

		Weapon_Base weapon = Weapon_Base.Cast(item);
		if (weapon)
		{
			for (int m = 0; m < weapon.GetMuzzleCount(); m++)
			{
				if (weapon.IsChamberEmpty(m))
					snap = snap + "|e";
				else if (weapon.IsChamberJammed(m))
					snap = snap + "|j";
				else if (weapon.IsChamberFiredOut(m))
					snap = snap + "|f";
				else
					snap = snap + "|L" + weapon.GetChamberAmmoTypeName(m);

				if (weapon.HasInternalMagazine(m))
				{
					int inner = weapon.GetInternalMagazineCartridgeCount(m);
					snap = snap + "|i" + inner;
					string lastInner = "";
					for (int j = 0; j < inner; j++)
					{
						float dmgInner;
						string cartInner;
						if (weapon.GetInternalMagazineCartridgeInfo(m, j, dmgInner, cartInner) && cartInner != lastInner)
						{
							snap = snap + "," + j + ":" + cartInner;
							lastInner = cartInner;
						}
					}
				}
			}
		}
		return snap;
	}

	protected static WRMIssued FindIssued(EntityAI item)
	{
		for (int i = 0; i < s_Issued.Count(); i++)
		{
			if (s_Issued.Get(i).m_Item == item)
				return s_Issued.Get(i);
		}
		return null;
	}

	protected static void Track(string owner, EntityAI item)
	{
		if (!item)
			return;
		WRMIssued entry = new WRMIssued();
		entry.m_Item = item;
		entry.m_Owner = owner;
		entry.m_Snap = Snapshot(item);
		s_Issued.Insert(entry);
	}

	protected static void Untrack(WRMIssued entry)
	{
		int index = s_Issued.Find(entry);
		if (index != -1)
			s_Issued.RemoveOrdered(index);
	}

	// Called (server) whenever an entity changes owner. An issued item that enters another identity's
	// possession is marked for good: it is never reclaimed again, even after it is dropped.
	static void OnItemMoved(EntityAI item, EntityAI newOwner)
	{
		if (!s_Inited || !newOwner || s_Issued.Count() == 0)
			return;

		PlayerBase holder = PlayerBase.Cast(newOwner);
		if (!holder)
			holder = PlayerBase.Cast(newOwner.GetHierarchyRootPlayer());
		if (!holder)
			return;

		WRMIssued entry = FindIssued(item);
		if (entry && OwnerKey(holder) != entry.m_Owner)
			entry.m_Tainted = true;
	}

	// live issued items of this identity (dead pointers are dropped on the way)
	protected static int CountLive(string owner)
	{
		int count = 0;
		int i = 0;
		while (i < s_Issued.Count())
		{
			if (!s_Issued.Get(i).m_Item)
			{
				s_Issued.RemoveOrdered(i);
				continue;
			}
			if (s_Issued.Get(i).m_Owner == owner)
				count++;
			i++;
		}
		return count;
	}

	// untainted, ground, no parent, same ammo state, and every child issued to the same owner, untainted and unchanged
	protected static bool IsReclaimable(WRMIssued entry)
	{
		EntityAI item = entry.m_Item;
		if (!item || entry.m_Tainted)
			return false;
		if (item.GetHierarchyParent() || item.GetParent())
			return false;

		GameInventory inventory = item.GetInventory();
		if (!inventory)
			return false;

		InventoryLocation location = new InventoryLocation();
		if (!inventory.GetCurrentInventoryLocation(location) || location.GetType() != InventoryLocationType.GROUND)
			return false;

		if (Snapshot(item) != entry.m_Snap)
			return false;

		array<EntityAI> children = new array<EntityAI>;
		inventory.EnumerateInventory(InventoryTraversalType.PREORDER, children);
		for (int i = 0; i < children.Count(); i++)
		{
			EntityAI child = children.Get(i);
			if (!child || child == item)
				continue;

			WRMIssued childEntry = FindIssued(child);
			if (!childEntry || childEntry.m_Owner != entry.m_Owner || childEntry.m_Tainted)
				return false;
			if (Snapshot(child) != childEntry.m_Snap)
				return false;
		}
		return true;
	}

	// oldest reclaimable issued item of this owner, null when there is none (nothing is deleted here)
	protected static WRMIssued FindVictim(string owner)
	{
		for (int i = 0; i < s_Issued.Count(); i++)
		{
			WRMIssued entry = s_Issued.Get(i);
			if (entry.m_Owner == owner && IsReclaimable(entry))
				return entry;
		}
		return null;
	}

	protected static void DeleteIssued(WRMIssued victim)
	{
		EntityAI item = victim.m_Item;
		array<EntityAI> children = new array<EntityAI>;
		item.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, children);
		for (int i = 0; i < children.Count(); i++)
		{
			WRMIssued childEntry = FindIssued(children.Get(i));
			if (childEntry)
				Untrack(childEntry);
		}
		Untrack(victim);
		item.Delete();		// deferred by Object.Delete
	}

	// Phase 1, before anything is changed: true when one more item may be issued, i.e. under the cap or
	// a victim exists. Nothing is deleted here.
	protected static bool HasCapacity(string owner)
	{
		if (CountLive(owner) < WRMConfig.Get().maxIssuedPerPlayer)
			return true;
		return FindVictim(owner) != null;
	}

	// Phase 2, only after the new item exists and is tracked: delete the oldest reclaimable items above the cap.
	protected static void TrimToCapacity(string owner)
	{
		int cap = WRMConfig.Get().maxIssuedPerPlayer;
		while (CountLive(owner) > cap)
		{
			WRMIssued victim = FindVictim(owner);
			if (!victim)
				return;
			DeleteIssued(victim);
		}
	}

	// ----- inventory helpers (never delete a player's item) -----

	// Move an item out of the way: into the inventory of the player, else onto the ground. True when it left its place.
	protected static bool MoveAway(PlayerBase player, EntityAI item)
	{
		if (player.ServerTakeEntityToTargetInventory(player, FindInventoryLocationType.CARGO | FindInventoryLocationType.ATTACHMENT, item))
			return true;
		return player.ServerDropEntity(item);
	}

	// Attachments go through the inventory of the weapon: there a SERVER move is immediate and replicated.
	// The same call on a living player only queues a juncture and returns true before anything moved.
	protected static bool MoveAttachmentAway(PlayerBase player, Weapon_Base weapon, EntityAI item, int slotId)
	{
		if (GameInventory.HasInventoryReservation(item, null))
			return false;

		GameInventory inventory = weapon.GetInventory();
		inventory.TakeEntityToTargetInventory(InventoryMode.SERVER, player, FindInventoryLocationType.CARGO | FindInventoryLocationType.ATTACHMENT, item);
		if (inventory.FindAttachment(slotId) == item)
			inventory.DropEntity(InventoryMode.SERVER, player, item);

		return inventory.FindAttachment(slotId) != item;
	}

	// ----- issuing: weapons -----

	protected static void IssueWeapon(PlayerBase player, PlayerIdentity identity, string key, string requested)
	{
		if (HasPending(key))
		{
			SendResult(player, identity, WRMStatus.BUSY);
			return;
		}

		string lowerName = requested;
		lowerName.ToLower();
		string type = WRMCatalogue.FindWeapon(lowerName);
		if (type == "")
		{
			SendResult(player, identity, WRMStatus.NOT_FOUND);
			return;
		}

		if (!HasCapacity(key))
		{
			SendResult(player, identity, WRMStatus.LIMIT);
			return;
		}

		EntityAI inHands = player.GetHumanInventory().GetEntityInHands();
		if (inHands && inHands.GetType() == type)
		{
			SendResult(player, identity, WRMStatus.ALREADY, type);		// a second click must not issue a copy
			return;
		}
		if (!inHands)
		{
			CreateWeaponInHands(player, identity, key, type, null);
			return;
		}

		// Hands are occupied: stow the item, else drop it. Hand events are deferred, so wait for the result.
		if (!player.GetHumanInventory().CanRemoveEntityInHands() || !MoveAway(player, inHands))
		{
			SendResult(player, identity, WRMStatus.NO_ROOM);
			return;
		}

		WRMPending pending = new WRMPending();
		pending.m_Player = player;
		pending.m_Identity = identity;
		pending.m_Key = key;
		pending.m_Type = type;
		pending.m_Stowed = inHands;
		pending.m_NextPoll = POLL_INTERVAL;
		s_Pending.Insert(pending);
	}

	protected static bool HasPending(string key)
	{
		for (int i = 0; i < s_Pending.Count(); i++)
		{
			if (s_Pending.Get(i).m_Key == key)
				return true;
		}
		return false;
	}

	protected static void CreateWeaponInHands(PlayerBase player, PlayerIdentity identity, string key, string type, EntityAI stowed)
	{
		EntityAI created = player.GetHumanInventory().CreateInHands(type);
		Weapon_Base createdWeapon = Weapon_Base.Cast(created);
		if (created && (!createdWeapon || !createdWeapon.CanProcessWeaponEvents()))
		{
			// a modded class without a working weapon script breaks the player weapon handling while held
			Print("[WRM] not a working weapon, refused: " + type);
			g_Game.ObjectDelete(created);
			created = null;
		}
		if (created)
		{
			Track(key, created);
			TrimToCapacity(key);		// only now, with the new item in place, old issued ground items may go
			SendResult(player, identity, WRMStatus.OK, type);
			return;
		}

		// could not create: best effort to give the previous item back to the hands
		if (stowed && stowed.GetHierarchyRootPlayer() == player && !player.GetHumanInventory().GetEntityInHands())
			player.ServerTakeEntityToHands(stowed);
		SendResult(player, identity, WRMStatus.FAILED, type);
	}

	protected static void TickPending(float dt)
	{
		int i = 0;
		while (i < s_Pending.Count())
		{
			WRMPending pending = s_Pending.Get(i);
			pending.m_Elapsed += dt;
			if (pending.m_Elapsed < pending.m_NextPoll)
			{
				i++;
				continue;
			}
			pending.m_NextPoll = pending.m_Elapsed + POLL_INTERVAL;

			if (PollPending(pending))
				s_Pending.RemoveOrdered(i);
			else
				i++;
		}
	}

	// true when the pending swap is finished (successfully or not)
	protected static bool PollPending(WRMPending pending)
	{
		PlayerBase player = pending.m_Player;
		if (!player || !player.IsAlive() || player.IsPlayerDisconnected())
			return true;		// nothing was deleted, the stowed item is in the inventory or on the ground

		if (pending.m_Elapsed > PENDING_TIMEOUT)
		{
			SendResult(player, pending.m_Identity, WRMStatus.BUSY, pending.m_Type);
			return true;
		}

		DayZPlayerInventory inventory = player.GetDayZPlayerInventory();
		if (inventory && inventory.IsProcessing())
			return false;

		EntityAI inHands = player.GetHumanInventory().GetEntityInHands();
		if (!inHands)
		{
			if (CheckPlayer(player, true) != WRMStatus.OK)
			{
				SendResult(player, pending.m_Identity, WRMStatus.BUSY, pending.m_Type);
				return true;
			}
			CreateWeaponInHands(player, pending.m_Identity, pending.m_Key, pending.m_Type, pending.m_Stowed);
			return true;
		}

		if (inHands == pending.m_Stowed)
		{
			if (pending.m_Dropped || pending.m_Elapsed < DROP_FALLBACK)
				return false;		// still travelling, PENDING_TIMEOUT ends the wait

			// the stow was refused by the hand state machine: drop instead
			pending.m_Dropped = true;
			if (player.ServerDropEntity(inHands))
				return false;
		}

		SendResult(player, pending.m_Identity, WRMStatus.NO_ROOM, pending.m_Type);
		return true;
	}

	// ----- issuing: attachments and magazines -----

	// Every vanilla attach rule, checked on the real instance: item side (CanPutAsAttachment, e.g. the bayonet vs
	// suppressor rule), weapon side (CanReceiveAttachment, occupancy / exclusion), engine slot test, magazine test.
	protected static bool CanAttachHere(Weapon_Base weapon, EntityAI created, int slotId, int muzzle)
	{
		if (!created.CanPutAsAttachment(weapon))
			return false;
		if (!weapon.CanReceiveAttachment(created, slotId))
			return false;
		if (!weapon.GetInventory().CanAddAttachmentEx(created, slotId))
			return false;
		if (muzzle >= 0)
		{
			Magazine mag = Magazine.Cast(created);
			if (!mag || !weapon.TestAttachMagazine(muzzle, mag, true, true))
				return false;
		}
		return true;
	}

	protected static void RestoreOld(PlayerBase player, Weapon_Base weapon, EntityAI old, int slotId)
	{
		if (old && !weapon.GetInventory().FindAttachment(slotId))
			weapon.ServerTakeEntityToTargetAttachmentEx(weapon, old, slotId);
		weapon.ValidateAndRepair();
		weapon.Synchronize();
	}

	protected static void IssueAttachment(PlayerBase player, PlayerIdentity identity, string key, string requestedSlot, string requestedClass, string expectedWeapon)
	{
		if (HasPending(key))
		{
			SendResult(player, identity, WRMStatus.BUSY);
			return;
		}

		Weapon_Base weapon = GetWeaponInHands(player);
		if (!weapon)
		{
			SendResult(player, identity, WRMStatus.NO_WEAPON);
			return;
		}
		if (weapon.GetType() != expectedWeapon)
		{
			SendResult(player, identity, WRMStatus.MISMATCH);
			return;
		}

		string lowerClass = requestedClass;
		lowerClass.ToLower();

		string type = "";
		int slotId = InventorySlots.INVALID;
		int muzzle = -1;
		if (requestedSlot == WRM_MAG_KEY)
		{
			muzzle = weapon.GetCurrentMuzzle();
			slotId = weapon.GetSlotFromMuzzleIndex(muzzle);
			TStringArray mags = WRMCatalogue.GetMagazines(weapon.GetType(), muzzle);
			for (int i = 0; i < mags.Count(); i++)
			{
				string magLower = mags.Get(i);
				magLower.ToLower();
				if (magLower == lowerClass)
					type = mags.Get(i);
			}
		}
		else
		{
			string slotName = ResolveSlot(weapon, requestedSlot);
			if (slotName != "")
			{
				string slotLower = slotName;
				slotLower.ToLower();
				type = WRMCatalogue.FindSlotItem(slotLower, lowerClass);
				slotId = InventorySlots.GetSlotIdFromString(slotName);
			}
		}
		if (type == "" || slotId == InventorySlots.INVALID || (muzzle < 0 && !weapon.GetInventory().HasAttachmentSlot(slotId)))
		{
			SendResult(player, identity, WRMStatus.NOT_FOUND);
			return;
		}

		EntityAI old = weapon.GetInventory().FindAttachment(slotId);
		if (old && old.GetType() == type)
		{
			SendResult(player, identity, WRMStatus.ALREADY, type);
			return;
		}

		// reserved by a running inventory operation, or a jammed weapon whose magazine we would swap
		if (weapon.IsSlotReserved(slotId) || (muzzle >= 0 && weapon.IsJammed()))
		{
			SendResult(player, identity, WRMStatus.BUSY, type);
			return;
		}

		if (!HasCapacity(key))
		{
			SendResult(player, identity, WRMStatus.LIMIT);
			return;
		}

		// 1. create the new item on the ground, nothing else has changed yet
		EntityAI created = EntityAI.Cast(g_Game.CreateObjectEx(type, player.GetPosition(), ECE_PLACE_ON_SURFACE));
		if (!created)
		{
			SendResult(player, identity, WRMStatus.FAILED, type);
			return;
		}

		// 2. occupied slot: the old item goes to the inventory, else to the ground, never lost
		if (old)
		{
			if (!weapon.GetInventory().CanRemoveAttachmentEx(old, slotId) || !MoveAttachmentAway(player, weapon, old, slotId))
			{
				g_Game.ObjectDelete(created);
				SendResult(player, identity, WRMStatus.NO_ROOM, type);
				return;
			}
		}

		// 3. validate on the real instance, 4. attach; any refusal removes the stray new item and restores the old one
		// the ground instance is only a probe: the real one is created in the slot, which is synchronous
		bool allowed = CanAttachHere(weapon, created, slotId, muzzle);
		g_Game.ObjectDelete(created);
		created = null;
		if (allowed)
			created = weapon.GetInventory().CreateAttachmentEx(type, slotId);
		if (!created)
		{
			RestoreOld(player, weapon, old, slotId);
			SendResult(player, identity, WRMStatus.FAILED, type);
			return;
		}

		// the weapon FSM validates itself against what is attached; the chamber and its cartridge are never touched
		if (muzzle >= 0)
		{
			weapon.ValidateAndRepair();
			weapon.Synchronize();
		}

		// the weapon keeps its issue-time snapshot: refreshing it here would adopt ammo the player loaded as "issued"
		Track(key, created);
		TrimToCapacity(key);
		SendResult(player, identity, WRMStatus.OK, type);
	}
}
