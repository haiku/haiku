/*
 * Copyright 2006-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "accelerant_protos.h"
#include "accelerant.h"


extern "C" void*
get_accelerant_hook(uint32 feature, void* data)
{
	switch (feature) {
		/* general */
		case B_INIT_ACCELERANT:
			return (void*)intel_init_accelerant;
		case B_UNINIT_ACCELERANT:
			return (void*)intel_uninit_accelerant;
		case B_CLONE_ACCELERANT:
			return (void*)intel_clone_accelerant;
		case B_ACCELERANT_CLONE_INFO_SIZE:
			return (void*)intel_accelerant_clone_info_size;
		case B_GET_ACCELERANT_CLONE_INFO:
			return (void*)intel_get_accelerant_clone_info;
		case B_GET_ACCELERANT_DEVICE_INFO:
			return (void*)intel_get_accelerant_device_info;
		case B_ACCELERANT_RETRACE_SEMAPHORE:
			return (void*)intel_accelerant_retrace_semaphore;

		/* mode configuration */
		case B_ACCELERANT_MODE_COUNT:
			return (void*)intel_accelerant_mode_count;
		case B_GET_MODE_LIST:
			return (void*)intel_get_mode_list;
		case B_PROPOSE_DISPLAY_MODE:
			return (void*)intel_propose_display_mode;
		case B_SET_DISPLAY_MODE:
			return (void*)intel_set_display_mode;
		case B_GET_DISPLAY_MODE:
			return (void*)intel_get_display_mode;
#ifdef __HAIKU__
		case B_GET_EDID_INFO:
			return (void*)intel_get_edid_info;
#endif
		case B_GET_FRAME_BUFFER_CONFIG:
			return (void*)intel_get_frame_buffer_config;
		case B_GET_PIXEL_CLOCK_LIMITS:
			return (void*)intel_get_pixel_clock_limits;
		case B_MOVE_DISPLAY:
			return (void*)intel_move_display;
		case B_SET_INDEXED_COLORS:
			return (void*)intel_set_indexed_colors;
		case B_GET_TIMING_CONSTRAINTS:
			return (void*)intel_get_timing_constraints;

		/* DPMS */
		case B_DPMS_CAPABILITIES:
			return (void*)intel_dpms_capabilities;
		case B_DPMS_MODE:
			return (void*)intel_dpms_mode;
		case B_SET_DPMS_MODE:
			return (void*)intel_set_dpms_mode;

		/* cursor managment */
		case B_SET_CURSOR_SHAPE:
			if (gInfo->shared_info->cursor_memory != NULL)
				return (void*)intel_set_cursor_shape;
		case B_MOVE_CURSOR:
			if (gInfo->shared_info->cursor_memory != NULL)
				return (void*)intel_move_cursor;
		case B_SHOW_CURSOR:
			if (gInfo->shared_info->cursor_memory != NULL)
				return (void*)intel_show_cursor;

			return NULL;

		/* engine/synchronization */
		case B_ACCELERANT_ENGINE_COUNT:
			return (void*)intel_accelerant_engine_count;
		case B_ACQUIRE_ENGINE:
			return (void*)intel_acquire_engine;
		case B_RELEASE_ENGINE:
			return (void*)intel_release_engine;
		case B_WAIT_ENGINE_IDLE:
			return (void*)intel_wait_engine_idle;
		case B_GET_SYNC_TOKEN:
			return (void*)intel_get_sync_token;
		case B_SYNC_TO_TOKEN:
			return (void*)intel_sync_to_token;

		/* 2D acceleration */
		case B_SCREEN_TO_SCREEN_BLIT:
			return (void*)intel_screen_to_screen_blit;
		case B_FILL_RECTANGLE:
			return (void*)intel_fill_rectangle;
		case B_INVERT_RECTANGLE:
			return (void*)intel_invert_rectangle;
		case B_FILL_SPAN:
			return NULL;//(void*)intel_fill_span;

		// overlay
		case B_OVERLAY_COUNT:
			return (void*)intel_overlay_count;
		case B_OVERLAY_SUPPORTED_SPACES:
			return (void*)intel_overlay_supported_spaces;
		case B_OVERLAY_SUPPORTED_FEATURES:
			return (void*)intel_overlay_supported_features;
		case B_ALLOCATE_OVERLAY_BUFFER:
			// TODO: overlay doesn't seem to work on these chips
			if (gInfo->shared_info->device_type.InGroup(INTEL_TYPE_91x)
				|| gInfo->shared_info->device_type.InGroup(INTEL_TYPE_94x)
				|| gInfo->shared_info->device_type.IsModel(INTEL_TYPE_965M)
				|| gInfo->shared_info->device_type.InGroup(INTEL_TYPE_G4x)
				|| gInfo->shared_info->device_type.InGroup(INTEL_TYPE_IGD)
				|| gInfo->shared_info->device_type.InGroup(INTEL_TYPE_SNB))
				return NULL;

			return (void*)intel_allocate_overlay_buffer;
		case B_RELEASE_OVERLAY_BUFFER:
			return (void*)intel_release_overlay_buffer;
		case B_GET_OVERLAY_CONSTRAINTS:
			return (void*)intel_get_overlay_constraints;
		case B_ALLOCATE_OVERLAY:
			return (void*)intel_allocate_overlay;
		case B_RELEASE_OVERLAY:
			return (void*)intel_release_overlay;
		case B_CONFIGURE_OVERLAY:
			return (void*)intel_configure_overlay;
	}

	return NULL;
}

