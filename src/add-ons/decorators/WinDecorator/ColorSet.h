#ifndef COLORSET_H_
#define COLORSET_H_

#include "RGBColor.h"

class ColorSet
{
public:
	ColorSet(void);
	ColorSet(const ColorSet &cs);
	ColorSet & operator=(const ColorSet &cs);
	void SetColors(const ColorSet &cs);
	
	RGBColor panel_background,
	panel_text,
	document_background,
	document_text,
	control_background,
	control_text,
	control_border,
	control_highlight,
	tooltip_background,
	tooltip_text,
	menu_background,
	menu_selected_background,
	menu_text,
	menu_selected_text,
	menu_separator_high,
	menu_separator_low,
	menu_triggers,
	window_tab,
	window_tab_text,
	keyboard_navigation,
	desktop;
};

#endif