/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "accelerant_protos.h"


extern "C" void *
get_accelerant_hook(uint32 feature, void *data)
{
	switch (feature) {
		/* general */
		case B_INIT_ACCELERANT:
			return intel_init_accelerant;
		case B_UNINIT_ACCELERANT:
			return intel_uninit_accelerant;
		case B_CLONE_ACCELERANT:
			return intel_clone_accelerant;
		case B_ACCELERANT_CLONE_INFO_SIZE:
			return intel_accelerant_clone_info_size;
		case B_GET_ACCELERANT_CLONE_INFO:
			return intel_get_accelerant_clone_info;
		case B_GET_ACCELERANT_DEVICE_INFO:
			return intel_get_accelerant_device_info;
		case B_ACCELERANT_RETRACE_SEMAPHORE:
			return intel_accelerant_retrace_semaphore;

		/* mode configuration */
		case B_ACCELERANT_MODE_COUNT:
			return intel_accelerant_mode_count;
		case B_GET_MODE_LIST:
			return intel_get_mode_list;
		case B_PROPOSE_DISPLAY_MODE:
			return intel_propose_display_mode;
		case B_SET_DISPLAY_MODE:
			return intel_set_display_mode;
		case B_GET_DISPLAY_MODE:
			return intel_get_display_mode;
		case B_GET_FRAME_BUFFER_CONFIG:
			return intel_get_frame_buffer_config;
		case B_GET_PIXEL_CLOCK_LIMITS:
			return intel_get_pixel_clock_limits;
		case B_MOVE_DISPLAY:
			return intel_move_display;
		case B_SET_INDEXED_COLORS:
			return intel_set_indexed_colors;
		case B_GET_TIMING_CONSTRAINTS:
			return intel_get_timing_constraints;

		/* DPMS */
		case B_DPMS_CAPABILITIES:
			return intel_dpms_capabilities;
		case B_DPMS_MODE:
			return intel_dpms_mode;
		case B_SET_DPMS_MODE:
			return intel_set_dpms_mode;

		/* cursor managment */
/*		case B_SET_CURSOR_SHAPE:
			return intel_set_cursor_shape;
		case B_MOVE_CURSOR:
			return intel_move_cursor;
		case B_SHOW_CURSOR:
			return intel_show_cursor;
*/
		/* engine/synchronization */
		case B_ACCELERANT_ENGINE_COUNT:
			return intel_accelerant_engine_count;
		case B_ACQUIRE_ENGINE:
			return intel_acquire_engine;
		case B_RELEASE_ENGINE:
			return intel_release_engine;
		case B_WAIT_ENGINE_IDLE:
			return intel_wait_engine_idle;
		case B_GET_SYNC_TOKEN:
			return intel_get_sync_token;
		case B_SYNC_TO_TOKEN:
			return intel_sync_to_token;

		/* 2D acceleration */
/*		HOOK(SCREEN_TO_SCREEN_BLIT);
		HOOK(FILL_RECTANGLE);
		HOOK(INVERT_RECTANGLE);
		HOOK(FILL_SPAN);
*/		
		// overlay
		case B_OVERLAY_COUNT:
			return intel_overlay_supported_spaces;
		case B_OVERLAY_SUPPORTED_SPACES:
			return intel_overlay_supported_spaces;
		case B_OVERLAY_SUPPORTED_FEATURES:
			return intel_overlay_supported_features;
		case B_ALLOCATE_OVERLAY_BUFFER:
			return intel_allocate_overlay_buffer;
		case B_RELEASE_OVERLAY_BUFFER:
			return intel_release_overlay_buffer;
		case B_GET_OVERLAY_CONSTRAINTS:
			return intel_get_overlay_constraints;
		case B_ALLOCATE_OVERLAY:
			return intel_allocate_overlay;
		case B_RELEASE_OVERLAY:
			return intel_release_overlay;
		case B_CONFIGURE_OVERLAY:
			return intel_configure_overlay;
	}

	return NULL;
}

