//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku, Inc.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		ColorSet.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Class for encapsulating GUI system colors
//					
//  
//------------------------------------------------------------------------------
#include <stdio.h>
#include <InterfaceDefs.h>
#include <Message.h>
#include <File.h>
#include <Entry.h>
#include <Directory.h>
#include <String.h>
#include "ColorSet.h"
#include "ServerConfig.h"

//! Constructor which does nothing
ColorSet::ColorSet(void)
{
}

/*!
	\brief Copy constructor which does a massive number of assignments
	\param cs Color set to copy from
*/
ColorSet::ColorSet(const ColorSet &cs)
{
	name=cs.name;
	SetColors(cs);
}

/*!
	\brief Overloaded assignment operator which does a massive number of assignments
	\param cs Color set to copy from
	\return The new values assigned to the color set
*/
ColorSet & ColorSet::operator=(const ColorSet &cs)
{
	name=cs.name;
	SetColors(cs);
	return *this;
}

/*!
	\brief Copy function which handles assignments, 
		and, yes, *IT EVEN MAKES french fries!!*
	\param cs Color set to copy from
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
void ColorSet::PrintToStream(void) const
{
	printf("Name: %s\n",name.String());
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
	printf("menu_selected_border "); menu_selected_border.PrintToStream();

	printf("keyboard_navigation_base "); keyboard_navigation_base.PrintToStream();
	printf("keyboard_navigation_pulse "); keyboard_navigation_pulse.PrintToStream();

	printf("success "); success.PrintToStream();
	printf("failure "); failure.PrintToStream();
	printf("shine "); shine.PrintToStream();
	printf("shadow "); shadow.PrintToStream();

	printf("window_tab "); window_tab.PrintToStream();
	printf("window_tab_text "); window_tab_text.PrintToStream();

	printf("inactive_window_tab "); inactive_window_tab.PrintToStream();
	printf("inactive_window_tab_text "); inactive_window_tab_text.PrintToStream();
}

/*!
	\brief Assigns the default system colors to the passed ColorSet object
	\param set The ColorSet object to set to defaults
*/
void ColorSet::SetToDefaults(void)
{
#ifdef DEBUG_COLORSET
printf("Initializing color settings to defaults\n");
#endif
	panel_background.SetColor(216,216,216);
	panel_text.SetColor(0,0,0);
	document_background.SetColor(255,255,255);
	document_text.SetColor(0,0,0);
	control_background.SetColor(245,245,245);
	control_text.SetColor(0,0,0);
	control_border.SetColor(0,0,0);
	control_highlight.SetColor(115,120,184);
	keyboard_navigation_base.SetColor(170,50,184);
	keyboard_navigation_pulse.SetColor(0,0,0);
	shine.SetColor(255,255,255);
	shadow.SetColor(0,0,0);
	menu_background.SetColor(216,216,216);
	menu_selected_background.SetColor(115,120,184);
	menu_text.SetColor(0,0,0);
	menu_selected_text.SetColor(255,255,255);
	menu_selected_border.SetColor(0,0,0);
	tooltip_background.SetColor(255,255,0);
	tooltip_text.SetColor(0,0,0);
	success.SetColor(0,255,0);
	failure.SetColor(255,0,0);
	window_tab.SetColor(255,203,0);

	// important, but not publically accessible GUI colors
	window_tab_text.SetColor(0,0,0);
	inactive_window_tab.SetColor(216,216,216);
	inactive_window_tab_text.SetColor(80,80,80);
}

