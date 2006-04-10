/*
 * Copyright 2001-2006, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Caz <turok2@currantbun.com>
 *		Axel Dörfler, axeld@pinc-software.de
 */

/**	Global functions and variables for the Interface Kit */

#include <Application.h>
#include <Font.h>
#include <InterfaceDefs.h>
#include <Menu.h>
#include <Roster.h>
#include <ScrollBar.h>
#include <Screen.h>
#include <String.h>
#include <TextView.h>

#include <stdlib.h>
#include <string.h>

#include <AppServerLink.h>
#include <WindowInfo.h>
#include <InputServerTypes.h>
#include <input_globals.h>
#include <ServerProtocol.h>
#include <WidthBuffer.h>
#include <ColorTools.h>

#include "moreUTF8.h"
#include "truncate_string.h"

// Private definitions not placed in public headers
void _init_global_fonts_();
extern "C" status_t _fini_interface_kit_();

using namespace BPrivate;

// some other weird struct exported by BeOS, it's not initialized, though
struct general_ui_info {
	rgb_color	background_color;
	rgb_color	mark_color;
	rgb_color	highlight_color;
	bool		color_frame;
	rgb_color	window_frame_color;
};

struct general_ui_info general_info;

menu_info *_menu_info_ptr_;

extern "C" const char B_NOTIFICATION_SENDER[] = "be:sender";


static status_t
mode2parms(uint32 space, uint32 *out_space, int32 *width, int32 *height)
{
	status_t status = B_OK;
	
	switch (space) {
		case B_8_BIT_640x480:
			*out_space = B_CMAP8;
			*width = 640; *height = 480;
			break;
		case B_8_BIT_800x600:
			*out_space = B_CMAP8;
			*width = 800; *height = 600;
			break;
		case B_8_BIT_1024x768:
			*out_space = B_CMAP8;
			*width = 1024; *height = 768;
			break;
		case B_8_BIT_1280x1024:
			*out_space = B_CMAP8;
			*width = 1280; *height = 1024;
			break;
		case B_8_BIT_1600x1200:
			*out_space = B_CMAP8;
			*width = 1600; *height = 1200;
			break;
		case B_16_BIT_640x480:
			*out_space = B_RGB16;
			*width = 640; *height = 480;
			break;
		case B_16_BIT_800x600:
			*out_space = B_RGB16;
			*width = 800; *height = 600;
			break;
		case B_16_BIT_1024x768:
			*out_space = B_RGB16;
			*width = 1024; *height = 768;
			break;
		case B_16_BIT_1280x1024:
			*out_space = B_RGB16;
			*width = 1280; *height = 1024;
			break;
		case B_16_BIT_1600x1200:
			*out_space = B_RGB16;
			*width = 1600; *height = 1200;
			break;
		case B_32_BIT_640x480:
			*out_space = B_RGB32;
			*width = 640; *height = 480;
			break;
		case B_32_BIT_800x600:
			*out_space = B_RGB32;
			*width = 800; *height = 600;
			break;
		case B_32_BIT_1024x768:
			*out_space = B_RGB32;
			*width = 1024; *height = 768;
			break;
		case B_32_BIT_1280x1024:
			*out_space = B_RGB32;
			*width = 1280; *height = 1024;
			break;
		case B_32_BIT_1600x1200:
			*out_space = B_RGB32;
			*width = 1600; *height = 1200;
			break;
    		case B_8_BIT_1152x900:
    			*out_space = B_CMAP8;
    			*width = 1152; *height = 900;
    			break;
    		case B_16_BIT_1152x900:
    			*out_space = B_RGB16;
    			*width = 1152; *height = 900;
    			break;
    		case B_32_BIT_1152x900:
    			*out_space = B_RGB32;
    			*width = 1152; *height = 900;
    			break;
		case B_15_BIT_640x480:
			*out_space = B_RGB15;
			*width = 640; *height = 480;
			break;
		case B_15_BIT_800x600:
			*out_space = B_RGB15;
			*width = 800; *height = 600;
			break;
		case B_15_BIT_1024x768:
			*out_space = B_RGB15;
			*width = 1024; *height = 768;
			break;
		case B_15_BIT_1280x1024:
			*out_space = B_RGB15;
			*width = 1280; *height = 1024;
			break;
		case B_15_BIT_1600x1200:
			*out_space = B_RGB15;
			*width = 1600; *height = 1200;
			break;
    		case B_15_BIT_1152x900:
    			*out_space = B_RGB15;
    			*width = 1152; *height = 900;
    			break;
    		default:
    			status = B_BAD_VALUE;
    			break;
	}
	
	return status;
}


