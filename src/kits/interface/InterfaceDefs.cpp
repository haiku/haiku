/*
 * Copyright 2001-2010, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Caz <turok2@currantbun.com>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Michael Lotz <mmlr@mlotz.ch>
 *		Wim van der Meer <WPJvanderMeer@gmail.com>
 */


/*!	Global functions and variables for the Interface Kit */


#include <InterfaceDefs.h>

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Bitmap.h>
#include <Clipboard.h>
#include <ControlLook.h>
#include <Font.h>
#include <Menu.h>
#include <Point.h>
#include <Roster.h>
#include <Screen.h>
#include <ScrollBar.h>
#include <String.h>
#include <TextView.h>
#include <Window.h>

#include <ApplicationPrivate.h>
#include <AppServerLink.h>
#include <ColorConversion.h>
#include <DecorInfo.h>
#include <DefaultColors.h>
#include <InputServerTypes.h>
#include <input_globals.h>
#include <InterfacePrivate.h>
#include <MenuPrivate.h>
#include <pr_server.h>
#include <ServerProtocol.h>
#include <ServerReadOnlyMemory.h>
#include <truncate_string.h>
#include <utf8_functions.h>
#include <WidthBuffer.h>
#include <WindowInfo.h>


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

static const rgb_color _kDefaultColors[kNumColors] = {
	{216, 216, 216, 255},	// B_PANEL_BACKGROUND_COLOR
	{216, 216, 216, 255},	// B_MENU_BACKGROUND_COLOR
	{255, 203, 0, 255},		// B_WINDOW_TAB_COLOR
	{0, 0, 229, 255},		// B_KEYBOARD_NAVIGATION_COLOR
	{51, 102, 152, 255},	// B_DESKTOP_COLOR
	{153, 153, 153, 255},	// B_MENU_SELECTED_BACKGROUND_COLOR
	{0, 0, 0, 255},			// B_MENU_ITEM_TEXT_COLOR
	{0, 0, 0, 255},			// B_MENU_SELECTED_ITEM_TEXT_COLOR
	{0, 0, 0, 255},			// B_MENU_SELECTED_BORDER_COLOR
	{0, 0, 0, 255},			// B_PANEL_TEXT_COLOR
	{255, 255, 255, 255},	// B_DOCUMENT_BACKGROUND_COLOR
	{0, 0, 0, 255},			// B_DOCUMENT_TEXT_COLOR
	{245, 245, 245, 255},	// B_CONTROL_BACKGROUND_COLOR
	{0, 0, 0, 255},			// B_CONTROL_TEXT_COLOR
	{0, 0, 0, 255},			// B_CONTROL_BORDER_COLOR
	{102, 152, 203, 255},	// B_CONTROL_HIGHLIGHT_COLOR
	{0, 0, 0, 255},			// B_NAVIGATION_PULSE_COLOR
	{255, 255, 255, 255},	// B_SHINE_COLOR
	{0, 0, 0, 255},			// B_SHADOW_COLOR
	{255, 255, 216, 255},	// B_TOOLTIP_BACKGROUND_COLOR
	{0, 0, 0, 255},			// B_TOOLTIP_TEXT_COLOR
	{0, 0, 0, 255},			// B_WINDOW_TEXT_COLOR
	{232, 232, 232, 255},	// B_WINDOW_INACTIVE_TAB_COLOR
	{80, 80, 80, 255},		// B_WINDOW_INACTIVE_TEXT_COLOR
	{224, 224, 224, 255},	// B_WINDOW_BORDER_COLOR
	{232, 232, 232, 255},	// B_WINDOW_INACTIVE_BORDER_COLOR
	// 100...
	{0, 255, 0, 255},		// B_SUCCESS_COLOR
	{255, 0, 0, 255},		// B_FAILURE_COLOR
	{}
};
const rgb_color* BPrivate::kDefaultColors = &_kDefaultColors[0];


