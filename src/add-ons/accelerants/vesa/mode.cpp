/*
 * Copyright 2005-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <string.h>

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


#define	T_POSITIVE_SYNC	(B_POSITIVE_HSYNC | B_POSITIVE_VSYNC)
#define MODE_FLAGS	(B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS | B_DPMS /*| B_SUPPORTS_OVERLAYS*/)

static const display_mode kBaseModeList[] = {
{ { 25175, 640, 656, 752, 800, 350, 387, 389, 449, B_POSITIVE_HSYNC}, B_CMAP8, 640, 350, 0, 0, MODE_FLAGS}, /* 640x350 - www.epanorama.net/documents/pc/vga_timing.html) */
{ { 25175, 640, 656, 752, 800, 400, 412, 414, 449, B_POSITIVE_VSYNC}, B_CMAP8, 640, 400, 0, 0, MODE_FLAGS}, /* 640x400 - www.epanorama.net/documents/pc/vga_timing.html) */
{ { 25175, 640, 656, 752, 800, 480, 490, 492, 525, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(640X480X8.Z1) */
{ { 27500, 640, 672, 768, 864, 480, 488, 494, 530, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* 640X480X60Hz */
{ { 30500, 640, 672, 768, 864, 480, 517, 523, 588, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* SVGA_640X480X60HzNI */
{ { 31500, 640, 664, 704, 832, 480, 489, 492, 520, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70-72Hz_(640X480X8.Z1) */
{ { 31500, 640, 656, 720, 840, 480, 481, 484, 500, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(640X480X8.Z1) */
{ { 36000, 640, 696, 752, 832, 480, 481, 484, 509, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(640X480X8.Z1) */
{ { 38100, 800, 832, 960, 1088, 600, 602, 606, 620, 0}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS}, /* SVGA_800X600X56HzNI */
{ { 40000, 800, 840, 968, 1056, 600, 601, 605, 628, T_POSITIVE_SYNC}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(800X600X8.Z1) */
{ { 49500, 800, 816, 896, 1056, 600, 601, 604, 625, T_POSITIVE_SYNC}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(800X600X8.Z1) */
{ { 50000, 800, 856, 976, 1040, 600, 637, 643, 666, T_POSITIVE_SYNC}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70-72Hz_(800X600X8.Z1) */
{ { 56250, 800, 832, 896, 1048, 600, 601, 604, 631, T_POSITIVE_SYNC}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(800X600X8.Z1) */
{ { 65000, 1024, 1048, 1184, 1344, 768, 771, 777, 806, 0}, B_CMAP8, 1024, 768, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1024X768X8.Z1) */
{ { 75000, 1024, 1048, 1184, 1328, 768, 771, 777, 806, 0}, B_CMAP8, 1024, 768, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70-72Hz_(1024X768X8.Z1) */
{ { 78750, 1024, 1040, 1136, 1312, 768, 769, 772, 800, T_POSITIVE_SYNC}, B_CMAP8, 1024, 768, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(1024X768X8.Z1) */
{ { 94500, 1024, 1072, 1168, 1376, 768, 769, 772, 808, T_POSITIVE_SYNC}, B_CMAP8, 1024, 768, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(1024X768X8.Z1) */
{ { 94200, 1152, 1184, 1280, 1472, 864, 865, 868, 914, T_POSITIVE_SYNC}, B_CMAP8, 1152, 864, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70Hz_(1152X864X8.Z1) */
{ { 108000, 1152, 1216, 1344, 1600, 864, 865, 868, 900, T_POSITIVE_SYNC}, B_CMAP8, 1152, 864, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(1152X864X8.Z1) */
{ { 121500, 1152, 1216, 1344, 1568, 864, 865, 868, 911, T_POSITIVE_SYNC}, B_CMAP8, 1152, 864, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(1152X864X8.Z1) */
{ { 108000, 1280, 1328, 1440, 1688, 1024, 1025, 1028, 1066, T_POSITIVE_SYNC}, B_CMAP8, 1280, 1024, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1280X1024X8.Z1) */
{ { 135000, 1280, 1296, 1440, 1688, 1024, 1025, 1028, 1066, T_POSITIVE_SYNC}, B_CMAP8, 1280, 1024, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(1280X1024X8.Z1) */
{ { 157500, 1280, 1344, 1504, 1728, 1024, 1025, 1028, 1072, T_POSITIVE_SYNC}, B_CMAP8, 1280, 1024, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(1280X1024X8.Z1) */
{ { 162000, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1600X1200X8.Z1) */
{ { 175500, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@65Hz_(1600X1200X8.Z1) */
{ { 189000, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70Hz_(1600X1200X8.Z1) */
{ { 202500, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(1600X1200X8.Z1) */
{ { 216000, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@80Hz_(1600X1200X8.Z1) */
{ { 229500, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}  /* Vesa_Monitor_@85Hz_(1600X1200X8.Z1) */
};
static const uint32 kNumBaseModes = sizeof(kBaseModeList) / sizeof(display_mode);
static const uint32 kMaxNumModes = kNumBaseModes * 4;


/**	Creates the initial mode list of the primary accelerant.
 *	It's called from vesa_init_accelerant().
 */

status_t
create_mode_list(void)
{
	// ToDo: basically, the boot loader should pass a list of all supported
	//	VESA modes to us.
	color_space spaces[4] = {B_RGB32_LITTLE, B_RGB16_LITTLE, B_RGB15_LITTLE, B_CMAP8};
	size_t size = ROUND_TO_PAGE_SIZE(kMaxNumModes * sizeof(display_mode));
	display_mode *list;

	gInfo->mode_list_area = create_area("vesa modes", (void **)&list, B_ANY_ADDRESS,
		size, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (gInfo->mode_list_area < B_OK)
		return gInfo->mode_list_area;

	// for now, just copy all of the modes with different color spaces

	const display_mode *source = kBaseModeList;
	uint32 count = 0;

	for (uint32 i = 0; i < kNumBaseModes; i++) {
		for (uint32 j = 0; j < (sizeof(spaces) / sizeof(color_space)); j++) {
			list[count] = *source;
			list[count].space = spaces[j];

			count++;
		}

		source++;
	}

	gInfo->mode_list = list;
	gInfo->shared_info->mode_list_area = gInfo->mode_list_area;
	gInfo->shared_info->mode_count = count;

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
#if 0
	gInfo->shared_info->current_mode = mode;

	switch (mode.space) {
		case B_RGB32_LITTLE:
			gInfo->shared_info->bytes_per_row = mode.virtual_width * 4;
			break;
		case B_RGB16_LITTLE:
		case B_RGB15_LITTLE:
			gInfo->shared_info->bytes_per_row = mode.virtual_width * 2;
			break;
		case B_CMAP8:
			gInfo->shared_info->bytes_per_row = mode.virtual_width;
			break;
	}
	return B_OK;
#endif
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