_IMPEXP_BE const color_map *
system_colors()
{
	return BScreen(B_MAIN_SCREEN_ID).ColorMap();
}


_IMPEXP_BE status_t
set_screen_space(int32 index, uint32 space, bool stick)
{
	int32 width;
	int32 height;
	uint32 depth;
	
	status_t status = mode2parms(space, &depth, &width, &height);
	if (status < B_OK)
		return status;
		
	BScreen screen(B_MAIN_SCREEN_ID);
	display_mode mode;
	
	// TODO: What about refresh rate ?
	// currently we get it from the current video mode, but
	// this might be not so wise.
	status = screen.GetMode(index, &mode);
	if (status < B_OK)
		return status;
	
	mode.virtual_width = width;
	mode.virtual_height = height;
	mode.space = depth;
	
	return screen.SetMode(index, &mode, stick);
}


_IMPEXP_BE status_t
get_scroll_bar_info(scroll_bar_info *info)
{
	if (info == NULL)
		return B_BAD_VALUE;

	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_SCROLLBAR_INFO);

	int32 code;
	if (link.FlushWithReply(code) == B_OK
		&& code == B_OK) {
		link.Read<scroll_bar_info>(info);
		return B_OK;
	}

	return B_ERROR;
}


_IMPEXP_BE status_t
set_scroll_bar_info(scroll_bar_info *info)
{
	if (info == NULL)
		return B_BAD_VALUE;

	BPrivate::AppServerLink link;
	int32 code;
	
	link.StartMessage(AS_SET_SCROLLBAR_INFO);
	link.Attach<scroll_bar_info>(*info);
	
	if (link.FlushWithReply(code) == B_OK
		&& code == B_OK)
		return B_OK;

	return B_ERROR;
}


_IMPEXP_BE status_t
get_mouse_type(int32 *type)
{
	BMessage command(IS_GET_MOUSE_TYPE);
	BMessage reply;
	
	_control_input_server_(&command, &reply);
	
	if(reply.FindInt32("mouse_type", type) != B_OK)
		return B_ERROR;
	
	return B_OK;
}


_IMPEXP_BE status_t
set_mouse_type(int32 type)
{
	BMessage command(IS_SET_MOUSE_TYPE);
	BMessage reply;

	command.AddInt32("mouse_type", type);
	return _control_input_server_(&command, &reply);
}


_IMPEXP_BE status_t
get_mouse_map(mouse_map *map)
{
	BMessage command(IS_GET_MOUSE_MAP);
	BMessage reply;
	const void *data = 0;
	ssize_t count;
	
	_control_input_server_(&command, &reply);
	
	if (reply.FindData("mousemap", B_ANY_TYPE, &data, &count) != B_OK)
		return B_ERROR;
	
	memcpy(map, data, count);
	
	return B_OK;
}


_IMPEXP_BE status_t
set_mouse_map(mouse_map *map)
{
	BMessage command(IS_SET_MOUSE_MAP);
	BMessage reply;
	
	command.AddData("mousemap", B_ANY_TYPE, map, sizeof(mouse_map));
	return _control_input_server_(&command, &reply);
}


_IMPEXP_BE status_t
get_click_speed(bigtime_t *speed)
{
	BMessage command(IS_GET_CLICK_SPEED);
	BMessage reply;
	
	_control_input_server_(&command, &reply);
	
	if (reply.FindInt64("speed", speed) != B_OK)
		*speed = 500000;
	
	return B_OK;
}


_IMPEXP_BE status_t
set_click_speed(bigtime_t speed)
{
	BMessage command(IS_SET_CLICK_SPEED);
	BMessage reply;
	command.AddInt64("speed", speed);
	return _control_input_server_(&command, &reply);
}


