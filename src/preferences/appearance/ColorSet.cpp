/*
 * Copyright 2001-2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Rene Gollent <rene@gollent.com>
 */

#include <stdio.h>
#include <InterfaceDefs.h>
#include <Message.h>
#include <File.h>
#include <Entry.h>
#include <Directory.h>
#include <String.h>
#include "ColorSet.h"


static ColorDescription sColorDescriptionTable[] = 
{
	{ B_PANEL_BACKGROUND_COLOR, "Panel Background" },
	{ B_PANEL_TEXT_COLOR, "Panel Text" },
	{ B_DOCUMENT_BACKGROUND_COLOR, "Document Background" },
	{ B_DOCUMENT_TEXT_COLOR, "Document Text" },
	{ B_CONTROL_BACKGROUND_COLOR, "Control Background" },
	{ B_CONTROL_TEXT_COLOR, "Control Text" },
	{ B_CONTROL_BORDER_COLOR, "Control Border" },
	{ B_CONTROL_HIGHLIGHT_COLOR, "Control Highlight" },
	{ B_NAVIGATION_BASE_COLOR, "Navigation Base" },
	{ B_NAVIGATION_PULSE_COLOR, "Navigation Pulse" },
	{ B_SHINE_COLOR, "Shine" },
	{ B_SHADOW_COLOR, "Shadow" },
	{ B_MENU_BACKGROUND_COLOR, "Menu Background" },
	{ B_MENU_SELECTED_BACKGROUND_COLOR, "Selected Menu Item Background" },
	{ B_MENU_ITEM_TEXT_COLOR, "Menu Item Text" },
	{ B_MENU_SELECTED_ITEM_TEXT_COLOR, "Selected Menu Item Text" },
	{ B_MENU_SELECTED_BORDER_COLOR, "Selected Menu Item Border" },
	{ B_TOOLTIP_BACKGROUND_COLOR, "Tooltip Background" },
	{ B_TOOLTIP_TEXT_COLOR, "Tooltip Text" },
	{ B_SUCCESS_COLOR, "Success" },
	{ B_FAILURE_COLOR, "Failure" },
	{ B_WINDOW_TAB_COLOR, "Window Tab" },
	{ B_WINDOW_TEXT_COLOR, "Window Tab Text" },
	{ B_WINDOW_INACTIVE_TAB_COLOR, "Inactive Window Tab" },
	{ B_WINDOW_INACTIVE_TEXT_COLOR, "Inactive Window Tab Text" }
};

const int32 sColorDescriptionCount = sizeof(sColorDescriptionTable) / sizeof(ColorDescription);

const ColorDescription* 
get_color_description(int32 index)
{
	if (index < 0 || index >= sColorDescriptionCount)
		return NULL;
	return &sColorDescriptionTable[index];
}

int32
color_description_count(void)
{
	return sColorDescriptionCount;
}

//	#pragma mark -


ColorSet::ColorSet()
{
}

/*!
	\brief Copy constructor which does a massive number of assignments
	\param cs Color set to copy from
*/
ColorSet::ColorSet(const ColorSet &cs)
{
	*this = cs;
}

/*!
	\brief Overloaded assignment operator which does a massive number of assignments
	\param cs Color set to copy from
	\return The new values assigned to the color set
*/
ColorSet &
ColorSet::operator=(const ColorSet &cs)
{
	fColors = cs.fColors;
	return *this;
}


/*!
	\brief Assigns the default system colors to the passed ColorSet object
	\param set The ColorSet object to set to defaults
*/
ColorSet
ColorSet::DefaultColorSet(void)
{
	ColorSet set;
	set.fColors[B_PANEL_BACKGROUND_COLOR] = make_color(216, 216, 216);
	set.fColors[B_PANEL_TEXT_COLOR] = make_color(0, 0, 0);
	set.fColors[B_DOCUMENT_BACKGROUND_COLOR] = make_color(255,255, 255);
	set.fColors[B_DOCUMENT_TEXT_COLOR] = make_color(0, 0, 0);
	set.fColors[B_CONTROL_BACKGROUND_COLOR] = make_color(245, 245, 245);
	set.fColors[B_CONTROL_TEXT_COLOR] = make_color(0, 0, 0);
	set.fColors[B_CONTROL_BORDER_COLOR] = make_color(0, 0, 0);
	set.fColors[B_CONTROL_HIGHLIGHT_COLOR] = make_color(102, 152, 203);
	set.fColors[B_NAVIGATION_BASE_COLOR] = make_color(0, 0, 229);
	set.fColors[B_NAVIGATION_PULSE_COLOR] = make_color(0, 0, 0);
	set.fColors[B_SHINE_COLOR] = make_color(255, 255, 255);
	set.fColors[B_SHADOW_COLOR] = make_color(0, 0, 0);
	set.fColors[B_MENU_BACKGROUND_COLOR] = make_color(216, 216, 216);
	set.fColors[B_MENU_SELECTED_BACKGROUND_COLOR] = make_color(115, 120, 184);
	set.fColors[B_MENU_ITEM_TEXT_COLOR] = make_color(0, 0, 0);
	set.fColors[B_MENU_SELECTED_ITEM_TEXT_COLOR] = make_color(255, 255, 255);
	set.fColors[B_MENU_SELECTED_BORDER_COLOR] = make_color(0, 0, 0);
	set.fColors[B_TOOLTIP_BACKGROUND_COLOR] = make_color(255, 255, 0);
	set.fColors[B_TOOLTIP_TEXT_COLOR] = make_color(0, 0, 0);
	set.fColors[B_SUCCESS_COLOR] = make_color(0, 255, 0);
	set.fColors[B_FAILURE_COLOR] = make_color(255, 0, 0);
	set.fColors[B_WINDOW_TAB_COLOR] = make_color(255, 203, 0);
	set.fColors[B_WINDOW_TEXT_COLOR] = make_color(0, 0, 0);
	set.fColors[B_WINDOW_INACTIVE_TAB_COLOR] = make_color(232, 232, 232);
	set.fColors[B_WINDOW_INACTIVE_TEXT_COLOR] = make_color(80, 80, 80);

	return set;
}

	
/*!
	\brief Assigns a value to a named color member
	\param string name of the color to receive the value
	\param value An rgb_color which is the new value of the member
*/
void
ColorSet::SetColor(color_which which, rgb_color value)
{
	fColors[which] = value;
}


rgb_color
ColorSet::GetColor(int32 which)
{
	return fColors[(color_which)which];
}


