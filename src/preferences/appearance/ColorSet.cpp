/*
 * Copyright 2001-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Rene Gollent <rene@gollent.com>
 */

#include <stdio.h>
#include <Catalog.h>
#include <DefaultColors.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <InterfaceDefs.h>
#include <Locale.h>
#include <Message.h>
#include <ServerReadOnlyMemory.h>
#include <String.h>
#include "ColorSet.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Colors tab"

static ColorDescription sColorDescriptionTable[] =
{
	{ B_PANEL_BACKGROUND_COLOR, B_TRANSLATE_MARK("Panel background") },
	{ B_PANEL_TEXT_COLOR, B_TRANSLATE_MARK("Panel text") },
	{ B_DOCUMENT_BACKGROUND_COLOR, B_TRANSLATE_MARK("Document background") },
	{ B_DOCUMENT_TEXT_COLOR, B_TRANSLATE_MARK("Document text") },
	{ B_CONTROL_BACKGROUND_COLOR, B_TRANSLATE_MARK("Control background") },
	{ B_CONTROL_TEXT_COLOR, B_TRANSLATE_MARK("Control text") },
	{ B_CONTROL_BORDER_COLOR, B_TRANSLATE_MARK("Control border") },
	{ B_CONTROL_HIGHLIGHT_COLOR, B_TRANSLATE_MARK("Control highlight") },
	{ B_NAVIGATION_BASE_COLOR, B_TRANSLATE_MARK("Navigation base") },
	{ B_NAVIGATION_PULSE_COLOR, B_TRANSLATE_MARK("Navigation pulse") },
	{ B_SHINE_COLOR, B_TRANSLATE_MARK("Shine") },
	{ B_SHADOW_COLOR, B_TRANSLATE_MARK("Shadow") },
	{ B_MENU_BACKGROUND_COLOR, B_TRANSLATE_MARK("Menu background") },
	{ B_MENU_SELECTED_BACKGROUND_COLOR,
		B_TRANSLATE_MARK("Selected menu item background") },
	{ B_MENU_ITEM_TEXT_COLOR, B_TRANSLATE_MARK("Menu item text") },
	{ B_MENU_SELECTED_ITEM_TEXT_COLOR,
		B_TRANSLATE_MARK("Selected menu item text") },
	{ B_MENU_SELECTED_BORDER_COLOR,
		B_TRANSLATE_MARK("Selected menu item border") },
	{ B_TOOL_TIP_BACKGROUND_COLOR, B_TRANSLATE_MARK("Tooltip background") },
	{ B_TOOL_TIP_TEXT_COLOR, B_TRANSLATE_MARK("Tooltip text") },
	{ B_SUCCESS_COLOR, B_TRANSLATE_MARK("Success") },
	{ B_FAILURE_COLOR, B_TRANSLATE_MARK("Failure") },
	{ B_WINDOW_TAB_COLOR, B_TRANSLATE_MARK("Window tab") },
	{ B_WINDOW_TEXT_COLOR, B_TRANSLATE_MARK("Window tab text") },
	{ B_WINDOW_INACTIVE_TAB_COLOR, B_TRANSLATE_MARK("Inactive window tab") },
	{ B_WINDOW_INACTIVE_TEXT_COLOR,
		B_TRANSLATE_MARK("Inactive window tab text") },
	{ B_WINDOW_BORDER_COLOR, B_TRANSLATE_MARK("Window border") },
	{ B_WINDOW_INACTIVE_BORDER_COLOR,
		B_TRANSLATE_MARK("Inactive window border") }
};

const int32 sColorDescriptionCount = sizeof(sColorDescriptionTable)
	/ sizeof(ColorDescription);

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
	
	for (int i = 0; i < sColorDescriptionCount; i++) {
		color_which which = get_color_description(i)->which;
		set.fColors[which] =
			BPrivate::kDefaultColors[color_which_to_index(which)];
	}
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