namespace BPrivate {


/*!	Fills the \a width, \a height, and \a colorSpace parameters according
	to the window screen's mode.
	Returns \c true if the mode is known.
*/
bool
get_mode_parameter(uint32 mode, int32& width, int32& height,
	uint32& colorSpace)
{
	switch (mode) {
		case B_8_BIT_640x480:
		case B_8_BIT_800x600:
		case B_8_BIT_1024x768:
		case B_8_BIT_1152x900:
		case B_8_BIT_1280x1024:
		case B_8_BIT_1600x1200:
			colorSpace = B_CMAP8;
			break;

		case B_15_BIT_640x480:
		case B_15_BIT_800x600:
		case B_15_BIT_1024x768:
		case B_15_BIT_1152x900:
		case B_15_BIT_1280x1024:
		case B_15_BIT_1600x1200:
			colorSpace = B_RGB15;
			break;

		case B_16_BIT_640x480:
		case B_16_BIT_800x600:
		case B_16_BIT_1024x768:
		case B_16_BIT_1152x900:
		case B_16_BIT_1280x1024:
		case B_16_BIT_1600x1200:
			colorSpace = B_RGB16;
			break;

		case B_32_BIT_640x480:
		case B_32_BIT_800x600:
		case B_32_BIT_1024x768:
		case B_32_BIT_1152x900:
		case B_32_BIT_1280x1024:
		case B_32_BIT_1600x1200:
			colorSpace = B_RGB32;
			break;

		default:
			return false;
	}

	switch (mode) {
		case B_8_BIT_640x480:
		case B_15_BIT_640x480:
		case B_16_BIT_640x480:
		case B_32_BIT_640x480:
			width = 640; height = 480;
			break;

		case B_8_BIT_800x600:
		case B_15_BIT_800x600:
		case B_16_BIT_800x600:
		case B_32_BIT_800x600:
			width = 800; height = 600;
			break;

		case B_8_BIT_1024x768:
		case B_15_BIT_1024x768:
		case B_16_BIT_1024x768:
		case B_32_BIT_1024x768:
			width = 1024; height = 768;
			break;

		case B_8_BIT_1152x900:
		case B_15_BIT_1152x900:
		case B_16_BIT_1152x900:
		case B_32_BIT_1152x900:
			width = 1152; height = 900;
			break;

		case B_8_BIT_1280x1024:
		case B_15_BIT_1280x1024:
		case B_16_BIT_1280x1024:
		case B_32_BIT_1280x1024:
			width = 1280; height = 1024;
			break;

		case B_8_BIT_1600x1200:
		case B_15_BIT_1600x1200:
		case B_16_BIT_1600x1200:
		case B_32_BIT_1600x1200:
			width = 1600; height = 1200;
			break;
	}

	return true;
}


void
get_workspaces_layout(uint32* _columns, uint32* _rows)
{
	int32 columns = 1;
	int32 rows = 1;

	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_WORKSPACE_LAYOUT);

	status_t status;
	if (link.FlushWithReply(status) == B_OK && status == B_OK) {
		link.Read<int32>(&columns);
		link.Read<int32>(&rows);
	}

	if (_columns != NULL)
		*_columns = columns;
	if (_rows != NULL)
		*_rows = rows;
}


void
set_workspaces_layout(uint32 columns, uint32 rows)
{
	if (columns < 1 || rows < 1)
		return;

	BPrivate::AppServerLink link;
	link.StartMessage(AS_SET_WORKSPACE_LAYOUT);
	link.Attach<int32>(columns);
	link.Attach<int32>(rows);
	link.Flush();
}


}	// namespace BPrivate


void
set_subpixel_antialiasing(bool subpix)
{
	BPrivate::AppServerLink link;

	link.StartMessage(AS_SET_SUBPIXEL_ANTIALIASING);
	link.Attach<bool>(subpix);
	link.Flush();
}


status_t
get_subpixel_antialiasing(bool* subpix)
{
	BPrivate::AppServerLink link;

	link.StartMessage(AS_GET_SUBPIXEL_ANTIALIASING);
	int32 status = B_ERROR;
	if (link.FlushWithReply(status) != B_OK || status < B_OK)
		return status;
	link.Read<bool>(subpix);
	return B_OK;
}


void
set_hinting_mode(uint8 hinting)
{
	BPrivate::AppServerLink link;

	link.StartMessage(AS_SET_HINTING);
	link.Attach<uint8>(hinting);
	link.Flush();
}


