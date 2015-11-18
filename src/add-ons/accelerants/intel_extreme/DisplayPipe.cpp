/*
 * Copyright 2011-2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz, mmlr@mlotz.ch
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */
#include "DisplayPipe.h"

#include "accelerant.h"
#include "intel_extreme.h"
#include <KernelExport.h>

#include <stdlib.h>
#include <string.h>

#include <new>


#define TRACE_PIPE
#ifdef TRACE_PIPE
extern "C" void _sPrintf(const char* format, ...);
#	define TRACE(x...) _sPrintf("intel_extreme: " x)
#else
#	define TRACE(x...) ;
#endif

#define ERROR(x...) _sPrintf("intel_extreme: " x)
#define CALLED(x...) TRACE("CALLED %s\n", __PRETTY_FUNCTION__)


// PIPE: 6
// PLANE: 7

void
program_pipe_color_modes(uint32 colorMode)
{
	// All pipes get the same color mode
	write32(INTEL_DISPLAY_A_CONTROL, (read32(INTEL_DISPLAY_A_CONTROL)
			& ~(DISPLAY_CONTROL_COLOR_MASK | DISPLAY_CONTROL_GAMMA))
        | colorMode);
	write32(INTEL_DISPLAY_B_CONTROL, (read32(INTEL_DISPLAY_B_CONTROL)
			& ~(DISPLAY_CONTROL_COLOR_MASK | DISPLAY_CONTROL_GAMMA))
		| colorMode);
}


// #pragma mark - DisplayPipe


DisplayPipe::DisplayPipe(pipe_index pipeIndex)
	:
	fFDILink(NULL),
//	fPanelFitter(NULL),
	fPipeIndex(pipeIndex),
	fPipeBase(REGS_NORTH_PIPE_AND_PORT),
	fPlaneBase(REGS_NORTH_PLANE_CONTROL)
{
	if (pipeIndex == INTEL_PIPE_B) {
		fPipeBase += INTEL_DISPLAY_OFFSET;
		fPlaneBase += INTEL_PLANE_OFFSET;
	}

	// Program FDILink if PCH
	if (gInfo->shared_info->device_type.HasPlatformControlHub()) {
		if (fFDILink == NULL)
			fFDILink = new(std::nothrow) FDILink(pipeIndex);
	}

	TRACE("DisplayPipe %s. Pipe Base: 0x%" B_PRIxADDR
		" Plane Base: 0x% " B_PRIxADDR "\n", (pipeIndex == INTEL_PIPE_A)
			? "A" : "B", fPipeBase, fPlaneBase);
}


DisplayPipe::~DisplayPipe()
{
}


bool
DisplayPipe::IsEnabled()
{
	CALLED();
	return (read32(fPlaneBase + INTEL_PIPE_CONTROL) & INTEL_PIPE_ENABLED) != 0;
}


void
DisplayPipe::Enable(display_mode* target, addr_t portAddress)
{
	CALLED();

	if (target == NULL) {
		ERROR("%s: Invalid display mode!\n", __func__);
		return;
	}
	if (portAddress == 0) {
		ERROR("%s: Invalid port address!\n", __func__);
		return;
	}

	// update timing (fPipeBase bumps the DISPLAY_A to B when needed)
	write32(fPipeBase + REGISTER_REGISTER(INTEL_DISPLAY_A_HTOTAL),
		((uint32)(target->timing.h_total - 1) << 16)
		| ((uint32)target->timing.h_display - 1));
	write32(fPipeBase + REGISTER_REGISTER(INTEL_DISPLAY_A_HBLANK),
		((uint32)(target->timing.h_total - 1) << 16)
		| ((uint32)target->timing.h_display - 1));
	write32(fPipeBase + REGISTER_REGISTER(INTEL_DISPLAY_A_HSYNC),
		((uint32)(target->timing.h_sync_end - 1) << 16)
		| ((uint32)target->timing.h_sync_start - 1));

	write32(fPipeBase + REGISTER_REGISTER(INTEL_DISPLAY_A_VTOTAL),
		((uint32)(target->timing.v_total - 1) << 16)
		| ((uint32)target->timing.v_display - 1));
	write32(fPipeBase + REGISTER_REGISTER(INTEL_DISPLAY_A_VBLANK),
		((uint32)(target->timing.v_total - 1) << 16)
		| ((uint32)target->timing.v_display - 1));
	write32(fPipeBase + REGISTER_REGISTER(INTEL_DISPLAY_A_VSYNC),
		((uint32)(target->timing.v_sync_end - 1) << 16)
		| ((uint32)target->timing.v_sync_start - 1));

	// XXX: Is it ok to do these on non-digital?
	write32(fPipeBase + REGISTER_REGISTER(INTEL_DISPLAY_A_POS), 0);
	write32(fPipeBase + REGISTER_REGISTER(INTEL_DISPLAY_A_IMAGE_SIZE),
		((uint32)(target->virtual_width - 1) << 16)
			| ((uint32)target->virtual_height - 1));

	write32(fPipeBase + REGISTER_REGISTER(INTEL_DISPLAY_A_PIPE_SIZE),
		((uint32)(target->timing.v_display - 1) << 16)
			| ((uint32)target->timing.h_display - 1));

	// This is useful for debugging: it sets the border to red, so you
	// can see what is border and what is porch (black area around the
	// sync)
	//write32(fPipeBase + REGISTER_REGISTER(INTEL_DISPLAY_A_RED), 0x00FF0000);

	// XXX: Is it ok to do this on non-analog?
	write32(portAddress, (read32(portAddress) & ~(DISPLAY_MONITOR_POLARITY_MASK
			| DISPLAY_MONITOR_VGA_POLARITY))
		| ((target->timing.flags & B_POSITIVE_HSYNC) != 0
			? DISPLAY_MONITOR_POSITIVE_HSYNC : 0)
		| ((target->timing.flags & B_POSITIVE_VSYNC) != 0
			? DISPLAY_MONITOR_POSITIVE_VSYNC : 0));

	// Enable display pipe
	_Enable(true);

}


