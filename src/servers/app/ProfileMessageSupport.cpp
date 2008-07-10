/*
 * Copyright 2007-2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include "ProfileMessageSupport.h"

#include <ServerProtocol.h>


void
string_for_message_code(uint32 code, BString& string)
{
	string = "";

	switch (code) {
		case AS_GET_DESKTOP: string = "AS_GET_DESKTOP"; break;
		case AS_REGISTER_INPUT_SERVER: string = "AS_REGISTER_INPUT_SERVER"; break;
		case AS_EVENT_STREAM_CLOSED: string = "AS_EVENT_STREAM_CLOSED"; break;

		// Desktop definitions (through the ServerApp, though)
		case AS_GET_WINDOW_LIST: string = "AS_GET_WINDOW_LIST"; break;
		case AS_GET_WINDOW_INFO: string = "AS_GET_WINDOW_INFO"; break;
		case AS_MINIMIZE_TEAM: string = "AS_MINIMIZE_TEAM"; break;
		case AS_BRING_TEAM_TO_FRONT: string = "AS_BRING_TEAM_TO_FRONT"; break;
		case AS_WINDOW_ACTION: string = "AS_WINDOW_ACTION"; break;

		// Application definitions
		case AS_CREATE_APP: string = "AS_CREATE_APP"; break;
		case AS_DELETE_APP: string = "AS_DELETE_APP"; break;
		case AS_QUIT_APP: string = "AS_QUIT_APP"; break;
		case AS_ACTIVATE_APP: string = "AS_ACTIVATE_APP"; break;
		case AS_APP_CRASHED: string = "AS_APP_CRASHED"; break;

		case AS_CREATE_WINDOW: string = "AS_CREATE_WINDOW"; break;
		case AS_CREATE_OFFSCREEN_WINDOW: string = "AS_CREATE_OFFSCREEN_WINDOW"; break;
		case AS_DELETE_WINDOW: string = "AS_DELETE_WINDOW"; break;
		case AS_CREATE_BITMAP: string = "AS_CREATE_BITMAP"; break;
		case AS_DELETE_BITMAP: string = "AS_DELETE_BITMAP"; break;
		case AS_GET_BITMAP_OVERLAY_RESTRICTIONS: string = "AS_GET_BITMAP_OVERLAY_RESTRICTIONS"; break;

		// Cursor commands
		case AS_SET_CURSOR: string = "AS_SET_CURSOR"; break;

		case AS_SHOW_CURSOR: string = "AS_SHOW_CURSOR"; break;
		case AS_HIDE_CURSOR: string = "AS_HIDE_CURSOR"; break;
		case AS_OBSCURE_CURSOR: string = "AS_OBSCURE_CURSOR"; break;
		case AS_QUERY_CURSOR_HIDDEN: string = "AS_QUERY_CURSOR_HIDDEN"; break;

		case AS_CREATE_CURSOR: string = "AS_CREATE_CURSOR"; break;
		case AS_DELETE_CURSOR: string = "AS_DELETE_CURSOR"; break;

		case AS_BEGIN_RECT_TRACKING: string = "AS_BEGIN_RECT_TRACKING"; break;
		case AS_END_RECT_TRACKING: string = "AS_END_RECT_TRACKING"; break;

		// Window definitions
		case AS_SHOW_WINDOW: string = "AS_SHOW_WINDOW"; break;
		case AS_HIDE_WINDOW: string = "AS_HIDE_WINDOW"; break;
		case AS_MINIMIZE_WINDOW: string = "AS_MINIMIZE_WINDOW"; break;
		case AS_QUIT_WINDOW: string = "AS_QUIT_WINDOW"; break;
		case AS_SEND_BEHIND: string = "AS_SEND_BEHIND"; break;
		case AS_SET_LOOK: string = "AS_SET_LOOK"; break;
		case AS_SET_FEEL: string = "AS_SET_FEEL"; break; 
		case AS_SET_FLAGS: string = "AS_SET_FLAGS"; break;
		case AS_DISABLE_UPDATES: string = "AS_DISABLE_UPDATES"; break;
		case AS_ENABLE_UPDATES: string = "AS_ENABLE_UPDATES"; break;
		case AS_BEGIN_UPDATE: string = "AS_BEGIN_UPDATE"; break;
		case AS_END_UPDATE: string = "AS_END_UPDATE"; break;
		case AS_NEEDS_UPDATE: string = "AS_NEEDS_UPDATE"; break;
		case AS_SET_WINDOW_TITLE: string = "AS_SET_WINDOW_TITLE"; break;
		case AS_ADD_TO_SUBSET: string = "AS_ADD_TO_SUBSET"; break;
		case AS_REMOVE_FROM_SUBSET: string = "AS_REMOVE_FROM_SUBSET"; break;
		case AS_SET_ALIGNMENT: string = "AS_SET_ALIGNMENT"; break;
		case AS_GET_ALIGNMENT: string = "AS_GET_ALIGNMENT"; break;
		case AS_GET_WORKSPACES: string = "AS_GET_WORKSPACES"; break;
		case AS_SET_WORKSPACES: string = "AS_SET_WORKSPACES"; break;
		case AS_WINDOW_RESIZE: string = "AS_WINDOW_RESIZE"; break;
		case AS_WINDOW_MOVE: string = "AS_WINDOW_MOVE"; break;
		case AS_SET_SIZE_LIMITS: string = "AS_SET_SIZE_LIMITS"; break;
		case AS_ACTIVATE_WINDOW: string = "AS_ACTIVATE_WINDOW"; break;
		case AS_IS_FRONT_WINDOW: string = "AS_IS_FRONT_WINDOW"; break;

		// BPicture definitions
		case AS_CREATE_PICTURE: string = "AS_CREATE_PICTURE"; break;
		case AS_DELETE_PICTURE: string = "AS_DELETE_PICTURE"; break;
		case AS_CLONE_PICTURE: string = "AS_CLONE_PICTURE"; break;
		case AS_DOWNLOAD_PICTURE: string = "AS_DOWNLOAD_PICTURE"; break;

		// Font-related server communications
		case AS_SET_SYSTEM_FONT: string = "AS_SET_SYSTEM_FONT"; break;
		case AS_GET_SYSTEM_FONTS: string = "AS_GET_SYSTEM_FONTS"; break;
		case AS_GET_SYSTEM_DEFAULT_FONT: string = "AS_GET_SYSTEM_DEFAULT_FONT"; break;

		case AS_GET_FONT_LIST_REVISION: string = "AS_GET_FONT_LIST_REVISION"; break;
		case AS_GET_FAMILY_AND_STYLES: string = "AS_GET_FAMILY_AND_STYLES"; break;

		case AS_GET_FAMILY_AND_STYLE: string = "AS_GET_FAMILY_AND_STYLE"; break;
		case AS_GET_FAMILY_AND_STYLE_IDS: string = "AS_GET_FAMILY_AND_STYLE_IDS"; break;
		case AS_GET_FONT_BOUNDING_BOX: string = "AS_GET_FONT_BOUNDING_BOX"; break;
		case AS_GET_TUNED_COUNT: string = "AS_GET_TUNED_COUNT"; break;
		case AS_GET_TUNED_INFO: string = "AS_GET_TUNED_INFO"; break;
		case AS_GET_FONT_HEIGHT: string = "AS_GET_FONT_HEIGHT"; break;
		case AS_GET_FONT_FILE_FORMAT: string = "AS_GET_FONT_FILE_FORMAT"; break;
		case AS_GET_EXTRA_FONT_FLAGS: string = "AS_GET_EXTRA_FONT_FLAGS"; break;

		case AS_GET_STRING_WIDTHS: string = "AS_GET_STRING_WIDTHS"; break;
		case AS_GET_EDGES: string = "AS_GET_EDGES"; break;
		case AS_GET_ESCAPEMENTS: string = "AS_GET_ESCAPEMENTS"; break;
		case AS_GET_ESCAPEMENTS_AS_FLOATS: string = "AS_GET_ESCAPEMENTS_AS_FLOATS"; break;
		case AS_GET_BOUNDINGBOXES_CHARS: string = "AS_GET_BOUNDINGBOXES_CHARS"; break;
		case AS_GET_BOUNDINGBOXES_STRING: string = "AS_GET_BOUNDINGBOXES_STRING"; break;
		case AS_GET_BOUNDINGBOXES_STRINGS: string = "AS_GET_BOUNDINGBOXES_STRINGS"; break;
		case AS_GET_HAS_GLYPHS: string = "AS_GET_HAS_GLYPHS"; break;
		case AS_GET_GLYPH_SHAPES: string = "AS_GET_GLYPH_SHAPES"; break;
		case AS_GET_TRUNCATED_STRINGS: string = "AS_GET_TRUNCATED_STRINGS"; break;

		// Screen methods
		case AS_VALID_SCREEN_ID: string = "AS_VALID_SCREEN_ID"; break;
		case AS_GET_NEXT_SCREEN_ID: string = "AS_GET_NEXT_SCREEN_ID"; break;
		case AS_SCREEN_GET_MODE: string = "AS_SCREEN_GET_MODE"; break;
		case AS_SCREEN_SET_MODE: string = "AS_SCREEN_SET_MODE"; break;
		case AS_PROPOSE_MODE: string = "AS_PROPOSE_MODE"; break;
		case AS_GET_MODE_LIST: string = "AS_GET_MODE_LIST"; break;

		case AS_GET_PIXEL_CLOCK_LIMITS: string = "AS_GET_PIXEL_CLOCK_LIMITS"; break;
		case AS_GET_TIMING_CONSTRAINTS: string = "AS_GET_TIMING_CONSTRAINTS"; break;

		case AS_SCREEN_GET_COLORMAP: string = "AS_SCREEN_GET_COLORMAP"; break;
		case AS_GET_DESKTOP_COLOR: string = "AS_GET_DESKTOP_COLOR"; break;
		case AS_SET_DESKTOP_COLOR: string = "AS_SET_DESKTOP_COLOR"; break;
		case AS_GET_SCREEN_ID_FROM_WINDOW: string = "AS_GET_SCREEN_ID_FROM_WINDOW"; break;

		case AS_READ_BITMAP: string = "AS_READ_BITMAP"; break;

		case AS_GET_RETRACE_SEMAPHORE: string = "AS_GET_RETRACE_SEMAPHORE"; break;
		case AS_GET_ACCELERANT_INFO: string = "AS_GET_ACCELERANT_INFO"; break;
		case AS_GET_MONITOR_INFO: string = "AS_GET_MONITOR_INFO"; break;
		case AS_GET_FRAME_BUFFER_CONFIG: string = "AS_GET_FRAME_BUFFER_CONFIG"; break;
	
		case AS_SET_DPMS: string = "AS_SET_DPMS"; break;
		case AS_GET_DPMS_STATE: string = "AS_GET_DPMS_STATE"; break;
		case AS_GET_DPMS_CAPABILITIES: string = "AS_GET_DPMS_CAPABILITIES"; break;

		// Misc stuff
		case AS_GET_ACCELERANT_PATH: string = "AS_GET_ACCELERANT_PATH"; break;
		case AS_GET_DRIVER_PATH: string = "AS_GET_DRIVER_PATH"; break;

		// Global function call defs
		case AS_SET_UI_COLORS: string = "AS_SET_UI_COLORS"; break;
		case AS_SET_UI_COLOR: string = "AS_SET_UI_COLOR"; break;
		case AS_SET_DECORATOR: string = "AS_SET_DECORATOR"; break;
		case AS_GET_DECORATOR: string = "AS_GET_DECORATOR"; break;
		case AS_R5_SET_DECORATOR: string = "AS_R5_SET_DECORATOR"; break;
		case AS_COUNT_DECORATORS: string = "AS_COUNT_DECORATORS"; break;
		case AS_GET_DECORATOR_NAME: string = "AS_GET_DECORATOR_NAME"; break;

		case AS_COUNT_WORKSPACES: string = "AS_COUNT_WORKSPACES"; break;
		case AS_SET_WORKSPACE_COUNT: string = "AS_SET_WORKSPACE_COUNT"; break;
		case AS_CURRENT_WORKSPACE: string = "AS_CURRENT_WORKSPACE"; break;
		case AS_ACTIVATE_WORKSPACE: string = "AS_ACTIVATE_WORKSPACE"; break;
		case AS_GET_SCROLLBAR_INFO: string = "AS_GET_SCROLLBAR_INFO"; break;
		case AS_SET_SCROLLBAR_INFO: string = "AS_SET_SCROLLBAR_INFO"; break;
		case AS_GET_MENU_INFO: string = "AS_GET_MENU_INFO"; break;
		case AS_SET_MENU_INFO: string = "AS_SET_MENU_INFO"; break;
		case AS_IDLE_TIME: string = "AS_IDLE_TIME"; break;
		case AS_SET_MOUSE_MODE: string = "AS_SET_MOUSE_MODE"; break;
		case AS_GET_MOUSE_MODE: string = "AS_GET_MOUSE_MODE"; break;
		case AS_GET_MOUSE: string = "AS_GET_MOUSE"; break;
		case AS_SET_DECORATOR_SETTINGS: string = "AS_SET_DECORATOR_SETTINGS"; break;
		case AS_GET_DECORATOR_SETTINGS: string = "AS_GET_DECORATOR_SETTINGS"; break;
		case AS_GET_SHOW_ALL_DRAGGERS: string = "AS_GET_SHOW_ALL_DRAGGERS"; break;
		case AS_SET_SHOW_ALL_DRAGGERS: string = "AS_SET_SHOW_ALL_DRAGGERS"; break;
		
		case AS_SET_FONT_SUBPIXEL_ANTIALIASING: string = "AS_SET_FONT_SUBPIXEL_ANTIALIASING"; break;
		case AS_GET_FONT_SUBPIXEL_ANTIALIASING: string = "AS_GET_FONT_SUBPIXEL_ANTIALIASING"; break;
		case AS_SET_HINTING: string = "AS_SET_HINTING"; break;
		case AS_GET_HINTING: string = "AS_GET_HINTING"; break;

		// Graphics calls
		case AS_SET_HIGH_COLOR: string = "AS_SET_HIGH_COLOR"; break;
		case AS_SET_LOW_COLOR: string = "AS_SET_LOW_COLOR"; break;
		case AS_SET_VIEW_COLOR: string = "AS_SET_VIEW_COLOR"; break;

		case AS_STROKE_ARC: string = "AS_STROKE_ARC"; break;
		case AS_STROKE_BEZIER: string = "AS_STROKE_BEZIER"; break;
		case AS_STROKE_ELLIPSE: string = "AS_STROKE_ELLIPSE"; break;
		case AS_STROKE_LINE: string = "AS_STROKE_LINE"; break;
		case AS_STROKE_LINEARRAY: string = "AS_STROKE_LINEARRAY"; break;
		case AS_STROKE_POLYGON: string = "AS_STROKE_POLYGON"; break;
		case AS_STROKE_RECT: string = "AS_STROKE_RECT"; break;
		case AS_STROKE_ROUNDRECT: string = "AS_STROKE_ROUNDRECT"; break;
		case AS_STROKE_SHAPE: string = "AS_STROKE_SHAPE"; break;
		case AS_STROKE_TRIANGLE: string = "AS_STROKE_TRIANGLE"; break;

		case AS_FILL_ARC: string = "AS_FILL_ARC"; break;
		case AS_FILL_BEZIER: string = "AS_FILL_BEZIER"; break;
		case AS_FILL_ELLIPSE: string = "AS_FILL_ELLIPSE"; break;
		case AS_FILL_POLYGON: string = "AS_FILL_POLYGON"; break;
		case AS_FILL_RECT: string = "AS_FILL_RECT"; break;
		case AS_FILL_REGION: string = "AS_FILL_REGION"; break;
		case AS_FILL_ROUNDRECT: string = "AS_FILL_ROUNDRECT"; break;
		case AS_FILL_SHAPE: string = "AS_FILL_SHAPE"; break;
		case AS_FILL_TRIANGLE: string = "AS_FILL_TRIANGLE"; break;

		case AS_DRAW_STRING: string = "AS_DRAW_STRING"; break;
		case AS_DRAW_STRING_WITH_DELTA: string = "AS_DRAW_STRING_WITH_DELTA"; break;

		case AS_SYNC: string = "AS_SYNC"; break;

		case AS_VIEW_CREATE: string = "AS_VIEW_CREATE"; break;
		case AS_VIEW_DELETE: string = "AS_VIEW_DELETE"; break;
		case AS_VIEW_CREATE_ROOT: string = "AS_VIEW_CREATE_ROOT"; break;
		case AS_VIEW_SHOW: string = "AS_VIEW_SHOW"; break;
		case AS_VIEW_HIDE: string = "AS_VIEW_HIDE"; break;
		case AS_VIEW_MOVE: string = "AS_VIEW_MOVE"; break;
		case AS_VIEW_RESIZE: string = "AS_VIEW_RESIZE"; break;
		case AS_VIEW_DRAW: string = "AS_VIEW_DRAW"; break;

		// View definitions
		case AS_VIEW_GET_COORD: string = "AS_VIEW_GET_COORD"; break;
		case AS_VIEW_SET_FLAGS: string = "AS_VIEW_SET_FLAGS"; break;
		case AS_VIEW_SET_ORIGIN: string = "AS_VIEW_SET_ORIGIN"; break;
		case AS_VIEW_GET_ORIGIN: string = "AS_VIEW_GET_ORIGIN"; break;
		case AS_VIEW_RESIZE_MODE: string = "AS_VIEW_RESIZE_MODE"; break;
		case AS_VIEW_SET_CURSOR: string = "AS_VIEW_SET_CURSOR"; break;
		case AS_VIEW_BEGIN_RECT_TRACK: string = "AS_VIEW_BEGIN_RECT_TRACK"; break;
		case AS_VIEW_END_RECT_TRACK: string = "AS_VIEW_END_RECT_TRACK"; break;
		case AS_VIEW_DRAG_RECT: string = "AS_VIEW_DRAG_RECT"; break;
		case AS_VIEW_DRAG_IMAGE: string = "AS_VIEW_DRAG_IMAGE"; break;
		case AS_VIEW_SCROLL: string = "AS_VIEW_SCROLL"; break;
		case AS_VIEW_SET_LINE_MODE: string = "AS_VIEW_SET_LINE_MODE"; break;
		case AS_VIEW_GET_LINE_MODE: string = "AS_VIEW_GET_LINE_MODE"; break;
		case AS_VIEW_PUSH_STATE: string = "AS_VIEW_PUSH_STATE"; break;
		case AS_VIEW_POP_STATE: string = "AS_VIEW_POP_STATE"; break;
		case AS_VIEW_SET_SCALE: string = "AS_VIEW_SET_SCALE"; break;
		case AS_VIEW_GET_SCALE: string = "AS_VIEW_GET_SCALE"; break;
		case AS_VIEW_SET_DRAWING_MODE: string = "AS_VIEW_SET_DRAWING_MODE"; break;
		case AS_VIEW_GET_DRAWING_MODE: string = "AS_VIEW_GET_DRAWING_MODE"; break;
		case AS_VIEW_SET_BLENDING_MODE: string = "AS_VIEW_SET_BLENDING_MODE"; break;
		case AS_VIEW_GET_BLENDING_MODE: string = "AS_VIEW_GET_BLENDING_MODE"; break;
		case AS_VIEW_SET_PEN_LOC: string = "AS_VIEW_SET_PEN_LOC"; break;
		case AS_VIEW_GET_PEN_LOC: string = "AS_VIEW_GET_PEN_LOC"; break;
		case AS_VIEW_SET_PEN_SIZE: string = "AS_VIEW_SET_PEN_SIZE"; break;
		case AS_VIEW_GET_PEN_SIZE: string = "AS_VIEW_GET_PEN_SIZE"; break;
		case AS_VIEW_SET_HIGH_COLOR: string = "AS_VIEW_SET_HIGH_COLOR"; break;
		case AS_VIEW_SET_LOW_COLOR: string = "AS_VIEW_SET_LOW_COLOR"; break;
		case AS_VIEW_SET_VIEW_COLOR: string = "AS_VIEW_SET_VIEW_COLOR"; break;
		case AS_VIEW_GET_HIGH_COLOR: string = "AS_VIEW_GET_HIGH_COLOR"; break;
		case AS_VIEW_GET_LOW_COLOR: string = "AS_VIEW_GET_LOW_COLOR"; break;
		case AS_VIEW_GET_VIEW_COLOR: string = "AS_VIEW_GET_VIEW_COLOR"; break;
		case AS_VIEW_PRINT_ALIASING: string = "AS_VIEW_PRINT_ALIASING"; break;
		case AS_VIEW_CLIP_TO_PICTURE: string = "AS_VIEW_CLIP_TO_PICTURE"; break;
		case AS_VIEW_GET_CLIP_REGION: string = "AS_VIEW_GET_CLIP_REGION"; break;
		case AS_VIEW_DRAW_BITMAP: string = "AS_VIEW_DRAW_BITMAP"; break;
		case AS_VIEW_SET_EVENT_MASK: string = "AS_VIEW_SET_EVENT_MASK"; break;
		case AS_VIEW_SET_MOUSE_EVENT_MASK: string = "AS_VIEW_SET_MOUSE_EVENT_MASK"; break;

		case AS_VIEW_DRAW_STRING: string = "AS_VIEW_DRAW_STRING"; break;
		case AS_VIEW_SET_CLIP_REGION: string = "AS_VIEW_SET_CLIP_REGION"; break;
		case AS_VIEW_LINE_ARRAY: string = "AS_VIEW_LINE_ARRAY"; break;
		case AS_VIEW_BEGIN_PICTURE: string = "AS_VIEW_BEGIN_PICTURE"; break;
		case AS_VIEW_APPEND_TO_PICTURE: string = "AS_VIEW_APPEND_TO_PICTURE"; break;
		case AS_VIEW_END_PICTURE: string = "AS_VIEW_END_PICTURE"; break;
		case AS_VIEW_COPY_BITS: string = "AS_VIEW_COPY_BITS"; break;
		case AS_VIEW_DRAW_PICTURE: string = "AS_VIEW_DRAW_PICTURE"; break;
		case AS_VIEW_INVALIDATE_RECT: string = "AS_VIEW_INVALIDATE_RECT"; break;
		case AS_VIEW_INVALIDATE_REGION: string = "AS_VIEW_INVALIDATE_REGION"; break;
		case AS_VIEW_INVERT_RECT: string = "AS_VIEW_INVERT_RECT"; break;
		case AS_VIEW_MOVE_TO: string = "AS_VIEW_MOVE_TO"; break;
		case AS_VIEW_RESIZE_TO: string = "AS_VIEW_RESIZE_TO"; break;
		case AS_VIEW_SET_STATE: string = "AS_VIEW_SET_STATE"; break;
		case AS_VIEW_SET_FONT_STATE: string = "AS_VIEW_SET_FONT_STATE"; break;
		case AS_VIEW_GET_STATE: string = "AS_VIEW_GET_STATE"; break;
		case AS_VIEW_SET_VIEW_BITMAP: string = "AS_VIEW_SET_VIEW_BITMAP"; break;
		case AS_VIEW_SET_PATTERN: string = "AS_VIEW_SET_PATTERN"; break;
		case AS_SET_CURRENT_VIEW: string = "AS_SET_CURRENT_VIEW"; break;

		// BDirectWindow codes
		case AS_DIRECT_WINDOW_GET_SYNC_DATA: string = "AS_DIRECT_WINDOW_GET_SYNC_DATA"; break;
		case AS_DIRECT_WINDOW_SET_FULLSCREEN: string = "AS_DIRECT_WINDOW_SET_FULLSCREEN"; break;
	
		default:
			string << "unkown code: " << code;
			break;
	}
}


