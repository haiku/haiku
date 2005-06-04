//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005, Haiku
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
//	File Name:		InterfaceDefs.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//					Caz <turok2@currantbun.com>
//					Axel Dörfler <axeld@pinc-software.de>
//	Description:	Global functions and variables for the Interface Kit
//
//------------------------------------------------------------------------------
#include <InterfaceDefs.h>
#include <Menu.h>
#include <Roster.h>
#include <ScrollBar.h>
#include <Screen.h>
#include <TextView.h>

#include <stdlib.h>
// TODO: remove this header
#include <stdio.h>

// Private definitions not placed in public headers
extern "C" void _init_global_fonts();
extern "C" status_t _fini_interface_kit_();

#include <AppServerLink.h>
#include <InputServerTypes.h>
#include <input_globals.h>
#include <ServerProtocol.h>
#include <WidthBuffer.h>

#include <string.h>
#include <Font.h>
#include "moreUTF8.h"
#include "truncate_string.h"

using namespace BPrivate;


_IMPEXP_BE const color_map *
system_colors()
{
	return BScreen(B_MAIN_SCREEN_ID).ColorMap();
}

#ifndef COMPILE_FOR_R5

_IMPEXP_BE status_t
set_screen_space(int32 index, uint32 res, bool stick)
{
	BAppServerLink link;
	int32 code = SERVER_FALSE;
	
	link.StartMessage(AS_SET_SCREEN_MODE);
	link.Attach<int32>(index);
	link.Attach<int32>((int32)res);
	link.Attach<bool>(stick);
	link.FlushWithReply(&code);

	return ((code==SERVER_TRUE)?B_OK:B_ERROR);
}


_IMPEXP_BE status_t
get_scroll_bar_info(scroll_bar_info *info)
{
	if (info == NULL)
		return B_BAD_VALUE;

	BAppServerLink link;
	int32 code;
	link.StartMessage(AS_GET_SCROLLBAR_INFO);
	link.FlushWithReply(&code);
	if(code==SERVER_TRUE)
		link.Read<scroll_bar_info>(info);
	
	return ((code==SERVER_TRUE)?B_OK:B_ERROR);
}


_IMPEXP_BE status_t
set_scroll_bar_info(scroll_bar_info *info)
{
	if (info == NULL)
		return B_BAD_VALUE;

	BAppServerLink link;
	int32 code;
	
	link.StartMessage(AS_SET_SCROLLBAR_INFO);
	link.Attach<scroll_bar_info>(*info);
	link.FlushWithReply(&code);
	return ((code==SERVER_TRUE)?B_OK:B_ERROR);
}

#endif // COMPILE_FOR_R5

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
	
	if(reply.FindData("mousemap", B_ANY_TYPE, &data, &count) != B_OK)
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
	
	if(reply.FindInt64("speed", speed) != B_OK)
		return B_ERROR;
	
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
	
	if(reply.FindInt32("speed", speed) != B_OK)
		return B_ERROR;
	
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
	
	if(reply.FindInt32("speed", speed) != B_OK)
		return B_ERROR;
	
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
	
	if(reply.FindInt32("rate", rate) != B_OK)
		return B_ERROR;
	
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
	
	if(reply.FindInt64("delay", delay) != B_OK)
		return B_ERROR;
	
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
	
	if(reply.FindInt32("status", &err) != B_OK)
		return 0;
	
	if(reply.FindInt32("modifiers", &modifier) != B_OK)
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
	BMessage command(IS_GET_KEY_MAP);
	BMessage reply;
	ssize_t map_count, key_count;
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


#ifndef COMPILE_FOR_R5

_IMPEXP_BE int32
count_workspaces()
{
	int32 count=1;
	
	BAppServerLink link;
	int32 code;
	link.StartMessage(AS_COUNT_WORKSPACES);
	link.FlushWithReply(&code);
	if(code==SERVER_TRUE)
		link.Read<int32>(&count);
	return count;
}


