/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 */
#ifndef COLORWHICH_ITEM_H
#define COLORWHICH_ITEM_H

#include <InterfaceDefs.h>
#include <ListItem.h>

#define B_INACTIVE_WINDOW_TAB_TEXT_COLOR 'iwtt'
#define B_WINDOW_TAB_TEXT_COLOR 'wttx'
#define B_INACTIVE_WINDOW_TAB_COLOR 'iwtb'

//#ifdef BUILD_UNDER_R5
	#define B_MENU_SELECTED_BACKGROUND_COLOR B_MENU_SELECTION_BACKGROUND_COLOR
	#define B_PANEL_TEXT_COLOR 'ptxt'
	#define B_DOCUMENT_BACKGROUND_COLOR 'dbgc'
	#define B_DOCUMENT_TEXT_COLOR 'dtxc'
	#define B_CONTROL_BACKGROUND_COLOR 'cbgc'
	#define B_CONTROL_TEXT_COLOR 'ctxc'
	#define B_CONTROL_BORDER_COLOR 'cboc'
	#define B_CONTROL_HIGHLIGHT_COLOR 'chic'
	#define B_NAVIGATION_BASE_COLOR 'navb'
	#define B_NAVIGATION_PULSE_COLOR 'navp'
	#define B_SHINE_COLOR 'shin'
	#define B_SHADOW_COLOR 'shad'
	#define B_MENU_SELECTED_BORDER_COLOR 'msbo'
	#define B_TOOLTIP_BACKGROUND_COLOR 'ttbg'
	#define B_TOOLTIP_TEXT_COLOR 'tttx'
	#define B_SUCCESS_COLOR 'sucs'
	#define B_FAILURE_COLOR 'fail'
//#endif

class ColorWhichItem : public BStringItem
{
public:
	ColorWhichItem(color_which which);
	~ColorWhichItem(void);
	void SetAttribute(color_which which);
	color_which GetAttribute(void);
private:
	color_which attribute;
};

#endif
