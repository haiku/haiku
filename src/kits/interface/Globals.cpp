//------------------------------------------------------------------------------
//	Copyright (c) 2001-2004, Haiku
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
//					Caz <turok2@currantbun.com>
//					Axel Dörfler <axeld@pinc-software.de>
//	Description:	Global functions and variables for the Interface Kit
//
//------------------------------------------------------------------------------

#include <AppServerLink.h>
#include <GraphicsDefs.h>
#include <InterfaceDefs.h>
#include <ServerProtocol.h>
#include <ScrollBar.h>
#include <Screen.h>
#include <PortMessage.h>
#include <Roster.h>
#include <Menu.h>
#include <stdlib.h>
#include <TextView.h>

#include <WidthBuffer.h>

// Private definitions not placed in public headers
extern "C" void _init_global_fonts();
extern "C" status_t _fini_interface_kit_();
extern status_t _control_input_server_(BMessage *command, BMessage *reply);

using namespace BPrivate;


// patterns
const pattern B_SOLID_HIGH = {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}};
const pattern B_MIXED_COLORS = {{0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55}};
const pattern B_SOLID_LOW = {{0, 0, 0, 0, 0, 0, 0, 0}};

// colors
const rgb_color B_TRANSPARENT_COLOR = {0x77, 0x74, 0x77, 0x00};
const rgb_color B_TRANSPARENT_32_BIT = {0x77, 0x74, 0x77, 0x00};
const uint8 B_TRANSPARENT_8_BIT = 0xff;

const uint8 B_TRANSPARENT_MAGIC_CMAP8 = 0xff;
const uint16 B_TRANSPARENT_MAGIC_RGBA15 = 0x39ce;
const uint16 B_TRANSPARENT_MAGIC_RGBA15_BIG = 0xce39;
const uint32 B_TRANSPARENT_MAGIC_RGBA32 = 0x00777477;
const uint32 B_TRANSPARENT_MAGIC_RGBA32_BIG = 0x77747700;

// misc.
const struct screen_id B_MAIN_SCREEN_ID = {0};


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


status_t
get_scroll_bar_info(scroll_bar_info *info)
{
	if (info == NULL)
		return B_BAD_VALUE;

	// ToDo: implement get_scroll_bar_info() for real
	info->proportional = true;
	info->double_arrows = false;
	info->knob = 0;
	info->min_knob_size = (int32)B_V_SCROLL_BAR_WIDTH;

	return B_ERROR;
}


_IMPEXP_BE status_t
set_scroll_bar_info(scroll_bar_info *info)
{
	if (info == NULL)
		return B_BAD_VALUE;

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
	BMessage command('Igmt');
	BMessage reply;
	
	_control_input_server_(&command, &reply);
	
	if(reply.FindInt32("mouse_type", type) != B_OK)
		return B_ERROR;
	
	return B_OK;
}


_IMPEXP_BE status_t
set_mouse_type(int32 type)
{
	BMessage command('Ismt');
	BMessage reply;

	command.AddInt32("mouse_type", type);
	return _control_input_server_(&command, &reply) == B_OK;
}


_IMPEXP_BE status_t
get_mouse_map(mouse_map *map)
{
	BMessage command('Igmm');
	BMessage reply;
	const void *data = 0;
	int32 count;
	
	_control_input_server_(&command, &reply);
	
	if(reply.FindData("mousemap", B_ANY_TYPE, &data, &count) != B_OK)
		return B_ERROR;
	
	memcpy(map, data, count);
	
	return B_OK;
}


_IMPEXP_BE status_t
set_mouse_map(mouse_map *map)
{
	BMessage command('Ismm');
	BMessage reply;
	
	command.AddData("mousemap", B_ANY_TYPE, map, sizeof(mouse_map));
	return _control_input_server_(&command, &reply) == B_OK;
}

_IMPEXP_BE status_t
get_click_speed(bigtime_t *speed)
{
	BMessage command('Igcs');
	BMessage reply;
	
	_control_input_server_(&command, &reply);
	
	if(reply.FindInt64("speed", speed) != B_OK)
		return B_ERROR;
	
	return B_OK;
}


_IMPEXP_BE status_t
set_click_speed(bigtime_t speed)
{
	BMessage command('Iscs');
	BMessage reply;
	command.AddInt64("speed", speed);
	return _control_input_server_(&command, &reply) == B_OK;
}


_IMPEXP_BE status_t
get_mouse_speed(int32 *speed)
{
	BMessage command('Igms');
	BMessage reply;
	
	_control_input_server_(&command, &reply);
	
	if(reply.FindInt32("speed", speed) != B_OK)
		return B_ERROR;
	
	return B_OK;
}


_IMPEXP_BE status_t
set_mouse_speed(int32 speed)
{
	BMessage command('Isms');
	BMessage reply;
	command.AddInt32("speed", speed);
	return _control_input_server_(&command, &reply) == B_OK;
}


