modded class TabberUI extends ScriptedWidgetEventHandler
{
	override void AlignTabbers()
	{
		if (!m_TabControlsRoot)
			return;

		Widget tabControlsContainer = m_TabControlsRoot.FindAnyWidget("Tab_Control_Container");
		Widget tabControlsScroller = m_TabControlsRoot.FindAnyWidget("Tab_Control_Scroller");
		if (!tabControlsContainer)
			return;

		float totalSize = 0;
		float childX;
		float childY;

		m_TabControlsRoot.Update();
		tabControlsContainer.Update();

		Widget tabChild = tabControlsContainer.GetChildren();
		while (tabChild)
		{
			if (tabChild.IsVisible())
			{
				TextWidget tabText = TextWidget.Cast(tabChild.FindAnyWidget(tabChild.GetName() + "_Title"));
				if (tabText)
				{
					int textWidth;
					int textHeight;
					tabText.Update();
					tabText.GetTextSize(textWidth, textHeight);

					float tabWidth = textWidth + 20;
					tabChild.SetSize(tabWidth, 1);
					tabControlsContainer.Update();
					totalSize += tabWidth;
				}
			}

			tabChild = tabChild.GetSibling();
		}

		tabControlsContainer.SetSize(totalSize, 1);
		tabChild = tabControlsContainer.GetChildren();

		float containerX;
		float containerY;
		tabControlsContainer.GetScreenPos(containerX, containerY);

		while (tabChild)
		{
			Widget tabBackground = tabChild.FindAnyWidget(tabChild.GetName() + "_Background");
			if (tabBackground)
			{
				tabChild.GetScreenPos(childX, childY);
				tabBackground.SetPos(containerX - childX, 0);
				tabBackground.SetSize(totalSize, 1);
			}

			tabChild = tabChild.GetSibling();
		}

		tabControlsContainer.Update();
		if (tabControlsScroller)
			tabControlsScroller.Update();
		m_TabControlsRoot.Update();
	}

	void ApplyTabColor(Widget tab_control, bool selected, bool hovered = false)
	{
		if (!tab_control) return;

		TextWidget title = TextWidget.Cast(tab_control.FindAnyWidget(tab_control.GetName() + "_Title"));
		if (!title) return;

		if (selected)
		{
			title.SetColor(colorScheme.TabSelectedColor());
			tab_control.SetColor(colorScheme.TabBackground());
			return;
		}

		if (hovered)
		{
			title.SetColor(colorScheme.TabHoverColor());
			tab_control.SetColor(colorScheme.TabBackground());
			return;
		}

		title.SetColor(colorScheme.TabIdle());
		tab_control.SetColor(UIColor.Transparent());
	}

	override void SelectTabControl(int index)
	{
		Widget tab_control = m_TabControls.Get(index);
		ApplyTabColor(tab_control, true, false);
	}

	override bool OnMouseEnter(Widget w, int x, int y)
	{
		int index = m_TabControls.GetKeyByValue(w);
		if (index != m_SelectedIndex)
			ApplyTabColor(m_TabControls.Get(index), false, true);
		return false;
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		int index = m_TabControls.GetKeyByValue(w);
		if (index != m_SelectedIndex)
			ApplyTabColor(m_TabControls.Get(index), false, false);
		return false;
	}

	override bool OnMouseWheel(Widget w, int x, int y, int wheel)
	{
		if (!m_TabControlsRoot)
			return super.OnMouseWheel(w, x, y, wheel);

		ScrollWidget tabScroller = ScrollWidget.Cast(m_TabControlsRoot.FindAnyWidget("Tab_Control_Scroller"));
		if (!tabScroller)
			return super.OnMouseWheel(w, x, y, wheel);

		Widget current = w;
		while (current)
		{
			if (current == tabScroller)
			{
				tabScroller.HScrollStep(-wheel);
				return true;
			}

			current = current.GetParent();
		}

		return super.OnMouseWheel(w, x, y, wheel);
	}

	override int AddTab(string name)
	{
		int new_index = m_Tabs.Count();

		Widget tab = GetGame().GetWorkspace().CreateWidgets("gui/layouts/new_ui/tabber_prefab/tab.layout", m_Root);
		Widget control = GetGame().GetWorkspace().CreateWidgets("Colorful-UI/GUI/layouts/components/tabber_prefab/cui.tab_control.layout", m_Root.FindAnyWidget("Tab_Control_Container"));
		TextWidget title = TextWidget.Cast(control.FindAnyWidget("Tab_Control_x_Title"));

		tab.SetName("Tab_" + new_index);
		control.SetName("Tab_Control_" + new_index);
		title.SetName("Tab_Control_" + new_index + "_Title");
		control.FindAnyWidget("Tab_Control_x_Background").SetName("Tab_Control_" + new_index + "_Background");

		title.SetText(name);

		control.SetHandler(this);
		m_TabControls.Insert(new_index, control);
		m_Tabs.Insert(new_index, tab);

		AlignTabbers();
		return new_index;
	}
}
