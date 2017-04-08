/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Support for i915 chipset and up based on the X driver,
 * Copyright 2006-2007 Intel Corporation.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <math.h>
#include <stdio.h>
#include <string.h>

#include <Debug.h>

#include <create_display_modes.h>
#include <ddc.h>
#include <edid.h>
#include <validate_display_mode.h>

#include "accelerant_protos.h"
#include "accelerant.h"
#include "pll.h"
#include "Ports.h"
#include "utility.h"


#undef TRACE
#define TRACE_MODE
#ifdef TRACE_MODE
#	define TRACE(x...) _sPrintf("intel_extreme: " x)
#else
#	define TRACE(x...)
#endif

#define ERROR(x...) _sPrintf("intel_extreme: " x)
#define CALLED(x...) TRACE("CALLED %s\n", __PRETTY_FUNCTION__)


#if 0
// This hack needs to die. Leaving in for a little while
// incase we *really* need it.
static void
retrieve_current_mode(display_mode& mode, uint32 pllRegister)
{
	uint32 pll = read32(pllRegister);
	uint32 pllDivisor;
	uint32 hTotalRegister;
	uint32 vTotalRegister;
	uint32 hSyncRegister;
	uint32 vSyncRegister;
	uint32 imageSizeRegister;
	uint32 controlRegister;

	if (pllRegister == INTEL_DISPLAY_A_PLL) {
		pllDivisor = read32((pll & DISPLAY_PLL_DIVISOR_1) != 0
			? INTEL_DISPLAY_A_PLL_DIVISOR_1 : INTEL_DISPLAY_A_PLL_DIVISOR_0);

		hTotalRegister = INTEL_DISPLAY_A_HTOTAL;
		vTotalRegister = INTEL_DISPLAY_A_VTOTAL;
		hSyncRegister = INTEL_DISPLAY_A_HSYNC;
		vSyncRegister = INTEL_DISPLAY_A_VSYNC;
		imageSizeRegister = INTEL_DISPLAY_A_IMAGE_SIZE;
		controlRegister = INTEL_DISPLAY_A_CONTROL;
	} else if (pllRegister == INTEL_DISPLAY_B_PLL) {
		pllDivisor = read32((pll & DISPLAY_PLL_DIVISOR_1) != 0
			? INTEL_DISPLAY_B_PLL_DIVISOR_1 : INTEL_DISPLAY_B_PLL_DIVISOR_0);

		hTotalRegister = INTEL_DISPLAY_B_HTOTAL;
		vTotalRegister = INTEL_DISPLAY_B_VTOTAL;
		hSyncRegister = INTEL_DISPLAY_B_HSYNC;
		vSyncRegister = INTEL_DISPLAY_B_VSYNC;
		imageSizeRegister = INTEL_DISPLAY_B_IMAGE_SIZE;
		controlRegister = INTEL_DISPLAY_B_CONTROL;
	} else {
		ERROR("%s: PLL not supported\n", __func__);
		return;
	}

	pll_divisors divisors;
	if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_PIN)) {
		divisors.m1 = 0;
		divisors.m2 = (pllDivisor & DISPLAY_PLL_IGD_M2_DIVISOR_MASK)
			>> DISPLAY_PLL_M2_DIVISOR_SHIFT;
		divisors.n = ((pllDivisor & DISPLAY_PLL_IGD_N_DIVISOR_MASK)
			>> DISPLAY_PLL_N_DIVISOR_SHIFT) - 1;
	} else {
		divisors.m1 = (pllDivisor & DISPLAY_PLL_M1_DIVISOR_MASK)
			>> DISPLAY_PLL_M1_DIVISOR_SHIFT;
		divisors.m2 = (pllDivisor & DISPLAY_PLL_M2_DIVISOR_MASK)
			>> DISPLAY_PLL_M2_DIVISOR_SHIFT;
		divisors.n = (pllDivisor & DISPLAY_PLL_N_DIVISOR_MASK)
			>> DISPLAY_PLL_N_DIVISOR_SHIFT;
	}

	pll_limits limits;
	get_pll_limits(&limits, false);
		// TODO: Detect LVDS connector vs assume no

	if (gInfo->shared_info->device_type.Generation() >= 3) {

		if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_PIN)) {
			divisors.post1 = (pll & DISPLAY_PLL_IGD_POST1_DIVISOR_MASK)
				>> DISPLAY_PLL_IGD_POST1_DIVISOR_SHIFT;
		} else {
			divisors.post1 = (pll & DISPLAY_PLL_9xx_POST1_DIVISOR_MASK)
				>> DISPLAY_PLL_POST1_DIVISOR_SHIFT;
		}

		if (pllRegister == INTEL_DISPLAY_B_PLL
			&& !gInfo->shared_info->device_type.InGroup(INTEL_GROUP_96x)) {
			// TODO: Fix this? Need to support dual channel LVDS.
			divisors.post2 = LVDS_POST2_RATE_SLOW;
		} else {
			if ((pll & DISPLAY_PLL_DIVIDE_HIGH) != 0)
				divisors.post2 = limits.max.post2;
			else
				divisors.post2 = limits.min.post2;
		}
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

	float referenceClock
		= gInfo->shared_info->pll_info.reference_frequency / 1000.0f;
	float pixelClock
		= ((referenceClock * divisors.m) / divisors.n) / divisors.post;

	// timing

	mode.timing.pixel_clock = uint32(pixelClock * 1000);
	mode.timing.flags = 0;

	uint32 value = read32(hTotalRegister);
	mode.timing.h_total = (value >> 16) + 1;
	mode.timing.h_display = (value & 0xffff) + 1;

	value = read32(hSyncRegister);
	mode.timing.h_sync_end = (value >> 16) + 1;
	mode.timing.h_sync_start = (value & 0xffff) + 1;

	value = read32(vTotalRegister);
	mode.timing.v_total = (value >> 16) + 1;
	mode.timing.v_display = (value & 0xffff) + 1;

	value = read32(vSyncRegister);
	mode.timing.v_sync_end = (value >> 16) + 1;
	mode.timing.v_sync_start = (value & 0xffff) + 1;

	// image size and color space

	value = read32(imageSizeRegister);
	mode.virtual_width = (value >> 16) + 1;
	mode.virtual_height = (value & 0xffff) + 1;

	// using virtual size based on image size is the 'proper' way to do it,
	// however the bios appears to be suggesting scaling or somesuch, so ignore
	// the proper virtual dimension for now if they'd suggest a smaller size.
	if (mode.virtual_width < mode.timing.h_display)
		mode.virtual_width = mode.timing.h_display;
	if (mode.virtual_height < mode.timing.v_display)
		mode.virtual_height = mode.timing.v_display;

	value = read32(controlRegister);
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
	mode.flags = B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS
		| B_DPMS | B_SUPPORTS_OVERLAYS;
}
#endif


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

	// Make sure bytesPerRow is a multiple of 64
	// TODO: check if the older chips have the same restriction!
	if ((bytesPerRow & 63) != 0)
		bytesPerRow = (bytesPerRow + 63) & ~63;
}


