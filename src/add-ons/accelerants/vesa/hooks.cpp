/*
 * Copyright 2005-2012, Axel Dörfler, axeld@pinc-software.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "accelerant_protos.h"
#include "accelerant.h"

#include <new>


#define FAKE_OVERLAY_SUPPORT 0
	// Enables a fake overlay support, making the app_server believe it can
	// use overlays with this driver; the actual buffers are in the frame
	// buffer so the on-screen graphics will be messed up.

#define FAKE_HARDWARE_CURSOR_SUPPORT 0
	// Enables the faking of a hardware cursor. The cursor will not be
	// visible, but it will still function.


#if FAKE_OVERLAY_SUPPORT
static int32 sOverlayToken;
static int32 sOverlayChannelUsed;


static uint32
vesa_overlay_count(const display_mode* mode)
{
	return 1;
}


static const uint32*
vesa_overlay_supported_spaces(const display_mode* mode)
{
	static const uint32 kSupportedSpaces[] = {B_RGB15, B_RGB16, B_RGB32,
		B_YCbCr422, 0};

	return kSupportedSpaces;
}


static uint32
vesa_overlay_supported_features(uint32 colorSpace)
{
	return B_OVERLAY_COLOR_KEY
		| B_OVERLAY_HORIZONTAL_FILTERING
		| B_OVERLAY_VERTICAL_FILTERING
		| B_OVERLAY_HORIZONTAL_MIRRORING;
}


static const overlay_buffer*
vesa_allocate_overlay_buffer(color_space colorSpace, uint16 width,
	uint16 height)
{
	debug_printf("allocate_overlay_buffer(width %u, height %u, colorSpace %u)\n",
		width, height, colorSpace);

	overlay_buffer* buffer = new(std::nothrow) overlay_buffer;
	if (buffer == NULL)
		return NULL;

	buffer->space = colorSpace;
	buffer->width = width;
	buffer->height = height;
	buffer->bytes_per_row = gInfo->shared_info->bytes_per_row;

	buffer->buffer = gInfo->shared_info->frame_buffer;
	buffer->buffer_dma = (uint8*)gInfo->shared_info->physical_frame_buffer;

	return buffer;
}


static status_t
vesa_release_overlay_buffer(const overlay_buffer* buffer)
{
	debug_printf("release_overlay_buffer(buffer %p)\n", buffer);

	delete buffer;
	return B_OK;
}


static status_t
vesa_get_overlay_constraints(const display_mode* mode,
	const overlay_buffer* buffer, overlay_constraints* constraints)
{
	debug_printf("get_overlay_constraints(buffer %p)\n", buffer);

	// position
	constraints->view.h_alignment = 0;
	constraints->view.v_alignment = 0;

	// alignment
	constraints->view.width_alignment = 7;
	constraints->view.height_alignment = 0;

	// size
	constraints->view.width.min = 4;
	constraints->view.height.min = 4;
	constraints->view.width.max = buffer->width;
	constraints->view.height.max = buffer->height;

	// scaler output restrictions
	constraints->window.h_alignment = 0;
	constraints->window.v_alignment = 0;
	constraints->window.width_alignment = 0;
	constraints->window.height_alignment = 0;
	constraints->window.width.min = 2;
	constraints->window.width.max = mode->virtual_width;
	constraints->window.height.min = 2;
	constraints->window.height.max = mode->virtual_height;

	constraints->h_scale.min = 1.0f / (1 << 4);
	constraints->h_scale.max = buffer->width * 7;
	constraints->v_scale.min = 1.0f / (1 << 4);
	constraints->v_scale.max = buffer->height * 7;

	return B_OK;
}


static overlay_token
vesa_allocate_overlay()
{
	debug_printf("allocate_overlay()\n");

	// we only use a single overlay channel
	if (atomic_or(&sOverlayChannelUsed, 1) != 0)
		return NULL;

	return (overlay_token)++sOverlayToken;
}


static status_t
vesa_release_overlay(overlay_token overlayToken)
{
	debug_printf("allocate_overlay(token %ld)\n", (uint32)overlayToken);

	if (overlayToken != (overlay_token)sOverlayToken)
		return B_BAD_VALUE;

	atomic_and(&sOverlayChannelUsed, 0);

	return B_OK;
}


static status_t
vesa_configure_overlay(overlay_token overlayToken, const overlay_buffer* buffer,
	const overlay_window* window, const overlay_view* view)
{
	debug_printf("configure_overlay: buffer %p, window %p, view %p\n",
		buffer, window, view);
	return B_OK;
}
#endif	// FAKE_OVERLAY_SUPPORT


#if FAKE_HARDWARE_CURSOR_SUPPORT
status_t
vesa_set_cursor_shape(uint16 width, uint16 height, uint16 hotX, uint16 hotY,
	const uint8* andMask, const uint8* xorMask)
{
	return B_OK;
}


status_t
vesa_set_cursor_bitmap(uint16 width, uint16 height, uint16 hotX, uint16 hotY,
	color_space colorSpace, uint16 bytesPerRow, const uint8* bitmapData)
{
	return B_OK;
}


void
vesa_move_cursor(uint16 x, uint16 y)
{
}


void
vesa_show_cursor(bool isVisible)
{
}
#endif	// # FAKE_HARDWARE_CURSOR_SUPPORT


extern "C" void*
get_accelerant_hook(uint32 feature, void* data)
{
	switch (feature) {
		/* general */
		case B_INIT_ACCELERANT:
			return (void*)vesa_init_accelerant;
		case B_UNINIT_ACCELERANT:
			return (void*)vesa_uninit_accelerant;
		case B_CLONE_ACCELERANT:
			return (void*)vesa_clone_accelerant;
		case B_ACCELERANT_CLONE_INFO_SIZE:
			return (void*)vesa_accelerant_clone_info_size;
		case B_GET_ACCELERANT_CLONE_INFO:
			return (void*)vesa_get_accelerant_clone_info;
		case B_GET_ACCELERANT_DEVICE_INFO:
			return (void*)vesa_get_accelerant_device_info;
		case B_ACCELERANT_RETRACE_SEMAPHORE:
			return (void*)vesa_accelerant_retrace_semaphore;

		/* mode configuration */
		case B_ACCELERANT_MODE_COUNT:
			return (void*)vesa_accelerant_mode_count;
		case B_GET_MODE_LIST:
			return (void*)vesa_get_mode_list;
		case B_PROPOSE_DISPLAY_MODE:
			return (void*)vesa_propose_display_mode;
		case B_SET_DISPLAY_MODE:
			return (void*)vesa_set_display_mode;
		case B_GET_DISPLAY_MODE:
			return (void*)vesa_get_display_mode;
		case B_GET_EDID_INFO:
			return (void*)vesa_get_edid_info;
		case B_GET_FRAME_BUFFER_CONFIG:
			return (void*)vesa_get_frame_buffer_config;
		case B_GET_PIXEL_CLOCK_LIMITS:
			return (void*)vesa_get_pixel_clock_limits;
		case B_MOVE_DISPLAY:
			return (void*)vesa_move_display;
		case B_SET_INDEXED_COLORS:
			return (void*)vesa_set_indexed_colors;
		case B_GET_TIMING_CONSTRAINTS:
			return (void*)vesa_get_timing_constraints;

		/* DPMS */
		case B_DPMS_CAPABILITIES:
			return (void*)vesa_dpms_capabilities;
		case B_DPMS_MODE:
			return (void*)vesa_dpms_mode;
		case B_SET_DPMS_MODE:
			return (void*)vesa_set_dpms_mode;

		/* cursor managment */