_IMPEXP_BE status_t
get_mouse_acceleration(int32 *speed)
{
	BMessage command('Igma');
	BMessage reply;
	
	_control_input_server_(&command, &reply);
	
	if(reply.FindInt32("speed", speed) != B_OK)
		return B_ERROR;
	
	return B_OK;
}


_IMPEXP_BE status_t
set_mouse_acceleration(int32 speed)
{
	BMessage command('Isma');
	BMessage reply;
	command.AddInt32("speed", speed);
	return _control_input_server_(&command, &reply) == B_OK;
}


_IMPEXP_BE status_t
get_key_repeat_rate(int32 *rate)
{
	BMessage command('Igrr');
	BMessage reply;
	
	_control_input_server_(&command, &reply);
	
	if(reply.FindInt32("rate", rate) != B_OK)
		return B_ERROR;
	
	return B_OK;
}


_IMPEXP_BE status_t
set_key_repeat_rate(int32 rate)
{
	BMessage command('Isrr');
	BMessage reply;
	command.AddInt32("rate", rate);
	return _control_input_server_(&command, &reply) == B_OK;
}


_IMPEXP_BE status_t
get_key_repeat_delay(bigtime_t *delay)
{
	BMessage command('Igrd');
	BMessage reply;
	
	_control_input_server_(&command, &reply);
	
	if(reply.FindInt64("delay", delay) != B_OK)
		return B_ERROR;
	
	return B_OK;
}


_IMPEXP_BE status_t
set_key_repeat_delay(bigtime_t  delay)
{
	BMessage command('Isrd');
	BMessage reply;
	command.AddInt64("delay", delay);
	return _control_input_server_(&command, &reply) == B_OK;
}


_IMPEXP_BE uint32
modifiers()
{
	BMessage command('Igmd');
	BMessage reply;
	int32 err, modifier;
	
	_control_input_server_(&command, &reply);
	
	if(reply.FindInt32("status", &err) != B_OK)
		return 0;
	
	if(reply.FindInt32("modifiers", &modifier) != B_OK)
		return 0;

	return modifier;
}


_IMPEXP_BE status_t
get_key_info(key_info *info)
{
	BMessage command('Igki');
	BMessage reply;
	const void *data = 0;
	int32 count, err;
	
	_control_input_server_(&command, &reply);
	
	if(reply.FindInt32("status", &err) != B_OK)
		return B_ERROR;
	
	if(reply.FindData("key_info", B_ANY_TYPE, &data, &count) != B_OK)
		return B_ERROR;
	
	memcpy(info, data, count);
	return B_OK;
}


_IMPEXP_BE void
get_key_map(key_map **map, char **key_buffer)
{
	BMessage command('Igkm');
	BMessage reply;
	int32 map_count, key_count;
	const void *map_array = 0, *key_array = 0;
	
	_control_input_server_(&command, &reply);
	
	if(reply.FindData("keymap", B_ANY_TYPE, &map_array, &map_count) != B_OK)
	{
		*map = 0; *key_buffer = 0;
		return;
	}
	
	if(reply.FindData("key_buffer", B_ANY_TYPE, &key_array, &key_count) != B_OK)
	{
		*map = 0; *key_buffer = 0;
		return;
	}
	
	*map = (key_map *)malloc(map_count);
	memcpy(*map, map_array, map_count);
	*key_buffer = (char *)malloc(key_count);
	memcpy(*key_buffer, key_array, key_count);
}


_IMPEXP_BE status_t
get_keyboard_id(uint16 *id)
{
	BMessage command('Igid');
	BMessage reply;
	uint16 kid;
	
	_control_input_server_(&command, &reply);
	
	reply.FindInt16("id", (int16 *)&kid);
	*id = kid;
	
	return B_OK;
}


_IMPEXP_BE void
set_modifier_key(uint32 modifier, uint32 key)
{
	BMessage command('Ismk');
	BMessage reply;
	
	command.AddInt32("modifier", modifier);
	command.AddInt32("key", key);
	_control_input_server_(&command, &reply);
}


_IMPEXP_BE void
set_keyboard_locks(uint32 modifiers)
{
	BMessage command('Iskl');
	BMessage reply;
	
	command.AddInt32("locks", modifiers);
	_control_input_server_(&command, &reply);
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
	rgb_color result;

	#define LIGHTEN(x) ((uint8)(255.0f - (255.0f - x) * tint))
	#define DARKEN(x)  ((uint8)(x * (2 - tint)))

	if (tint < 1.0f)
	{
		result.red   = LIGHTEN(color.red);
		result.green = LIGHTEN(color.green);
		result.blue  = LIGHTEN(color.blue);
		result.alpha = color.alpha;
	}
	else
	{
		result.red   = DARKEN(color.red);
		result.green = DARKEN(color.green);
		result.blue  = DARKEN(color.blue);
		result.alpha = color.alpha;
	}

	#undef LIGHTEN
	#undef DARKEN

	return result;
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