status_t
get_hinting_mode(uint8* hinting)
{
	BPrivate::AppServerLink link;

	link.StartMessage(AS_GET_HINTING);
	int32 status = B_ERROR;
	if (link.FlushWithReply(status) != B_OK || status < B_OK)
		return status;
	link.Read<uint8>(hinting);
	return B_OK;
}


void
set_average_weight(uint8 averageWeight)
{
	BPrivate::AppServerLink link;

	link.StartMessage(AS_SET_SUBPIXEL_AVERAGE_WEIGHT);
	link.Attach<uint8>(averageWeight);
	link.Flush();
}


status_t
get_average_weight(uint8* averageWeight)
{
	BPrivate::AppServerLink link;

	link.StartMessage(AS_GET_SUBPIXEL_AVERAGE_WEIGHT);
	int32 status = B_ERROR;
	if (link.FlushWithReply(status) != B_OK || status < B_OK)
		return status;
	link.Read<uint8>(averageWeight);
	return B_OK;
}


void
set_is_subpixel_ordering_regular(bool subpixelOrdering)
{
	BPrivate::AppServerLink link;

	link.StartMessage(AS_SET_SUBPIXEL_ORDERING);
	link.Attach<bool>(subpixelOrdering);
	link.Flush();
}


status_t
get_is_subpixel_ordering_regular(bool* subpixelOrdering)
{
	BPrivate::AppServerLink link;

	link.StartMessage(AS_GET_SUBPIXEL_ORDERING);
	int32 status = B_ERROR;
	if (link.FlushWithReply(status) != B_OK || status < B_OK)
		return status;
	link.Read<bool>(subpixelOrdering);
	return B_OK;
}


const color_map *
system_colors()
{
	return BScreen(B_MAIN_SCREEN_ID).ColorMap();
}


status_t
set_screen_space(int32 index, uint32 space, bool stick)
{
	int32 width;
	int32 height;
	uint32 depth;
	if (!BPrivate::get_mode_parameter(space, width, height, depth))
		return B_BAD_VALUE;

	BScreen screen(B_MAIN_SCREEN_ID);
	display_mode mode;

	// TODO: What about refresh rate ?
	// currently we get it from the current video mode, but
	// this might be not so wise.
	status_t status = screen.GetMode(index, &mode);
	if (status < B_OK)
		return status;

	mode.virtual_width = width;
	mode.virtual_height = height;
	mode.space = depth;

	return screen.SetMode(index, &mode, stick);
}


status_t
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


status_t
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


status_t
get_mouse_type(int32 *type)
{
	BMessage command(IS_GET_MOUSE_TYPE);
	BMessage reply;

	status_t err = _control_input_server_(&command, &reply);
	if (err != B_OK)
		return err;
	return reply.FindInt32("mouse_type", type);
}


status_t
set_mouse_type(int32 type)
{
	BMessage command(IS_SET_MOUSE_TYPE);
	BMessage reply;

	status_t err = command.AddInt32("mouse_type", type);
	if (err != B_OK)
		return err;
	return _control_input_server_(&command, &reply);
}


status_t
get_mouse_map(mouse_map *map)
{
	BMessage command(IS_GET_MOUSE_MAP);
	BMessage reply;
	const void *data = 0;
	ssize_t count;

	status_t err = _control_input_server_(&command, &reply);
	if (err == B_OK)
		err = reply.FindData("mousemap", B_RAW_TYPE, &data, &count);
	if (err != B_OK)
		return err;

	memcpy(map, data, count);

	return B_OK;
}


status_t
set_mouse_map(mouse_map *map)
{
	BMessage command(IS_SET_MOUSE_MAP);
	BMessage reply;

	status_t err = command.AddData("mousemap", B_RAW_TYPE, map,
		sizeof(mouse_map));
	if (err != B_OK)
		return err;
	return _control_input_server_(&command, &reply);
}


status_t
get_click_speed(bigtime_t *speed)
{
	BMessage command(IS_GET_CLICK_SPEED);
	BMessage reply;

	status_t err = _control_input_server_(&command, &reply);
	if (err != B_OK)
		return err;

	if (reply.FindInt64("speed", speed) != B_OK)
		*speed = 500000;

	return B_OK;
}