#if FAKE_HARDWARE_CURSOR_SUPPORT
		case B_SET_CURSOR_SHAPE:
			return (void*)vesa_set_cursor_shape;
		case B_MOVE_CURSOR:
			return (void*)vesa_move_cursor;
		case B_SHOW_CURSOR:
			return (void*)vesa_show_cursor;
		case B_SET_CURSOR_BITMAP:
			return (void*)vesa_set_cursor_bitmap;
#endif

		/* engine/synchronization */
		case B_ACCELERANT_ENGINE_COUNT:
			return (void*)vesa_accelerant_engine_count;
		case B_ACQUIRE_ENGINE:
			return (void*)vesa_acquire_engine;
		case B_RELEASE_ENGINE:
			return (void*)vesa_release_engine;
		case B_WAIT_ENGINE_IDLE:
			return (void*)vesa_wait_engine_idle;
		case B_GET_SYNC_TOKEN:
			return (void*)vesa_get_sync_token;
		case B_SYNC_TO_TOKEN:
			return (void*)vesa_sync_to_token;

#if FAKE_OVERLAY_SUPPORT
		// overlay
		case B_OVERLAY_COUNT:
			return (void*)vesa_overlay_count;
		case B_OVERLAY_SUPPORTED_SPACES:
			return (void*)vesa_overlay_supported_spaces;
		case B_OVERLAY_SUPPORTED_FEATURES:
			return (void*)vesa_overlay_supported_features;
		case B_ALLOCATE_OVERLAY_BUFFER:
			return (void*)vesa_allocate_overlay_buffer;
		case B_RELEASE_OVERLAY_BUFFER:
			return (void*)vesa_release_overlay_buffer;
		case B_GET_OVERLAY_CONSTRAINTS:
			return (void*)vesa_get_overlay_constraints;
		case B_ALLOCATE_OVERLAY:
			return (void*)vesa_allocate_overlay;
		case B_RELEASE_OVERLAY:
			return (void*)vesa_release_overlay;
		case B_CONFIGURE_OVERLAY:
			return (void*)vesa_configure_overlay;
#endif	// FAKE_OVERLAY_SUPPORT
	}

	return NULL;
}

