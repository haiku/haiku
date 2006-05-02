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


static void
set_color_key(overlay_registers *registers, uint8 red, uint8 green, uint8 blue,
	uint8 redMask, uint8 greenMask, uint8 blueMask)
{
	registers->color_key_red = red;
	registers->color_key_green = green;
	registers->color_key_blue = blue;
	registers->color_key_mask_red = redMask;
	registers->color_key_mask_green = greenMask;
	registers->color_key_mask_blue = blueMask;
	registers->color_key_enabled = true;
}


static void
update_overlay(void)
{
	if (!gInfo->shared_info->overlay_active)
		return;

	ring_buffer &ringBuffer = gInfo->shared_info->primary_ring_buffer;
	write_to_ring(ringBuffer, COMMAND_FLUSH);
	write_to_ring(ringBuffer, COMMAND_NOOP);
	write_to_ring(ringBuffer, COMMAND_WAIT_FOR_EVENT | COMMAND_WAIT_FOR_OVERLAY_FLIP);
	write_to_ring(ringBuffer, COMMAND_NOOP);
	write_to_ring(ringBuffer, COMMAND_OVERLAY_FLIP | COMMAND_OVERLAY_CONTINUE);
	write_to_ring(ringBuffer, (uint32)gInfo->shared_info->physical_overlay_registers | 0x01);
	ring_command_complete(ringBuffer);

	TRACE(("update overlay: UPDATE: %lx, TEST: %lx, STATUS: %lx, EXTENDED_STATUS: %lx\n",
		read32(INTEL_OVERLAY_UPDATE), read32(INTEL_OVERLAY_TEST), read32(INTEL_OVERLAY_STATUS),
		read32(INTEL_OVERLAY_EXTENDED_STATUS)));
}


static void
show_overlay(void)
{
	if (gInfo->shared_info->overlay_active)
		return;

	overlay_registers *registers = gInfo->overlay_registers;

	gInfo->shared_info->overlay_active = true;
	registers->overlay_enabled = true;

	ring_buffer &ringBuffer = gInfo->shared_info->primary_ring_buffer;
	write_to_ring(ringBuffer, COMMAND_FLUSH);
	write_to_ring(ringBuffer, COMMAND_NOOP);
	write_to_ring(ringBuffer, COMMAND_OVERLAY_FLIP | COMMAND_OVERLAY_ON);
	write_to_ring(ringBuffer, (uint32)gInfo->shared_info->physical_overlay_registers | 0x01);
	ring_command_complete(ringBuffer);
}


static void
hide_overlay(void)
{
	if (!gInfo->shared_info->overlay_active)
		return;

	overlay_registers *registers = gInfo->overlay_registers;

	gInfo->shared_info->overlay_active = false;
	registers->overlay_enabled = false;

	update_overlay();

	ring_buffer &ringBuffer = gInfo->shared_info->primary_ring_buffer;

	// flush pending commands
	write_to_ring(ringBuffer, COMMAND_FLUSH);
	write_to_ring(ringBuffer, COMMAND_NOOP);
	write_to_ring(ringBuffer, COMMAND_WAIT_FOR_EVENT | COMMAND_WAIT_FOR_OVERLAY_FLIP);
	write_to_ring(ringBuffer, COMMAND_NOOP);

	// clear overlay enabled bit
	write_to_ring(ringBuffer, COMMAND_OVERLAY_FLIP | COMMAND_OVERLAY_CONTINUE);
	write_to_ring(ringBuffer, (uint32)gInfo->shared_info->physical_overlay_registers | 0x01);
	write_to_ring(ringBuffer, COMMAND_WAIT_FOR_EVENT | COMMAND_WAIT_FOR_OVERLAY_FLIP);
	write_to_ring(ringBuffer, COMMAND_NOOP);

	// turn off overlay engine
	write_to_ring(ringBuffer, COMMAND_OVERLAY_FLIP | COMMAND_OVERLAY_OFF);
	write_to_ring(ringBuffer, (uint32)gInfo->shared_info->physical_overlay_registers | 0x01);
	write_to_ring(ringBuffer, COMMAND_WAIT_FOR_EVENT | COMMAND_WAIT_FOR_OVERLAY_FLIP);
	write_to_ring(ringBuffer, COMMAND_NOOP);
	ring_command_complete(ringBuffer);

	gInfo->current_overlay = NULL;
}


//	#pragma mark -


uint32
intel_overlay_count(const display_mode *mode)
{
	// TODO: make this depending on the amount of RAM and the screen mode
	return 1;
}