_IMPEXP_BE void
set_workspace_count(int32 count)
{
	BAppServerLink link;
	link.StartMessage(AS_SET_WORKSPACE_COUNT);
	link.Attach<int32>(count);
	link.Flush();
}


_IMPEXP_BE int32
current_workspace()
{
	int32 index = 1;
	
	BAppServerLink link;
	int32 code;
	link.StartMessage(AS_CURRENT_WORKSPACE);
	link.FlushWithReply(&code);
	if(code==SERVER_TRUE)
		link.Read<int32>(&index);
	
	return index;
}


_IMPEXP_BE void
activate_workspace(int32 workspace)
{
	BAppServerLink link;
	link.StartMessage(AS_ACTIVATE_WORKSPACE);
	link.Attach<int32>(workspace);
	link.Flush();
}


_IMPEXP_BE bigtime_t
idle_time()
{
	bigtime_t idletime = 0;
	
	BAppServerLink link;
	int32 code;
	link.StartMessage(AS_IDLE_TIME);
	link.FlushWithReply(&code);
	if(code==SERVER_TRUE)
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
	// Unsure about how to implement this.
	// TODO: Implement
}


_IMPEXP_BE void
set_focus_follows_mouse(bool follow)
{
	BAppServerLink link;
	link.StartMessage(AS_SET_FOCUS_FOLLOWS_MOUSE);
	link.Attach<bool>(follow);
	link.Flush();
}


_IMPEXP_BE bool
focus_follows_mouse()
{
	bool ffm = false;
	
	BAppServerLink link;
	int32 code;
	link.StartMessage(AS_FOCUS_FOLLOWS_MOUSE);
	link.FlushWithReply(&code);
	if(code==SERVER_TRUE)
		link.Read<bool>(&ffm);
	return ffm;
}


_IMPEXP_BE void
set_mouse_mode(mode_mouse mode)
{
	BAppServerLink link;
	link.StartMessage(AS_SET_MOUSE_MODE);
	link.Attach<mode_mouse>(mode);
	link.Flush();
}


_IMPEXP_BE mode_mouse
mouse_mode()
{
	mode_mouse mode;
	
	BAppServerLink link;
	int32 code;
	link.StartMessage(AS_GET_MOUSE_MODE);
	link.FlushWithReply(&code);
	if(code==SERVER_TRUE)
		link.Read<mode_mouse>(&mode);
	return mode;
}


