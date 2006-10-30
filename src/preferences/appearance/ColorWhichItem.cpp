/*
 * Copyright 2002-2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm (darkwyrm@earthlink.net)
 */
#include "ColorWhichItem.h"
#include <stdio.h>

ColorWhichItem::ColorWhichItem(color_which which)
 :	BStringItem(NULL,0,false)
{
	SetAttribute(which);
}

ColorWhichItem::~ColorWhichItem(void)
{
	// Empty, but exists for just-in-case
}

void
ColorWhichItem::SetAttribute(color_which which)
{
	switch(which) {
		// cases not existing in R5 which exist in OpenBeOS
		case B_PANEL_BACKGROUND_COLOR: {
			attribute=which;
			SetText("Panel Background");
			break;
		}
		case B_PANEL_TEXT_COLOR: {
			attribute=which;
			SetText("Panel Text");
			break;
		}
		case B_DOCUMENT_BACKGROUND_COLOR: {
			attribute=which;
			SetText("Document Background");
			break;
		}
		case B_DOCUMENT_TEXT_COLOR: {
			attribute=which;
			SetText("Document Text");
			break;
		}
		case B_CONTROL_BACKGROUND_COLOR: {
			attribute=which;
			SetText("Control Background");
			break;
		}
		case B_CONTROL_TEXT_COLOR: {
			attribute=which;
			SetText("Control Text");
			break;
		}
		case B_CONTROL_BORDER_COLOR: {
			attribute=which;
			SetText("Control Border");
			break;
		}
		case B_CONTROL_HIGHLIGHT_COLOR: {
			attribute=which;
			SetText("Control Highlight");
			break;
		}
		case B_TOOLTIP_BACKGROUND_COLOR: {
			attribute=which;
			SetText("Tooltip Background");
			break;
		}
		case B_TOOLTIP_TEXT_COLOR: {
			attribute=which;
			SetText("Tooltip Text");
			break;
		}
		case B_MENU_BACKGROUND_COLOR: {
			attribute=which;
			SetText("Menu Background");
			break;
		}
		case B_MENU_SELECTION_BACKGROUND_COLOR: {
			attribute=which;
			SetText("Selected Menu Item Background");
			break;
		}
		case B_MENU_ITEM_TEXT_COLOR: {
			attribute=which;
			SetText("Menu Item Text");
			break;
		}
		case B_MENU_SELECTED_ITEM_TEXT_COLOR: {
			attribute=which;
			SetText("Selected Menu Item Text");
			break;
		}
		case B_MENU_SELECTED_BORDER_COLOR: {
			attribute=which;
			SetText("Selected Menu Item Border");
			break;
		}
		case B_NAVIGATION_BASE_COLOR: {
			attribute=which;
			SetText("Navigation Base");
			break;
		}
		case B_NAVIGATION_PULSE_COLOR: {
			attribute=which;
			SetText("Navigation Pulse");
			break;
		}
		case B_SUCCESS_COLOR: {
			attribute=which;
			SetText("Success");
			break;
		}
		case B_FAILURE_COLOR: {
			attribute=which;
			SetText("Failure");
			break;
		}
		case B_SHINE_COLOR: {
			attribute=which;
			SetText("Shine");
			break;
		}
		case B_SHADOW_COLOR: {
			attribute=which;
			SetText("Shadow");
			break;
		}
		case B_WINDOW_TAB_COLOR: {
			attribute=which;
			SetText("Window Tab");
			break;
		}
		case B_WINDOW_TAB_TEXT_COLOR: {
			attribute=which;
			SetText("Window Tab Text");
			break;
		}
		case B_INACTIVE_WINDOW_TAB_COLOR: {
			attribute=which;
			SetText("Inactive Window Tab");
			break;
		}
		case B_INACTIVE_WINDOW_TAB_TEXT_COLOR: {
			attribute=which;
			SetText("Inactive Window Tab Text");
			break;
		}
		default: {
			printf("unknown code '%c%c%c%c'\n",(char)((which & 0xFF000000) >>  24),
				(char)((which & 0x00FF0000) >>  16),
				(char)((which & 0x0000FF00) >>  8),
				(char)((which & 0x000000FF)) );
			break;
		}
	}
}

color_which
ColorWhichItem::GetAttribute(void)
{
	return attribute;
}

