modded class PlayerBase
{
	override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		super.OnRPC(sender, rpc_type, ctx);

		if (rpc_type == CUIQS_RPC_ID)
		{
			HandleQuickSlotRPC(sender, ctx);
			return;
		}

		if (rpc_type != WRM_RPC_ID || !WeaponRadialMenu)
			return;

		WRMMsg msg = new WRMMsg();
		if (!msg.Read(ctx))
			return;

		if (g_Game.IsServer())
		{
			// The player is the sender's own character. The RPC target is client controlled and is only accepted
			// when it is that very character, so nobody can act through another player's object.
			if (!sender)
				return;
			PlayerBase senderPlayer = PlayerBase.Cast(sender.GetPlayer());
			if (!senderPlayer || senderPlayer != this)
				return;

			WRMServer.Handle(senderPlayer, sender, msg);
		}
		else
		{
			if (PlayerBase.Cast(g_Game.GetPlayer()) != this)
				return;

			WRMClient.OnMessage(msg);
		}
	}

	// Quick slot wheel: same sender check as the weapon menu, then the server side of the wheel.
	protected void HandleQuickSlotRPC(PlayerIdentity sender, ParamsReadContext ctx)
	{
		WRMMsg msg = CUIQSMsg.Read(ctx);
		if (!msg)
			return;

		if (g_Game.IsServer())
		{
			if (!sender)
				return;
			PlayerBase senderPlayer = PlayerBase.Cast(sender.GetPlayer());
			if (!senderPlayer || senderPlayer != this)
				return;

			CUIQuickSlotServer.Handle(senderPlayer, sender, msg);
		}
		else
		{
			if (PlayerBase.Cast(g_Game.GetPlayer()) != this)
				return;

			CUIQuickSlotLocal.OnMessage(msg);
		}
	}
}