static bool
sanitize_display_mode(display_mode& mode)
{
	// Some cards only support even pixel counts, while others require an odd
	// one.
	uint16 pixelCount = 1;
	if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_Gxx)
			|| gInfo->shared_info->device_type.InGroup(INTEL_GROUP_96x)
			|| gInfo->shared_info->device_type.InGroup(INTEL_GROUP_94x)
			|| gInfo->shared_info->device_type.InGroup(INTEL_GROUP_91x)
			|| gInfo->shared_info->device_type.InFamily(INTEL_FAMILY_8xx)
			|| gInfo->shared_info->device_type.InFamily(INTEL_FAMILY_7xx)) {
		pixelCount = 2;
	}

	// TODO: verify constraints - these are more or less taken from the
	// radeon driver!
	display_constraints constraints = {
		// resolution
		320, 8192, 200, 4096,
		// pixel clock
		gInfo->shared_info->pll_info.min_frequency,
		gInfo->shared_info->pll_info.max_frequency,
		// horizontal
		{pixelCount, 0, 8160, 32, 8192, 0, 8192},
		{1, 1, 4092, 2, 63, 1, 4096}
	};

	return sanitize_display_mode(mode, constraints,
		gInfo->has_edid ? &gInfo->edid_info : NULL);
}


// #pragma mark -


