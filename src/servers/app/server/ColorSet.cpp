//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
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

/*!
	\brief Overloaded assignment operator which does a massive number of assignments
	\param cs Color set to copy from
	\return The new values assigned to the color set
*/
ColorSet & ColorSet::operator=(const ColorSet &cs)
{
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
void SetDefaultGUIColors(ColorSet *set)
{
#ifdef DEBUG_COLORSET
printf("Initializing color settings to defaults\n");
#endif
	set->panel_background.SetColor(216,216,216);
	set->panel_text.SetColor(0,0,0);
	set->document_background.SetColor(255,255,255);
	set->document_text.SetColor(0,0,0);
	set->control_background.SetColor(245,245,245);
	set->control_text.SetColor(0,0,0);
	set->control_border.SetColor(0,0,0);
	set->control_highlight.SetColor(115,120,184);
	set->keyboard_navigation_base.SetColor(170,50,184);
	set->keyboard_navigation_pulse.SetColor(0,0,0);
	set->shine.SetColor(255,255,255);
	set->shadow.SetColor(0,0,0);
	set->menu_background.SetColor(216,216,216);
	set->menu_selected_background.SetColor(115,120,184);
	set->menu_text.SetColor(0,0,0);
	set->menu_selected_text.SetColor(255,255,255);
	set->menu_selected_border.SetColor(0,0,0);
	set->tooltip_background.SetColor(255,255,0);
	set->tooltip_text.SetColor(0,0,0);
	set->success.SetColor(0,255,0);
	set->failure.SetColor(255,0,0);
	set->window_tab.SetColor(255,203,0);

	// important, but not publically accessible GUI colors
	set->window_tab_text.SetColor(0,0,0);
	set->inactive_window_tab.SetColor(232,232,232);
	set->inactive_window_tab_text.SetColor(80,80,80);
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

	rgb_color *col;
	ssize_t size;
	
	if(msg.FindData("Panel Background",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		set->panel_background=*col;
	if(msg.FindData("Panel Text",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		set->panel_text=*col;
	if(msg.FindData("Document Background",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		set->document_background=*col;
	if(msg.FindData("Document Text",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		set->document_text=*col;
	if(msg.FindData("Control Background",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		set->control_background=*col;
	if(msg.FindData("Control Text",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		set->control_text=*col;
	if(msg.FindData("Control Highlight",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		set->control_highlight=*col;
	if(msg.FindData("Control Border",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		set->control_border=*col;
	if(msg.FindData("Tooltip Background",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		set->tooltip_background=*col;
	if(msg.FindData("Tooltip Text",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		set->tooltip_text=*col;
	if(msg.FindData("Menu Background",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		set->menu_background=*col;
	if(msg.FindData("Selected Menu Item Background",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		set->menu_selected_background=*col;
	if(msg.FindData("Keyboard Navigation Base",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		set->keyboard_navigation_base=*col;
	if(msg.FindData("Keyboard Navigation Pulse",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		set->keyboard_navigation_pulse=*col;
	if(msg.FindData("Menu Item Text",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		set->menu_text=*col;
	if(msg.FindData("Selected Menu Item Text",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		set->menu_selected_text=*col;
	if(msg.FindData("Selected Menu Item Border",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		set->menu_selected_border=*col;
	if(msg.FindData("Success",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		set->success=*col;
	if(msg.FindData("Failure",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		set->failure=*col;
	if(msg.FindData("Shine",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		set->shine=*col;
	if(msg.FindData("Shadow",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		set->shadow=*col;
	if(msg.FindData("Window Tab",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		set->window_tab=*col;
	if(msg.FindData("Window Tab Text",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		set->window_tab_text=*col;
	if(msg.FindData("Inactive Window Tab",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		set->inactive_window_tab=*col;
	if(msg.FindData("Inactive Window Tab Text",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		set->inactive_window_tab_text=*col;
	
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
	rgb_color col;
	
	col=set.panel_background.GetColor32();
	msg.AddData("Panel Background",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=set.panel_text.GetColor32();
	msg.AddData("Panel Text",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=set.document_background.GetColor32();
	msg.AddData("Document Background",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=set.document_text.GetColor32();
	msg.AddData("Document Text",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=set.control_background.GetColor32();
	msg.AddData("Control Background",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=set.control_text.GetColor32();
	msg.AddData("Control Text",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=set.control_highlight.GetColor32();
	msg.AddData("Control Highlight",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=set.control_border.GetColor32();
	msg.AddData("Control Border",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=set.tooltip_background.GetColor32();
	msg.AddData("Tooltip Background",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=set.tooltip_text.GetColor32();
	msg.AddData("Tooltip Text",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=set.menu_background.GetColor32();
	msg.AddData("Menu Background",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=set.menu_selected_background.GetColor32();
	msg.AddData("Selected Menu Item Background",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=set.keyboard_navigation_base.GetColor32();
	msg.AddData("Keyboard Navigation Base",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=set.keyboard_navigation_pulse.GetColor32();
	msg.AddData("Keyboard Navigation Pulse",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=set.menu_text.GetColor32();
	msg.AddData("Menu Item Text",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=set.menu_selected_text.GetColor32();
	msg.AddData("Selected Menu Item Text",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=set.menu_selected_border.GetColor32();
	msg.AddData("Selected Menu Item Border",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=set.success.GetColor32();
	msg.AddData("Success",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=set.failure.GetColor32();
	msg.AddData("Failure",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=set.shine.GetColor32();
	msg.AddData("Shine",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=set.shadow.GetColor32();
	msg.AddData("Shadow",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=set.window_tab.GetColor32();
	msg.AddData("Window Tab",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=set.window_tab_text.GetColor32();
	msg.AddData("Window Tab Text",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=set.inactive_window_tab.GetColor32();
	msg.AddData("Inactive Window Tab",(type_code)'RGBC',&col,sizeof(rgb_color));
	col=set.inactive_window_tab_text.GetColor32();
	msg.AddData("Inactive Window Tab Text",(type_code)'RGBC',&col,sizeof(rgb_color));

	msg.Flatten(&file);	
}

