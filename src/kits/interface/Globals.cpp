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
//	File Name:		Globals.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Global functions and variables for the Interface Kit
//
//------------------------------------------------------------------------------

#include <AppServerLink.h>
#include <GraphicsDefs.h>
#include <InterfaceDefs.h>
#include <Menu.h>
#include <ServerProtocol.h>
#include <Screen.h>
#include <PortMessage.h>
#include <Roster.h>
#include <TextView.h>

#include <WidthBuffer.h>

// Private definitions not placed in public headers
extern "C" void _init_global_fonts();
extern "C" status_t _fini_interface_kit_();

using namespace BPrivate;


// InterfaceDefs.h
_IMPEXP_BE const color_map *
system_colors()
{
	return BScreen(B_MAIN_SCREEN_ID).ColorMap();
}


_IMPEXP_BE status_t
set_screen_space(int32 index, uint32 res, bool stick)
{
	BAppServerLink link;
	link.SetOpCode(AS_SET_SCREEN_MODE);
	link.Attach<int32>(index);
	link.Attach<int32>((int32)res);
	link.Attach<bool>(stick);
	link.Flush();

	//TODO: Read back the status from the app_server's reply
	return B_OK;
}


_IMPEXP_BE status_t
set_scroll_bar_info(scroll_bar_info *info)
{
	if(!info)
		return B_ERROR;
	
	BAppServerLink link;
	PortMessage msg;
	
	link.SetOpCode(AS_SET_SCROLLBAR_INFO);
	link.Attach<scroll_bar_info>(*info);
	link.FlushWithReply(&msg);
	return B_OK;
}


_IMPEXP_BE status_t
get_mouse_type(int32 *type)
{
	// Contacts the input_server via _control_input_server_(BMessage *, BMessage *)
	// TODO: Implement
}


_IMPEXP_BE status_t
set_mouse_type(int32 type)
{
	// Contacts the input_server via _control_input_server_(BMessage *, BMessage *)
	// TODO: Implement
}


_IMPEXP_BE status_t
get_mouse_map(mouse_map *map)
{
	// Contacts the input_server via _control_input_server_(BMessage *, BMessage *)
	// TODO: Implement
}


_IMPEXP_BE status_t
set_mouse_map(mouse_map *map)
{
	// Contacts the input_server via _control_input_server_(BMessage *, BMessage *)
	// TODO: Implement
}

_IMPEXP_BE status_t
get_click_speed(bigtime_t *speed)
{
	// Contacts the input_server via _control_input_server_(BMessage *, BMessage *)
	// TODO: Implement
}


_IMPEXP_BE status_t
set_click_speed(bigtime_t speed)
{
	// Contacts the input_server via _control_input_server_(BMessage *, BMessage *)
	// TODO: Implement
}


_IMPEXP_BE status_t
get_mouse_speed(int32 *speed)
{
	// Contacts the input_server via _control_input_server_(BMessage *, BMessage *)
	// TODO: Implement
}


_IMPEXP_BE status_t
set_mouse_speed(int32 speed)
{
	// Contacts the input_server via _control_input_server_(BMessage *, BMessage *)
	// TODO: Implement
}


_IMPEXP_BE status_t
get_mouse_acceleration(int32 *speed)
{
	// Contacts the input_server via _control_input_server_(BMessage *, BMessage *)
	// TODO: Implement
}


_IMPEXP_BE status_t
set_mouse_acceleration(int32 speed)
{
	// Contacts the input_server via _control_input_server_(BMessage *, BMessage *)
	// TODO: Implement
}


_IMPEXP_BE status_t
get_key_repeat_rate(int32 *rate)
{
	// Contacts the input_server via _control_input_server_(BMessage *, BMessage *)
	// TODO: Implement
}


_IMPEXP_BE status_t
set_key_repeat_rate(int32 rate)
{
	// Contacts the input_server via _control_input_server_(BMessage *, BMessage *)
	// TODO: Implement
}


_IMPEXP_BE status_t
get_key_repeat_delay(bigtime_t *delay)
{
	// Contacts the input_server via _control_input_server_(BMessage *, BMessage *)
	// TODO: Implement
}


_IMPEXP_BE status_t
set_key_repeat_delay(bigtime_t  delay)
{
	// Contacts the input_server via _control_input_server_(BMessage *, BMessage *)
	// TODO: Implement
}


_IMPEXP_BE uint32
modifiers()
{
	// Contacts the input_server via _control_input_server_(BMessage *, BMessage *)
	// TODO: Implement
}


_IMPEXP_BE status_t
get_key_info(key_info *info)
{
	// Contacts the input_server via _control_input_server_(BMessage *, BMessage *)
	// TODO: Implement
}


_IMPEXP_BE void
get_key_map(key_map **map, char **key_buffer)
{
	// Contacts the input_server via _control_input_server_(BMessage *, BMessage *)
	// TODO: Implement
}


_IMPEXP_BE status_t
get_keyboard_id(uint16 *id)
{
	// Contacts the input_server via _control_input_server_(BMessage *, BMessage *)
	// TODO: Implement
}


_IMPEXP_BE void
set_modifier_key(uint32 modifier, uint32 key)
{
	// Contacts the input_server via _control_input_server_(BMessage *, BMessage *)
	// TODO: Implement
}


_IMPEXP_BE void
set_keyboard_locks(uint32 modifiers)
{
	// Contacts the input_server via _control_input_server_(BMessage *, BMessage *)
	// TODO: Implement
}


