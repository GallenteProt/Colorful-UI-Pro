// The weapon radial menu's colours, taken from the Colorful UI theme (colorScheme, Scheme.c) so the
// wheel matches the inventory: dark rounded cards with a thin grey edge, the theme colour on the
// edge of the item in hands / the installed attachment and on the hovered card, the same soft
// theme-colour hover wash as the inventory tools' menus (SortMenu), and the inventory's warm white text.
class WRMLook
{
	static int CardBack()			{ return ARGB(235, 18, 18, 18); }
	static int CardBackDisabled()	{ return ARGB(140, 18, 18, 18); }
	static int Edge()				{ return ARGB(255, 58, 58, 58); }
	static int EdgeDisabled()		{ return ARGB(120, 58, 58, 58); }
	static int EdgeActive()			{ return colorScheme.BrandColor(); }
	static int EdgeHover()			{ return (colorScheme.BrandColor() & 0x00FFFFFF) | 0xB4000000; }
	static int Wash()				{ return (colorScheme.BrandColor() & 0x00FFFFFF) | 0x2E000000; }
	static int ActiveWash()			{ return (colorScheme.BrandColor() & 0x00FFFFFF) | 0x1A000000; }
	static int Text()				{ return ARGB(255, 220, 217, 207); }
	static int Icon()				{ return ARGB(255, 220, 217, 207); }
	static int IconDisabled()		{ return ARGB(110, 150, 150, 150); }
}
