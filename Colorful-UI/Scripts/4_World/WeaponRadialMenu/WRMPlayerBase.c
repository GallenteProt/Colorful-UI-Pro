modded class PlayerBase
{
	override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		super.OnRPC(sender, rpc_type, ctx);

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
}