status_t
set_click_speed(bigtime_t speed)
{
	BMessage command(IS_SET_CLICK_SPEED);
	BMessage reply;
	command.AddInt64("speed", speed);
	return _control_input_server_(&command, &reply);
}


status_t
get_mouse_speed(int32 *speed)
{
	BMessage command(IS_GET_MOUSE_SPEED);
	BMessage reply;

	status_t err = _control_input_server_(&command, &reply);
	if (err != B_OK)
		return err;

	if (reply.FindInt32("speed", speed) != B_OK)
		*speed = 65536;

	return B_OK;
}


status_t
set_mouse_speed(int32 speed)
{
	BMessage command(IS_SET_MOUSE_SPEED);
	BMessage reply;
	command.AddInt32("speed", speed);
	return _control_input_server_(&command, &reply);
}


status_t
get_mouse_acceleration(int32 *speed)
{
	BMessage command(IS_GET_MOUSE_ACCELERATION);
	BMessage reply;

	_control_input_server_(&command, &reply);

	if (reply.FindInt32("speed", speed) != B_OK)
		*speed = 65536;

	return B_OK;
}


status_t
set_mouse_acceleration(int32 speed)
{
	BMessage command(IS_SET_MOUSE_ACCELERATION);
	BMessage reply;
	command.AddInt32("speed", speed);
	return _control_input_server_(&command, &reply);
}


status_t
get_key_repeat_rate(int32 *rate)
{
	BMessage command(IS_GET_KEY_REPEAT_RATE);
	BMessage reply;

	_control_input_server_(&command, &reply);

	if (reply.FindInt32("rate", rate) != B_OK)
		*rate = 250000;

	return B_OK;
}


status_t
set_key_repeat_rate(int32 rate)
{
	BMessage command(IS_SET_KEY_REPEAT_RATE);
	BMessage reply;
	command.AddInt32("rate", rate);
	return _control_input_server_(&command, &reply);
}


status_t
get_key_repeat_delay(bigtime_t *delay)
{
	BMessage command(IS_GET_KEY_REPEAT_DELAY);
	BMessage reply;

	_control_input_server_(&command, &reply);

	if (reply.FindInt64("delay", delay) != B_OK)
		*delay = 200;

	return B_OK;
}


status_t
set_key_repeat_delay(bigtime_t  delay)
{
	BMessage command(IS_SET_KEY_REPEAT_DELAY);
	BMessage reply;
	command.AddInt64("delay", delay);
	return _control_input_server_(&command, &reply);
}


uint32
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


status_t
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


void
get_key_map(key_map **map, char **key_buffer)
{
	_get_key_map(map, key_buffer, NULL);
}


void
_get_key_map(key_map **map, char **key_buffer, ssize_t *key_buffer_size)
{
	BMessage command(IS_GET_KEY_MAP);
	BMessage reply;
	ssize_t map_count, key_count;
	const void *map_array = 0, *key_array = 0;
	if (key_buffer_size == NULL)
		key_buffer_size = &key_count;

	_control_input_server_(&command, &reply);

	if (reply.FindData("keymap", B_ANY_TYPE, &map_array, &map_count) != B_OK) {
		*map = 0; *key_buffer = 0;
		return;
	}

	if (reply.FindData("key_buffer", B_ANY_TYPE, &key_array, key_buffer_size)
			!= B_OK) {
		*map = 0; *key_buffer = 0;
		return;
	}

	*map = (key_map *)malloc(map_count);
	memcpy(*map, map_array, map_count);
	*key_buffer = (char *)malloc(*key_buffer_size);
	memcpy(*key_buffer, key_array, *key_buffer_size);
}


status_t
get_keyboard_id(uint16 *id)
{
	BMessage command(IS_GET_KEYBOARD_ID);
	BMessage reply;
	uint16 kid;

	_control_input_server_(&command, &reply);

	status_t err = reply.FindInt16("id", (int16 *)&kid);
	if (err != B_OK)
		return err;
	*id = kid;

	return B_OK;
}