_IMPEXP_BE rgb_color
keyboard_navigation_color()
{
	// Queries the app_server
	// TODO: Implement
	return ui_color(B_KEYBOARD_NAVIGATION_COLOR);
}


_IMPEXP_BE int32
count_workspaces()
{
	int32 count;
	
	BAppServerLink link;
	PortMessage msg;
	link.SetOpCode(AS_COUNT_WORKSPACES);
	link.FlushWithReply(&msg);
	msg.Read<int32>(&count);
	return count;
}


_IMPEXP_BE void
set_workspace_count(int32 count)
{
	BAppServerLink link;
	link.SetOpCode(AS_SET_WORKSPACE_COUNT);
	link.Attach<int32>(count);
	link.Flush();
}


_IMPEXP_BE int32
current_workspace()
{
	int32 index;
	
	BAppServerLink link;
	PortMessage msg;
	link.SetOpCode(AS_CURRENT_WORKSPACE);
	link.FlushWithReply(&msg);
	msg.Read<int32>(&index);
	
	return index;
}


_IMPEXP_BE void
activate_workspace(int32 workspace)
{
	BAppServerLink link;
	link.SetOpCode(AS_ACTIVATE_WORKSPACE);
	link.Attach<int32>(workspace);
	link.Flush();
}


_IMPEXP_BE bigtime_t
idle_time()
{
	bigtime_t idletime;
	
	BAppServerLink link;
	PortMessage msg;
	link.SetOpCode(AS_IDLE_TIME);
	link.FlushWithReply(&msg);
	msg.Read<int64>(&idletime);
	
	return idletime;
}


_IMPEXP_BE void
run_select_printer_panel()
{
	// Launches the Printer prefs app via the Roster
	be_roster->Launch("application/x-vnd.Be-PRNT");
}


_IMPEXP_BE void
run_add_printer_panel()
{
	// Launches the Printer prefs app via the Roster and asks it to 
	// add a printer
	// TODO: Implement
}


_IMPEXP_BE void
run_be_about()
{
	// Unsure about how to implement this.
	// TODO: Implement
}


_IMPEXP_BE void
set_focus_follows_mouse(bool follow)
{
	BAppServerLink link;
	link.SetOpCode(AS_SET_FOCUS_FOLLOWS_MOUSE);
	link.Attach<bool>(follow);
	link.Flush();
}


_IMPEXP_BE bool
focus_follows_mouse()
{
	bool ffm;
	
	BAppServerLink link;
	PortMessage msg;
	link.SetOpCode(AS_FOCUS_FOLLOWS_MOUSE);
	link.FlushWithReply(&msg);
	msg.Read<bool>(&ffm);
	return ffm;
}


_IMPEXP_BE void
set_mouse_mode(mode_mouse mode)
{
	BAppServerLink link;
	link.SetOpCode(AS_SET_MOUSE_MODE);
	link.Attach<mode_mouse>(mode);
	link.Flush();
}


_IMPEXP_BE mode_mouse
mouse_mode()
{
	mode_mouse mode;
	
	BAppServerLink link;
	PortMessage msg;
	link.SetOpCode(AS_GET_MOUSE_MODE);
	link.FlushWithReply(&msg);
	msg.Read<mode_mouse>(&mode);
	return mode;
}


_IMPEXP_BE rgb_color
ui_color(color_which which)
{
	rgb_color color;
	
	BAppServerLink link;
	PortMessage msg;
	link.SetOpCode(AS_GET_UI_COLOR);
	link.Attach<color_which>(which);
	link.FlushWithReply(&msg);
	msg.Read<rgb_color>(&color);
	return color;
}

_IMPEXP_BE rgb_color
tint_color(rgb_color color, float tint)
{
	// Internally calculates the color
	// TODO: Implement
}


extern "C" status_t
_init_interface_kit_()
{
	sem_id widthSem = create_sem(1, "TextView WidthBuffer Sem");
	if (widthSem < 0)
		return widthSem;
	BTextView::sWidthSem = widthSem;
	BTextView::sWidthAtom = 0;
	BTextView::sWidths = new _BWidthBuffer_;
		
	status_t result = get_menu_info(&BMenu::sMenuInfo);
	if (result != B_OK)  
		return result;
	
	//TODO: fill the other static members
		
	return B_OK;
}


extern "C" status_t
_fini_interface_kit_()
{
	//TODO: Implement ?
	
	return B_OK;
}


extern "C" void
_init_global_fonts()
{
	// This function will initialize the client-side font list
	// TODO: Implement
}


// GraphicsDefs.h

_IMPEXP_BE status_t
get_pixel_size_for(color_space space, size_t * pixel_chunk, size_t * row_alignment, size_t * pixels_per_chunk)
{
	// TODO: Implement
}


_IMPEXP_BE bool
bitmaps_support_space(color_space space, uint32 * support_flags)
{
	// TODO: Implement
}

/*!
	\brief private function used by Tracker to set window decor
	\param theme The theme to choose
	
	- \c 0: BeOS
	- \c 1: AmigaOS
	- \c 2: Win95
	- \c 3: MacOS
*/
void __set_window_decor(int32 theme)
{
	BAppServerLink link;
	link.SetOpCode(AS_R5_SET_DECORATOR);
	link.Attach<int32>(theme);
	link.Flush();
}