_IMPEXP_BE status_t
get_mouse_speed(int32 *speed)
{
	BMessage command(IS_GET_MOUSE_SPEED);
	BMessage reply;
	
	_control_input_server_(&command, &reply);
	
	if (reply.FindInt32("speed", speed) != B_OK)
		*speed = 65536;
	
	return B_OK;
}


_IMPEXP_BE status_t
set_mouse_speed(int32 speed)
{
	BMessage command(IS_SET_MOUSE_SPEED);
	BMessage reply;
	command.AddInt32("speed", speed);
	return _control_input_server_(&command, &reply);
}


_IMPEXP_BE status_t
get_mouse_acceleration(int32 *speed)
{
	BMessage command(IS_GET_MOUSE_ACCELERATION);
	BMessage reply;
	
	_control_input_server_(&command, &reply);
	
	if (reply.FindInt32("speed", speed) != B_OK)
		*speed = 65536;
	
	return B_OK;
}


_IMPEXP_BE status_t
set_mouse_acceleration(int32 speed)
{
	BMessage command(IS_SET_MOUSE_ACCELERATION);
	BMessage reply;
	command.AddInt32("speed", speed);
	return _control_input_server_(&command, &reply);
}


_IMPEXP_BE status_t
get_key_repeat_rate(int32 *rate)
{
	BMessage command(IS_GET_KEY_REPEAT_RATE);
	BMessage reply;
	
	_control_input_server_(&command, &reply);
	
	if (reply.FindInt32("rate", rate) != B_OK)
		*rate = 250000;

	return B_OK;
}


_IMPEXP_BE status_t
set_key_repeat_rate(int32 rate)
{
	BMessage command(IS_SET_KEY_REPEAT_RATE);
	BMessage reply;
	command.AddInt32("rate", rate);
	return _control_input_server_(&command, &reply);
}


_IMPEXP_BE status_t
get_key_repeat_delay(bigtime_t *delay)
{
	BMessage command(IS_GET_KEY_REPEAT_DELAY);
	BMessage reply;
	
	_control_input_server_(&command, &reply);
	
	if (reply.FindInt64("delay", delay) != B_OK)
		*delay = 200;
	
	return B_OK;
}


_IMPEXP_BE status_t
set_key_repeat_delay(bigtime_t  delay)
{
	BMessage command(IS_SET_KEY_REPEAT_DELAY);
	BMessage reply;
	command.AddInt64("delay", delay);
	return _control_input_server_(&command, &reply);
}


_IMPEXP_BE uint32
modifiers()
{
	BMessage command(IS_GET_MODIFIERS);
	BMessage reply;
	int32 err, modifier;
	
	_control_input_server_(&command, &reply);
	
	if (reply.FindInt32("status", &err) != B_OK)
		return 0;
	
	if (reply.FindInt32("modifiers", &modifier) != B_OK)
		return 0;

	return modifier;
}


_IMPEXP_BE status_t
get_key_info(key_info *info)
{
	BMessage command(IS_GET_KEY_INFO);
	BMessage reply;
	const void *data = 0;
	int32 err;
	ssize_t count;
	
	_control_input_server_(&command, &reply);
	
	if (reply.FindInt32("status", &err) != B_OK)
		return B_ERROR;
	
	if (reply.FindData("key_info", B_ANY_TYPE, &data, &count) != B_OK)
		return B_ERROR;
	
	memcpy(info, data, count);
	return B_OK;
}