status_t
get_modifier_key(uint32 modifier, uint32 *key)
{
	BMessage command(IS_GET_MODIFIER_KEY);
	BMessage reply;
	uint32 rkey;

	command.AddInt32("modifier", modifier);
	_control_input_server_(&command, &reply);

	status_t err = reply.FindInt32("key", (int32 *) &rkey);
	if (err != B_OK)
		return err;
	*key = rkey;

	return B_OK;
}


void
set_modifier_key(uint32 modifier, uint32 key)
{
	BMessage command(IS_SET_MODIFIER_KEY);
	BMessage reply;

	command.AddInt32("modifier", modifier);
	command.AddInt32("key", key);
	_control_input_server_(&command, &reply);
}


void
set_keyboard_locks(uint32 modifiers)
{
	BMessage command(IS_SET_KEYBOARD_LOCKS);
	BMessage reply;

	command.AddInt32("locks", modifiers);
	_control_input_server_(&command, &reply);
}


status_t
_restore_key_map_()
{
	BMessage message(IS_RESTORE_KEY_MAP);
	BMessage reply;

	return _control_input_server_(&message, &reply);
}


rgb_color
keyboard_navigation_color()
{
	// Queries the app_server
	return ui_color(B_KEYBOARD_NAVIGATION_COLOR);
}


int32
count_workspaces()
{
	uint32 columns;
	uint32 rows;
	BPrivate::get_workspaces_layout(&columns, &rows);

	return columns * rows;
}


void
set_workspace_count(int32 count)
{
	int32 squareRoot = (int32)sqrt(count);

	int32 rows = 1;
	for (int32 i = 2; i <= squareRoot; i++) {
		if (count % i == 0)
			rows = i;
	}

	int32 columns = count / rows;

	BPrivate::set_workspaces_layout(columns, rows);
}


int32
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


void
activate_workspace(int32 workspace)
{
	BPrivate::AppServerLink link;
	link.StartMessage(AS_ACTIVATE_WORKSPACE);
	link.Attach<int32>(workspace);
	link.Attach<bool>(false);
	link.Flush();
}


bigtime_t
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


void
run_select_printer_panel()
{
	if (be_roster == NULL)
		return;

	// Launches the Printer prefs app via the Roster
	be_roster->Launch(PRNT_SIGNATURE_TYPE);
}


void
run_add_printer_panel()
{
	// Launches the Printer prefs app via the Roster and asks it to
	// add a printer
	run_select_printer_panel();

	BMessenger printerPanelMessenger(PRNT_SIGNATURE_TYPE);
	printerPanelMessenger.SendMessage(PRINTERS_ADD_PRINTER);
}


void
run_be_about()
{
	if (be_roster != NULL)
		be_roster->Launch("application/x-vnd.Haiku-About");
}


void
set_focus_follows_mouse(bool follow)
{
	// obviously deprecated API
	set_mouse_mode(B_FOCUS_FOLLOWS_MOUSE);
}


bool
focus_follows_mouse()
{
	return mouse_mode() == B_FOCUS_FOLLOWS_MOUSE;
}


void
set_mouse_mode(mode_mouse mode)
{
	BPrivate::AppServerLink link;
	link.StartMessage(AS_SET_MOUSE_MODE);
	link.Attach<mode_mouse>(mode);
	link.Flush();
}


mode_mouse
mouse_mode()
{
	// Gets the mouse focus style, such as activate to click,
	// focus to click, ...
	mode_mouse mode = B_NORMAL_MOUSE;

	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_MOUSE_MODE);

	int32 code;
	if (link.FlushWithReply(code) == B_OK && code == B_OK)
		link.Read<mode_mouse>(&mode);

	return mode;
}


void
set_focus_follows_mouse_mode(mode_focus_follows_mouse mode)
{
	BPrivate::AppServerLink link;
	link.StartMessage(AS_SET_FOCUS_FOLLOWS_MOUSE_MODE);
	link.Attach<mode_focus_follows_mouse>(mode);
	link.Flush();
}


mode_focus_follows_mouse
focus_follows_mouse_mode()
{
	mode_focus_follows_mouse mode = B_NORMAL_FOCUS_FOLLOWS_MOUSE;

	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_FOCUS_FOLLOWS_MOUSE_MODE);

	int32 code;
	if (link.FlushWithReply(code) == B_OK && code == B_OK)
		link.Read<mode_focus_follows_mouse>(&mode);

	return mode;
}


