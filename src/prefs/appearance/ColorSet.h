#ifndef COLORSET_H_
#define COLORSET_H_

#include "RGBColor.h"

typedef struct
{
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
	menu_triggers;
} ColorSet;

#endif