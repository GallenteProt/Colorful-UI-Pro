// Marks issued items that enter another identity's possession (see WRMServer.OnItemMoved). Weapon and Magazine inherit ItemBase.
modded class ItemBase
{
	override void OnItemLocationChanged(EntityAI old_owner, EntityAI new_owner)
	{
		super.OnItemLocationChanged(old_owner, new_owner);

		if (g_Game.IsServer())
			WRMServer.OnItemMoved(this, new_owner);
	}
}
