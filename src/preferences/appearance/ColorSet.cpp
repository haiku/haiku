/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include <stdio.h>
#include <InterfaceDefs.h>
#include <Message.h>
#include <File.h>
#include <Entry.h>
#include <Directory.h>
#include <String.h>
#include "ColorSet.h"

static void
set_rgb_color(rgb_color& color, uint8 red, uint8 green, uint8 blue)
{
	color.red = red;
	color.green = green;
	color.blue = blue;
	color.alpha = 255;
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
	SetColors(cs);
}

/*!
	\brief Overloaded assignment operator which does a massive number of assignments
	\param cs Color set to copy from
	\return The new values assigned to the color set
*/
ColorSet &
ColorSet::operator=(const ColorSet &cs)
{
	SetColors(cs);
	return *this;
}

/*!
	\brief Copy function which handles assignments, 
		and, yes, *IT EVEN MAKES french fries!!*
	\param cs Color set to copy from
*/
void
ColorSet::SetColors(const ColorSet &cs)
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
	menu_selected_border=cs.menu_selected_border;

	keyboard_navigation_base=cs.keyboard_navigation_base;
	keyboard_navigation_pulse=cs.keyboard_navigation_pulse;

	success=cs.success;
	failure=cs.failure;
	shine=cs.shine;
	shadow=cs.shadow;

	window_tab=cs.window_tab;
	window_tab_text=cs.window_tab_text;
	inactive_window_tab=cs.inactive_window_tab;
	inactive_window_tab_text=cs.inactive_window_tab_text;
}

//! Prints all color set elements to stdout
void
ColorSet::PrintToStream(void) const
{
	printf("panel_background "); PrintMember(panel_background);
	printf("panel_text "); PrintMember(panel_text);

	printf("document_background "); PrintMember(document_background);
	printf("document_text "); PrintMember(document_text);

	printf("control_background "); PrintMember(control_background);
	printf("control_text "); PrintMember(control_text);
	printf("control_highlight "); PrintMember(control_highlight);
	printf("control_border "); PrintMember(control_border);

	printf("tooltip_background "); PrintMember(tooltip_background);
	printf("tooltip_text "); PrintMember(tooltip_text);

	printf("menu_background "); PrintMember(menu_background);
	printf("menu_selected_background "); PrintMember(menu_selected_background);
	printf("menu_text "); PrintMember(menu_text);
	printf("menu_selected_text "); PrintMember(menu_selected_text);
	printf("menu_selected_border "); PrintMember(menu_selected_border);

	printf("keyboard_navigation_base "); PrintMember(keyboard_navigation_base);
	printf("keyboard_navigation_pulse "); PrintMember(keyboard_navigation_pulse);

	printf("success "); PrintMember(success);
	printf("failure "); PrintMember(failure);
	printf("shine "); PrintMember(shine);
	printf("shadow "); PrintMember(shadow);

	printf("window_tab "); PrintMember(window_tab);
	printf("window_tab_text "); PrintMember(window_tab_text);

	printf("inactive_window_tab "); PrintMember(inactive_window_tab);
	printf("inactive_window_tab_text "); PrintMember(inactive_window_tab_text);
}

/*!
	\brief Assigns the default system colors to the passed ColorSet object
	\param set The ColorSet object to set to defaults
*/
void
ColorSet::SetToDefaults(void)
{
#ifdef DEBUG_COLORSET
printf("Initializing color settings to defaults\n");
#endif
	set_rgb_color(panel_background, 216, 216, 216);
	set_rgb_color(panel_text, 0, 0, 0);
	set_rgb_color(document_background, 255, 255, 255);
	set_rgb_color(document_text, 0, 0, 0);
	set_rgb_color(control_background, 245, 245, 245);
	set_rgb_color(control_text, 0, 0, 0);
	set_rgb_color(control_border, 0, 0, 0);
	set_rgb_color(control_highlight, 102, 152, 203);
	set_rgb_color(keyboard_navigation_base, 0, 0, 229);
	set_rgb_color(keyboard_navigation_pulse, 0, 0, 0);
	set_rgb_color(shine, 255, 255, 255);
	set_rgb_color(shadow, 0, 0, 0);
	set_rgb_color(menu_background, 216, 216, 216);
	set_rgb_color(menu_selected_background, 115, 120, 184);
	set_rgb_color(menu_text, 0, 0, 0);
	set_rgb_color(menu_selected_text, 255, 255, 255);
	set_rgb_color(menu_selected_border, 0, 0, 0);
	set_rgb_color(tooltip_background, 255, 255, 0);
	set_rgb_color(tooltip_text, 0, 0, 0);
	set_rgb_color(success, 0, 255, 0);
	set_rgb_color(failure, 255, 0, 0);
	set_rgb_color(window_tab, 255, 203, 0);
	set_rgb_color(window_tab_text, 0, 0, 0);
	set_rgb_color(inactive_window_tab, 232, 232, 232);
	set_rgb_color(inactive_window_tab_text, 80, 80, 80);
}

