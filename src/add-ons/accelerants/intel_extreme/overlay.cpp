/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "accelerant.h"
#include "accelerant_protos.h"

#include <stdlib.h>


#define TRACE_OVERLAY
#ifdef TRACE_OVERLAY
extern "C" void _sPrintf(const char *format, ...);
#	define TRACE(x) _sPrintf x
#else
#	define TRACE(x) ;
#endif


uint32
intel_overlay_count(const display_mode *mode)
{
	// TODO: make this depending on the amount of RAM and the screen mode
	return 1;
}


const uint32 *
intel_overlay_supported_spaces(const display_mode *mode)
{
	static const uint32 kSupportedSpaces[] = {B_YCbCr422, 0};

	return kSupportedSpaces;
}


uint32
intel_overlay_supported_features(uint32 colorSpace)
{
	return B_OVERLAY_COLOR_KEY
		| B_OVERLAY_HORIZONTAL_FILTERING
		| B_OVERLAY_VERTICAL_FILTERING;
}


const overlay_buffer *
intel_allocate_overlay_buffer(color_space colorSpace, uint16 width,
	uint16 height)
{
	uint32 bpp;

	switch (colorSpace) {
		case B_RGB15:
			bpp = 2;
			break;
		case B_RGB16:
			bpp = 2; 
			break;
		case B_RGB32:
			bpp = 4; 
			break;
		case B_YCbCr422:
			bpp = 2;
			break;
		default:
			return NULL;
	}

	struct overlay *overlay = (struct overlay *)malloc(sizeof(struct overlay));
	if (overlay == NULL)
		return NULL;

	// TODO: locking!

	// alloc graphics mem
	overlay_buffer *buffer = &overlay->buffer;

	buffer->space = colorSpace;
	buffer->width = width;
	buffer->height = height;
	buffer->bytes_per_row = (width * bpp + 0xf) & ~0xf;

	status_t status = intel_allocate_memory(buffer->bytes_per_row * height,
		overlay->buffer_handle, overlay->buffer_offset);
	if (status < B_OK) {
		free(overlay);
		return NULL;
	}

	buffer->buffer = gInfo->shared_info->graphics_memory + overlay->buffer_offset;
	buffer->buffer_dma = gInfo->shared_info->physical_graphics_memory + overlay->buffer_offset;

	// add to list of overlays


	TRACE(("allocated overlay buffer: handle=%x, offset=%x, address=%x, phys-address=%x", 
		overlay->buffer_handle, overlay->buffer_offset, buffer->buffer, buffer->buffer_dma));

	return buffer;
}


status_t
intel_release_overlay_buffer(const overlay_buffer *buffer)
{
	struct overlay *overlay = (struct overlay *)buffer;

	// TODO: locking!
	// TODO: if overlay is still active, hide it

	intel_free_memory(overlay->buffer_handle);
	free(overlay);

	return B_OK;
}


status_t
intel_get_overlay_constraints(const display_mode *mode, const overlay_buffer *buffer,
	overlay_constraints *constraints)
{
	// taken from the Radeon driver...

	// scaler input restrictions
	// TODO: check all these values; most of them are probably too restrictive

	// position
	constraints->view.h_alignment = 0;
	constraints->view.v_alignment = 0;

	// alignment
	switch (buffer->space) {
		case B_RGB15:
			constraints->view.width_alignment = 7;
			break;
		case B_RGB16:
			constraints->view.width_alignment = 7;
			break;
		case B_RGB32:
			constraints->view.width_alignment = 3;
			break;
		case B_YCbCr422:
			constraints->view.width_alignment = 7;
			break;
		case B_YUV12:
			constraints->view.width_alignment = 7;
		default:
			return B_BAD_VALUE;
	}
	constraints->view.height_alignment = 0;

	// size
	constraints->view.width.min = 4;		// make 4-tap filter happy
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

	// TODO: these values need to be checked
	constraints->h_scale.min = 1.0f / (1 << 4);
	constraints->h_scale.max = 1 << 12;
	constraints->v_scale.min = 1.0f / (1 << 4);
	constraints->v_scale.max = 1 << 12;

	return B_OK;
}


overlay_token
intel_allocate_overlay(void)
{
	// we only have a single overlay channel
	if (atomic_or(&gInfo->shared_info->overlay_channel_used, 1) != 0)
		return NULL;

	return (overlay_token)++gInfo->shared_info->overlay_token;
}


status_t
intel_release_overlay(overlay_token overlayToken)
{
	// we only have a single token, which simplifies this
	if (overlayToken != (overlay_token)gInfo->shared_info->overlay_token)
		return B_BAD_VALUE;

	atomic_and(&gInfo->shared_info->overlay_channel_used, 0);
	return B_OK;
}


status_t
intel_configure_overlay(overlay_token overlayToken, const overlay_buffer *buffer,
	const overlay_window *window, const overlay_view *view)
{
	return B_OK;
}