_IMPEXP_BE rgb_color
ui_color(color_which which)
{
	rgb_color color;
	
	BAppServerLink link;
	int32 code;
	link.StartMessage(AS_GET_UI_COLOR);
	link.Attach<color_which>(which);
	link.FlushWithReply(&code);
	if(code==SERVER_TRUE)
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


static status_t
load_menu_settings(menu_info &into)
{
	// TODO: Load settings from the settings file,
	// and only if it fails, fallback to the defaults

	into.font_size = be_plain_font->Size();
	be_plain_font->GetFamilyAndStyle(&into.f_family, &into.f_style);
	into.background_color = ui_color(B_MENU_BACKGROUND_COLOR);
	into.separator = 0;
	into.click_to_open = false;
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
	
	_init_global_fonts();

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
	BAppServerLink link;
	link.StartMessage(AS_R5_SET_DECORATOR);
	link.Attach<int32>(theme);
	link.Flush();
}


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

menu_info *_menu_info_ptr_ = NULL;

void 
do_window_action(int32 window_id, int32 action, 
	BRect zoomRect, bool zoom)
{
	// ToDo: implement me, needed for Deskbar!
}


window_info	*
get_window_info(int32 a_token)
{
	// ToDo: implement me, needed for Deskbar!
	return NULL;
}


int32 *
get_token_list(team_id app, int32 *count)
{
	// ToDo: implement me, needed for Deskbar!
	return NULL;
}


void
do_bring_to_front_team(BRect zoomRect, team_id app, bool zoom)
{
	// ToDo: implement me, needed for Deskbar!
}


void
do_minimize_team(BRect zoomRect, team_id team, bool zoom)
{
	// ToDo: implement me, needed for Deskbar!
}

// copy_from_start
static char*
copy_from_start(const char* src, char* dst, uint32 numChars,
				const float* escapementArray, float width, float ellipsisWidth, float size)
{
//printf("copy_from_start: %.1f\n", width);
	float currentWidth = 0.0;
	for (uint32 c = 0; c < numChars; c++) {
//printf("forward: %c (%ld) (%.1f + %.1f = %.1f)\n", *src, c, currentWidth, escapementArray[c] * size, currentWidth + escapementArray[c] * size);
		currentWidth += escapementArray[c] * size;
		if (currentWidth > width) {
//*dst = *src;
			// ups, we definitely don't fit. go back until the ellipsis fits
			currentWidth += ellipsisWidth;
			for (int32 c2 = c; c2 >= 0; c2--) {
//printf(" backward: %c (%ld) (%.1f - %.1f = %.1f)\n", *dst, c2, currentWidth, escapementArray[c2] * size, currentWidth - escapementArray[c2] * size);
				currentWidth -= escapementArray[c2] * size;
				do {
					dst--;
				} while (IsInsideGlyph(*dst));
				// see if we went back enough
				if (currentWidth <= width)
					break;
			}
			// dst needs to point behind the last glyph we want to use
			dst++;
			break;
		} else {
			// copy one glyph
			do {
				*dst++ = *src++;
			} while (IsInsideGlyph(*src));
		}
	}
	return dst;
}

// copy_from_end
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
			for (int32 c2 = c; c2 < (int32)numChars; c2++) {
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

static char*
write_ellipsis(char* dst)
{
	strcpy(dst, B_UTF8_ELLIPSIS);
//strcpy(dst, "...");
	return dst + 3;
}

// truncated_string
void
truncate_string(const char* string,
				uint32 mode, float width, char* result,
				const float* escapementArray, float fontSize,
				float ellipsisWidth, int32 length, int32 numChars)
{
	if (string) {
		// skip calculation if we don't even have enough room for that
		if (width >= ellipsisWidth) {

			// iterate over glyphs and copy source into result string
			// one glyph at a time as long as we have room for the "…" yet
			char* dst = result;
			const char* src = string;

			switch (mode) {
				case B_TRUNCATE_BEGINNING: {

					dst = copy_from_end(src, dst, numChars, length,
										escapementArray, width, ellipsisWidth, fontSize);
					// "dst" points to the position behind the last glyph that
					// was copied.
					int32 dist = dst - result;
					// we didn't terminate yet
					*dst = 0;
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

					dst = copy_from_start(src, dst, numChars,
										  escapementArray, width, ellipsisWidth, fontSize);
					// "dst" points to the position behind the last glyph that
					// was copied.
					if (dst - result < length) {
						// append "…" and terminate with 0
						write_ellipsis(dst);
					} else {
						*dst = 0;
					}
					break;

				case B_TRUNCATE_SMART:
					// TODO: implement, though it was never implemented on R5
					// FALL THROUGH (at least do something)
				case B_TRUNCATE_MIDDLE:

					float halfWidth = width / 2.0;

					dst = copy_from_start(src, dst, numChars,
										  escapementArray, halfWidth, ellipsisWidth, fontSize);
					// insert "…"
					dst = write_ellipsis(dst);

					dst = copy_from_end(src, dst, numChars, length,
										escapementArray, halfWidth, 0.0, fontSize);
					// terminate
					*dst = 0;
					break;
			}
		} else {
			// we don't have room for a single glyph
			strcpy(result, "");
		}
	}
}


#endif // !COMPILE_FOR_R5
