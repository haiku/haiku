#include <stdio.h>
#include "ColorSet.h"

//! Constructor which does nothing
ColorSet::ColorSet(void)
{
}

/*!
	\brief Copy constructor which does a massive number of assignments
	\param Color set to copy from
*/
ColorSet::ColorSet(const ColorSet &cs)
{
	panel_background=cs.panel_background;
	panel_text=cs.panel_text;

	document_background=cs.document_background;
	document_text=cs.document_text;

	control_background=cs.control_background;
	control_text=cs.control_text;
	control_highlight=cs.control_highlight;
	control_border=cs.control_border;

	tooltip_background=cs.tooltip_background;
	tooltip_text=cs.tooltip_text;

	menu_background=cs.menu_background;
	menu_selected_background=cs.menu_selected_background;
	menu_text=cs.menu_text;
	menu_selected_text=cs.menu_selected_text;

	window_tab=cs.window_tab;
	window_tab_text=cs.window_tab_text;
	inactive_window_tab=cs.inactive_window_tab;
	inactive_window_tab_text=cs.inactive_window_tab_text;
	keyboard_navigation=cs.keyboard_navigation;
	desktop=cs.desktop;
}

/*!
	\brief Overloaded assignment operator which does a massive number of assignments
	\param Color set to copy from
	\param The new values assigned to the color set
*/
ColorSet & ColorSet::operator=(const ColorSet &cs)
{
	SetColors(cs);
	return *this;
}

/*!
	\brief Copy function which handles assignments, 
		and, yes, *IT EVEN MAKES french fries!!*
	\param Color set to copy from
*/
void ColorSet::SetColors(const ColorSet &cs)
{
	panel_background=cs.panel_background;
	panel_text=cs.panel_text;

	document_background=cs.document_background;
	document_text=cs.document_text;

	control_background=cs.control_background;
	control_text=cs.control_text;
	control_highlight=cs.control_highlight;
	control_border=cs.control_border;

	tooltip_background=cs.tooltip_background;
	tooltip_text=cs.tooltip_text;

	menu_background=cs.menu_background;
	menu_selected_background=cs.menu_selected_background;
	menu_text=cs.menu_text;
	menu_selected_text=cs.menu_selected_text;

	window_tab=cs.window_tab;
	window_tab_text=cs.window_tab_text;
	inactive_window_tab=cs.inactive_window_tab;
	inactive_window_tab_text=cs.inactive_window_tab_text;
	
	keyboard_navigation=cs.keyboard_navigation;
	desktop=cs.desktop;
}

/*!
	\brief Prints all color set elements to stdout
*/
void ColorSet::PrintToStream(void)
{
	printf("panel_background "); panel_background.PrintToStream();
	printf("panel_text "); panel_text.PrintToStream();

	printf("document_background "); document_background.PrintToStream();
	printf("document_text "); document_text.PrintToStream();

	printf("control_background "); control_background.PrintToStream();
	printf("control_text "); control_text.PrintToStream();
	printf("control_highlight "); control_highlight.PrintToStream();
	printf("control_border "); control_border.PrintToStream();

	printf("tooltip_background "); tooltip_background.PrintToStream();
	printf("tooltip_text "); tooltip_text.PrintToStream();

	printf("menu_background "); menu_background.PrintToStream();
	printf("menu_selected_background "); menu_selected_background.PrintToStream();
	printf("menu_text "); menu_text.PrintToStream();
	printf("menu_selected_text "); menu_selected_text.PrintToStream();

	printf("window_tab "); window_tab.PrintToStream();
	printf("window_tab_text "); window_tab_text.PrintToStream();

	printf("inactive_window_tab "); inactive_window_tab.PrintToStream();
	printf("inactive_window_tab_text "); inactive_window_tab_text.PrintToStream();

	printf("keyboard_navigation "); keyboard_navigation.PrintToStream();
	printf("desktop "); desktop.PrintToStream();
}
