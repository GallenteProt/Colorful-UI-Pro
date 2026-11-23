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
	// The wheel's own outline: a dark groove under the cards, so the ring and its sector dividers stay
	// readable on a bright background (snow, sky) without fighting the card edges. Each line is drawn
	// twice - the dark groove first, then a light line on top of it - so the outline reads on a dark
	// night scene and on snow alike.
	static int Ring()				{ return ARGB(150, 0, 0, 0); }
	static int RingLight()			{ return ARGB(95, 220, 217, 207); }
	static int RingActive()			{ return (colorScheme.BrandColor() & 0x00FFFFFF) | 0x96000000; }

	// C13 palette, taken from C13_ItemInfo's own layouts so the quick slot wheel and the C13 HUD read as
	// one interface: a warm off-white for text, a muted tan for secondary text and the C13 gold accent
	// for anything that must stand out. The values are the exact floats C13 uses, converted to 0..255.
	//   0.9373 0.9176 0.8627  text      -> 239 234 220
	//   0.6078 0.5882 0.5294  muted     -> 155 150 135
	//   0.7961 0.7176 0.4157  accent    -> 203 183 106
	static int C13Text()			{ return ARGB(255, 239, 234, 220); }
	static int C13Muted()			{ return ARGB(255, 155, 150, 135); }
	static int C13Accent()			{ return ARGB(255, 203, 183, 106); }
	static int C13AccentDim()		{ return ARGB(110, 203, 183, 106); }
}
