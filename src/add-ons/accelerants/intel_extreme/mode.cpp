/*
 * Copyright 2006-2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Support for i915 chipset and up based on the X driver,
 * Copyright 2006-2007 Intel Corporation.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "accelerant_protos.h"
#include "accelerant.h"
#include "utility.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <create_display_modes.h>
#include <ddc.h>
#include <edid.h>


#define TRACE_MODE
#ifdef TRACE_MODE
extern "C" void _sPrintf(const char *format, ...);
#	define TRACE(x) _sPrintf x
#else
#	define TRACE(x) ;
#endif


struct display_registers {
	uint32	pll;
	uint32	divisors;
	uint32	control;
	uint32	pipe_config;
	uint32	horiz_total;
	uint32	horiz_blank;
	uint32	horiz_sync;
	uint32	vert_total;
	uint32	vert_blank;
	uint32	vert_sync;
	uint32	size;
	uint32	stride;
	uint32	position;
	uint32	pipe_source;
};

struct pll_divisors {
	uint32	post;
	uint32	post1;
	uint32	post2;
	bool	post2_high;
	uint32	n;
	uint32	m;
	uint32	m1;
	uint32	m2;
};

struct pll_limits {
	pll_divisors	min;
	pll_divisors	max;
	uint32			min_post2_frequency;
	uint32			min_vco;
	uint32			max_vco;
};


static status_t
get_i2c_signals(void* cookie, int* _clock, int* _data)
{
	uint32 ioRegister = (uint32)cookie;
	uint32 value = read32(ioRegister);

	*_clock = (value & I2C_CLOCK_VALUE_IN) != 0;
	*_data = (value & I2C_DATA_VALUE_IN) != 0;

	return B_OK;
}


static status_t
set_i2c_signals(void* cookie, int clock, int data)
{
	uint32 ioRegister = (uint32)cookie;
	uint32 value;

	if (gInfo->shared_info->device_type == INTEL_TYPE_83x) {
		// on these chips, the reserved values are fixed
		value = 0;
	} else {
		// on all others, we have to preserve them manually
		value = read32(ioRegister) & I2C_RESERVED;
	}

	if (data != 0)
		value |= I2C_DATA_DIRECTION_MASK;
	else
		value |= I2C_DATA_DIRECTION_MASK | I2C_DATA_DIRECTION_OUT | I2C_DATA_VALUE_MASK;

	if (clock != 0)
		value |= I2C_CLOCK_DIRECTION_MASK;
	else
		value |= I2C_CLOCK_DIRECTION_MASK | I2C_CLOCK_DIRECTION_OUT | I2C_CLOCK_VALUE_MASK;

	write32(ioRegister, value);
	read32(ioRegister);
		// make sure the PCI bus has flushed the write

	return B_OK;
}


void
set_frame_buffer_base()
{
	intel_shared_info &sharedInfo = *gInfo->shared_info;
	display_mode &mode = sharedInfo.current_mode;
	uint32 baseRegister;
	uint32 surfaceRegister;

	if (gInfo->head_mode & HEAD_MODE_A_ANALOG) {
		baseRegister = INTEL_DISPLAY_A_BASE;
		surfaceRegister = INTEL_DISPLAY_A_SURFACE;
	} else {
		baseRegister = INTEL_DISPLAY_B_BASE;
		surfaceRegister = INTEL_DISPLAY_B_SURFACE;
	}

	if (sharedInfo.device_type == INTEL_TYPE_965) {
		write32(baseRegister, mode.v_display_start * sharedInfo.bytes_per_row
			+ mode.h_display_start * (sharedInfo.bits_per_pixel + 7) / 8);
		read32(baseRegister);
		write32(surfaceRegister, sharedInfo.frame_buffer_offset);
		read32(surfaceRegister);
	} else {
		write32(baseRegister, sharedInfo.frame_buffer_offset
			+ mode.v_display_start * sharedInfo.bytes_per_row
			+ mode.h_display_start * (sharedInfo.bits_per_pixel + 7) / 8);
		read32(baseRegister);
	}
}


/*!	Creates the initial mode list of the primary accelerant.
	It's called from intel_init_accelerant().
*/
status_t
create_mode_list(void)
{
	i2c_bus bus;
	bus.cookie = (void*)INTEL_I2C_IO_A;
	bus.set_signals = &set_i2c_signals;
	bus.get_signals = &get_i2c_signals;
	ddc2_init_timing(&bus);

	if (ddc2_read_edid1(&bus, &gInfo->edid_info, NULL, NULL) == B_OK) {
		edid_dump(&gInfo->edid_info);
		gInfo->has_edid = true;
	} else {
		TRACE(("intel_extreme: getting EDID failed!\n"));
	}

	// TODO: support lower modes via scaling and windowing
	if (gInfo->head_mode & HEAD_MODE_LVDS_PANEL
		&& ((gInfo->head_mode & HEAD_MODE_A_ANALOG) == 0)) {
		size_t size = (sizeof(display_mode) + B_PAGE_SIZE - 1)
			& ~(B_PAGE_SIZE - 1);

		display_mode *list;
		area_id area = create_area("intel extreme modes", (void **)&list,
			B_ANY_ADDRESS, size, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
		if (area < B_OK)
			return area;

		memcpy(list, &gInfo->lvds_panel_mode, sizeof(display_mode));

		gInfo->mode_list_area = area;
		gInfo->mode_list = list;
		gInfo->shared_info->mode_list_area = gInfo->mode_list_area;
		gInfo->shared_info->mode_count = 1;
		return B_OK;
	}

	// Otherwise return the 'real' list of modes
	display_mode *list;
	uint32 count = 0;
	gInfo->mode_list_area = create_display_modes("intel extreme modes",
		gInfo->has_edid ? &gInfo->edid_info : NULL, NULL, 0, NULL, 0, NULL,
		&list, &count);
	if (gInfo->mode_list_area < B_OK)
		return gInfo->mode_list_area;

	gInfo->mode_list = list;
	gInfo->shared_info->mode_list_area = gInfo->mode_list_area;
	gInfo->shared_info->mode_count = count;

	return B_OK;
}


void
wait_for_vblank(void)
{
	acquire_sem_etc(gInfo->shared_info->vblank_sem, 1, B_RELATIVE_TIMEOUT, 25000);
		// With the output turned off via DPMS, we might not get any interrupts anymore
		// that's why we don't wait forever for it.
}


static void
get_pll_limits(pll_limits &limits)
{
	// Note, the limits are taken from the X driver; they have not yet been
	// tested

	if ((gInfo->shared_info->device_type & INTEL_TYPE_9xx) != 0) {
		// TODO: support LVDS output limits as well
		// (Update: Output limits are adjusted in the computation (post2=7/14))
		// Should move them here!
		static const pll_limits kLimits = {
			// p, p1, p2, high,   n,   m, m1, m2
			{  5,  1, 10, false,  5,  70, 12,  7},	// min
			{ 80,  8,  5, true,  10, 120, 22, 11},	// max
			200000, 1400000, 2800000
		};
		limits = kLimits;
	} else {
		// TODO: support LVDS output limits as well
		static const pll_limits kLimits = {
			// p, p1, p2, high,   n,   m, m1, m2
			{  4,  2,  4, false,  5,  96, 20,  8},
			{128, 33,  2, true,  18, 140, 28, 18},
			165000, 930000, 1400000
		};
		limits = kLimits;
	}

	TRACE(("PLL limits, min: p %lu (p1 %lu, p2 %lu), n %lu, m %lu (m1 %lu, m2 %lu)\n",
		limits.min.post, limits.min.post1, limits.min.post2, limits.min.n,
		limits.min.m, limits.min.m1, limits.min.m2));
	TRACE(("PLL limits, max: p %lu (p1 %lu, p2 %lu), n %lu, m %lu (m1 %lu, m2 %lu)\n",
		limits.max.post, limits.max.post1, limits.max.post2, limits.max.n,
		limits.max.m, limits.max.m1, limits.max.m2));
}


static bool
valid_pll_divisors(const pll_divisors& divisors, const pll_limits& limits)
{
	pll_info &info = gInfo->shared_info->pll_info;
	uint32 vco = info.reference_frequency * divisors.m / divisors.n;
	uint32 frequency = vco / divisors.post;

	if (divisors.post < limits.min.post || divisors.post > limits.max.post
		|| divisors.m < limits.min.m || divisors.m > limits.max.m
		|| vco < limits.min_vco || vco > limits.max_vco
		|| frequency < info.min_frequency || frequency > info.max_frequency)
		return false;

	return true;
}


static void
compute_pll_divisors(const display_mode &current, pll_divisors& divisors,
	bool isLVDS)
{
	float requestedPixelClock = current.timing.pixel_clock / 1000.0f;
	float referenceClock = gInfo->shared_info->pll_info.reference_frequency / 1000.0f;
	pll_limits limits;
	get_pll_limits(limits);

	TRACE(("required MHz: %g\n", requestedPixelClock));

	if (isLVDS) {
		if ((read32(INTEL_DISPLAY_LVDS_PORT) & LVDS_CLKB_POWER_MASK)
				== LVDS_CLKB_POWER_UP)
			divisors.post2 = LVDS_POST2_RATE_FAST;
		else
			divisors.post2 = LVDS_POST2_RATE_SLOW;	
	} else {
		if (current.timing.pixel_clock < limits.min_post2_frequency) {
			// slow DAC timing
			divisors.post2 = limits.min.post2;
			divisors.post2_high = limits.min.post2_high;
		} else {
			// fast DAC timing
			divisors.post2 = limits.max.post2;
			divisors.post2_high = limits.max.post2_high;
		}
	}

	float best = requestedPixelClock;
	pll_divisors bestDivisors;

	for (divisors.m1 = limits.min.m1; divisors.m1 <= limits.max.m1; divisors.m1++) {
		for (divisors.m2 = limits.min.m2; divisors.m2 < divisors.m1
				&& divisors.m2 <= limits.max.m2; divisors.m2++) {
			for (divisors.n = limits.min.n; divisors.n <= limits.max.n;
					divisors.n++) {
				for (divisors.post1 = limits.min.post1;
						divisors.post1 <= limits.max.post1; divisors.post1++) {
					divisors.m = 5 * divisors.m1 + divisors.m2;
					divisors.post = divisors.post1 * divisors.post2;

					if (!valid_pll_divisors(divisors, limits))
						continue;

					float error = fabs(requestedPixelClock
						- ((referenceClock * divisors.m) / divisors.n) / divisors.post);
					if (error < best) {
						best = error;
						bestDivisors = divisors;

						if (error == 0)
							break;
					}
				}
			}
		}
	}

	divisors = bestDivisors;

	TRACE(("found: %g MHz, p = %lu (p1 = %lu, p2 = %lu), n = %lu, m = %lu (m1 = %lu, m2 = %lu)\n",
		((referenceClock * divisors.m) / divisors.n) / divisors.post,
		divisors.post, divisors.post1, divisors.post2, divisors.n,
		divisors.m, divisors.m1, divisors.m2));
}


/*! Store away panel information if identified on startup
	(used for pipe B->lvds).
*/
void
save_lvds_mode(void)
{
	// dump currently programmed mode.
	display_mode biosMode;
	
	uint32 pll = read32(INTEL_DISPLAY_B_PLL);
	uint32 pllDivisor = read32(INTEL_DISPLAY_B_PLL_DIVISOR_0);

	pll_divisors divisors;
	divisors.m1 = (pllDivisor & DISPLAY_PLL_M1_DIVISOR_MASK)
		>> DISPLAY_PLL_M1_DIVISOR_SHIFT;
	divisors.m2 = (pllDivisor & DISPLAY_PLL_M2_DIVISOR_MASK)
		>> DISPLAY_PLL_M2_DIVISOR_SHIFT;
	divisors.n = (pllDivisor & DISPLAY_PLL_N_DIVISOR_MASK)
		>> DISPLAY_PLL_N_DIVISOR_SHIFT;

	pll_limits limits;
	get_pll_limits(limits);

	if ((gInfo->shared_info->device_type & INTEL_TYPE_9xx) != 0) {
		divisors.post1 = (pll & DISPLAY_PLL_9xx_POST1_DIVISOR_MASK)
			>> DISPLAY_PLL_POST1_DIVISOR_SHIFT;

		if ((pll & DISPLAY_PLL_DIVIDE_HIGH) != 0)
			divisors.post2 = limits.max.post2;
		else
			divisors.post2 = limits.min.post2;

		// Fix this? Need to support dual channel LVDS.
		divisors.post2 = LVDS_POST2_RATE_SLOW;
	} else {
		// 8xx
		divisors.post1 = (pll & DISPLAY_PLL_POST1_DIVISOR_MASK)
			>> DISPLAY_PLL_POST1_DIVISOR_SHIFT;

		if ((pll & DISPLAY_PLL_DIVIDE_4X) != 0)
			divisors.post2 = limits.max.post2;
		else
			divisors.post2 = limits.min.post2;
	}

	divisors.m = 5 * divisors.m1 + divisors.m2;
	divisors.post = divisors.post1 * divisors.post2;

	float referenceClock = gInfo->shared_info->pll_info.reference_frequency
		/ 1000.0f;
	float pixelClock = ((referenceClock * divisors.m) / divisors.n)
		/ divisors.post;

	// timing

	biosMode.timing.pixel_clock = uint32(pixelClock * 1000);
	biosMode.timing.flags = 0;

	uint32 value = read32(INTEL_DISPLAY_B_HTOTAL);
	biosMode.timing.h_total = (value >> 16) + 1;
	biosMode.timing.h_display = (value & 0xffff) + 1;

	value = read32(INTEL_DISPLAY_B_HSYNC);
	biosMode.timing.h_sync_end = (value >> 16) + 1;
	biosMode.timing.h_sync_start = (value & 0xffff) + 1;

	value = read32(INTEL_DISPLAY_B_VTOTAL);
	biosMode.timing.v_total = (value >> 16) + 1;
	biosMode.timing.v_display = (value & 0xffff) + 1;

	value = read32(INTEL_DISPLAY_B_VSYNC);
	biosMode.timing.v_sync_end = (value >> 16) + 1;
	biosMode.timing.v_sync_start = (value & 0xffff) + 1;

	// image size and color space

	// using virtual size based on image size is the 'proper' way to do it, however the bios appears to be
	// suggesting scaling or somesuch, so ignore the proper virtual way for now.

	biosMode.virtual_width = biosMode.timing.h_display;
	biosMode.virtual_height = biosMode.timing.v_display;

	//value = read32(INTEL_DISPLAY_B_IMAGE_SIZE);
	//biosMode.virtual_width = (value >> 16) + 1;
	//biosMode.virtual_height = (value & 0xffff) + 1;

	value = read32(INTEL_DISPLAY_B_CONTROL);
	switch (value & DISPLAY_CONTROL_COLOR_MASK) {
		case DISPLAY_CONTROL_RGB32:
		default:
			biosMode.space = B_RGB32;
			break;
		case DISPLAY_CONTROL_RGB16:
			biosMode.space = B_RGB16;
			break;
		case DISPLAY_CONTROL_RGB15:
			biosMode.space = B_RGB15;
			break;
		case DISPLAY_CONTROL_CMAP8:
			biosMode.space = B_CMAP8;
			break;
	}

	biosMode.h_display_start = 0;
	biosMode.v_display_start = 0;
	biosMode.flags = 0;

	gInfo->lvds_panel_mode = biosMode;
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
	memcpy(modeList, gInfo->mode_list,
		gInfo->shared_info->mode_count * sizeof(display_mode));
	return B_OK;
}


status_t
intel_propose_display_mode(display_mode *target, const display_mode *low,
	const display_mode *high)
{
	TRACE(("intel_propose_display_mode()\n"));

	// just search for the specified mode in the list

	for (uint32 i = 0; i < gInfo->shared_info->mode_count; i++) {
		display_mode *mode = &gInfo->mode_list[i];

		// TODO: improve this, ie. adapt pixel clock to allowed values!!!

		if (target->virtual_width != mode->virtual_width
			|| target->virtual_height != mode->virtual_height
			|| target->space != mode->space)
			continue;

		*target = *mode;
		return B_OK;
	}
	return B_BAD_VALUE;
}


status_t
intel_set_display_mode(display_mode *mode)
{
	TRACE(("intel_set_display_mode()\n"));

	if (mode == NULL || intel_propose_display_mode(&target, mode, mode))
		return B_BAD_VALUE;

	display_mode target = *mode;

	uint32 colorMode, bytesPerRow, bitsPerPixel;
	get_color_space_format(target, colorMode, bytesPerRow, bitsPerPixel);

#if 0
static bool first = true;
if (first) {
	int fd = open("/boot/home/ie_.regs", O_CREAT | O_WRONLY, 0644);
	if (fd >= 0) {
		for (int32 i = 0; i < 0x80000; i += 16) {
			char line[512];
			int length = sprintf(line, "%05lx: %08lx %08lx %08lx %08lx\n",
				i, read32(i), read32(i + 4), read32(i + 8), read32(i + 12));
			write(fd, line, length);
		}
		close(fd);
		sync();
	}
	first = false;
}
#endif

	intel_shared_info &sharedInfo = *gInfo->shared_info;
	Autolock locker(sharedInfo.accelerant_lock);

	// TODO: This may not be neccesary
	set_display_power_mode(B_DPMS_OFF);

	// free old and allocate new frame buffer in graphics memory

	intel_free_memory(sharedInfo.frame_buffer);

	uint32 base;
	if (intel_allocate_memory(bytesPerRow * target.virtual_height, 0,
			base) < B_OK) {
		// oh, how did that happen? Unfortunately, there is no really good way back
		if (intel_allocate_memory(sharedInfo.current_mode.virtual_height
				* sharedInfo.bytes_per_row, 0, base) == B_OK) {
			sharedInfo.frame_buffer = base;
			sharedInfo.frame_buffer_offset = base
				- (addr_t)sharedInfo.graphics_memory;
			set_frame_buffer_base();
		}

		return B_NO_MEMORY;
	}

	// clear frame buffer before using it
	memset((uint8 *)base, 0, bytesPerRow * target.virtual_height);
	sharedInfo.frame_buffer = base;
	sharedInfo.frame_buffer_offset = base - (addr_t)sharedInfo.graphics_memory;

	// make sure VGA display is disabled
	write32(INTEL_VGA_DISPLAY_CONTROL, VGA_DISPLAY_DISABLED);
	read32(INTEL_VGA_DISPLAY_CONTROL);

	if (gInfo->shared_info->device_type != INTEL_TYPE_85x) {
	}

	if ((gInfo->head_mode & HEAD_MODE_B_DIGITAL) != 0) {
		pll_divisors divisors;
		compute_pll_divisors(target, divisors,true);

		uint32 dpll = DISPLAY_PLL_NO_VGA_CONTROL;
		if ((gInfo->shared_info->device_type & INTEL_TYPE_9xx) != 0) {
			 dpll |= LVDS_PLL_MODE_LVDS;
			 	// DPLL mode LVDS for i915+
		}

		// compute bitmask from p1 value
		dpll |= (1 << (divisors.post1 - 1)) << 16;
		switch (divisors.post2) {
			case 5:
			case 7:
				dpll |= DISPLAY_PLL_DIVIDE_HIGH;
				break;
		}

		dpll |= (1 << (divisors.post1 - 1)) << DISPLAY_PLL_POST1_DIVISOR_SHIFT;

		uint32 displayControl = ~(DISPLAY_CONTROL_COLOR_MASK
			| DISPLAY_CONTROL_GAMMA) | colorMode;
		displayControl |= 1 << 24; // select pipe B

		// runs in dpms also?
		displayControl |= DISPLAY_PIPE_ENABLED;
		dpll |= DISPLAY_PLL_ENABLED;

		write32(INTEL_PANEL_FIT_CONTROL, 0);

		if ((dpll & DISPLAY_PLL_ENABLED) != 0) {
			write32(INTEL_DISPLAY_B_PLL_DIVISOR_0,
				(((divisors.n - 2) << DISPLAY_PLL_N_DIVISOR_SHIFT) & DISPLAY_PLL_N_DIVISOR_MASK)
				| (((divisors.m1 - 2) << DISPLAY_PLL_M1_DIVISOR_SHIFT) & DISPLAY_PLL_M1_DIVISOR_MASK)
				| (((divisors.m2 - 2) << DISPLAY_PLL_M2_DIVISOR_SHIFT) & DISPLAY_PLL_M2_DIVISOR_MASK));
			write32(INTEL_DISPLAY_B_PLL, dpll & ~DISPLAY_PLL_ENABLED);
			read32(INTEL_DISPLAY_B_PLL);
			spin(150);
		}

		uint32 lvds = read32(INTEL_DISPLAY_LVDS_PORT)
			| LVDS_PORT_EN | LVDS_A0A2_CLKA_POWER_UP | LVDS_PIPEB_SELECT;

		float referenceClock = gInfo->shared_info->pll_info.reference_frequency
			/ 1000.0f;

		// Set the B0-B3 data pairs corresponding to whether we're going to
		// set the DPLLs for dual-channel mode or not.
		if (divisors.post2 == LVDS_POST2_RATE_FAST)
			lvds |= LVDS_B0B3PAIRS_POWER_UP | LVDS_CLKB_POWER_UP;
		else
			lvds &= ~( LVDS_B0B3PAIRS_POWER_UP | LVDS_CLKB_POWER_UP);

		write32(INTEL_DISPLAY_LVDS_PORT, lvds);
		read32(INTEL_DISPLAY_LVDS_PORT);

		write32(INTEL_DISPLAY_B_PLL_DIVISOR_0,
			(((divisors.n - 2) << DISPLAY_PLL_N_DIVISOR_SHIFT) & DISPLAY_PLL_N_DIVISOR_MASK)
			| (((divisors.m1 - 2) << DISPLAY_PLL_M1_DIVISOR_SHIFT) & DISPLAY_PLL_M1_DIVISOR_MASK)
			| (((divisors.m2 - 2) << DISPLAY_PLL_M2_DIVISOR_SHIFT) & DISPLAY_PLL_M2_DIVISOR_MASK));

		write32(INTEL_DISPLAY_B_PLL, dpll);
		read32(INTEL_DISPLAY_B_PLL);

		// Wait for the clocks to stabilize
		spin(150);

		if (gInfo->shared_info->device_type == INTEL_TYPE_965) {
			float adjusted = ((referenceClock * divisors.m) / divisors.n)
				/ divisors.post;
			uint32 pixelMultiply = uint32(adjusted
				/ (target.timing.pixel_clock / 1000.0f));

			write32(INTEL_DISPLAY_B_PLL_MULTIPLIER_DIVISOR, (0 << 24)
				| ((pixelMultiply - 1) << 8));
		} else
			write32(INTEL_DISPLAY_B_PLL, dpll);

		read32(INTEL_DISPLAY_B_PLL);
		spin(150);

		// update timing parameters
		write32(INTEL_DISPLAY_B_HTOTAL, ((uint32)(target.timing.h_total - 1) << 16)
			| ((uint32)target.timing.h_display - 1));
		write32(INTEL_DISPLAY_B_HBLANK, ((uint32)(target.timing.h_total - 1) << 16)
			| ((uint32)target.timing.h_display - 1));
		write32(INTEL_DISPLAY_B_HSYNC, ((uint32)(target.timing.h_sync_end - 1) << 16)
			| ((uint32)target.timing.h_sync_start - 1));

		write32(INTEL_DISPLAY_B_VTOTAL, ((uint32)(target.timing.v_total - 1) << 16)
			| ((uint32)target.timing.v_display - 1));
		write32(INTEL_DISPLAY_B_VBLANK, ((uint32)(target.timing.v_total - 1) << 16)
			| ((uint32)target.timing.v_display - 1));
		write32(INTEL_DISPLAY_B_VSYNC, ((uint32)(target.timing.v_sync_end - 1) << 16)
			| ((uint32)target.timing.v_sync_start - 1));

		write32(INTEL_DISPLAY_B_IMAGE_SIZE, ((uint32)(target.timing.h_display - 1) << 16)
			| ((uint32)target.timing.v_display - 1));

		write32(INTEL_DISPLAY_B_POS, 0);
		write32(INTEL_DISPLAY_B_PIPE_SIZE, ((uint32)(target.timing.v_display - 1) << 16)
			| ((uint32)target.timing.h_display - 1));

		write32(INTEL_DISPLAY_B_PIPE_CONTROL,
			read32(INTEL_DISPLAY_B_PIPE_CONTROL) | DISPLAY_PIPE_ENABLED);
		read32(INTEL_DISPLAY_B_PIPE_CONTROL);
	}
	
	if (gInfo->head_mode & HEAD_MODE_A_ANALOG) {
		pll_divisors divisors;
		compute_pll_divisors(target, divisors,false);

		write32(INTEL_DISPLAY_A_PLL_DIVISOR_0,
			(((divisors.n - 2) << DISPLAY_PLL_N_DIVISOR_SHIFT) & DISPLAY_PLL_N_DIVISOR_MASK)
			| (((divisors.m1 - 2) << DISPLAY_PLL_M1_DIVISOR_SHIFT) & DISPLAY_PLL_M1_DIVISOR_MASK)
			| (((divisors.m2 - 2) << DISPLAY_PLL_M2_DIVISOR_SHIFT) & DISPLAY_PLL_M2_DIVISOR_MASK));

		uint32 pll = DISPLAY_PLL_ENABLED | DISPLAY_PLL_NO_VGA_CONTROL;
		if ((gInfo->shared_info->device_type & INTEL_TYPE_9xx) != 0) {
			pll |= ((1 << (divisors.post1 - 1)) << DISPLAY_PLL_POST1_DIVISOR_SHIFT)
				& DISPLAY_PLL_9xx_POST1_DIVISOR_MASK;
//			pll |= ((divisors.post1 - 1) << DISPLAY_PLL_POST1_DIVISOR_SHIFT)
//				& DISPLAY_PLL_9xx_POST1_DIVISOR_MASK;
			if (divisors.post2_high)
				pll |= DISPLAY_PLL_DIVIDE_HIGH;

			pll |= DISPLAY_PLL_MODE_ANALOG;

			if (gInfo->shared_info->device_type == INTEL_TYPE_965)
				pll |= 6 << DISPLAY_PLL_PULSE_PHASE_SHIFT;
		} else {
			if (!divisors.post2_high)
				pll |= DISPLAY_PLL_DIVIDE_4X;

			pll |= DISPLAY_PLL_2X_CLOCK;

			if (divisors.post1 > 2) {
				pll |= (((divisors.post1 - 2) << DISPLAY_PLL_POST1_DIVISOR_SHIFT)
					& DISPLAY_PLL_POST1_DIVISOR_MASK);
			} else
				pll |= DISPLAY_PLL_POST1_DIVIDE_2;
		}

		write32(INTEL_DISPLAY_A_PLL, pll);
		read32(INTEL_DISPLAY_A_PLL);
		spin(150);
		write32(INTEL_DISPLAY_A_PLL, pll);
		read32(INTEL_DISPLAY_A_PLL);
		spin(150);

		// update timing parameters
		write32(INTEL_DISPLAY_A_HTOTAL, ((uint32)(target.timing.h_total - 1) << 16)
			| ((uint32)target.timing.h_display - 1));
		write32(INTEL_DISPLAY_A_HBLANK, ((uint32)(target.timing.h_total - 1) << 16)
			| ((uint32)target.timing.h_display - 1));
		write32(INTEL_DISPLAY_A_HSYNC, ((uint32)(target.timing.h_sync_end - 1) << 16)
			| ((uint32)target.timing.h_sync_start - 1));

		write32(INTEL_DISPLAY_A_VTOTAL, ((uint32)(target.timing.v_total - 1) << 16)
			| ((uint32)target.timing.v_display - 1));
		write32(INTEL_DISPLAY_A_VBLANK, ((uint32)(target.timing.v_total - 1) << 16)
			| ((uint32)target.timing.v_display - 1));
		write32(INTEL_DISPLAY_A_VSYNC, ((uint32)(target.timing.v_sync_end - 1) << 16)
			| ((uint32)target.timing.v_sync_start - 1));

		write32(INTEL_DISPLAY_A_IMAGE_SIZE, ((uint32)(target.timing.h_display - 1) << 16)
			| ((uint32)target.timing.v_display - 1));

		write32(INTEL_DISPLAY_A_ANALOG_PORT, (read32(INTEL_DISPLAY_A_ANALOG_PORT)
			& ~(DISPLAY_MONITOR_POLARITY_MASK | DISPLAY_MONITOR_VGA_POLARITY))
			| ((target.timing.flags & B_POSITIVE_HSYNC) != 0 ? DISPLAY_MONITOR_POSITIVE_HSYNC : 0)
			| ((target.timing.flags & B_POSITIVE_VSYNC) != 0 ? DISPLAY_MONITOR_POSITIVE_VSYNC : 0));

		// TODO: verify the two comments below: the X driver doesn't seem to
		//		care about both of them!

		// These two have to be set for display B, too - this obviously means
		// that the second head always must adopt the color space of the first
		// head.
		write32(INTEL_DISPLAY_A_CONTROL, (read32(INTEL_DISPLAY_A_CONTROL)
			& ~(DISPLAY_CONTROL_COLOR_MASK | DISPLAY_CONTROL_GAMMA)) | colorMode);

		if (gInfo->head_mode & HEAD_MODE_B_DIGITAL) {
			write32(INTEL_DISPLAY_B_IMAGE_SIZE, ((uint32)(target.timing.h_display - 1) << 16)
				| ((uint32)target.timing.v_display - 1));

			write32(INTEL_DISPLAY_B_CONTROL, (read32(INTEL_DISPLAY_B_CONTROL)
				& ~(DISPLAY_CONTROL_COLOR_MASK | DISPLAY_CONTROL_GAMMA)) | colorMode);
		}
	}

	set_display_power_mode(sharedInfo.dpms_mode);

	// changing bytes per row seems to be ignored if the plane/pipe is turned off

	if (gInfo->head_mode & HEAD_MODE_A_ANALOG)
		write32(INTEL_DISPLAY_A_BYTES_PER_ROW, bytesPerRow);
	if (gInfo->head_mode & HEAD_MODE_B_DIGITAL)
		write32(INTEL_DISPLAY_B_BYTES_PER_ROW, bytesPerRow);

	set_frame_buffer_base();
		// triggers writing back double-buffered registers

	// update shared info
	sharedInfo.bytes_per_row = bytesPerRow;
	sharedInfo.current_mode = target;
	sharedInfo.bits_per_pixel = bitsPerPixel;

	return B_OK;
}


status_t
intel_get_display_mode(display_mode *_currentMode)
{
	TRACE(("intel_get_display_mode()\n"));

	display_mode &mode = *_currentMode;

	uint32 pll = read32(INTEL_DISPLAY_A_PLL);
	uint32 pllDivisor = read32((pll & DISPLAY_PLL_DIVISOR_1) != 0
		? INTEL_DISPLAY_A_PLL_DIVISOR_1 : INTEL_DISPLAY_A_PLL_DIVISOR_0);

	pll_divisors divisors;
	divisors.m1 = (pllDivisor & DISPLAY_PLL_M1_DIVISOR_MASK)
		>> DISPLAY_PLL_M1_DIVISOR_SHIFT;
	divisors.m2 = (pllDivisor & DISPLAY_PLL_M2_DIVISOR_MASK)
		>> DISPLAY_PLL_M2_DIVISOR_SHIFT;
	divisors.n = (pllDivisor & DISPLAY_PLL_N_DIVISOR_MASK)
		>> DISPLAY_PLL_N_DIVISOR_SHIFT;

	pll_limits limits;
	get_pll_limits(limits);

	if ((gInfo->shared_info->device_type & INTEL_TYPE_9xx) != 0) {
		divisors.post1 = (pll & DISPLAY_PLL_9xx_POST1_DIVISOR_MASK)
			>> DISPLAY_PLL_POST1_DIVISOR_SHIFT;

		if ((pll & DISPLAY_PLL_DIVIDE_HIGH) != 0)
			divisors.post2 = limits.max.post2;
		else
			divisors.post2 = limits.min.post2;
	} else {
		// 8xx
		divisors.post1 = (pll & DISPLAY_PLL_POST1_DIVISOR_MASK)
			>> DISPLAY_PLL_POST1_DIVISOR_SHIFT;

		if ((pll & DISPLAY_PLL_DIVIDE_4X) != 0)
			divisors.post2 = limits.max.post2;
		else
			divisors.post2 = limits.min.post2;
	}

	divisors.m = 5 * divisors.m1 + divisors.m2;
	divisors.post = divisors.post1 * divisors.post2;

	float referenceClock = gInfo->shared_info->pll_info.reference_frequency / 1000.0f;
	float pixelClock = ((referenceClock * divisors.m) / divisors.n) / divisors.post;

	// timing

	mode.timing.pixel_clock = uint32(pixelClock * 1000);
	mode.timing.flags = 0;

	uint32 value = read32(INTEL_DISPLAY_A_HTOTAL);
	mode.timing.h_total = (value >> 16) + 1;
	mode.timing.h_display = (value & 0xffff) + 1;

	value = read32(INTEL_DISPLAY_A_HSYNC);
	mode.timing.h_sync_end = (value >> 16) + 1;
	mode.timing.h_sync_start = (value & 0xffff) + 1;

	value = read32(INTEL_DISPLAY_A_VTOTAL);
	mode.timing.v_total = (value >> 16) + 1;
	mode.timing.v_display = (value & 0xffff) + 1;

	value = read32(INTEL_DISPLAY_A_VSYNC);
	mode.timing.v_sync_end = (value >> 16) + 1;
	mode.timing.v_sync_start = (value & 0xffff) + 1;

	// image size and color space

	value = read32(INTEL_DISPLAY_A_IMAGE_SIZE);
	mode.virtual_width = (value >> 16) + 1;
	mode.virtual_height = (value & 0xffff) + 1;

	value = read32(INTEL_DISPLAY_A_CONTROL);
	switch (value & DISPLAY_CONTROL_COLOR_MASK) {
		case DISPLAY_CONTROL_RGB32:
		default:
			mode.space = B_RGB32;
			break;
		case DISPLAY_CONTROL_RGB16:
			mode.space = B_RGB16;
			break;
		case DISPLAY_CONTROL_RGB15:
			mode.space = B_RGB15;
			break;
		case DISPLAY_CONTROL_CMAP8:
			mode.space = B_CMAP8;
			break;
	}

	mode.h_display_start = 0;
	mode.v_display_start = 0;
	mode.flags = 0;
	return B_OK;
}

#ifdef __HAIKU__

status_t
intel_get_edid_info(void* info, size_t size, uint32* _version)
{
	TRACE(("intel_get_edid_info()\n"));

	if (!gInfo->has_edid)
		return B_ERROR;
	if (size < sizeof(struct edid1_info))
		return B_BUFFER_OVERFLOW;

	memcpy(info, &gInfo->edid_info, sizeof(struct edid1_info));
	*_version = EDID_VERSION_1;
	return B_OK;
}

#endif	// __HAIKU__

status_t
intel_get_frame_buffer_config(frame_buffer_config *config)
{
	TRACE(("intel_get_frame_buffer_config()\n"));

	uint32 offset = gInfo->shared_info->frame_buffer_offset;

	config->frame_buffer = gInfo->shared_info->graphics_memory + offset;
	config->frame_buffer_dma
		= (uint8 *)gInfo->shared_info->physical_graphics_memory + offset;
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

	set_frame_buffer_base();

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

		if (gInfo->head_mode & HEAD_MODE_A_ANALOG)
			write32(INTEL_DISPLAY_A_PALETTE + first * sizeof(uint32), color);
		if (gInfo->head_mode & HEAD_MODE_B_DIGITAL)
			write32(INTEL_DISPLAY_B_PALETTE + first * sizeof(uint32), color);
	}
}

