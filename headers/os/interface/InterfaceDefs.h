/*
 * Copyright 2001-2008, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_INTERFACE_DEFS_H
#define	_INTERFACE_DEFS_H


#include <GraphicsDefs.h>
#include <OS.h>


class BBitmap;
class BPoint;
class BRect;


// some handy UTF-8 characters

#define B_UTF8_BULLET		"\xE2\x80\xA2"
#define B_UTF8_ELLIPSIS		"\xE2\x80\xA6"
#define B_UTF8_OPEN_QUOTE	"\xE2\x80\x9C"
#define B_UTF8_CLOSE_QUOTE	"\xE2\x80\x9D"
#define B_UTF8_COPYRIGHT	"\xC2\xA9"
#define B_UTF8_REGISTERED	"\xC2\xAE"
#define B_UTF8_TRADEMARK	"\xE2\x84\xA2"
#define B_UTF8_SMILING_FACE	"\xE2\x98\xBB"
#define B_UTF8_HIROSHI		"\xE5\xBC\x98"
#ifdef COMPILE_FOR_R5
	#define B_MAX_MOUSE_BUTTONS 3
#else
	#define B_MAX_MOUSE_BUTTONS 16
#endif


// Key definitions

struct key_info {
	uint32	modifiers;
	uint8	key_states[16];
};

enum {
	B_BACKSPACE			= 0x08,
	B_RETURN			= 0x0a,
	B_ENTER				= 0x0a,
	B_SPACE				= 0x20,
	B_TAB				= 0x09,
	B_ESCAPE			= 0x1b,
	B_SUBSTITUTE		= 0x1a,

	B_LEFT_ARROW		= 0x1c,
	B_RIGHT_ARROW		= 0x1d,
	B_UP_ARROW			= 0x1e,
	B_DOWN_ARROW		= 0x1f,

	B_INSERT			= 0x05,
	B_DELETE			= 0x7f,
	B_HOME				= 0x01,
	B_END				= 0x04,
	B_PAGE_UP			= 0x0b,
	B_PAGE_DOWN			= 0x0c,

	B_FUNCTION_KEY		= 0x10,

	// for Japanese keyboards
	B_KATAKANA_HIRAGANA	= 0xf2,
	B_HANKAKU_ZENKAKU	= 0xf3
};

enum {
	B_F1_KEY			= 0x02,
	B_F2_KEY			= 0x03,
	B_F3_KEY			= 0x04,
	B_F4_KEY			= 0x05,
	B_F5_KEY			= 0x06,
	B_F6_KEY			= 0x07,
	B_F7_KEY			= 0x08,
	B_F8_KEY			= 0x09,
	B_F9_KEY			= 0x0a,
	B_F10_KEY			= 0x0b,
	B_F11_KEY			= 0x0c,
	B_F12_KEY			= 0x0d,
	B_PRINT_KEY			= 0x0e,
	B_SCROLL_KEY		= 0x0f,
	B_PAUSE_KEY			= 0x10
};

struct key_map {
	uint32	version;
	uint32	caps_key;
	uint32	scroll_key;
	uint32	num_key;
	uint32	left_shift_key;
	uint32	right_shift_key;
	uint32	left_command_key;
	uint32	right_command_key;
	uint32	left_control_key;
	uint32	right_control_key;
	uint32	left_option_key;
	uint32	right_option_key;
	uint32	menu_key;
	uint32	lock_settings;
	int32	control_map[128];
	int32	option_caps_shift_map[128];
	int32	option_caps_map[128];
	int32	option_shift_map[128];
	int32	option_map[128];
	int32	caps_shift_map[128];
	int32	caps_map[128];
	int32	shift_map[128];
	int32	normal_map[128];
	int32	acute_dead_key[32];
	int32	grave_dead_key[32];
	int32	circumflex_dead_key[32];
	int32	dieresis_dead_key[32];
	int32	tilde_dead_key[32];
	uint32	acute_tables;
	uint32	grave_tables;
	uint32	circumflex_tables;
	uint32	dieresis_tables;
	uint32	tilde_tables;
};

enum {
	B_CONTROL_TABLE				= 0x00000001,
	B_OPTION_CAPS_SHIFT_TABLE	= 0x00000002,
	B_OPTION_CAPS_TABLE			= 0x00000004,
	B_OPTION_SHIFT_TABLE		= 0x00000008,
	B_OPTION_TABLE				= 0x00000010,
	B_CAPS_SHIFT_TABLE			= 0x00000020,
	B_CAPS_TABLE				= 0x00000040,
	B_SHIFT_TABLE				= 0x00000080,
	B_NORMAL_TABLE				= 0x00000100
};

// modifiers
enum {
	B_SHIFT_KEY			= 0x00000001,
	B_COMMAND_KEY		= 0x00000002,
	B_CONTROL_KEY		= 0x00000004,
	B_CAPS_LOCK			= 0x00000008,
	B_SCROLL_LOCK		= 0x00000010,
	B_NUM_LOCK			= 0x00000020,
	B_OPTION_KEY		= 0x00000040,
	B_MENU_KEY			= 0x00000080,
	B_LEFT_SHIFT_KEY	= 0x00000100,
	B_RIGHT_SHIFT_KEY	= 0x00000200,
	B_LEFT_COMMAND_KEY	= 0x00000400,
	B_RIGHT_COMMAND_KEY	= 0x00000800,
	B_LEFT_CONTROL_KEY	= 0x00001000,
	B_RIGHT_CONTROL_KEY	= 0x00002000,
	B_LEFT_OPTION_KEY	= 0x00004000,
	B_RIGHT_OPTION_KEY	= 0x00008000
};


// Mouse definitions

struct mouse_map {
	uint32	button[B_MAX_MOUSE_BUTTONS];
};

enum mode_mouse {
	B_NORMAL_MOUSE 			= 0,
	B_CLICK_TO_FOCUS_MOUSE	= -1,
	B_FOCUS_FOLLOWS_MOUSE	= 1
};

enum mode_focus_follows_mouse {
	B_NORMAL_FOCUS_FOLLOWS_MOUSE		= 0,
	B_WARP_FOCUS_FOLLOWS_MOUSE			= 1,
	B_INSTANT_WARP_FOCUS_FOLLOWS_MOUSE	= 2
};


// View orientation/alignment/style

enum border_style {
	B_PLAIN_BORDER,
	B_FANCY_BORDER,
	B_NO_BORDER
};

enum orientation {
	B_HORIZONTAL,
	B_VERTICAL
};

enum button_width {
	B_WIDTH_AS_USUAL,
	B_WIDTH_FROM_WIDEST,
	B_WIDTH_FROM_LABEL
};

struct scroll_bar_info {
	bool	proportional;
	bool	double_arrows;
	int32	knob;
	int32	min_knob_size;
};

enum alignment {
	B_ALIGN_LEFT,
	B_ALIGN_RIGHT,
	B_ALIGN_CENTER,

	B_ALIGN_HORIZONTAL_CENTER	= B_ALIGN_CENTER,

	B_ALIGN_HORIZONTAL_UNSET	= -1L,
	B_ALIGN_USE_FULL_WIDTH		= -2L
};

enum vertical_alignment {
	B_ALIGN_TOP					= 0x10L,
	B_ALIGN_MIDDLE				= 0x20,
	B_ALIGN_BOTTOM				= 0x30,

	B_ALIGN_VERTICAL_CENTER		= B_ALIGN_MIDDLE,

	B_ALIGN_VERTICAL_UNSET		= -1L,
	B_ALIGN_NO_VERTICAL			= B_ALIGN_VERTICAL_UNSET,
	B_ALIGN_USE_FULL_HEIGHT		= -2L
};


// Layout spacing and insets, see BControlLook::ComposeSpacing()


enum {
	B_USE_DEFAULT_SPACING = -2,
	B_USE_ITEM_SPACING = -3,
	B_USE_ITEM_INSETS = -3,
	B_USE_HALF_ITEM_SPACING = -4,
	B_USE_HALF_ITEM_INSETS = -4,
	B_USE_WINDOW_INSETS = -5,
	B_USE_WINDOW_SPACING = -5,
	B_USE_SMALL_INSETS = -6,
	B_USE_SMALL_SPACING = -6,
	B_USE_BIG_INSETS = -7,
	B_USE_BIG_SPACING = -7
};


// Line join and cap modes

enum join_mode {
	B_ROUND_JOIN = 0,
	B_MITER_JOIN,
	B_BEVEL_JOIN,
	B_BUTT_JOIN,
	B_SQUARE_JOIN
};

enum cap_mode {
	B_ROUND_CAP		= B_ROUND_JOIN,
	B_BUTT_CAP		= B_BUTT_JOIN,
	B_SQUARE_CAP	= B_SQUARE_JOIN
};

const float B_DEFAULT_MITER_LIMIT = 10.0F;


// Bitmap and overlay constants

enum bitmap_tiling {
	B_TILE_BITMAP_X				= 0x00000001,
	B_TILE_BITMAP_Y				= 0x00000002,
	B_TILE_BITMAP				= 0x00000003
};

enum overlay_options {
	B_OVERLAY_FILTER_HORIZONTAL	= 0x00010000,
	B_OVERLAY_FILTER_VERTICAL	= 0x00020000,
	B_OVERLAY_MIRROR			= 0x00040000,
	B_OVERLAY_TRANSFER_CHANNEL	= 0x00080000
};

enum bitmap_drawing_options {
	B_FILTER_BITMAP_BILINEAR	= 0x00000100,

	B_WAIT_FOR_RETRACE			= 0x00000800
};


// Default UI Colors

enum color_which {
	B_PANEL_BACKGROUND_COLOR = 1,
	B_PANEL_TEXT_COLOR = 10,
	B_DOCUMENT_BACKGROUND_COLOR = 11,
	B_DOCUMENT_TEXT_COLOR = 12,
	B_CONTROL_BACKGROUND_COLOR = 13,
	B_CONTROL_TEXT_COLOR = 14,
	B_CONTROL_BORDER_COLOR = 15,
	B_CONTROL_HIGHLIGHT_COLOR = 16,
	B_NAVIGATION_BASE_COLOR = 4,
	B_NAVIGATION_PULSE_COLOR = 17,
	B_SHINE_COLOR = 18,
	B_SHADOW_COLOR = 19,

	B_MENU_BACKGROUND_COLOR = 2,
	B_MENU_SELECTED_BACKGROUND_COLOR = 6,
	B_MENU_ITEM_TEXT_COLOR = 7,
	B_MENU_SELECTED_ITEM_TEXT_COLOR = 8,
	B_MENU_SELECTED_BORDER_COLOR = 9,

	B_TOOL_TIP_BACKGROUND_COLOR = 20,
	B_TOOL_TIP_TEXT_COLOR = 21,

	B_SUCCESS_COLOR = 100,
	B_FAILURE_COLOR = 101,

	// Old name synonyms.
	B_KEYBOARD_NAVIGATION_COLOR = B_NAVIGATION_BASE_COLOR,
	B_MENU_SELECTION_BACKGROUND_COLOR = B_MENU_SELECTED_BACKGROUND_COLOR,

	// These are deprecated -- do not use in new code.  See BScreen for
	// the replacement for B_DESKTOP_COLOR.
	B_DESKTOP_COLOR = 5,

	B_WINDOW_TAB_COLOR = 3,
	B_WINDOW_TEXT_COLOR = 22,
	B_WINDOW_INACTIVE_TAB_COLOR = 23,
	B_WINDOW_INACTIVE_TEXT_COLOR = 24
};


// Color tinting

const float B_LIGHTEN_MAX_TINT	= 0.0f;		// 216 --> 255.0 (255)
const float B_LIGHTEN_2_TINT	= 0.385f;	// 216 --> 240.0 (240)
const float B_LIGHTEN_1_TINT	= 0.590f;	// 216 --> 232.0 (232)

const float B_NO_TINT			= 1.0f;		// 216 --> 216.0 (216)

const float B_DARKEN_1_TINT		= 1.147f;	// 216 --> 184.2 (184)
const float B_DARKEN_2_TINT		= 1.295f;	// 216 --> 152.3 (152)
const float B_DARKEN_3_TINT		= 1.407f;	// 216 --> 128.1 (128)
const float B_DARKEN_4_TINT		= 1.555f;	// 216 -->  96.1  (96)
const float B_DARKEN_MAX_TINT	= 2.0f;		// 216 -->   0.0   (0)
											// effects on standard gray level

const float B_DISABLED_LABEL_TINT		= B_DARKEN_3_TINT;
const float B_HIGHLIGHT_BACKGROUND_TINT	= B_DARKEN_2_TINT;
const float B_DISABLED_MARK_TINT		= B_LIGHTEN_2_TINT;


status_t		get_deskbar_frame(BRect* frame);

const color_map* system_colors();

status_t		set_screen_space(int32 index, uint32 resolution,
					bool save = true);

status_t		get_scroll_bar_info(scroll_bar_info* info);
status_t		set_scroll_bar_info(scroll_bar_info* info);

status_t		get_mouse_type(int32* type);
status_t		set_mouse_type(int32 type);
status_t		get_mouse_map(mouse_map* map);
status_t		set_mouse_map(mouse_map* map);
status_t		get_click_speed(bigtime_t* speed);
status_t		set_click_speed(bigtime_t speed);
status_t		get_mouse_speed(int32* speed);
status_t		set_mouse_speed(int32 speed);
status_t		get_mouse_acceleration(int32* speed);
status_t		set_mouse_acceleration(int32 speed);

status_t		get_key_repeat_rate(int32* rate);
status_t		set_key_repeat_rate(int32 rate);
status_t		get_key_repeat_delay(bigtime_t* delay);
status_t		set_key_repeat_delay(bigtime_t delay);

uint32			modifiers();
status_t		get_key_info(key_info* info);
void			get_key_map(key_map** _map, char** _keyBuffer);
status_t		get_keyboard_id(uint16* _id);
status_t		get_modifier_key(uint32 modifier, uint32 *key);
void			set_modifier_key(uint32 modifier, uint32 key);
void			set_keyboard_locks(uint32 modifiers);

rgb_color		keyboard_navigation_color();

int32			count_workspaces();
void			set_workspace_count(int32 count);
int32			current_workspace();
void			activate_workspace(int32 workspace);

bigtime_t		idle_time();

void			run_select_printer_panel();
void			run_add_printer_panel();
void			run_be_about();

void			set_focus_follows_mouse(bool follow);
bool			focus_follows_mouse();

void			set_mouse_mode(mode_mouse mode);
mode_mouse		mouse_mode();

void			set_focus_follows_mouse_mode(mode_focus_follows_mouse mode);
mode_focus_follows_mouse	focus_follows_mouse_mode();

status_t		get_mouse(BPoint* screenWhere, uint32* buttons);
status_t		get_mouse_bitmap(BBitmap** bitmap, BPoint* hotspot);

void			set_accept_first_click(bool acceptFirstClick);
bool			accept_first_click();

rgb_color		ui_color(color_which which);
void			set_ui_color(const color_which& which, const rgb_color& color);
rgb_color		tint_color(rgb_color color, float tint);

extern "C" status_t _init_interface_kit_();
	// for convenience, should be removed including the friend declarations
	// in Menu.h, ...

#endif	// _INTERFACE_DEFS_H