status_t
get_mouse(BPoint* screenWhere, uint32* buttons)
{
	if (screenWhere == NULL && buttons == NULL)
		return B_BAD_VALUE;

	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_CURSOR_POSITION);
	
	int32 code;
	status_t ret = link.FlushWithReply(code);
	if (ret != B_OK)
		return ret;
	if (code != B_OK)
		return code;

	if (screenWhere != NULL)
		ret = link.Read<BPoint>(screenWhere);
	else {
		BPoint dummy;
		ret = link.Read<BPoint>(&dummy);
	}
	if (ret != B_OK)
		return ret;

	if (buttons != NULL)
		ret = link.Read<uint32>(buttons);
	else {
		uint32 dummy;
		ret = link.Read<uint32>(&dummy);
	}

	return ret;
}


status_t
get_mouse_bitmap(BBitmap** bitmap, BPoint* hotspot)
{
	if (bitmap == NULL && hotspot == NULL)
		return B_BAD_VALUE;
	
	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_CURSOR_BITMAP);
	
	int32 code;
	status_t status = link.FlushWithReply(code);
	if (status != B_OK)
		return status;
	if (code != B_OK)
		return code;
	
	uint32 size = 0;
	uint32 cursorWidth = 0;
	uint32 cursorHeight = 0;
	
	// if link.Read() returns an error, the same error will be returned on
	// subsequent calls, so we'll check only the return value of the last call
	link.Read<uint32>(&size);
	link.Read<uint32>(&cursorWidth);
	link.Read<uint32>(&cursorHeight);
	if (hotspot == NULL) {
		BPoint dummy;
		link.Read<BPoint>(&dummy);
	} else
		link.Read<BPoint>(hotspot);

	void* data = NULL;
	if (size > 0)
		data = malloc(size);
	if (data == NULL)
		return B_NO_MEMORY;
	
	status = link.Read(data, size);
	if (status != B_OK) {
		free(data);
		return status;
	}
	
	BBitmap* cursorBitmap = new (std::nothrow) BBitmap(BRect(0, 0,
		cursorWidth - 1, cursorHeight - 1), B_RGBA32);
	
	if (cursorBitmap == NULL) {
		free(data);
		return B_NO_MEMORY;
	}
	status = cursorBitmap->InitCheck();
	if (status == B_OK)
		cursorBitmap->SetBits(data, size, 0, B_RGBA32);

	free(data);
	
	if (status == B_OK && bitmap != NULL)
		*bitmap = cursorBitmap;
	else
		delete cursorBitmap;
	
	return status;
}


void
set_accept_first_click(bool acceptFirstClick)
{
	BPrivate::AppServerLink link;
	link.StartMessage(AS_SET_ACCEPT_FIRST_CLICK);
	link.Attach<bool>(acceptFirstClick);
	link.Flush();
}


bool
accept_first_click()
{
	// Gets the accept first click status
	bool acceptFirstClick = false;

	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_ACCEPT_FIRST_CLICK);

	int32 code;
	if (link.FlushWithReply(code) == B_OK && code == B_OK)
		link.Read<bool>(&acceptFirstClick);

	return acceptFirstClick;
}


rgb_color
ui_color(color_which which)
{
	int32 index = color_which_to_index(which);
	if (index < 0 || index >= kNumColors) {
		fprintf(stderr, "ui_color(): unknown color_which %d\n", which);
		return make_color(0, 0, 0);
	}

	if (be_app) {
		server_read_only_memory* shared
			= BApplication::Private::ServerReadOnlyMemory();
		return shared->colors[index];
	}

	return kDefaultColors[index];
}


void
set_ui_color(const color_which &which, const rgb_color &color)
{
	int32 index = color_which_to_index(which);
	if (index < 0 || index >= kNumColors) {
		fprintf(stderr, "set_ui_color(): unknown color_which %d\n", which);
		return;
	}

	BPrivate::AppServerLink link;
	link.StartMessage(AS_SET_UI_COLOR);
	link.Attach<color_which>(which);
	link.Attach<rgb_color>(color);
	link.Flush();
}