_IMPEXP_BE void
get_key_map(key_map **map, char **key_buffer)
{
	BMessage command(IS_GET_KEY_MAP);
	BMessage reply;
	ssize_t map_count, key_count;
	const void *map_array = 0, *key_array = 0;
	
	_control_input_server_(&command, &reply);
	
	if (reply.FindData("keymap", B_ANY_TYPE, &map_array, &map_count) != B_OK) {
		*map = 0; *key_buffer = 0;
		return;
	}
	
	if (reply.FindData("key_buffer", B_ANY_TYPE, &key_array, &key_count) != B_OK) {
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
	BMessage command(IS_GET_KEYBOARD_ID);
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
	BMessage command(IS_SET_MODIFIER_KEY);
	BMessage reply;
	
	command.AddInt32("modifier", modifier);
	command.AddInt32("key", key);
	_control_input_server_(&command, &reply);
}


_IMPEXP_BE void
set_keyboard_locks(uint32 modifiers)
{
	BMessage command(IS_SET_KEYBOARD_LOCKS);
	BMessage reply;
	
	command.AddInt32("locks", modifiers);
	_control_input_server_(&command, &reply);
}


_IMPEXP_BE status_t
_restore_key_map_()
{
	BMessage message(IS_RESTORE_KEY_MAP);
	BMessage reply;

	return _control_input_server_(&message, &reply);	
}


_IMPEXP_BE rgb_color
keyboard_navigation_color()
{
	// Queries the app_server
	return ui_color(B_KEYBOARD_NAVIGATION_COLOR);
}


_IMPEXP_BE int32
count_workspaces()
{
	int32 count = 1;

	BPrivate::AppServerLink link;
	link.StartMessage(AS_COUNT_WORKSPACES);

	status_t status;
	if (link.FlushWithReply(status) == B_OK && status == B_OK)
		link.Read<int32>(&count);

	return count;
}


_IMPEXP_BE void
set_workspace_count(int32 count)
{
	BPrivate::AppServerLink link;
	link.StartMessage(AS_SET_WORKSPACE_COUNT);
	link.Attach<int32>(count);
	link.Flush();
}


_IMPEXP_BE int32
current_workspace()
{
	int32 index = 0;

	BPrivate::AppServerLink link;
	link.StartMessage(AS_CURRENT_WORKSPACE);	

	int32 status;
	if (link.FlushWithReply(status) == B_OK && status == B_OK)
		link.Read<int32>(&index);

	return index;
}


_IMPEXP_BE void
activate_workspace(int32 workspace)
{
	BPrivate::AppServerLink link;
	link.StartMessage(AS_ACTIVATE_WORKSPACE);
	link.Attach<int32>(workspace);
	link.Flush();
}


_IMPEXP_BE bigtime_t
idle_time()
{
	bigtime_t idletime = 0;

	BPrivate::AppServerLink link;
	link.StartMessage(AS_IDLE_TIME);

	int32 code;
	if (link.FlushWithReply(code) == B_OK && code == B_OK)
		link.Read<int64>(&idletime);

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
	if (be_roster != NULL)
		be_roster->Launch("application/x-vnd.haiku-AboutHaiku");
}


_IMPEXP_BE void
set_focus_follows_mouse(bool follow)
{
	// obviously deprecated API
	set_mouse_mode(B_WARP_MOUSE);
}


_IMPEXP_BE bool
focus_follows_mouse()
{
	return mouse_mode() != B_NORMAL_MOUSE;
}


_IMPEXP_BE void
set_mouse_mode(mode_mouse mode)
{
	BPrivate::AppServerLink link;
	link.StartMessage(AS_SET_MOUSE_MODE);
	link.Attach<mode_mouse>(mode);
	link.Flush();
}


_IMPEXP_BE mode_mouse
mouse_mode()
{
	// Gets the focus-follows-mouse style, such as normal, B_WARP_MOUSE, etc.
	mode_mouse mode = B_NORMAL_MOUSE;
	
	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_MOUSE_MODE);

	int32 code;
	if (link.FlushWithReply(code) == B_OK && code == B_OK)
		link.Read<mode_mouse>(&mode);
	
	return mode;
}