static void
set_frame_buffer_registers(uint32 baseRegister, uint32 surfaceRegister)
{
	intel_shared_info &sharedInfo = *gInfo->shared_info;
	display_mode &mode = gInfo->current_mode;

	if (sharedInfo.device_type.InGroup(INTEL_GROUP_96x)
		|| sharedInfo.device_type.InGroup(INTEL_GROUP_G4x)
		|| sharedInfo.device_type.InGroup(INTEL_GROUP_ILK)
		|| sharedInfo.device_type.InFamily(INTEL_FAMILY_SER5)
		|| sharedInfo.device_type.InFamily(INTEL_FAMILY_SOC0)) {
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


void
set_frame_buffer_base()
{
	// TODO we always set both displays to the same address. When we support
	// multiple framebuffers, they should get different addresses here.
	set_frame_buffer_registers(INTEL_DISPLAY_A_BASE, INTEL_DISPLAY_A_SURFACE);
	set_frame_buffer_registers(INTEL_DISPLAY_B_BASE, INTEL_DISPLAY_B_SURFACE);
}


/*!	Creates the initial mode list of the primary accelerant.
	It's called from intel_init_accelerant().
*/
status_t
create_mode_list(void)
{
	for (uint32 i = 0; i < gInfo->port_count; i++) {
		if (gInfo->ports[i] == NULL)
			continue;

		status_t status = gInfo->ports[i]->GetEDID(&gInfo->edid_info);
		if (status == B_OK)
			gInfo->has_edid = true;
	}

	// If no EDID, but have vbt from driver, use that mode
	if (!gInfo->has_edid && gInfo->shared_info->got_vbt) {
		// We could not read any EDID info. Fallback to creating a list with
		// only the mode set up by the BIOS.

		// TODO: support lower modes via scaling and windowing
		size_t size = (sizeof(display_mode) + B_PAGE_SIZE - 1)
			& ~(B_PAGE_SIZE - 1);

		display_mode* list;
		area_id area = create_area("intel extreme modes",
			(void**)&list, B_ANY_ADDRESS, size, B_NO_LOCK,
			B_READ_AREA | B_WRITE_AREA);
		if (area < 0)
			return area;

		memcpy(list, &gInfo->shared_info->panel_mode, sizeof(display_mode));

		gInfo->mode_list_area = area;
		gInfo->mode_list = list;
		gInfo->shared_info->mode_list_area = gInfo->mode_list_area;
		gInfo->shared_info->mode_count = 1;
		return B_OK;
	}

	// Otherwise return the 'real' list of modes
	display_mode* list;
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
	acquire_sem_etc(gInfo->shared_info->vblank_sem, 1, B_RELATIVE_TIMEOUT,
		25000);
		// With the output turned off via DPMS, we might not get any interrupts
		// anymore that's why we don't wait forever for it.
}


//	#pragma mark -


uint32
intel_accelerant_mode_count(void)
{
	CALLED();
	return gInfo->shared_info->mode_count;
}


status_t
intel_get_mode_list(display_mode* modeList)
{
	CALLED();
	memcpy(modeList, gInfo->mode_list,
		gInfo->shared_info->mode_count * sizeof(display_mode));
	return B_OK;
}


status_t
intel_propose_display_mode(display_mode* target, const display_mode* low,
	const display_mode* high)
{
	CALLED();

	// first search for the specified mode in the list, if no mode is found
	// try to fix the target mode in sanitize_display_mode
	// TODO: Only sanitize_display_mode should be used. However, at the moments
	// the mode constraints are not optimal and do not work for all
	// configurations.
	for (uint32 i = 0; i < gInfo->shared_info->mode_count; i++) {
		display_mode *mode = &gInfo->mode_list[i];

		// TODO: improve this, ie. adapt pixel clock to allowed values!!!

		if (target->virtual_width != mode->virtual_width
			|| target->virtual_height != mode->virtual_height
			|| target->space != mode->space) {
			continue;
		}

		*target = *mode;
		return B_OK;
	}

	sanitize_display_mode(*target);

	return is_display_mode_within_bounds(*target, *low, *high)
		? B_OK : B_BAD_VALUE;
}


status_t
intel_set_display_mode(display_mode* mode)
{
	TRACE("%s(%" B_PRIu16 "x%" B_PRIu16 ")\n", __func__,
		mode->virtual_width, mode->virtual_height);

	if (mode == NULL)
		return B_BAD_VALUE;

	display_mode target = *mode;

	// TODO: it may be acceptable to continue when using panel fitting or
	// centering, since the data from propose_display_mode will not actually be
	// used as is in this case.
	if (sanitize_display_mode(target)) {
		TRACE("Video mode was adjusted by sanitize_display_mode\n");
		TRACE("Initial mode: Hd %d Hs %d He %d Ht %d Vd %d Vs %d Ve %d Vt %d\n",
			mode->timing.h_display, mode->timing.h_sync_start,
			mode->timing.h_sync_end, mode->timing.h_total,
			mode->timing.v_display, mode->timing.v_sync_start,
			mode->timing.v_sync_end, mode->timing.v_total);
		TRACE("Sanitized: Hd %d Hs %d He %d Ht %d Vd %d Vs %d Ve %d Vt %d\n",
			target.timing.h_display, target.timing.h_sync_start,
			target.timing.h_sync_end, target.timing.h_total,
			target.timing.v_display, target.timing.v_sync_start,
			target.timing.v_sync_end, target.timing.v_total);
	}

	uint32 colorMode, bytesPerRow, bitsPerPixel;
	get_color_space_format(target, colorMode, bytesPerRow, bitsPerPixel);

	// TODO: do not go further if the mode is identical to the current one.
	// This would avoid the screen being off when switching workspaces when they
	// have the same resolution.

	intel_shared_info &sharedInfo = *gInfo->shared_info;
	Autolock locker(sharedInfo.accelerant_lock);

	// First register dump
	//dump_registers();

	// TODO: This may not be neccesary
	set_display_power_mode(B_DPMS_OFF);

	// free old and allocate new frame buffer in graphics memory

	intel_free_memory(sharedInfo.frame_buffer);

	addr_t base;
	if (intel_allocate_memory(bytesPerRow * target.virtual_height, 0,
			base) < B_OK) {
		// oh, how did that happen? Unfortunately, there is no really good way
		// back
		if (intel_allocate_memory(gInfo->current_mode.virtual_height
				* sharedInfo.bytes_per_row, 0, base) == B_OK) {
			sharedInfo.frame_buffer = base;
			sharedInfo.frame_buffer_offset = base
				- (addr_t)sharedInfo.graphics_memory;
			set_frame_buffer_base();
		}

		TRACE("%s: Failed to allocate framebuffer !\n", __func__);
		return B_NO_MEMORY;
	}

	// clear frame buffer before using it
	memset((uint8*)base, 0, bytesPerRow * target.virtual_height);
	sharedInfo.frame_buffer = base;
	sharedInfo.frame_buffer_offset = base - (addr_t)sharedInfo.graphics_memory;

#if 0
	if ((gInfo->head_mode & HEAD_MODE_TESTING) != 0) {
		// 1. Enable panel power as needed to retrieve panel configuration
		// (use AUX VDD enable bit)
			// skip, did detection already, might need that before that though

		// 2. Enable PCH clock reference source and PCH SSC modulator,
		// wait for warmup (Can be done anytime before enabling port)
			// skip, most certainly already set up by bios to use other ports,
			// will need for coldstart though

		// 3. If enabling CPU embedded DisplayPort A: (Can be done anytime
		// before enabling CPU pipe or port)
		//	a.	Enable PCH 120MHz clock source output to CPU, wait for DMI
		//		latency
		//	b.	Configure and enable CPU DisplayPort PLL in the DisplayPort A
		//		register, wait for warmup
			// skip, not doing eDP right now, should go into
			// EmbeddedDisplayPort class though

		// 4. If enabling port on PCH: (Must be done before enabling CPU pipe
		// or FDI)
		//	a.	Enable PCH FDI Receiver PLL, wait for warmup plus DMI latency
		//	b.	Switch from Rawclk to PCDclk in FDI Receiver (FDI A OR FDI B)
		//	c.	[DevSNB] Enable CPU FDI Transmitter PLL, wait for warmup
		//	d.	[DevILK] CPU FDI PLL is always on and does not need to be
		//		enabled
		FDILink* link = pipe->FDILink();
		if (link != NULL) {
			link->Receiver().EnablePLL();
			link->Receiver().SwitchClock(true);
			link->Transmitter().EnablePLL();
		}

		// 5. Enable CPU panel fitter if needed for hires, required for VGA
		// (Can be done anytime before enabling CPU pipe)
		PanelFitter* fitter = pipe->PanelFitter();
		if (fitter != NULL)
			fitter->Enable(mode);

		// 6. Configure CPU pipe timings, M/N/TU, and other pipe settings
		// (Can be done anytime before enabling CPU pipe)
		pll_divisors divisors;
		compute_pll_divisors(target, divisors, false);
		pipe->ConfigureTimings(divisors);

		// 7. Enable CPU pipe
		pipe->Enable();

8. Configure and enable CPU planes (VGA or hires)
9. If enabling port on PCH:
		//	a.   Program PCH FDI Receiver TU size same as Transmitter TU size for TU error checking
		//	b.   Train FDI
		//		i. Set pre-emphasis and voltage (iterate if training steps fail)
                    ii. Enable CPU FDI Transmitter and PCH FDI Receiver with Training Pattern 1 enabled.
                   iii. Wait for FDI training pattern 1 time
                   iv. Read PCH FDI Receiver ISR ([DevIBX-B+] IIR) for bit lock in bit 8 (retry at least once if no lock)
                    v. Enable training pattern 2 on CPU FDI Transmitter and PCH FDI Receiver
                   vi.  Wait for FDI training pattern 2 time
                  vii. Read PCH FDI Receiver ISR ([DevIBX-B+] IIR) for symbol lock in bit 9 (retry at least once if no
                        lock)
                  viii. Enable normal pixel output on CPU FDI Transmitter and PCH FDI Receiver
                   ix.  Wait for FDI idle pattern time for link to become active
         c.   Configure and enable PCH DPLL, wait for PCH DPLL warmup (Can be done anytime before enabling
              PCH transcoder)
         d.   [DevCPT] Configure DPLL SEL to set the DPLL to transcoder mapping and enable DPLL to the
              transcoder.
         e.   [DevCPT] Configure DPLL_CTL DPLL_HDMI_multipler.
         f.   Configure PCH transcoder timings, M/N/TU, and other transcoder settings (should match CPU settings).
         g.   [DevCPT] Configure and enable Transcoder DisplayPort Control if DisplayPort will be used
         h.   Enable PCH transcoder
10. Enable ports (DisplayPort must enable in training pattern 1)
11. Enable panel power through panel power sequencing
12. Wait for panel power sequencing to reach enabled steady state
13. Disable panel power override
14. If DisplayPort, complete link training
15. Enable panel backlight
	}
#endif

	// make sure VGA display is disabled
	write32(INTEL_VGA_DISPLAY_CONTROL, VGA_DISPLAY_DISABLED);
	read32(INTEL_VGA_DISPLAY_CONTROL);

	// Go over each port and set the display mode
	for (uint32 i = 0; i < gInfo->port_count; i++) {
		if (gInfo->ports[i] == NULL)
			continue;
		if (!gInfo->ports[i]->IsConnected())
			continue;

		status_t status = gInfo->ports[i]->SetDisplayMode(&target, colorMode);
		if (status != B_OK)
			ERROR("%s: Unable to set display mode!\n", __func__);
	}

	TRACE("%s: Port configuration completed successfully!\n", __func__);

	// We set the same color mode across all pipes
	program_pipe_color_modes(colorMode);

	// TODO: This may not be neccesary (see DPMS OFF at top)
	set_display_power_mode(sharedInfo.dpms_mode);

	// Changing bytes per row seems to be ignored if the plane/pipe is turned
	// off

	// Always set both pipes, just in case
	// TODO rework this when we get multiple head support with different
	// resolutions
	write32(INTEL_DISPLAY_A_BYTES_PER_ROW, bytesPerRow);
	write32(INTEL_DISPLAY_B_BYTES_PER_ROW, bytesPerRow);

	// update shared info
	gInfo->current_mode = target;

	// TODO: move to gInfo
	sharedInfo.bytes_per_row = bytesPerRow;
	sharedInfo.bits_per_pixel = bitsPerPixel;

	set_frame_buffer_base();
		// triggers writing back double-buffered registers

	// Second register dump
	//dump_registers();

	return B_OK;
}


status_t
intel_get_display_mode(display_mode* _currentMode)
{
	CALLED();

	*_currentMode = gInfo->current_mode;

	// This seems unreliable. We should always know the current_mode
	//retrieve_current_mode(*_currentMode, INTEL_DISPLAY_A_PLL);
	return B_OK;
}


status_t
intel_get_edid_info(void* info, size_t size, uint32* _version)
{
	CALLED();

	if (!gInfo->has_edid)
		return B_ERROR;
	if (size < sizeof(struct edid1_info))
		return B_BUFFER_OVERFLOW;

	memcpy(info, &gInfo->edid_info, sizeof(struct edid1_info));
	*_version = EDID_VERSION_1;
	return B_OK;
}


status_t
intel_get_frame_buffer_config(frame_buffer_config* config)
{
	CALLED();

	uint32 offset = gInfo->shared_info->frame_buffer_offset;

	config->frame_buffer = gInfo->shared_info->graphics_memory + offset;
	config->frame_buffer_dma
		= (uint8*)gInfo->shared_info->physical_graphics_memory + offset;
	config->bytes_per_row = gInfo->shared_info->bytes_per_row;

	return B_OK;
}


status_t
intel_get_pixel_clock_limits(display_mode* mode, uint32* _low, uint32* _high)
{
	CALLED();

	if (_low != NULL) {
		// lower limit of about 48Hz vertical refresh
		uint32 totalClocks = (uint32)mode->timing.h_total
			* (uint32)mode->timing.v_total;
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
	CALLED();

	intel_shared_info &sharedInfo = *gInfo->shared_info;
	Autolock locker(sharedInfo.accelerant_lock);

	display_mode &mode = gInfo->current_mode;

	if (horizontalStart + mode.timing.h_display > mode.virtual_width
		|| verticalStart + mode.timing.v_display > mode.virtual_height)
		return B_BAD_VALUE;

	mode.h_display_start = horizontalStart;
	mode.v_display_start = verticalStart;

	set_frame_buffer_base();

	return B_OK;
}


status_t
intel_get_timing_constraints(display_timing_constraints* constraints)
{
	CALLED();
	return B_ERROR;
}


void
intel_set_indexed_colors(uint count, uint8 first, uint8* colors, uint32 flags)
{
	TRACE("%s(colors = %p, first = %u)\n", __func__, colors, first);

	if (colors == NULL)
		return;

	Autolock locker(gInfo->shared_info->accelerant_lock);

	for (; count-- > 0; first++) {
		uint32 color = colors[0] << 16 | colors[1] << 8 | colors[2];
		colors += 3;

		write32(INTEL_DISPLAY_A_PALETTE + first * sizeof(uint32), color);
		write32(INTEL_DISPLAY_B_PALETTE + first * sizeof(uint32), color);
	}
}
