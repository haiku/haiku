/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "accelerant_protos.h"


// ToDo: these are some temporary dummy functions to see if this helps with our current app_server

status_t
vesa_set_cursor_shape(uint16 width, uint16 height, uint16 hot_x, uint16 hot_y, uint8 *andMask, uint8 *xorMask)
{
	return B_OK;
}


void 
vesa_move_cursor(uint16 x, uint16 y)
{
}


void 
vesa_show_cursor(bool is_visible)
{
}


extern "C" void *
get_accelerant_hook(uint32 feature, void *data)
{
	switch (feature) {
		/* general */
		case B_INIT_ACCELERANT:
			return vesa_init_accelerant;
		case B_UNINIT_ACCELERANT:
			return vesa_uninit_accelerant;
		case B_CLONE_ACCELERANT:
			return vesa_clone_accelerant;
		case B_ACCELERANT_CLONE_INFO_SIZE:
			return vesa_accelerant_clone_info_size;
		case B_GET_ACCELERANT_CLONE_INFO:
			return vesa_get_accelerant_clone_info;
		case B_GET_ACCELERANT_DEVICE_INFO:
			return vesa_get_accelerant_device_info;
		case B_ACCELERANT_RETRACE_SEMAPHORE:
			return vesa_accelerant_retrace_semaphore;

		/* mode configuration */
		case B_ACCELERANT_MODE_COUNT:
			return vesa_accelerant_mode_count;
		case B_GET_MODE_LIST:
			return vesa_get_mode_list;
		case B_PROPOSE_DISPLAY_MODE:
			return vesa_propose_display_mode;
		case B_SET_DISPLAY_MODE:
			return vesa_set_display_mode;
		case B_GET_DISPLAY_MODE:
			return vesa_get_display_mode;
		case B_GET_FRAME_BUFFER_CONFIG:
			return vesa_get_frame_buffer_config;
		case B_GET_PIXEL_CLOCK_LIMITS:
			return vesa_get_pixel_clock_limits;
		case B_MOVE_DISPLAY:
			return vesa_move_display;
		case B_SET_INDEXED_COLORS:
			return vesa_set_indexed_colors;
		case B_GET_TIMING_CONSTRAINTS:
			return vesa_get_timing_constraints;

		/* DPMS */
		case B_DPMS_CAPABILITIES:
			return vesa_dpms_capabilities;
		case B_DPMS_MODE:
			return vesa_dpms_mode;
		case B_SET_DPMS_MODE:
			return vesa_set_dpms_mode;

		/* cursor managment */
		case B_SET_CURSOR_SHAPE:
			return vesa_set_cursor_shape;
		case B_MOVE_CURSOR:
			return vesa_move_cursor;
		case B_SHOW_CURSOR:
			return vesa_show_cursor;

		/* engine/synchronization */
		case B_ACCELERANT_ENGINE_COUNT:
			return vesa_accelerant_engine_count;
		case B_ACQUIRE_ENGINE:
			return vesa_acquire_engine;
		case B_RELEASE_ENGINE:
			return vesa_release_engine;
		case B_WAIT_ENGINE_IDLE:
			return vesa_wait_engine_idle;
		case B_GET_SYNC_TOKEN:
			return vesa_get_sync_token;
		case B_SYNC_TO_TOKEN:
			return vesa_sync_to_token;
	}

	return NULL;
}

