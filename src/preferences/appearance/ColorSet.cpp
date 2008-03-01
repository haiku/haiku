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

static ColorSet sDefaults = ColorSet::DefaultColorSet(); 
static std::map<color_which, BString> sColorNames = ColorSet::DefaultColorNames();

bool 
match_rgb_color(rgb_color& color, uint8 red, uint8 green, uint8 blue)
{
	return color.red == red
		&& color.green == green
		&& color.blue == blue;
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
	fColors = cs.fColors;
}

//! Prints all color set elements to stdout
void
ColorSet::PrintToStream(void) const
{
	for (std::map<color_which, BString>::const_iterator it = sColorNames.begin(); it != sColorNames.end(); ++it) {
		printf("%s ", it->second.String());
		PrintMember(it->first);
		printf("\n");
	}
}

/*!
	\brief Assigns the default system colors to the passed ColorSet object
	\param set The ColorSet object to set to defaults
*/
ColorSet
ColorSet::DefaultColorSet(void)
{
#ifdef DEBUG_COLORSET
printf("Initializing color settings to defaults\n");
#endif
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
	\brief Checks if the ColorSet can be set to defaults.
*/
bool
ColorSet::IsDefaultable()
{
	return (*this == sDefaults);
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
	rgb_color color;

	for (std::map<color_which, BString>::const_iterator it = sColorNames.begin(); it != sColorNames.end(); ++it) {
		std::map<color_which, rgb_color>::const_iterator cit = fColors.find(it->first);
		if (cit != fColors.end())
			msg->AddData(it->second.String(), (type_code)'RGBC', &(cit->second), sizeof(rgb_color));
		else
			msg->AddData(it->second.String(), (type_code)'RGBC', &color, sizeof(rgb_color));
	}

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

	for (std::map<color_which, BString>::const_iterator it = sColorNames.begin(); 
			it != sColorNames.end(); ++it) {
		if (msg->FindData(it->second.String(),(type_code)'RGBC',(const void**)&col,&size)==B_OK)
			fColors[it->first] = *col;
	}
	
	return true;
}

std::map<color_which, BString>
ColorSet::DefaultColorNames(void)
{
	std::map<color_which, BString> names;

	names[B_PANEL_BACKGROUND_COLOR] = "Panel Background";
	names[B_PANEL_TEXT_COLOR] = "Panel Text";
	names[B_DOCUMENT_BACKGROUND_COLOR] = "Document Background";
	names[B_DOCUMENT_TEXT_COLOR] = "Document Text";
	names[B_CONTROL_BACKGROUND_COLOR] = "Control Background";
	names[B_CONTROL_TEXT_COLOR] = "Control Text";
	names[B_CONTROL_BORDER_COLOR] = "Control Border";
	names[B_CONTROL_HIGHLIGHT_COLOR] = "Control Highlight";
	names[B_NAVIGATION_BASE_COLOR] = "Navigation Base";
	names[B_NAVIGATION_PULSE_COLOR] = "Navigation Pulse";
	names[B_SHINE_COLOR] = "Shine";
	names[B_SHADOW_COLOR] = "Shadow";
	names[B_MENU_BACKGROUND_COLOR] = "Menu Background";
	names[B_MENU_SELECTED_BACKGROUND_COLOR] = "Selected Menu Item Background";
	names[B_MENU_ITEM_TEXT_COLOR] = "Menu Item Text";
	names[B_MENU_SELECTED_ITEM_TEXT_COLOR] = "Selected Menu Item Text";
	names[B_MENU_SELECTED_BORDER_COLOR] = "Selected Menu Item Border";
	names[B_TOOLTIP_BACKGROUND_COLOR] = "Tooltip Background";
	names[B_TOOLTIP_TEXT_COLOR] = "Tooltip Text";
	names[B_SUCCESS_COLOR] = "Success";
	names[B_FAILURE_COLOR] = "Failure";
	names[B_WINDOW_TAB_COLOR] = "Window Tab";
	names[B_WINDOW_TEXT_COLOR] = "Window Tab Text";
	names[B_WINDOW_INACTIVE_TAB_COLOR] = "Inactive Window Tab";
	names[B_WINDOW_INACTIVE_TEXT_COLOR] = "Inactive Window Tab Text";
	
	return names;
}
	
/*!
	\brief Assigns a value to a named color member
	\param string name of the color to receive the value
	\param value An rgb_color which is the new value of the member
*/
status_t
ColorSet::SetColor(color_which which, rgb_color value)
{
	fColors[which] = value;
	return B_OK;
}

/*!
	\brief Obtains the set's color member based on a specified string
	\param string name of the color member to obtain
	\return An RGBColor pointer or NULL if not found
*/

rgb_color
ColorSet::StringToColor(const char *string) const
{
	rgb_color color;
	color.set_to(0,0,0);

	if (!string)
		return color;

	color_which which = StringToWhich(string);
	if (which != -1) {
		std::map<color_which, rgb_color>::const_iterator it = fColors.find(which);
		if (it != fColors.end())
			return it->second;
	}
	
	return color;
}

color_which
ColorSet::StringToWhich(const char *string) const
{
	if(!string)
		return (color_which)-1;

	for (std::map<color_which, BString>::const_iterator it = sColorNames.begin(); it != sColorNames.end(); ++it)
		if (it->second == string)
			return it->first;

	return (color_which)-1;
	
}

rgb_color
ColorSet::AttributeToColor(int32 which)
{
	return fColors[(color_which)which];
}

void
ColorSet::PrintMember(color_which which) const
{
	std::map<color_which, rgb_color>::const_iterator it = fColors.find(which);
	if (it != fColors.end())
		printf("rgb_color(%d, %d, %d, %d)", it->second.red,it->second.green,it->second.blue,
			it->second.alpha);
	else
		printf("color (%d) not found\n", which);
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