_IMPEXP_BE rgb_color
ui_color(color_which which)
{		
	if (!be_app) {
			switch (which) {
				case B_PANEL_BACKGROUND_COLOR:
					return make_color(216,216,216);
				case B_PANEL_TEXT_COLOR:
					return make_color(0,0,0);
				case B_DOCUMENT_BACKGROUND_COLOR:
					return make_color(255,255,255);
				case B_DOCUMENT_TEXT_COLOR:
					return make_color(0,0,0);
				case B_CONTROL_BACKGROUND_COLOR:
					return make_color(245,245,245);
				case B_CONTROL_TEXT_COLOR:
					return make_color(0,0,0);
				case B_CONTROL_BORDER_COLOR:
					return make_color(0,0,0);
				case B_CONTROL_HIGHLIGHT_COLOR:
					return make_color(102, 152, 203);
				case B_NAVIGATION_BASE_COLOR:
					return make_color(0,0,229);
				case B_NAVIGATION_PULSE_COLOR:
					return make_color(0,0,0);
				case B_SHINE_COLOR:
					return make_color(255,255,255);
				case B_SHADOW_COLOR:
					return make_color(0,0,0);
				case B_MENU_BACKGROUND_COLOR:
					return make_color(216,216,216);
				case B_MENU_SELECTED_BACKGROUND_COLOR:
					return make_color(115,120,184);
				case B_MENU_ITEM_TEXT_COLOR:
					return make_color(0,0,0);
				case B_MENU_SELECTED_BORDER_COLOR:
					return make_color(0,0,0);
				case B_TOOLTIP_BACKGROUND_COLOR:
					return make_color(255,255,0);
				case B_TOOLTIP_TEXT_COLOR:
					return make_color(0,0,0);
				case B_SUCCESS_COLOR:
					return make_color(0,255,0);
				case B_FAILURE_COLOR:
					return make_color(255,0,0);
				case B_WINDOW_TAB_COLOR:
					return make_color(255,0,203);
				default:
					fprintf(stderr, "ui_color(): unknown color_which %d\n", which);
					return make_color(0,0,0);
			}
	}
	
	rgb_color color;
	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_UI_COLOR);
	link.Attach<color_which>(which);

	int32 code;
	if (link.FlushWithReply(code) == B_OK && code == B_OK)
		link.Read<rgb_color>(&color);

	return color;
}


_IMPEXP_BE rgb_color
tint_color(rgb_color color, float tint)
{
	rgb_color result;

	#define LIGHTEN(x) ((uint8)(255.0f - (255.0f - x) * tint))
	#define DARKEN(x)  ((uint8)(x * (2 - tint)))

	if (tint < 1.0f) {
		result.red   = LIGHTEN(color.red);
		result.green = LIGHTEN(color.green);
		result.blue  = LIGHTEN(color.blue);
		result.alpha = color.alpha;
	} else {
		result.red   = DARKEN(color.red);
		result.green = DARKEN(color.green);
		result.blue  = DARKEN(color.blue);
		result.alpha = color.alpha;
	}

	#undef LIGHTEN
	#undef DARKEN

	return result;
}


rgb_color shift_color(rgb_color color, float shift);

rgb_color
shift_color(rgb_color color, float shift)
{
	// TODO: dunno what this is supposed to do, but BeIDE expects it to be there...
	return color;
}


static status_t
load_menu_settings(menu_info &into)
{
	// TODO: Load settings from the settings file,
	// and only if it fails, fallback to the defaults

	// TODO: shouldn't the server handle the details? It could broadcast
	//	change messages to the applications, which could also relayout
	//	there menus and menu bars then (as soon as we have a layout management)
	into.font_size = be_plain_font->Size();
	be_plain_font->GetFamilyAndStyle(&into.f_family, &into.f_style);
	into.background_color = ui_color(B_MENU_BACKGROUND_COLOR);
	into.separator = 0;
	into.click_to_open = true;
	into.triggers_always_shown = false;

	return B_OK;
}


extern "C" status_t
_init_interface_kit_()
{
	sem_id widthSem = create_sem(0, "BTextView WidthBuffer Sem");
	if (widthSem < 0)
		return widthSem;
	BTextView::sWidthSem = widthSem;
	BTextView::sWidthAtom = 0;
	BTextView::sWidths = new _BWidthBuffer_;

	_init_global_fonts_();

	_menu_info_ptr_ = &BMenu::sMenuInfo;
	status_t status = load_menu_settings(BMenu::sMenuInfo);

	// TODO: fill the other static members

	return status;
}


extern "C" status_t
_fini_interface_kit_()
{
	//TODO: Implement ?
	
	return B_OK;
}


//	#pragma mark -


