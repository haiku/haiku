/*
 * Copyright 2001-2015, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Rene Gollent <rene@gollent.com>
 *		Joseph Groover <looncraz@looncraz.net>
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
#include "Colors.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Colors tab"


static ColorDescription sColorDescriptionTable[] = {
	{ B_PANEL_BACKGROUND_COLOR, B_TRANSLATE_MARK("Panel background") },
	{ B_PANEL_TEXT_COLOR, B_TRANSLATE_MARK("Panel text") },
	{ B_DOCUMENT_BACKGROUND_COLOR, B_TRANSLATE_MARK("Document background") },
	{ B_DOCUMENT_TEXT_COLOR, B_TRANSLATE_MARK("Document text") },
	{ B_CONTROL_BACKGROUND_COLOR, B_TRANSLATE_MARK("Control background") },
	{ B_CONTROL_TEXT_COLOR, B_TRANSLATE_MARK("Control text") },
	{ B_CONTROL_BORDER_COLOR, B_TRANSLATE_MARK("Control border") },
	{ B_CONTROL_HIGHLIGHT_COLOR, B_TRANSLATE_MARK("Control highlight") },
	{ B_CONTROL_MARK_COLOR, B_TRANSLATE_MARK("Control mark") },
	{ B_NAVIGATION_BASE_COLOR, B_TRANSLATE_MARK("Navigation base") },
	{ B_NAVIGATION_PULSE_COLOR, B_TRANSLATE_MARK("Navigation pulse") },
	{ B_SHINE_COLOR, B_TRANSLATE_MARK("Shine") },
	{ B_SHADOW_COLOR, B_TRANSLATE_MARK("Shadow") },
	{ B_LINK_TEXT_COLOR, B_TRANSLATE_MARK("Link text") },
	{ B_LINK_HOVER_COLOR, B_TRANSLATE_MARK("Link hover") },
	{ B_LINK_VISITED_COLOR, B_TRANSLATE_MARK("Link visited") },
	{ B_LINK_ACTIVE_COLOR, B_TRANSLATE_MARK("Link active") },
	{ B_MENU_BACKGROUND_COLOR, B_TRANSLATE_MARK("Menu background") },
	{ B_MENU_SELECTED_BACKGROUND_COLOR,
		B_TRANSLATE_MARK("Selected menu item background") },
	{ B_MENU_ITEM_TEXT_COLOR, B_TRANSLATE_MARK("Menu item text") },
	{ B_MENU_SELECTED_ITEM_TEXT_COLOR,
		B_TRANSLATE_MARK("Selected menu item text") },
	{ B_MENU_SELECTED_BORDER_COLOR,
		B_TRANSLATE_MARK("Selected menu item border") },
	{ B_LIST_BACKGROUND_COLOR, B_TRANSLATE_MARK("List background") },
	{ B_LIST_SELECTED_BACKGROUND_COLOR,
		B_TRANSLATE_MARK("Selected list item background") },
	{ B_LIST_ITEM_TEXT_COLOR, B_TRANSLATE_MARK("List item text") },
	{ B_LIST_SELECTED_ITEM_TEXT_COLOR,
		B_TRANSLATE_MARK("Selected list item text") },
	{ B_SCROLL_BAR_THUMB_COLOR,
		B_TRANSLATE_MARK("Scroll bar thumb") },
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


void
get_default_colors(BMessage* message)
{
	if (message == NULL)
		return;

	for (int32 index = 0; index < kColorWhichCount; ++index) {
		color_which which = index_to_color_which(index);
		message->AddColor(ui_color_name(which),
			BPrivate::kDefaultColors[index]);
	}
}


void
get_current_colors(BMessage* message)
{
	if (message == NULL)
		return;

	for (int32 index = 0; index < kColorWhichCount; ++index) {
		color_which which = index_to_color_which(index);
		message->AddColor(ui_color_name(which), ui_color(which));
	}
}