const uint32 *
intel_overlay_supported_spaces(const display_mode *mode)
{
	static const uint32 kSupportedSpaces[] = {/*B_RGB15, B_RGB16,*/ B_RGB32,
		/*B_YCbCr411,*/ /*B_YCbCr422,*/ 0};

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
	TRACE(("intel_allocate_overlay_buffer(width %u, height %u, colorSpace %lu)\n",
		width, height, colorSpace));

	uint32 bytesPerPixel;

	switch (colorSpace) {
		case B_RGB15:
			bytesPerPixel = 2;
			break;
		case B_RGB16:
			bytesPerPixel = 2; 
			break;
		case B_RGB32:
			bytesPerPixel = 4; 
			break;
		case B_YCbCr422:
			bytesPerPixel = 2;
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
	buffer->bytes_per_row = (width * bytesPerPixel + 0xf) & ~0xf;

	status_t status = intel_allocate_memory(buffer->bytes_per_row * height,
		overlay->buffer_handle, overlay->buffer_offset);
	if (status < B_OK) {
		free(overlay);
		return NULL;
	}

	buffer->buffer = gInfo->shared_info->graphics_memory + overlay->buffer_offset;
	buffer->buffer_dma = gInfo->shared_info->physical_graphics_memory + overlay->buffer_offset;

	TRACE(("allocated overlay buffer: handle=%x, offset=%x, address=%x, physical address=%x\n", 
		overlay->buffer_handle, overlay->buffer_offset, buffer->buffer, buffer->buffer_dma));

	return buffer;
}


status_t
intel_release_overlay_buffer(const overlay_buffer *buffer)
{
	TRACE(("intel_release_overlay_buffer(buffer %p)\n", buffer));

	struct overlay *overlay = (struct overlay *)buffer;

	// TODO: locking!
	// TODO: if overlay is still active, hide it
	if (gInfo->current_overlay == overlay)
		hide_overlay();

	intel_free_memory(overlay->buffer_handle);
	free(overlay);

	return B_OK;
}


status_t
intel_get_overlay_constraints(const display_mode *mode, const overlay_buffer *buffer,
	overlay_constraints *constraints)
{
	TRACE(("intel_get_overlay_constraints(buffer %p)\n", buffer));

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
	TRACE(("intel_allocate_overlay()\n"));

	// we only have a single overlay channel
	if (atomic_or(&gInfo->shared_info->overlay_channel_used, 1) != 0)
		return NULL;

	return (overlay_token)++gInfo->shared_info->overlay_token;
}


status_t
intel_release_overlay(overlay_token overlayToken)
{
	TRACE(("intel_allocate_overlay(token %ld)\n", (uint32)overlayToken));

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
	TRACE(("intel_configure_overlay: buffer %p, window %p, view %p\n",
		buffer, window, view));

	if (overlayToken != (overlay_token)gInfo->shared_info->overlay_token)
		return B_BAD_VALUE;

	if (window == NULL && view == NULL) {
		hide_overlay();
		return B_OK;
	}

	struct overlay *overlay = (struct overlay *)buffer;
	overlay_registers *registers = gInfo->overlay_registers;

	switch (gInfo->shared_info->current_mode.space) {
		case B_CMAP8:
			set_color_key(registers, 0, 0, 0, 0xff, 0xff, 0xff);
			break;
		case B_RGB15:
			set_color_key(registers, window->red.value, window->green.value,
				window->blue.value, 0x07, 0x07, 0x07);
			break;
		case B_RGB16:
			set_color_key(registers, window->red.value, window->green.value,
				window->blue.value, 0x07, 0x03, 0x07);
			break;
		default:
			set_color_key(registers, window->red.value, window->green.value,
				window->blue.value, window->red.mask, window->green.mask, window->blue.mask);
			break;
	}

	// program window and scaling factor

	registers->window_left = 0;
	registers->window_top = 0;
	registers->window_width = view->width;//window->width;
	registers->window_height = view->height;//window->height;
	
	registers->source_width_rgb = buffer->width;
	registers->source_width_uv = buffer->width;
	registers->source_height_rgb = buffer->height;
	registers->source_height_uv = buffer->height;
	registers->source_bytes_per_row_rgb = buffer->bytes_per_row;
	registers->source_bytes_per_row_uv = buffer->bytes_per_row;
	registers->scale_rgb.horizontal_downscale_factor = 1;
	registers->scale_uv.horizontal_downscale_factor = 1;

	// program buffer

	registers->buffer_rgb0 = overlay->buffer_offset;
	registers->buffer_rgb1 = overlay->buffer_offset;
	registers->buffer_u0 = overlay->buffer_offset;
	registers->buffer_u1 = overlay->buffer_offset;
	registers->buffer_v0 = overlay->buffer_offset;
	registers->buffer_v1 = overlay->buffer_offset;
	registers->stride_rgb = buffer->bytes_per_row;
	registers->stride_uv = buffer->bytes_per_row;

	registers->ycbcr422_order = 1;

	if (!gInfo->shared_info->overlay_active)
		show_overlay();
	else
		update_overlay();

	gInfo->current_overlay = overlay;
	return B_OK;
}

