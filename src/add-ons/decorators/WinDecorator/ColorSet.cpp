#include "ColorSet.h"

ColorSet::ColorSet(void)
{
}

ColorSet::ColorSet(const ColorSet &cs)
{
	panel_background=cs.panel_background;
	panel_text=cs.panel_text;
	document_background=cs.document_background;
	document_text=cs.document_text;
	control_background=cs.control_background;
	control_text=cs.control_text;
	control_border=cs.control_border;
	control_highlight=cs.control_highlight;
	tooltip_background=cs.tooltip_background;
	tooltip_text=cs.tooltip_text;
	menu_background=cs.menu_background;
	menu_selected_background=cs.menu_selected_background;
	menu_text=cs.menu_text;
	menu_selected_text=cs.menu_selected_text;
	menu_separator_high=cs.menu_separator_high;
	menu_separator_low=cs.menu_separator_low;
	menu_triggers=cs.menu_triggers;
	window_tab=cs.window_tab;
	window_tab_text=cs.window_tab_text;
	keyboard_navigation=cs.keyboard_navigation;
	desktop=cs.desktop;
}

ColorSet & ColorSet::operator=(const ColorSet &cs)
{
	SetColors(cs);
	return *this;
}

void ColorSet::SetColors(const ColorSet &cs)
{
	panel_background=cs.panel_background;
	panel_text=cs.panel_text;
	document_background=cs.document_background;
	document_text=cs.document_text;
	control_background=cs.control_background;
	control_text=cs.control_text;
	control_border=cs.control_border;
	control_highlight=cs.control_highlight;
	tooltip_background=cs.tooltip_background;
	tooltip_text=cs.tooltip_text;
	menu_background=cs.menu_background;
	menu_selected_background=cs.menu_selected_background;
	menu_text=cs.menu_text;
	menu_selected_text=cs.menu_selected_text;
	menu_separator_high=cs.menu_separator_high;
	menu_separator_low=cs.menu_separator_low;
	menu_triggers=cs.menu_triggers;
	window_tab=cs.window_tab;
	window_tab_text=cs.window_tab_text;
	keyboard_navigation=cs.keyboard_navigation;
	desktop=cs.desktop;
}