/*!
	\brief private function used by Deskbar to set window decor
	Note, we don't have to be compatible here, and could just change
	the Deskbar not to use this anymore
	\param theme The theme to choose
	
	- \c 0: BeOS
	- \c 1: AmigaOS
	- \c 2: Win95
	- \c 3: MacOS
*/
void
__set_window_decor(int32 theme)
{
	BPrivate::AppServerLink link;
	link.StartMessage(AS_R5_SET_DECORATOR);
	link.Attach<int32>(theme);
	link.Flush();
}


namespace BPrivate {

/*!
	\brief queries the server for the number of available decorators
	\return the number of available decorators
*/
int32
count_decorators(void)
{
	BPrivate::AppServerLink link;
	link.StartMessage(AS_COUNT_DECORATORS);

	int32 code;
	int32 count = -1;
	if (link.FlushWithReply(code) == B_OK && code == B_OK)
		link.Read<int32>(&count);
	
	return count;
}

/*!
	\brief queries the server for the index of the current decorators
	\return the current decorator's index
	
	If for some bizarre reason this function fails, it returns -1
*/
int32
get_decorator(void)
{
	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_DECORATOR);

	int32 code;
	int32 index = -1;
	if (link.FlushWithReply(code) == B_OK && code == B_OK)
		link.Read<int32>(&index);

	return index;
}


/*!
	\brief queries the server for the name of the decorator with a certain index
	\param index The index of the decorator to get the name for
	\param name BString to receive the name of the decorator
	\return B_OK if successful, B_ERROR if not
*/
status_t
get_decorator_name(const int32 &index, BString &name)
{
	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_DECORATOR_NAME);
	link.Attach<int32>(index);

	int32 code;
	if (link.FlushWithReply(code) == B_OK && code == B_OK) {
		char *string;
		if (link.ReadString(&string) == B_OK) {
			name = string;
			free(string);
			return B_OK;
		}
	}

	return B_ERROR;
}

/*!
	\brief asks the server to draw a decorator preview into a BBitmap
	\param index The index of the decorator to get the name for
	\param bitmap BBitmap to receive the preview
	\return B_OK if successful, B_ERROR if not.
	
	This is currently unimplemented.
*/
status_t
get_decorator_preview(const int32 &index, BBitmap *bitmap)
{
	// TODO: implement get_decorator_preview
	return B_ERROR;
}


/*!
	\brief Private function which sets the window decorator for the system.
	\param index Index of the decorator to set
	
	If the index is invalid, this function and the server do nothing
*/
status_t
set_decorator(const int32 &index)
{
	if (index < 0)
		return B_BAD_VALUE;

	BPrivate::AppServerLink link;

	link.StartMessage(AS_SET_DECORATOR);
	link.Attach<int32>(index);
	link.Flush();

	return B_OK;
}

}	// namespace BPrivate

// These methods were marked with "Danger, will Robinson!" in
// the OpenTracker source, so we might not want to be compatible
// here.
// In any way, we would need to update Deskbar to use our 
// replacements, so we could as well just implement them...
//
// They are defined (also the complete window_info structure) in
// src/apps/deskbar/WindowMenuItem.h

struct window_info;

void do_window_action(int32 window_id, int32 action, 
		BRect zoomRect, bool zoom);
window_info	*get_window_info(int32 a_token);
int32 *get_token_list(team_id app, int32 *count);
void do_bring_to_front_team(BRect zoomRect, team_id app, bool zoom);
void do_minimize_team(BRect zoomRect, team_id team, bool zoom);


void 
do_window_action(int32 windowToken, int32 action, 
	BRect zoomRect, bool zoom)
{
	BPrivate::AppServerLink link;

	link.StartMessage(AS_WINDOW_ACTION);
	link.Attach<int32>(windowToken);
	link.Attach<int32>(action);
		// we don't have any zooming effect

	link.Flush();
}


window_info	*
get_window_info(int32 serverToken)
{
	BPrivate::AppServerLink link;
	
	link.StartMessage(AS_GET_WINDOW_INFO);
	link.Attach<int32>(serverToken);

	int32 code;
	if (link.FlushWithReply(code) != B_OK || code != B_OK)
		return NULL;

	int32 size;
	link.Read<int32>(&size);
	
	client_window_info* info = (client_window_info*)malloc(size);
	if (info == NULL)
		return NULL;

	link.Read(info, size);
	return info;
}


