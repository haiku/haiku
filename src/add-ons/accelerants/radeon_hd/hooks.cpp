/*
 * Copyright 2006-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "accelerant_protos.h"
#include "accelerant.h"


extern "C" void *
get_accelerant_hook(uint32 feature, void *data)
{
	switch (feature) {
		/* general */
		case B_INIT_ACCELERANT:
			return (void*)radeon_init_accelerant;
		case B_UNINIT_ACCELERANT:
			return (void*)radeon_uninit_accelerant;
		/*case B_CLONE_ACCELERANT:
			return (void*)radeon_clone_accelerant;
		case B_ACCELERANT_CLONE_INFO_SIZE:
			return (void*)radeon_accelerant_clone_info_size;
		case B_GET_ACCELERANT_CLONE_INFO:
			return (void*)radeon_get_accelerant_clone_info;
		*/
		case B_GET_ACCELERANT_DEVICE_INFO:
			return (void*)radeon_get_accelerant_device_info;
		/*
		case B_ACCELERANT_RETRACE_SEMAPHORE:
			return (void*)radeon_accelerant_retrace_semaphore;
		*/

		/* DPMS */
		case B_DPMS_CAPABILITIES:
			return (void*)radeon_dpms_capabilities;
		case B_DPMS_MODE:
			return (void*)radeon_dpms_mode;
		case B_SET_DPMS_MODE:
			return (void*)radeon_dpms_set;

		/* mode configuration */
		case B_ACCELERANT_MODE_COUNT:
			return (void*)radeon_accelerant_mode_count;
		case B_GET_MODE_LIST:
			return (void*)radeon_get_mode_list;
		case B_SET_DISPLAY_MODE:
			return (void*)radeon_set_display_mode;
		case B_GET_DISPLAY_MODE:
			return (void*)radeon_get_display_mode;
		case B_GET_EDID_INFO:
			return (void*)radeon_get_edid_info;

		case B_GET_FRAME_BUFFER_CONFIG:
			return (void*)radeon_get_frame_buffer_config;
		case B_GET_PIXEL_CLOCK_LIMITS:
			return (void*)radeon_get_pixel_clock_limits;

		case B_ACQUIRE_ENGINE:
			return (void*)radeon_acquire_engine;
		case B_RELEASE_ENGINE:
			return (void*)radeon_release_engine;
	}

	return NULL;
}