/*!
	\brief Attaches the color set's members as data to the given BMessage
	\param msg The message to receive the attributes
*/
bool ColorSet::ConvertToMessage(BMessage *msg) const
{
	if(!msg)
		return false;

	msg->MakeEmpty();
	
	rgb_color col;
	
	msg->AddString("Name",name);
	col=panel_background.GetColor32();
	msg->AddData("Panel Background",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=panel_text.GetColor32();
	msg->AddData("Panel Text",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=document_background.GetColor32();
	msg->AddData("Document Background",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=document_text.GetColor32();
	msg->AddData("Document Text",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=control_background.GetColor32();
	msg->AddData("Control Background",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=control_text.GetColor32();
	msg->AddData("Control Text",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=control_highlight.GetColor32();
	msg->AddData("Control Highlight",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=control_border.GetColor32();
	msg->AddData("Control Border",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=tooltip_background.GetColor32();
	msg->AddData("Tooltip Background",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=tooltip_text.GetColor32();
	msg->AddData("Tooltip Text",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=menu_background.GetColor32();
	msg->AddData("Menu Background",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=menu_selected_background.GetColor32();
	msg->AddData("Selected Menu Item Background",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=keyboard_navigation_base.GetColor32();
	msg->AddData("Keyboard Navigation Base",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=keyboard_navigation_pulse.GetColor32();
	msg->AddData("Keyboard Navigation Pulse",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=menu_text.GetColor32();
	msg->AddData("Menu Item Text",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=menu_selected_text.GetColor32();
	msg->AddData("Selected Menu Item Text",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=menu_selected_border.GetColor32();
	msg->AddData("Selected Menu Item Border",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=success.GetColor32();
	msg->AddData("Success",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=failure.GetColor32();
	msg->AddData("Failure",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=shine.GetColor32();
	msg->AddData("Shine",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=shadow.GetColor32();
	msg->AddData("Shadow",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=window_tab.GetColor32();
	msg->AddData("Window Tab",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=window_tab_text.GetColor32();
	msg->AddData("Window Tab Text",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=inactive_window_tab.GetColor32();
	msg->AddData("Inactive Window Tab",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=inactive_window_tab_text.GetColor32();
	msg->AddData("Inactive Window Tab Text",(type_code)'RGBC',&col,sizeof(rgb_color));
	return true;
}

/*!
	\brief Assigns values to the color set's members based on values in the BMessage
	\param msg The message containing the data for the color set's colors
*/

bool ColorSet::ConvertFromMessage(const BMessage *msg)
{
	if(!msg)
		return false;

	rgb_color *col;
	ssize_t size;
	BString str;

	if(msg->FindString("Name",&str)==B_OK)
		name=str;
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
status_t ColorSet::SetColor(const char *string, rgb_color value)
{
	if(!string)
		return B_BAD_VALUE;
	
	RGBColor *col=StringToMember(string);
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
RGBColor ColorSet::StringToColor(const char *string)
{
	RGBColor *col=StringToMember(string);
	if(!col)
		return RGBColor(0,0,0,0);
		
	return *col;
}

/*!
	\brief Obtains the set's color member based on a specified string
	\param string name of the color member to obtain
	\return An RGBColor pointer or NULL if not found
*/
RGBColor *ColorSet::StringToMember(const char *string)
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

RGBColor ColorSet::AttributeToColor(int32 which)
{
	switch(which)
	{
		case B_PANEL_BACKGROUND_COLOR:
		{
			return panel_background;
			break;
		}
		case B_PANEL_TEXT_COLOR:
		{
			return panel_text;
			break;
		}
		case B_DOCUMENT_BACKGROUND_COLOR:
		{
			return document_background;
			break;
		}
		case B_DOCUMENT_TEXT_COLOR:
		{
			return document_text;
			break;
		}
		case B_CONTROL_BACKGROUND_COLOR:
		{
			return control_background;
			break;
		}
		case B_CONTROL_TEXT_COLOR:
		{
			return control_text;
			break;
		}
		case B_CONTROL_BORDER_COLOR:
		{
			return control_highlight;
			break;
		}
		case B_CONTROL_HIGHLIGHT_COLOR:
		{
			return control_border;
			break;
		}
		case B_NAVIGATION_BASE_COLOR:
		{
			return keyboard_navigation_base;
			break;
		}
		case B_NAVIGATION_PULSE_COLOR:
		{
			return keyboard_navigation_pulse;
			break;
		}
		case B_SHINE_COLOR:
		{
			return shine;
			break;
		}
		case B_SHADOW_COLOR:
		{
			return shadow;
			break;
		}
		case B_MENU_BACKGROUND_COLOR:
		{
			return menu_background;
			break;
		}
		case B_MENU_SELECTED_BACKGROUND_COLOR:
		{
			return menu_selected_background;
			break;
		}
		case B_MENU_ITEM_TEXT_COLOR:
		{
			return menu_text;
			break;
		}
		case B_MENU_SELECTED_ITEM_TEXT_COLOR:
		{
			return menu_selected_text;
			break;
		}
		case B_MENU_SELECTED_BORDER_COLOR:
		{
			return menu_selected_border;
			break;
		}
		case B_TOOLTIP_BACKGROUND_COLOR:
		{
			return tooltip_background;
			break;
		}
		case B_TOOLTIP_TEXT_COLOR:
		{
			return tooltip_text;
			break;
		}
		case B_SUCCESS_COLOR:
		{
			return success;
			break;
		}
		case B_FAILURE_COLOR:
		{
			return failure;
			break;
		}
		case B_WINDOW_TAB_COLOR:
		{
			return window_tab;
			break;
		}
		
		// DANGER! DANGER, WILL ROBINSON!!
		// These are magic numbers to work around compatibility difficulties while still keeping
		// functionality. This __will__ break following R1
		case 22:
		{
			return window_tab_text;
			break;
		}
		case 23:
		{
			return inactive_window_tab;
			break;
		}
		case 24:
		{
			return inactive_window_tab_text;
			break;
		}
		
		default:
		{
			return RGBColor(0,0,0,0);
			break;
		}
	}
}

/*!
	\brief Loads the saved system colors into a ColorSet
	\param set the set to receive the system colors
	\return true if successful, false if not
	
	Note that this function automatically looks for the file named in 
	COLOR_SETTINGS_NAME in the folder SERVER_SETTINGS_DIR as defined in
	ServerConfig.h
*/
bool LoadGUIColors(ColorSet *set)
{
	BString path(SERVER_SETTINGS_DIR);
	path+=COLOR_SETTINGS_NAME;
	
	BFile file(path.String(),B_READ_ONLY);
	if(file.InitCheck()!=B_OK)
		return false;
	
	BMessage msg;
	if(msg.Unflatten(&file)!=B_OK)
		return false;

	set->ConvertFromMessage(&msg);
	
	return true;
}

/*!
	\brief Saves the saved system colors into a flattened BMessage
	\param set ColorSet containing the colors to save
	
	Note that this function automatically looks for the file named in 
	COLOR_SETTINGS_NAME in the folder SERVER_SETTINGS_DIR as defined in
	ServerConfig.h. If SERVER_SETTINGS_DIR does not exist, it is created, and the same for 
	the color settings file.
*/
void SaveGUIColors(const ColorSet &set)
{
	BEntry *entry=new BEntry(SERVER_SETTINGS_DIR);
	if(!entry->Exists())
		create_directory(SERVER_SETTINGS_DIR,0777);
	delete entry;
	
	BString path(SERVER_SETTINGS_DIR);
	path+=COLOR_SETTINGS_NAME;
	BFile file(path.String(), B_READ_WRITE | B_ERASE_FILE | B_CREATE_FILE);

	BMessage msg;
	
	set.ConvertToMessage(&msg);

	msg.Flatten(&file);	
}