int32 *
get_token_list(team_id team, int32 *_count)
{
	BPrivate::AppServerLink link;

	link.StartMessage(AS_GET_WINDOW_LIST);
	link.Attach<team_id>(team);

	int32 code;
	if (link.FlushWithReply(code) != B_OK || code != B_OK)
		return NULL;

	int32 count;
	link.Read<int32>(&count);

	int32* tokens = (int32*)malloc(count * sizeof(int32));
	if (tokens == NULL)
		return NULL;

	link.Read(tokens, count * sizeof(int32));
	*_count = count;
	return tokens;
}


void
do_bring_to_front_team(BRect zoomRect, team_id team, bool zoom)
{
	BPrivate::AppServerLink link;

	link.StartMessage(AS_BRING_TEAM_TO_FRONT);
	link.Attach<team_id>(team);
		// we don't have any zooming effect

	link.Flush();
}


void
do_minimize_team(BRect zoomRect, team_id team, bool zoom)
{
	BPrivate::AppServerLink link;

	link.StartMessage(AS_MINIMIZE_TEAM);
	link.Attach<team_id>(team);
		// we don't have any zooming effect

	link.Flush();
}


//	#pragma mark - truncate string


static char*
write_ellipsis(char* dst)
{
	strcpy(dst, B_UTF8_ELLIPSIS);
	// The UTF-8 character spans over 3 bytes
	return dst + 3;
}


bool
truncate_end(const char* source, char* dest, uint32 numChars,
	const float* escapementArray, float width, float ellipsisWidth, float size)
{
	float currentWidth = 0.0;
	ellipsisWidth /= size;	// test if this is as accurate as escapementArray * size
	width /= size;
	uint32 lastFit = 0, c;

	for (c = 0; c < numChars; c++) {
		currentWidth += escapementArray[c];
		if (currentWidth + ellipsisWidth <= width)
			lastFit = c;

		if (currentWidth > width)
			break;
	}

	if (c == numChars) {
		// string fits into width
		return false;
	}

	if (c == 0) {
		// there is no space for the ellipsis
		strcpy(dest, "");
		return true;
	}

	// copy string to destination

	for (uint32 i = 0; i < lastFit + 1; i++) {
		// copy one glyph
		do {
			*dest++ = *source++;
		} while (IsInsideGlyph(*source));
	}

	// write ellipsis and terminate

	dest = write_ellipsis(dest);
	*dest = '\0';
	return true;
}


static char*
copy_from_end(const char* src, char* dst, uint32 numChars, uint32 length,
	const float* escapementArray, float width, float ellipsisWidth, float size)
{
	const char* originalStart = src;
	src += length - 1;
	float currentWidth = 0.0;
	for (int32 c = numChars - 1; c > 0; c--) {
		currentWidth += escapementArray[c] * size;
		if (currentWidth > width) {
			// ups, we definitely don't fit. go back until the ellipsis fits
			currentWidth += ellipsisWidth;
			// go forward again until ellipsis fits (already beyond the target)
			for (uint32 c2 = c; c2 < numChars; c2++) {
//printf(" backward: %c (%ld) (%.1f - %.1f = %.1f)\n", *dst, c2, currentWidth, escapementArray[c2] * size, currentWidth - escapementArray[c2] * size);
				currentWidth -= escapementArray[c2] * size;
				do {
					src++;
				} while (IsInsideGlyph(*src));
				// see if we went back enough
				if (currentWidth <= width)
					break;
			}
			break;
		} else {
			// go back one glyph
			do {
				src--;
			} while (IsInsideGlyph(*src));
		}
	}
	// copy from the end of the string
	uint32 bytesToCopy = originalStart + length - src;
	memcpy(dst, src, bytesToCopy);
	dst += bytesToCopy;
	return dst;
}


