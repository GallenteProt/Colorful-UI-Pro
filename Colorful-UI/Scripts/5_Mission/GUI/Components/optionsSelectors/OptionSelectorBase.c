// OptionSelectorBase — minimal scheme color swap.
// -----------------------------------------------------------------------------
// Vanilla highlights option rows with red ARGB(255,255,0,0) and uses white
// for the normal text. We just swap those two values for our scheme — every
// other vanilla widget-finding / event behavior is untouched.
//
// Vanilla source: P:\scripts\5_mission\gui\newui\optionselectorbase.c

modded class OptionSelectorBase
{
	override void ColorHighlight(Widget w)
	{
		if (!w) return;
		ButtonSetColor(w, colorScheme.ButtonHover());

		// The parent menu highlights the whole row before this method runs.
		// Keep the selector value on a dark background so it cannot become
		// black-on-black or white-on-white when either chevron is hovered.
		int lbl = colorScheme.TextHover();
		Widget title_label  = w.FindAnyWidget(w.GetName() + "_label");
		Widget option_wrapper;
		Widget option_label;
		if (m_Root)
		{
			option_wrapper = m_Root.GetParent();
			option_label = m_Root.FindAnyWidget("option_label");
		}

		if (title_label)  title_label.SetColor(lbl);
		if (option_wrapper) option_wrapper.SetColor(colorScheme.OptionBGHover());
		if (option_label) option_label.SetColor(colorScheme.OptionSelectionText());
	}

	override void ColorHighlightConsole(Widget w)
	{
		if (!w) return;

		ButtonSetColorConsole(w, colorScheme.ButtonHover());
		ButtonSetAlphaAnimConsole(null);
		ButtonSetTextColorConsole(w, colorScheme.PrimaryText());
	}
}
