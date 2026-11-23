modded class DayZPlayerImplement
{
	override void StopDeathDarkeningEffect()
	{
		super.StopDeathDarkeningEffect();

		PPERequesterBank.GetRequester(PPERequester_DeathDarkening).Stop();
	}
}