bool
truncate_middle(const char* source, char* dest, uint32 numChars,
	const float* escapementArray, float width, float ellipsisWidth, float size)
{
	// find visual center

	ellipsisWidth /= size;	// test if this is as accurate as escapementArray * size
	width /= size;

	float halfWidth = (width - ellipsisWidth) / 2.0;
	float leftWidth = 0.0, rightWidth = 0.0;
	uint32 left, right;

	// coming from left...

	for (left = 0; left < numChars; left++) {
		if (leftWidth + escapementArray[left] > halfWidth)
			break;

		leftWidth += escapementArray[left];
	}

	if (left == numChars) {
		// string is smaller than half of the maximum width
		return false;
	}

	// coming from right...

	for (right = numChars; right-- > left; ) {
		if (rightWidth + escapementArray[right] > halfWidth)
			break;

		rightWidth += escapementArray[right];
	}

	if (left >= right) {
		// string is smaller than the maximum width
		return false;
	}

	if (left == 0 || right >= numChars - 1) {
		// there is no space for the ellipsis
		strcpy(dest, "");
		return true;
	}
	
	// see if the gap between left/right is smaller than the ellipsis

	float totalWidth = rightWidth + leftWidth;

	for (uint32 i = left; i < right; i++) {
		totalWidth += escapementArray[i];
		if (totalWidth > width)
			break;
	}

	if (totalWidth <= width) {
		// the whole string fits!
		return false;
	}

	// The ellipsis now definitely fits, but let's
	// see if we can add another character
	
	totalWidth = ellipsisWidth + rightWidth + leftWidth;

	if (left > numChars - right) {
		// try right letter first
		if (escapementArray[right] + totalWidth <= width)
			right--;
		else if (escapementArray[left] + totalWidth <= width)
			left++;
	} else {
		// try left letter first
		if (escapementArray[left] + totalWidth <= width)
			left++;
		else if (escapementArray[right] + totalWidth <= width)
			right--;
	}

	// copy characters

	for (uint32 i = 0; i < left; i++) {
		// copy one glyph
		do {
			*dest++ = *source++;
		} while (IsInsideGlyph(*source));
	}

	dest = write_ellipsis(dest);

	for (uint32 i = left; i < numChars; i++) {
		// copy one glyph
		do {
			if (i >= right)
				*dest++ = *source++;
			else
				source++;
		} while (IsInsideGlyph(*source));
	}

	// terminate
	dest[0] = '\0';
	return true;
}


// ToDo: put into BPrivate namespace
void
truncate_string(const char* string, uint32 mode, float width,
	char* result, const float* escapementArray, float fontSize,
	float ellipsisWidth, int32 length, int32 numChars)
{
	// ToDo: that's actually not correct: the string could be smaller than ellipsisWidth
	if (string == NULL /*|| width < ellipsisWidth*/) {
		// we don't have room for a single glyph
		strcpy(result, "");
		return;
	}

	// iterate over glyphs and copy source into result string
	// one glyph at a time as long as we have room for the "…" yet
	char* dest = result;
	const char* source = string;
	bool truncated = true;

	switch (mode) {
		case B_TRUNCATE_BEGINNING: {
			dest = copy_from_end(source, dest, numChars, length,
				escapementArray, width, ellipsisWidth, fontSize);
			// "dst" points to the position behind the last glyph that
			// was copied.
			int32 dist = dest - result;
			// we didn't terminate yet
			*dest = 0;
			if (dist < length) {
				// TODO: Is there a smarter way?
				char* temp = new char[dist + 4];
				char* t = temp;
				// append "…"
				t = write_ellipsis(t);
				// shuffle arround strings so that "…" is prepended
				strcpy(t, result);
				strcpy(result, temp);
				delete[] temp;
/*						char t = result[3];
				memmove(&result[3], result, dist + 1);
				write_ellipsis(result);
				result[3] = t;*/
			}
			break;
		}

		case B_TRUNCATE_END:
			truncated = truncate_end(source, dest, numChars, escapementArray,
				width, ellipsisWidth, fontSize);
			break;

		case B_TRUNCATE_SMART:
			// TODO: implement, though it was never implemented on R5
			// FALL THROUGH (at least do something)
		case B_TRUNCATE_MIDDLE:
			truncated = truncate_middle(source, dest, numChars, escapementArray,
				width, ellipsisWidth, fontSize);
			break;
	}

	if (!truncated) {
		// copy string to destination verbatim
		strlcpy(dest, source, length + 1);
	}
}

