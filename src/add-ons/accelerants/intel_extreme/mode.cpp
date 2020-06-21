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
	if ((bytesPerRow & 63) != 0)
		bytesPerRow = (bytesPerRow + 63) & ~63;
}


static bool
sanitize_display_mode(display_mode& mode)
{
	uint16 pixelCount = 1;
	// Older cards require pixel count to be even
	if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_Gxx)
			|| gInfo->shared_info->device_type.InGroup(INTEL_GROUP_96x)
			|| gInfo->shared_info->device_type.InGroup(INTEL_GROUP_94x)
			|| gInfo->shared_info->device_type.InGroup(INTEL_GROUP_91x)
			|| gInfo->shared_info->device_type.InFamily(INTEL_FAMILY_8xx)) {
		pixelCount = 2;
	}

	display_constraints constraints = {
		// resolution
		320, 4096, 200, 4096,
		// pixel clock
		gInfo->shared_info->pll_info.min_frequency,
		gInfo->shared_info->pll_info.max_frequency,
		// horizontal
		{pixelCount, 0, 8160, 32, 8192, 0, 8192},
		{1, 1, 8190, 2, 8192, 1, 8192}
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


static bool
limit_modes_for_gen3_lvds(display_mode* mode)
{
	// Filter out modes with resolution higher than the internal LCD can
	// display.
	// FIXME do this only for that display. The whole display mode logic
	// needs to be adjusted to know which display we're talking about.
	if (gInfo->shared_info->panel_mode.virtual_width < mode->virtual_width)
		return false;
	if (gInfo->shared_info->panel_mode.virtual_height < mode->virtual_height)
		return false;

	return true;
}

/*!	Creates the initial mode list of the primary accelerant.
	It's called from intel_init_accelerant().
*/
status_t
create_mode_list(void)
{
	CALLED();

	for (uint32 i = 0; i < gInfo->port_count; i++) {
		if (gInfo->ports[i] == NULL)
			continue;

		status_t status = gInfo->ports[i]->GetEDID(&gInfo->edid_info);
		if (status == B_OK)
			gInfo->has_edid = true;
	}

	display_mode* list;
	uint32 count = 0;

	const color_space kSupportedSpaces[] = {B_RGB32_LITTLE, B_RGB16_LITTLE,
		B_CMAP8};
	const color_space* supportedSpaces;
	int colorSpaceCount;

	if (gInfo->shared_info->device_type.Generation() >= 4) {
		// No B_RGB15, use our custom colorspace list
		supportedSpaces = kSupportedSpaces;
		colorSpaceCount = B_COUNT_OF(kSupportedSpaces);
	} else {
		supportedSpaces = NULL;
		colorSpaceCount = 0;
	}

	// If no EDID, but have vbt from driver, use that mode
	if (!gInfo->has_edid && gInfo->shared_info->got_vbt) {
		// We could not read any EDID info. Fallback to creating a list with
		// only the mode set up by the BIOS.

		check_display_mode_hook limitModes = NULL;
		if (gInfo->shared_info->device_type.Generation() < 4)
			limitModes = limit_modes_for_gen3_lvds;

		// TODO: support lower modes via scaling and windowing
		gInfo->mode_list_area = create_display_modes("intel extreme modes",
			NULL, &gInfo->shared_info->panel_mode, 1,
			supportedSpaces, colorSpaceCount, limitModes, &list, &count);
	} else {
		// Otherwise return the 'real' list of modes
		gInfo->mode_list_area = create_display_modes("intel extreme modes",
			gInfo->has_edid ? &gInfo->edid_info : NULL, NULL, 0,
			supportedSpaces, colorSpaceCount, NULL, &list, &count);
	}

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
		21000);
		// With the output turned off via DPMS, we might not get any interrupts
		// anymore that's why we don't wait forever for it. At 50Hz, we're sure
		// to get a vblank in at most 20ms, so there is no need to wait longer
		// than that.
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
	// TODO: Only sanitize_display_mode should be used. However, at the moment
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
	if (mode == NULL)
		return B_BAD_VALUE;

	TRACE("%s(%" B_PRIu16 "x%" B_PRIu16 ")\n", __func__,
		mode->virtual_width, mode->virtual_height);

	display_mode target = *mode;

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
		// back. Try to restore a framebuffer for the previous mode, at least.
		if (intel_allocate_memory(gInfo->current_mode.virtual_height
				* sharedInfo.bytes_per_row, 0, base) == B_OK) {
			sharedInfo.frame_buffer = base;
			sharedInfo.frame_buffer_offset = base
				- (addr_t)sharedInfo.graphics_memory;
			set_frame_buffer_base();
		}

		ERROR("%s: Failed to allocate framebuffer !\n", __func__);
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
	if (!gInfo->has_edid)
		return B_ERROR;
	if (size < sizeof(struct edid1_info))
		return B_BUFFER_OVERFLOW;

	memcpy(info, &gInfo->edid_info, sizeof(struct edid1_info));
	*_version = EDID_VERSION_1;
	return B_OK;
}


static int32_t
intel_get_backlight_register(bool read)
{
	if (gInfo->shared_info->pch_info == INTEL_PCH_NONE)
		return MCH_BLC_PWM_CTL;
	
	if (read)
		return PCH_SBLC_PWM_CTL2;
	else
		return PCH_BLC_PWM_CTL;
}


status_t
intel_set_brightness(float brightness)
{
	CALLED();

	if (brightness < 0 || brightness > 1)
		return B_BAD_VALUE;

	uint32_t period = read32(intel_get_backlight_register(true)) >> 16;

	// The "duty cycle" is a proportion of the period (0 = backlight off,
	// period = maximum brightness). The low bit must be masked out because
	// it is apparently used for something else on some Atom machines (no
	// reference to that in the documentation that I know of).
	// Additionally we don't want it to be completely 0 here, because then
	// it becomes hard to turn the display on again (at least until we get
	// working ACPI keyboard shortcuts for this). So always keep the backlight
	// at least a little bit on for now.
	uint32_t duty = (uint32_t)(period * brightness) & 0xfffe;
	if (duty == 0 && period != 0)
		duty = 2;

	write32(intel_get_backlight_register(false), duty | (period << 16));

	return B_OK;
}


status_t
intel_get_brightness(float* brightness)
{
	CALLED();

	if (brightness == NULL)
		return B_BAD_VALUE;

	uint16_t period = read32(intel_get_backlight_register(true)) >> 16;
	uint16_t   duty = read32(intel_get_backlight_register(false)) & 0xffff;
	*brightness = (float)duty / period;

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
