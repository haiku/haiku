/*
 * Copyright 2005-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <string.h>

#include <create_display_modes.h>

#include "accelerant_protos.h"
#include "accelerant.h"
#include "utility.h"


//#define TRACE_MODE
#ifdef TRACE_MODE
extern "C" void _sPrintf(const char *format, ...);
#	define TRACE(x) _sPrintf x
#else
#	define TRACE(x) ;
#endif



/*!	Creates the initial mode list of the primary accelerant.
	It's called from vesa_init_accelerant().
*/
status_t
create_mode_list(void)
{
	// ToDo: basically, the boot loader should pass a list of all supported
	//	VESA modes to us.
	
	gInfo->mode_list_area = create_display_modes("vesa modes", NULL, NULL, 0,
		NULL, 0, NULL, &gInfo->mode_list, &gInfo->shared_info->mode_count);
	if (gInfo->mode_list_area < B_OK)
		return gInfo->mode_list_area;

	gInfo->shared_info->mode_list_area = gInfo->mode_list_area;
	return B_OK;
}


//	#pragma mark -


uint32
vesa_accelerant_mode_count(void)
{
	TRACE(("vesa_accelerant_mode_count()\n"));
	return gInfo->shared_info->mode_count;
}


status_t
vesa_get_mode_list(display_mode *modeList)
{
	TRACE(("vesa_get_mode_info()\n"));
	memcpy(modeList, gInfo->mode_list, gInfo->shared_info->mode_count * sizeof(display_mode));
	return B_OK;
}


status_t
vesa_propose_display_mode(display_mode *target, const display_mode *low,
	const display_mode *high)
{
	TRACE(("vesa_propose_display_mode()\n"));

	// just search for the specified mode in the list
	
	for (uint32 i = 0; i < gInfo->shared_info->mode_count; i++) {
		display_mode *current = &gInfo->mode_list[i];

		if (target->virtual_width != current->virtual_width
			|| target->virtual_height != current->virtual_height
			|| target->space != current->space)
			continue;

		*target = *current;
		return B_OK;
	}
	return B_BAD_VALUE;
}


status_t
vesa_set_display_mode(display_mode *_mode)
{
	TRACE(("vesa_set_display_mode()\n"));

	display_mode mode = *_mode;
	if (vesa_propose_display_mode(&mode, &mode, &mode) != B_OK)
		return B_BAD_VALUE;

	// ToDo: unless we have a VBE3 device, we can't change the display mode
	return B_UNSUPPORTED;
}


status_t
vesa_get_display_mode(display_mode *_currentMode)
{
	TRACE(("vesa_get_display_mode()\n"));
	*_currentMode = gInfo->shared_info->current_mode;
	return B_OK;
}


status_t
vesa_get_frame_buffer_config(frame_buffer_config *config)
{
	TRACE(("vesa_get_frame_buffer_config()\n"));

	config->frame_buffer = gInfo->shared_info->frame_buffer;
	config->frame_buffer_dma = gInfo->shared_info->physical_frame_buffer;
	config->bytes_per_row = gInfo->shared_info->bytes_per_row;

	return B_OK;
}


status_t
vesa_get_pixel_clock_limits(display_mode *mode, uint32 *low, uint32 *high)
{
	TRACE(("vesa_get_pixel_clock_limits()\n"));

	// ToDo: do some real stuff here (taken from radeon driver)
	uint32 total_pix = (uint32)mode->timing.h_total * (uint32)mode->timing.v_total;
	uint32 clock_limit = 2000000;

	/* lower limit of about 48Hz vertical refresh */
	*low = (total_pix * 48L) / 1000L;
	if (*low > clock_limit) 
		return B_ERROR;

	*high = clock_limit;
	return B_OK;
}


status_t
vesa_move_display(uint16 h_display_start, uint16 v_display_start)
{
	TRACE(("vesa_move_display()\n"));
	return B_ERROR;
}


status_t
vesa_get_timing_constraints(display_timing_constraints *dtc)
{
	TRACE(("vesa_get_timing_constraints()\n"));
	return B_ERROR;
}


void
vesa_set_indexed_colors(uint count, uint8 first, uint8 *colors, uint32 flags)
{
	TRACE(("vesa_set_indexed_colors()\n"));
	vga_set_indexed_colors_args args;
	args.first = first;
	args.count = count;
	args.colors = colors;
	ioctl(gInfo->device, VGA_SET_INDEXED_COLORS, &args, sizeof(args));
}

