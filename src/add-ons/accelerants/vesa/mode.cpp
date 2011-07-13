/*
 * Copyright 2005-2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <stdlib.h>
#include <string.h>

#include <compute_display_timing.h>
#include <create_display_modes.h>

#include "accelerant_protos.h"
#include "accelerant.h"
#include "utility.h"


//#define TRACE_MODE
#ifdef TRACE_MODE
extern "C" void _sPrintf(const char* format, ...);
#	define TRACE(x) _sPrintf x
#else
#	define TRACE(x) ;
#endif


static uint32
get_color_space_for_depth(uint32 depth)
{
	switch (depth) {
		case 4:
			return B_GRAY8;
				// the app_server is smart enough to translate this to VGA mode
		case 8:
			return B_CMAP8;
		case 15:
			return B_RGB15;
		case 16:
			return B_RGB16;
		case 24:
			return B_RGB24;
		case 32:
			return B_RGB32;
	}

	return 0;
}


/*!	Checks if the specified \a mode can be set using VESA. */
static bool
is_mode_supported(display_mode* mode)
{
	vesa_mode* modes = gInfo->vesa_modes;

	for (uint32 i = gInfo->shared_info->vesa_mode_count; i-- > 0;) {
		// search mode in VESA mode list
		// TODO: list is ordered, we could use binary search
		if (modes[i].width == mode->virtual_width
			&& modes[i].height == mode->virtual_height
			&& get_color_space_for_depth(modes[i].bits_per_pixel)
				== mode->space)
			return true;
	}

	return false;
}


/*!	Creates the initial mode list of the primary accelerant.
	It's called from vesa_init_accelerant().
*/
status_t
create_mode_list(void)
{
	const color_space kVesaSpaces[] = {B_RGB32_LITTLE, B_RGB24_LITTLE,
		B_RGB16_LITTLE, B_RGB15_LITTLE, B_CMAP8};

	// Create the initial list from the support mode list
	// TODO: filter by monitor timing constraints!
	display_mode* initialModes = (display_mode*)malloc(
		sizeof(display_mode) * gInfo->shared_info->vesa_mode_count);
	if (initialModes != NULL) {
		vesa_mode* vesaModes = gInfo->vesa_modes;

		for (uint32 i = gInfo->shared_info->vesa_mode_count; i-- > 0;) {
			compute_display_timing(vesaModes[i].width, vesaModes[i].height, 60,
				false, &initialModes[i].timing);
			fill_display_mode(&initialModes[i]);
		}
	}

	gInfo->mode_list_area = create_display_modes("vesa modes",
		gInfo->shared_info->has_edid ? &gInfo->shared_info->edid_info : NULL,
		initialModes, gInfo->shared_info->vesa_mode_count,
		kVesaSpaces, sizeof(kVesaSpaces) / sizeof(kVesaSpaces[0]),
		is_mode_supported, &gInfo->mode_list, &gInfo->shared_info->mode_count);

	free(initialModes);

	if (gInfo->mode_list_area < 0)
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
vesa_get_mode_list(display_mode* modeList)
{
	TRACE(("vesa_get_mode_info()\n"));
	memcpy(modeList, gInfo->mode_list,
		gInfo->shared_info->mode_count * sizeof(display_mode));
	return B_OK;
}


status_t
vesa_propose_display_mode(display_mode* target, const display_mode* low,
	const display_mode* high)
{
	TRACE(("vesa_propose_display_mode()\n"));

	// just search for the specified mode in the list

	for (uint32 i = 0; i < gInfo->shared_info->mode_count; i++) {
		display_mode* current = &gInfo->mode_list[i];

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
vesa_set_display_mode(display_mode* _mode)
{
	TRACE(("vesa_set_display_mode()\n"));

	display_mode mode = *_mode;
	if (vesa_propose_display_mode(&mode, &mode, &mode) != B_OK)
		return B_BAD_VALUE;

	vesa_mode* modes = gInfo->vesa_modes;
	for (uint32 i = gInfo->shared_info->vesa_mode_count; i-- > 0;) {
		// search mode in VESA mode list
		// TODO: list is ordered, we could use binary search
		if (modes[i].width == mode.virtual_width
			&& modes[i].height == mode.virtual_height
			&& get_color_space_for_depth(modes[i].bits_per_pixel)
				== mode.space)
			return ioctl(gInfo->device, VESA_SET_DISPLAY_MODE, &i, sizeof(i));
	}

	return B_UNSUPPORTED;
}


status_t
vesa_get_display_mode(display_mode* _currentMode)
{
	TRACE(("vesa_get_display_mode()\n"));
	*_currentMode = gInfo->shared_info->current_mode;
	return B_OK;
}


status_t
vesa_get_edid_info(void* info, size_t size, uint32* _version)
{
	TRACE(("intel_get_edid_info()\n"));

	if (!gInfo->shared_info->has_edid)
		return B_ERROR;
	if (size < sizeof(struct edid1_info))
		return B_BUFFER_OVERFLOW;

	memcpy(info, &gInfo->shared_info->edid_info, sizeof(struct edid1_info));
	*_version = EDID_VERSION_1;
	return B_OK;
}


status_t
vesa_get_frame_buffer_config(frame_buffer_config* config)
{
	TRACE(("vesa_get_frame_buffer_config()\n"));

	config->frame_buffer = gInfo->shared_info->frame_buffer;
	config->frame_buffer_dma = gInfo->shared_info->physical_frame_buffer;
	config->bytes_per_row = gInfo->shared_info->bytes_per_row;

	return B_OK;
}


status_t
vesa_get_pixel_clock_limits(display_mode* mode, uint32* _low, uint32* _high)
{
	TRACE(("vesa_get_pixel_clock_limits()\n"));

	// TODO: do some real stuff here (taken from radeon driver)
	uint32 totalPixel = (uint32)mode->timing.h_total
		* (uint32)mode->timing.v_total;
	uint32 clockLimit = 2000000;

	// lower limit of about 48Hz vertical refresh
	*_low = totalPixel * 48L / 1000L;
	if (*_low > clockLimit)
		return B_ERROR;

	*_high = clockLimit;
	return B_OK;
}


status_t
vesa_move_display(uint16 h_display_start, uint16 v_display_start)
{
	TRACE(("vesa_move_display()\n"));
	return B_ERROR;
}


status_t
vesa_get_timing_constraints(display_timing_constraints* constraints)
{
	TRACE(("vesa_get_timing_constraints()\n"));
	return B_ERROR;
}


void
vesa_set_indexed_colors(uint count, uint8 first, uint8* colors, uint32 flags)
{
	TRACE(("vesa_set_indexed_colors()\n"));

	vesa_set_indexed_colors_args args;
	args.first = first;
	args.count = count;
	args.colors = colors;
	ioctl(gInfo->device, VESA_SET_INDEXED_COLORS, &args, sizeof(args));
}

