/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "accelerant_protos.h"
#include "accelerant.h"
#include "utility.h"


#define TRACE_MODE
#ifdef TRACE_MODE
extern "C" void _sPrintf(const char *format, ...);
#	define TRACE(x) _sPrintf x
#else
#	define TRACE(x) ;
#endif


#define	POSITIVE_SYNC \
	(B_POSITIVE_HSYNC | B_POSITIVE_VSYNC)
#define MODE_FLAGS \
	(B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS | B_DPMS | B_SUPPORTS_OVERLAYS)


static const display_mode kBaseModeList[] = {
	{{25175, 640, 656, 752, 800, 350, 387, 389, 449, B_POSITIVE_HSYNC}, B_CMAP8, 640, 350, 0, 0, MODE_FLAGS}, /* 640x350 - www.epanorama.net/documents/pc/vga_timing.html) */
	{{25175, 640, 656, 752, 800, 400, 412, 414, 449, B_POSITIVE_VSYNC}, B_CMAP8, 640, 400, 0, 0, MODE_FLAGS}, /* 640x400 - www.epanorama.net/documents/pc/vga_timing.html) */
	{{25175, 640, 656, 752, 800, 480, 490, 492, 525, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(640X480X8.Z1) */
	{{27500, 640, 672, 768, 864, 480, 488, 494, 530, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* 640X480X60Hz */
	{{30500, 640, 672, 768, 864, 480, 517, 523, 588, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* SVGA_640X480X60HzNI */
	{{31500, 640, 664, 704, 832, 480, 489, 492, 520, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70-72Hz_(640X480X8.Z1) */
	{{31500, 640, 656, 720, 840, 480, 481, 484, 500, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(640X480X8.Z1) */
	{{36000, 640, 696, 752, 832, 480, 481, 484, 509, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(640X480X8.Z1) */
	{{38100, 800, 832, 960, 1088, 600, 602, 606, 620, 0}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS}, /* SVGA_800X600X56HzNI */
	{{40000, 800, 840, 968, 1056, 600, 601, 605, 628, POSITIVE_SYNC}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(800X600X8.Z1) */
	{{49500, 800, 816, 896, 1056, 600, 601, 604, 625, POSITIVE_SYNC}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(800X600X8.Z1) */
	{{50000, 800, 856, 976, 1040, 600, 637, 643, 666, POSITIVE_SYNC}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70-72Hz_(800X600X8.Z1) */
	{{56250, 800, 832, 896, 1048, 600, 601, 604, 631, POSITIVE_SYNC}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(800X600X8.Z1) */
	{{65000, 1024, 1048, 1184, 1344, 768, 771, 777, 806, 0}, B_CMAP8, 1024, 768, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1024X768X8.Z1) */
	{{75000, 1024, 1048, 1184, 1328, 768, 771, 777, 806, 0}, B_CMAP8, 1024, 768, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70-72Hz_(1024X768X8.Z1) */
	{{78750, 1024, 1040, 1136, 1312, 768, 769, 772, 800, POSITIVE_SYNC}, B_CMAP8, 1024, 768, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(1024X768X8.Z1) */
	{{94500, 1024, 1072, 1168, 1376, 768, 769, 772, 808, POSITIVE_SYNC}, B_CMAP8, 1024, 768, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(1024X768X8.Z1) */
	{{94200, 1152, 1184, 1280, 1472, 864, 865, 868, 914, POSITIVE_SYNC}, B_CMAP8, 1152, 864, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70Hz_(1152X864X8.Z1) */
	{{108000, 1152, 1216, 1344, 1600, 864, 865, 868, 900, POSITIVE_SYNC}, B_CMAP8, 1152, 864, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(1152X864X8.Z1) */
	{{121500, 1152, 1216, 1344, 1568, 864, 865, 868, 911, POSITIVE_SYNC}, B_CMAP8, 1152, 864, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(1152X864X8.Z1) */
	{{108000, 1280, 1328, 1440, 1688, 1024, 1025, 1028, 1066, POSITIVE_SYNC}, B_CMAP8, 1280, 1024, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1280X1024X8.Z1) */
	{{135000, 1280, 1296, 1440, 1688, 1024, 1025, 1028, 1066, POSITIVE_SYNC}, B_CMAP8, 1280, 1024, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(1280X1024X8.Z1) */
	{{157500, 1280, 1344, 1504, 1728, 1024, 1025, 1028, 1072, POSITIVE_SYNC}, B_CMAP8, 1280, 1024, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(1280X1024X8.Z1) */
	{{162000, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1600X1200X8.Z1) */
	{{175500, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@65Hz_(1600X1200X8.Z1) */
	{{189000, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70Hz_(1600X1200X8.Z1) */
	{{202500, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(1600X1200X8.Z1) */
	{{216000, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@80Hz_(1600X1200X8.Z1) */
	{{229500, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}  /* Vesa_Monitor_@85Hz_(1600X1200X8.Z1) */
};
static const uint32 kNumBaseModes = sizeof(kBaseModeList) / sizeof(display_mode);
static const uint32 kMaxNumModes = kNumBaseModes * 4;


/**	Creates the initial mode list of the primary accelerant.
 *	It's called from intel_init_accelerant().
 */

status_t
create_mode_list(void)
{
	color_space spaces[4] = {B_RGB32_LITTLE, B_RGB16_LITTLE, B_RGB15_LITTLE, B_CMAP8};
	size_t size = ROUND_TO_PAGE_SIZE(kMaxNumModes * sizeof(display_mode));
	display_mode *list;

	gInfo->mode_list_area = create_area("intel extreme modes", (void **)&list, B_ANY_ADDRESS,
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


static void
compute_pll_divisors(const display_mode &current, uint32 &postDivisor,
	uint32 &nDivisor, uint32 &m1Divisor, uint32 &m2Divisor)
{
	float requestedPixelClock = current.timing.pixel_clock / 1000.0f;
	float referenceClock = gInfo->shared_info->pll_info.reference_frequency / 1000.0f;

	TRACE(("required MHz: %g\n", requestedPixelClock));

	float best = requestedPixelClock;
	uint32 bestP = 0, bestN = 0, bestM = 0;

	// Note, the limits are taken from the X driver; they have not yet been tested

	for (uint32 p = 3; p < 31; p++) {
		for (uint32 n = 3; n < 16; n++) {
			for (uint32 m1 = 6; m1 < 26; m1++) {
				for (uint32 m2 = 6; m2 < 16; m2++) {
					uint32 m = m1 * 5 + m2;
					float error = fabs(requestedPixelClock - ((referenceClock * m) / n) / (p*4));
					if (error < best) {
						best = error;
						bestP = p;
						bestN = n;
						bestM = m;
						if (error == 0)
							break;
					}
				}
			}
		}
	}

	postDivisor = bestP;
	nDivisor = bestN;

	TRACE(("found: %g MHz (p = %lu, n = %lu, m = %lu (m1 = %lu, m2 = %lu)\n",
		((referenceClock * bestM) / bestN) / (bestP*4), bestP, bestN, bestM,
		m1Divisor, m2Divisor));

	m1Divisor = bestM / 5;
	m2Divisor = bestM % 5;
	while (m2Divisor < 6) {
		m1Divisor--;
		m2Divisor += 5;
	}
}


static void
get_color_space_format(const display_mode &mode, uint32 &colorMode,
	uint32 &bytesPerRow, uint32 &bitsPerPixel)
{
	uint32 bytesPerPixel;

	switch (mode.space) {
		case B_RGB32_LITTLE:
			colorMode = DISPLAY_CONTROL_RGB32;
			bytesPerPixel = 4;
			bitsPerPixel = 32;
			break;
		case B_RGB16_LITTLE:
			colorMode = DISPLAY_CONTROL_RGB16;
			bytesPerPixel = 2;
			bitsPerPixel = 16;
			break;
		case B_RGB15_LITTLE:
			colorMode = DISPLAY_CONTROL_RGB15;
			bytesPerPixel = 2;
			bitsPerPixel = 15;
			break;
		case B_CMAP8:
		default:
			colorMode = DISPLAY_CONTROL_CMAP8;
			bytesPerPixel = 1;
			bitsPerPixel = 8;
			break;
	}

	bytesPerRow = mode.virtual_width * bytesPerPixel;
}


//	#pragma mark -


uint32
intel_accelerant_mode_count(void)
{
	TRACE(("intel_accelerant_mode_count()\n"));
	return gInfo->shared_info->mode_count;
}


status_t
intel_get_mode_list(display_mode *modeList)
{
	TRACE(("intel_get_mode_info()\n"));
	memcpy(modeList, gInfo->mode_list, gInfo->shared_info->mode_count * sizeof(display_mode));
	return B_OK;
}


status_t
intel_propose_display_mode(display_mode *target, const display_mode *low,
	const display_mode *high)
{
	TRACE(("intel_propose_display_mode()\n"));

	// just search for the specified mode in the list

	for (uint32 i = 0; i < gInfo->shared_info->mode_count; i++) {
		display_mode *current = &gInfo->mode_list[i];

		// TODO: improve this, ie. adapt pixel clock to allowed values!!!

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
intel_set_display_mode(display_mode *mode)
{
	TRACE(("intel_set_display_mode()\n"));

	intel_shared_info &sharedInfo = *gInfo->shared_info;

	if (mode == NULL)
		return B_BAD_VALUE;

	Autolock locker(sharedInfo.accelerant_lock);
	display_mode current = *mode;

	set_display_power_mode(B_DPMS_OFF);

	uint32 colorMode, bytesPerRow, bitsPerPixel;
	get_color_space_format(current, colorMode, bytesPerRow, bitsPerPixel);

	// free old and allocate new frame buffer in graphics memory

	intel_free_memory(gInfo->frame_buffer_handle);

	uint32 offset;
	if (intel_allocate_memory(bytesPerRow * current.virtual_height,
			gInfo->frame_buffer_handle, offset) < B_OK) {
		// oh, how did that happen? Unfortunately, there is no really good way back
		if (intel_allocate_memory(sharedInfo.current_mode.virtual_height
				* sharedInfo.bytes_per_row, gInfo->frame_buffer_handle,
				offset) == B_OK) {
			sharedInfo.frame_buffer_offset = offset;
			write32(INTEL_DISPLAY_BASE, offset);
		}

		return B_NO_MEMORY;
	}

	sharedInfo.frame_buffer_offset = offset;

	// update timing parameters

	write32(INTEL_DISPLAY_HTOTAL, ((uint32)(current.timing.h_total - 1) << 16)
		| ((uint32)current.timing.h_display - 1));
	write32(INTEL_DISPLAY_HBLANK, ((uint32)(current.timing.h_total - 1) << 16)
		| ((uint32)current.timing.h_display - 1));
	write32(INTEL_DISPLAY_HSYNC, ((uint32)(current.timing.h_sync_end - 1) << 16)
		| ((uint32)current.timing.h_sync_start - 1));

	write32(INTEL_DISPLAY_VTOTAL, ((uint32)(current.timing.v_total - 1) << 16)
		| ((uint32)current.timing.v_display - 1));
	write32(INTEL_DISPLAY_VBLANK, ((uint32)(current.timing.v_total - 1) << 16)
		| ((uint32)current.timing.v_display - 1));
	write32(INTEL_DISPLAY_VSYNC, ((uint32)(current.timing.v_sync_end - 1) << 16)
		| ((uint32)current.timing.v_sync_start - 1));

	write32(INTEL_DISPLAY_IMAGE_SIZE, ((uint32)(current.timing.h_display - 1) << 16)
		| ((uint32)current.timing.v_display - 1));

	write32(INTEL_DISPLAY_ANALOG_PORT, (read32(INTEL_DISPLAY_ANALOG_PORT)
		& ~(DISPLAY_MONITOR_POLARITY_MASK | DISPLAY_MONITOR_VGA_POLARITY))
		| ((current.timing.flags & B_POSITIVE_HSYNC) != 0 ? DISPLAY_MONITOR_POSITIVE_HSYNC : 0)
		| ((current.timing.flags & B_POSITIVE_VSYNC) != 0 ? DISPLAY_MONITOR_POSITIVE_VSYNC : 0));

	uint32 postDivisor, nDivisor, m1Divisor, m2Divisor;
	compute_pll_divisors(current, postDivisor, nDivisor, m1Divisor, m2Divisor);

	// switch divisor register with every mode change (not required)
	uint32 divisorRegister;
	if (gInfo->shared_info->pll_info.divisor_register == INTEL_DISPLAY_PLL_DIVISOR_0)
		divisorRegister = INTEL_DISPLAY_PLL_DIVISOR_1;
	else
		divisorRegister = INTEL_DISPLAY_PLL_DIVISOR_0;

	write32(divisorRegister,
		(((nDivisor - 2) << DISPLAY_PLL_N_DIVISOR_SHIFT) & DISPLAY_PLL_N_DIVISOR_MASK)
		| (((m1Divisor - 2) << DISPLAY_PLL_M1_DIVISOR_SHIFT) & DISPLAY_PLL_M1_DIVISOR_MASK)
		| (((m2Divisor - 2) << DISPLAY_PLL_M2_DIVISOR_SHIFT) & DISPLAY_PLL_M2_DIVISOR_MASK));
	write32(INTEL_DISPLAY_PLL, DISPLAY_PLL_ENABLED | DISPLAY_PLL_2X_CLOCK
		| DISPLAY_PLL_NO_VGA_CONTROL | DISPLAY_PLL_DIVIDE_4X
		| (((postDivisor - 2) << DISPLAY_PLL_POST_DIVISOR_SHIFT) & DISPLAY_PLL_POST_DIVISOR_MASK)
		| (divisorRegister == INTEL_DISPLAY_PLL_DIVISOR_1 ? DISPLAY_PLL_DIVISOR_1 : 0));

	write32(INTEL_DISPLAY_CONTROL, (read32(INTEL_DISPLAY_CONTROL)
		& ~DISPLAY_CONTROL_COLOR_MASK) | colorMode);

	set_display_power_mode(sharedInfo.dpms_mode);

	// changing bytes per row seems to be ignored if the plane/pipe is turned off
	write32(INTEL_DISPLAY_BYTES_PER_ROW, bytesPerRow);
	write32(INTEL_DISPLAY_BASE, sharedInfo.frame_buffer_offset);
		// triggers writing back double-buffered registers

	// update shared info
	sharedInfo.bytes_per_row = bytesPerRow;
	sharedInfo.current_mode = current;
	sharedInfo.bits_per_pixel = bitsPerPixel;

	return B_OK;
}


status_t
intel_get_display_mode(display_mode *_currentMode)
{
	TRACE(("intel_get_display_mode()\n"));
	*_currentMode = gInfo->shared_info->current_mode;
	return B_OK;
}


status_t
intel_get_frame_buffer_config(frame_buffer_config *config)
{
	TRACE(("intel_get_frame_buffer_config()\n"));

	uint32 offset = gInfo->shared_info->frame_buffer_offset;

	config->frame_buffer = gInfo->shared_info->graphics_memory + offset;
	config->frame_buffer_dma = gInfo->shared_info->physical_graphics_memory + offset;
	config->bytes_per_row = gInfo->shared_info->bytes_per_row;

	return B_OK;
}


status_t
intel_get_pixel_clock_limits(display_mode *mode, uint32 *_low, uint32 *_high)
{
	TRACE(("intel_get_pixel_clock_limits()\n"));

	if (_low != NULL) {
		// lower limit of about 48Hz vertical refresh
		uint32 totalClocks = (uint32)mode->timing.h_total * (uint32)mode->timing.v_total;
		uint32 low = (totalClocks * 48L) / 1000L;
		if (low < gInfo->shared_info->pll_info.min_frequency)
			low = gInfo->shared_info->pll_info.min_frequency;
		else if (low > gInfo->shared_info->pll_info.max_frequency)
			return B_ERROR;

		*_low = low;
	}

	if (_high != NULL)
		*_high = gInfo->shared_info->pll_info.max_frequency;

	return B_OK;
}


status_t
intel_move_display(uint16 horizontalStart, uint16 verticalStart)
{
	TRACE(("intel_move_display()\n"));

	intel_shared_info &sharedInfo = *gInfo->shared_info;
	Autolock locker(sharedInfo.accelerant_lock);

	display_mode &mode = sharedInfo.current_mode;

	if (horizontalStart + mode.timing.h_display > mode.virtual_width
		|| verticalStart + mode.timing.v_display > mode.virtual_height)
		return B_BAD_VALUE;

	mode.h_display_start = horizontalStart;
	mode.v_display_start = verticalStart;

	write32(INTEL_DISPLAY_BASE, sharedInfo.frame_buffer_offset
		+ verticalStart * sharedInfo.bytes_per_row
		+ horizontalStart * (sharedInfo.bits_per_pixel + 7) / 8);

	return B_OK;
}


status_t
intel_get_timing_constraints(display_timing_constraints *constraints)
{
	TRACE(("intel_get_timing_contraints()\n"));
	return B_ERROR;
}


void
intel_set_indexed_colors(uint count, uint8 first, uint8 *colors, uint32 flags)
{
	TRACE(("intel_set_indexed_colors(colors = %p, first = %u)\n", colors, first));

	if (colors == NULL)
		return;

	Autolock locker(gInfo->shared_info->accelerant_lock);

	for (; count-- > 0; first++) {
		uint32 color = colors[0] << 16 | colors[1] << 8 | colors[2];
		colors += 3;

		write32(INTEL_DISPLAY_PALETTE + first * sizeof(uint32), color);
	}
}