/*!
	\brief Attaches the color set's members as data to the given BMessage
	\param msg The message to receive the attributes
*/
bool
ColorSet::ConvertToMessage(BMessage *msg) const
{
	if(!msg)
		return false;

	msg->MakeEmpty();
	
	msg->AddData("Panel Background",(type_code)'RGBC',&panel_background,
				sizeof(rgb_color) );
	msg->AddData("Panel Text",(type_code)'RGBC',&panel_text,sizeof(rgb_color));
	msg->AddData("Document Background",(type_code)'RGBC',&document_background,
				sizeof(rgb_color));
	msg->AddData("Document Text",(type_code)'RGBC',&document_text,sizeof(rgb_color));
	msg->AddData("Control Background",(type_code)'RGBC',&control_background,
				sizeof(rgb_color));
	msg->AddData("Control Text",(type_code)'RGBC',&control_text,sizeof(rgb_color));
	msg->AddData("Control Highlight",(type_code)'RGBC',&control_highlight,
				sizeof(rgb_color));
	msg->AddData("Control Border",(type_code)'RGBC',&control_border,sizeof(rgb_color));
	msg->AddData("Tooltip Background",(type_code)'RGBC',&tooltip_background,
				sizeof(rgb_color));
	msg->AddData("Tooltip Text",(type_code)'RGBC',&tooltip_text,sizeof(rgb_color));
	msg->AddData("Menu Background",(type_code)'RGBC',&menu_background,
				sizeof(rgb_color));
	msg->AddData("Selected Menu Item Background",(type_code)'RGBC',&menu_selected_background,
				sizeof(rgb_color));
	msg->AddData("Keyboard Navigation Base",(type_code)'RGBC',&keyboard_navigation_base,
				sizeof(rgb_color));
	msg->AddData("Keyboard Navigation Pulse",(type_code)'RGBC',
				&keyboard_navigation_pulse,sizeof(rgb_color));
	msg->AddData("Menu Item Text",(type_code)'RGBC',&menu_text,sizeof(rgb_color));
	msg->AddData("Selected Menu Item Text",(type_code)'RGBC',&menu_selected_text,
				sizeof(rgb_color));
	msg->AddData("Selected Menu Item Border",(type_code)'RGBC',&menu_selected_border,
				sizeof(rgb_color));
	msg->AddData("Success",(type_code)'RGBC',&success,sizeof(rgb_color));
	msg->AddData("Failure",(type_code)'RGBC',&failure,sizeof(rgb_color));
	msg->AddData("Shine",(type_code)'RGBC',&shine,sizeof(rgb_color));
	msg->AddData("Shadow",(type_code)'RGBC',&shadow,sizeof(rgb_color));
	msg->AddData("Window Tab",(type_code)'RGBC',&window_tab,sizeof(rgb_color));
	msg->AddData("Window Tab Text",(type_code)'RGBC',&window_tab_text,
				sizeof(rgb_color));
	msg->AddData("Inactive Window Tab",(type_code)'RGBC',&inactive_window_tab,
				sizeof(rgb_color));
	msg->AddData("Inactive Window Tab Text",(type_code)'RGBC',
				&inactive_window_tab_text,sizeof(rgb_color));
	return true;
}

/*!
	\brief Assigns values to the color set's members based on values in the BMessage
	\param msg The message containing the data for the color set's colors
*/