void
DisplayPipe::Disable()
{
	_Enable(false);
}


void
DisplayPipe::ConfigureTimings(const pll_divisors& divisors, uint32 pixelClock,
	uint32 extraFlags)
{
	CALLED();

	addr_t pllDivisorA = INTEL_DISPLAY_A_PLL_DIVISOR_0;
	addr_t pllDivisorB = INTEL_DISPLAY_A_PLL_DIVISOR_1;
	addr_t pllControl = INTEL_DISPLAY_A_PLL;
	addr_t pllMD = INTEL_DISPLAY_A_PLL_MD;

	if (fPipeIndex == INTEL_PIPE_B) {
		pllDivisorA = INTEL_DISPLAY_B_PLL_DIVISOR_0;
		pllDivisorB = INTEL_DISPLAY_B_PLL_DIVISOR_1;
		pllControl = INTEL_DISPLAY_B_PLL;
		pllMD = INTEL_DISPLAY_B_PLL_MD;
	}

	float refFreq = gInfo->shared_info->pll_info.reference_frequency / 1000.0f;

	if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_96x)) {
		float adjusted = ((refFreq * divisors.m) / divisors.n) 	/ divisors.post;
		uint32 pixelMultiply = uint32(adjusted 	/ (pixelClock / 1000.0f));
		write32(pllMD, (0 << 24) | ((pixelMultiply - 1) << 8));
	}

	if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_IGD)) {
		write32(pllDivisorA, (((1 << divisors.n) << DISPLAY_PLL_N_DIVISOR_SHIFT)
				& DISPLAY_PLL_IGD_N_DIVISOR_MASK)
			| (((divisors.m2 - 2) << DISPLAY_PLL_M2_DIVISOR_SHIFT)
				& DISPLAY_PLL_IGD_M2_DIVISOR_MASK));
	} else {
		write32(pllDivisorA, (((divisors.n - 2) << DISPLAY_PLL_N_DIVISOR_SHIFT)
				& DISPLAY_PLL_N_DIVISOR_MASK)
			| (((divisors.m1 - 2) << DISPLAY_PLL_M1_DIVISOR_SHIFT)
				& DISPLAY_PLL_M1_DIVISOR_MASK)
			| (((divisors.m2 - 2) << DISPLAY_PLL_M2_DIVISOR_SHIFT)
				& DISPLAY_PLL_M2_DIVISOR_MASK));
	}

	uint32 pll = DISPLAY_PLL_ENABLED | DISPLAY_PLL_NO_VGA_CONTROL | extraFlags;

	if (gInfo->shared_info->device_type.Generation() >= 4) {
		if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_IGD)) {
			pll |= ((1 << (divisors.post1 - 1))
					<< DISPLAY_PLL_IGD_POST1_DIVISOR_SHIFT)
				& DISPLAY_PLL_IGD_POST1_DIVISOR_MASK;
		} else {
			pll |= ((1 << (divisors.post1 - 1))
					<< DISPLAY_PLL_POST1_DIVISOR_SHIFT)
				& DISPLAY_PLL_9xx_POST1_DIVISOR_MASK;
	//		pll |= ((divisors.post1 - 1) << DISPLAY_PLL_POST1_DIVISOR_SHIFT)
	//			& DISPLAY_PLL_9xx_POST1_DIVISOR_MASK;
		}
		#if 0
		// TODO: ??? LVDS?
		switch (divisors.post2) {
			case 5:
			case 7:
				pll |= DISPLAY_PLL_DIVIDE_HIGH;
				break;
		}
		#endif
		if (divisors.post2_high)
			pll |= DISPLAY_PLL_DIVIDE_HIGH;

		if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_96x))
			pll |= 6 << DISPLAY_PLL_PULSE_PHASE_SHIFT;
	} else {
		if (!divisors.post2_high)
			pll |= DISPLAY_PLL_DIVIDE_4X;

		pll |= DISPLAY_PLL_2X_CLOCK;

		if (divisors.post1 > 2) {
			pll |= ((divisors.post1 - 2) << DISPLAY_PLL_POST1_DIVISOR_SHIFT)
				& DISPLAY_PLL_POST1_DIVISOR_MASK;
		} else
			pll |= DISPLAY_PLL_POST1_DIVIDE_2;
	}

	// Allow the PLL to warm up by masking its bit.
	write32(pllControl, pll & ~DISPLAY_PLL_NO_VGA_CONTROL);
	read32(pllControl);
	spin(150);
	write32(pllControl, pll);
	read32(pllControl);
	spin(150);
}


void
DisplayPipe::_Enable(bool enable)
{
	CALLED();

	addr_t targetRegister = fPlaneBase + INTEL_PIPE_CONTROL;

	write32(targetRegister, (read32(targetRegister) & ~INTEL_PIPE_ENABLED)
		| (enable ? INTEL_PIPE_ENABLED : 0));
	read32(targetRegister);
}