rgb_color
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
	return tint_color(color, shift);
}


extern "C" status_t
_init_interface_kit_()
{
	status_t status = BPrivate::PaletteConverter::InitializeDefault(true);
	if (status < B_OK)
		return status;

	// init global clipboard
	if (be_clipboard == NULL)
		be_clipboard = new BClipboard(NULL);

	// TODO: Could support different themes here in the future.
	be_control_look = new BControlLook();

	_init_global_fonts_();

	BPrivate::gWidthBuffer = new BPrivate::WidthBuffer;
	status = BPrivate::MenuPrivate::CreateBitmaps();
	if (status != B_OK)
		return status;

	_menu_info_ptr_ = &BMenu::sMenuInfo;

	status = get_menu_info(&BMenu::sMenuInfo);
	if (status != B_OK)
		return status;

	general_info.background_color = ui_color(B_PANEL_BACKGROUND_COLOR);
	general_info.mark_color.set_to(0, 0, 0);
	general_info.highlight_color = ui_color(B_CONTROL_HIGHLIGHT_COLOR);
	general_info.window_frame_color = ui_color(B_WINDOW_TAB_COLOR);
	general_info.color_frame = true;

	// TODO: fill the other static members

	return status;
}


extern "C" status_t
_fini_interface_kit_()
{
	BPrivate::MenuPrivate::DeleteBitmaps();

	delete BPrivate::gWidthBuffer;
	BPrivate::gWidthBuffer = NULL;

	delete be_control_look;
	be_control_look = NULL;

	// TODO: Anything else?

	return B_OK;
}



namespace BPrivate {


/*!	\brief queries the server for the current decorator
	\param ref entry_ref into which to store current decorator's location
	\return boolean true/false
*/
bool
get_decorator(BString& path)
{
	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_DECORATOR);

	int32 code;
	if (link.FlushWithReply(code) != B_OK || code != B_OK)
		return false;

 	return link.ReadString(path) == B_OK;
}


/*!	\brief Private function which sets the window decorator for the system.
	\param entry_ref to the decorator to set

	Will return detailed error status via status_t
*/
status_t
set_decorator(const BString& path)
{
	BPrivate::AppServerLink link;

	link.StartMessage(AS_SET_DECORATOR);

	link.AttachString(path.String());
	link.Flush();

	status_t error = B_OK;
	link.Read<status_t>(&error);

	return error;
}


/*! \brief sets a window to preview a given decorator
	\param path path to any given decorator add-on
	\param window pointer to BWindow which will show decorator

	Piggy-backs on BWindow::SetDecoratorSettings(...)
*/
status_t
preview_decorator(const BString& path, BWindow* window)
{
	if (window == NULL)
		return B_ERROR;

	BMessage msg('prVu');
	msg.AddString("preview", path.String());

	return window->SetDecoratorSettings(msg);
}


status_t
get_application_order(int32 workspace, team_id** _applications,
	int32* _count)
{
	BPrivate::AppServerLink link;

	link.StartMessage(AS_GET_APPLICATION_ORDER);
	link.Attach<int32>(workspace);

	int32 code;
	status_t status = link.FlushWithReply(code);
	if (status != B_OK)
		return status;
	if (code != B_OK)
		return code;

	int32 count;
	link.Read<int32>(&count);

	*_applications = (team_id*)malloc(count * sizeof(team_id));
	if (*_applications == NULL)
		return B_NO_MEMORY;

	link.Read(*_applications, count * sizeof(team_id));
	*_count = count;
	return B_OK;
}


status_t
get_window_order(int32 workspace, int32** _tokens, int32* _count)
{
	BPrivate::AppServerLink link;

	link.StartMessage(AS_GET_WINDOW_ORDER);
	link.Attach<int32>(workspace);

	int32 code;
	status_t status = link.FlushWithReply(code);
	if (status != B_OK)
		return status;
	if (code != B_OK)
		return code;

	int32 count;
	link.Read<int32>(&count);

	*_tokens = (int32*)malloc(count * sizeof(int32));
	if (*_tokens == NULL)
		return B_NO_MEMORY;

	link.Read(*_tokens, count * sizeof(int32));
	*_count = count;
	return B_OK;
}


}	// namespace BPrivate