bool
ColorSet::ConvertFromMessage(const BMessage *msg)
{
	if(!msg)
		return false;

	rgb_color *col;
	ssize_t size;
	BString str;

	if(msg->FindData("Panel Background",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		panel_background=*col;
	if(msg->FindData("Panel Text",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		panel_text=*col;
	if(msg->FindData("Document Background",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		document_background=*col;
	if(msg->FindData("Document Text",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		document_text=*col;
	if(msg->FindData("Control Background",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		control_background=*col;
	if(msg->FindData("Control Text",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		control_text=*col;
	if(msg->FindData("Control Highlight",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		control_highlight=*col;
	if(msg->FindData("Control Border",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		control_border=*col;
	if(msg->FindData("Tooltip Background",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		tooltip_background=*col;
	if(msg->FindData("Tooltip Text",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		tooltip_text=*col;
	if(msg->FindData("Menu Background",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		menu_background=*col;
	if(msg->FindData("Selected Menu Item Background",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		menu_selected_background=*col;
	if(msg->FindData("Navigation Base",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		keyboard_navigation_base=*col;
	if(msg->FindData("Navigation Pulse",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		keyboard_navigation_pulse=*col;
	if(msg->FindData("Menu Item Text",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		menu_text=*col;
	if(msg->FindData("Selected Menu Item Text",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		menu_selected_text=*col;
	if(msg->FindData("Selected Menu Item Border",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		menu_selected_border=*col;
	if(msg->FindData("Success",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		success=*col;
	if(msg->FindData("Failure",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		failure=*col;
	if(msg->FindData("Shine",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		shine=*col;
	if(msg->FindData("Shadow",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		shadow=*col;
	if(msg->FindData("Window Tab",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		window_tab=*col;
	if(msg->FindData("Window Tab Text",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		window_tab_text=*col;
	if(msg->FindData("Inactive Window Tab",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		inactive_window_tab=*col;
	if(msg->FindData("Inactive Window Tab Text",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		inactive_window_tab_text=*col;
	return true;
}

	
/*!
	\brief Assigns a value to a named color member
	\param string name of the color to receive the value
	\param value An rgb_color which is the new value of the member
*/
status_t
ColorSet::SetColor(const char *string, rgb_color value)
{
	if(!string)
		return B_BAD_VALUE;
	
	rgb_color *col=StringToMember(string);
	if(!col)
		return B_NAME_NOT_FOUND;
	*col=value;
	return B_OK;
}

/*!
	\brief Obtains a color based on a specified string
	\param string name of the color to obtain
	\return The set's color or (0,0,0,0) if not found
*/
rgb_color
ColorSet::StringToColor(const char *string)
{
	rgb_color *col=StringToMember(string);
	if(!col) {
		rgb_color c;
		return c;
	}
		
	return *col;
}

/*!
	\brief Obtains the set's color member based on a specified string
	\param string name of the color member to obtain
	\return An RGBColor pointer or NULL if not found
*/
rgb_color *
ColorSet::StringToMember(const char *string)
{
	if(!string)
		return NULL;

	if(strcmp(string,"Panel Background")==0)
		return &panel_background;
	if(strcmp(string,"Panel Text")==0)
		return &panel_text;
	if(strcmp(string,"Document Background")==0)
		return &document_background;
	if(strcmp(string,"Document Text")==0)
		return &document_text;
	if(strcmp(string,"Control Background")==0)
		return &control_background;
	if(strcmp(string,"Control Text")==0)
		return &control_text;
	if(strcmp(string,"Control Highlight")==0)
		return &control_highlight;
	if(strcmp(string,"Control Border")==0)
		return &control_border;
	if(strcmp(string,"Tooltip Background")==0)
		return &tooltip_background;
	if(strcmp(string,"Tooltip Text")==0)
		return &tooltip_text;
	if(strcmp(string,"Menu Background")==0)
		return &menu_background;
	if(strcmp(string,"Selected Menu Item Background")==0)
		return &menu_selected_background;
	if(strcmp(string,"Navigation Base")==0)
		return &keyboard_navigation_base;
	if(strcmp(string,"Navigation Pulse")==0)
		return &keyboard_navigation_pulse;
	if(strcmp(string,"Menu Item Text")==0)
		return &menu_text;
	if(strcmp(string,"Selected Menu Item Text")==0)
		return &menu_selected_text;
	if(strcmp(string,"Selected Menu Item Border")==0)
		return &menu_selected_border;
	if(strcmp(string,"Success")==0)
		return &success;
	if(strcmp(string,"Failure")==0)
		return &failure;
	if(strcmp(string,"Shine")==0)
		return &shine;
	if(strcmp(string,"Shadow")==0)
		return &shadow;
	if(strcmp(string,"Window Tab")==0)
		return &window_tab;
	if(strcmp(string,"Window Tab Text")==0)
		return &window_tab_text;
	if(strcmp(string,"Inactive Window Tab")==0)
		return &inactive_window_tab;
	if(strcmp(string,"Inactive Window Tab Text")==0)
		return &inactive_window_tab_text;

	return NULL;
	
}

rgb_color
ColorSet::AttributeToColor(int32 which)
{
	switch(which) {
		case B_PANEL_BACKGROUND_COLOR: {
			return panel_background;
			break;
		}
		
#ifndef HAIKU_TARGET_PLATFORM_BEOS
		case B_PANEL_TEXT_COLOR: {
			return panel_text;
			break;
		}
		case B_DOCUMENT_BACKGROUND_COLOR: {
			return document_background;
			break;
		}
		case B_DOCUMENT_TEXT_COLOR: {
			return document_text;
			break;
		}
		case B_CONTROL_BACKGROUND_COLOR: {
			return control_background;
			break;
		}
		case B_CONTROL_TEXT_COLOR: {
			return control_text;
			break;
		}
		case B_CONTROL_BORDER_COLOR: {
			return control_border;
			break;
		}
		case B_CONTROL_HIGHLIGHT_COLOR: {
			return control_highlight;
			break;
		}
		case B_NAVIGATION_BASE_COLOR: {
			return keyboard_navigation_base;
			break;
		}
		case B_NAVIGATION_PULSE_COLOR: {
			return keyboard_navigation_pulse;
			break;
		}
		case B_SHINE_COLOR: {
			return shine;
			break;
		}
		case B_SHADOW_COLOR: {
			return shadow;
			break;
		}
		case B_MENU_SELECTED_BACKGROUND_COLOR: {
			return menu_selected_background;
			break;
		}
		case B_MENU_SELECTED_BORDER_COLOR: {
			return menu_selected_border;
			break;
		}
		case B_TOOLTIP_BACKGROUND_COLOR: {
			return tooltip_background;
			break;
		}
		case B_TOOLTIP_TEXT_COLOR: {
			return tooltip_text;
			break;
		}
		case B_SUCCESS_COLOR: {
			return success;
			break;
		}
		case B_FAILURE_COLOR: {
			return failure;
			break;
		}
#endif
		case B_MENU_BACKGROUND_COLOR: {
			return menu_background;
			break;
		}
		case B_MENU_ITEM_TEXT_COLOR: {
			return menu_text;
			break;
		}
		case B_MENU_SELECTED_ITEM_TEXT_COLOR: {
			return menu_selected_text;
			break;
		}
		case B_WINDOW_TAB_COLOR: {
			return window_tab;
			break;
		}
		
		// DANGER! DANGER, WILL ROBINSON!!
		// These are magic numbers to work around compatibility difficulties while still keeping
		// functionality. This __will__ break following R1
		case 22: {
			return window_tab_text;
			break;
		}
		case 23: {
			return inactive_window_tab;
			break;
		}
		case 24: {
			return inactive_window_tab_text;
			break;
		}
		
		default: {
			rgb_color c;
			return c;
			break;
		}
	}
}

void
ColorSet::PrintMember(const rgb_color &color) const
{
	printf("rgb_color(%d, %d, %d, %d)",color.red,color.green,color.blue,
			color.alpha);
}


/*!
	\brief Loads the saved system colors into a ColorSet
	\param set the set to receive the system colors
	\return B_OK if successful. See BFile for other error codes
*/
status_t
ColorSet::LoadColorSet(const char *path, ColorSet *set)
{
	BFile file(path,B_READ_ONLY);
	if (file.InitCheck()!=B_OK)
		return file.InitCheck();

	BMessage msg;
	status_t status = msg.Unflatten(&file);
	if (status != B_OK)
		return status;

	set->ConvertFromMessage(&msg);
	return B_OK;
}


/*!
	\brief Saves the saved system colors into a flattened BMessage
	\param set ColorSet containing the colors to save
	\return B_OK if successful. See BFile for other error codes
*/
status_t
ColorSet::SaveColorSet(const char *path, const ColorSet &set)
{
	BFile file(path, B_READ_WRITE | B_ERASE_FILE | B_CREATE_FILE);
	status_t status = file.InitCheck();
	if (status != B_OK)
		return status;

	BMessage msg;
	set.ConvertToMessage(&msg);

	return msg.Flatten(&file);	
}