// These methods were marked with "Danger, will Robinson!" in
// the OpenTracker source, so we might not want to be compatible
// here.
// In any way, we would need to update Deskbar to use our
// replacements, so we could as well just implement them...

void
do_window_action(int32 windowToken, int32 action, BRect zoomRect, bool zoom)
{
	BPrivate::AppServerLink link;

	link.StartMessage(AS_WINDOW_ACTION);
	link.Attach<int32>(windowToken);
	link.Attach<int32>(action);
		// we don't have any zooming effect

	link.Flush();
}


client_window_info*
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


int32*
get_token_list(team_id team, int32* _count)
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


void
truncate_string(BString& string, uint32 mode, float width,
	const float* escapementArray, float fontSize, float ellipsisWidth,
	int32 charCount)
{
	// add a tiny amount to the width to make floating point inaccuracy
	// not drop chars that would actually fit exactly
	width += 0.00001;

	switch (mode) {
		case B_TRUNCATE_BEGINNING:
		{
			float totalWidth = 0;
			for (int32 i = charCount - 1; i >= 0; i--) {
				float charWidth = escapementArray[i] * fontSize;
				if (totalWidth + charWidth > width) {
					// we need to truncate
					while (totalWidth + ellipsisWidth > width) {
						// remove chars until there's enough space for the
						// ellipsis
						if (++i == charCount) {
							// we've reached the end of the string and still
							// no space, so return an empty string
							string.Truncate(0);
							return;
						}

						totalWidth -= escapementArray[i] * fontSize;
					}

					string.RemoveChars(0, i + 1);
					string.PrependChars(B_UTF8_ELLIPSIS, 1);
					return;
				}

				totalWidth += charWidth;
			}

			break;
		}

		case B_TRUNCATE_END:
		{
			float totalWidth = 0;
			for (int32 i = 0; i < charCount; i++) {
				float charWidth = escapementArray[i] * fontSize;
				if (totalWidth + charWidth > width) {
					// we need to truncate
					while (totalWidth + ellipsisWidth > width) {
						// remove chars until there's enough space for the
						// ellipsis
						if (i-- == 0) {
							// we've reached the start of the string and still
							// no space, so return an empty string
							string.Truncate(0);
							return;
						}

						totalWidth -= escapementArray[i] * fontSize;
					}

					string.RemoveChars(i, charCount - i);
					string.AppendChars(B_UTF8_ELLIPSIS, 1);
					return;
				}

				totalWidth += charWidth;
			}

			break;
		}

		case B_TRUNCATE_MIDDLE:
		case B_TRUNCATE_SMART:
		{
			float leftWidth = 0;
			float rightWidth = 0;
			int32 leftIndex = 0;
			int32 rightIndex = charCount - 1;
			bool left = true;

			for (int32 i = 0; i < charCount; i++) {
				float charWidth
					= escapementArray[left ? leftIndex : rightIndex] * fontSize;

				if (leftWidth + rightWidth + charWidth > width) {
					// we need to truncate
					while (leftWidth + rightWidth + ellipsisWidth > width) {
						// remove chars until there's enough space for the
						// ellipsis
						if (leftIndex == 0 && rightIndex == charCount - 1) {
							// we've reached both ends of the string and still
							// no space, so return an empty string
							string.Truncate(0);
							return;
						}

						if (leftIndex > 0 && (rightIndex == charCount - 1
								|| leftWidth > rightWidth)) {
							// remove char on the left
							leftWidth -= escapementArray[--leftIndex]
								* fontSize;
						} else {
							// remove char on the right
							rightWidth -= escapementArray[++rightIndex]
								* fontSize;
						}
					}

					string.RemoveChars(leftIndex, rightIndex + 1 - leftIndex);
					string.InsertChars(B_UTF8_ELLIPSIS, 1, leftIndex);
					return;
				}

				if (left) {
					leftIndex++;
					leftWidth += charWidth;
				} else {
					rightIndex--;
					rightWidth += charWidth;
				}

				left = rightWidth > leftWidth;
			}

			break;
		}
	}

	// we've run through without the need to truncate, leave the string as it is
}
