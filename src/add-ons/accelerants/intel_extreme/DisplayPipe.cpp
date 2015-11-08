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


#define TRACE_PIPE
#ifdef TRACE_PIPE
extern "C" void _sPrintf(const char* format, ...);
#	define TRACE(x) _sPrintf x
#else
#	define TRACE(x) ;
#endif


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
//	fFDILink(NULL),
//	fPanelFitter(NULL),
	fBaseRegister(INTEL_PIPE_BASE_REGISTER + pipeIndex * INTEL_PIPE_OFFSET),
	fPipeIndex(pipeIndex)
{
}


bool
DisplayPipe::IsEnabled()
{
	return (read32(fBaseRegister + INTEL_PIPE_CONTROL)
		& INTEL_PIPE_ENABLED) != 0;
}


void
DisplayPipe::Enable(const display_mode& mode)
{
	_Enable(true);
	write32(INTEL_DISPLAY_B_IMAGE_SIZE, ((uint32)(mode.virtual_width - 1) << 16)
		| (uint32)(mode.virtual_height - 1));
}


void
DisplayPipe::Disable()
{
	_Enable(false);
}


void
DisplayPipe::ConfigureTimings(const pll_divisors& divisors)
{
	if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_IGD)) {
			write32(INTEL_DISPLAY_A_PLL_DIVISOR_0,
				(((1 << divisors.n) << DISPLAY_PLL_N_DIVISOR_SHIFT)
					& DISPLAY_PLL_IGD_N_DIVISOR_MASK)
				| (((divisors.m2 - 2) << DISPLAY_PLL_M2_DIVISOR_SHIFT)
					& DISPLAY_PLL_IGD_M2_DIVISOR_MASK));
		} else {
			write32(INTEL_DISPLAY_A_PLL_DIVISOR_0,
				(((divisors.n - 2) << DISPLAY_PLL_N_DIVISOR_SHIFT)
					& DISPLAY_PLL_N_DIVISOR_MASK)
				| (((divisors.m1 - 2) << DISPLAY_PLL_M1_DIVISOR_SHIFT)
					& DISPLAY_PLL_M1_DIVISOR_MASK)
				| (((divisors.m2 - 2) << DISPLAY_PLL_M2_DIVISOR_SHIFT)
					& DISPLAY_PLL_M2_DIVISOR_MASK));
		}

		uint32 pll = DISPLAY_PLL_ENABLED | DISPLAY_PLL_NO_VGA_CONTROL;
		if (gInfo->shared_info->device_type.InFamily(INTEL_FAMILY_9xx)) {
			if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_IGD)) {
				pll |= ((1 << (divisors.post1 - 1))
						<< DISPLAY_PLL_IGD_POST1_DIVISOR_SHIFT)
					& DISPLAY_PLL_IGD_POST1_DIVISOR_MASK;
			} else {
				pll |= ((1 << (divisors.post1 - 1))
						<< DISPLAY_PLL_POST1_DIVISOR_SHIFT)
					& DISPLAY_PLL_9xx_POST1_DIVISOR_MASK;
//				pll |= ((divisors.post1 - 1) << DISPLAY_PLL_POST1_DIVISOR_SHIFT)
//					& DISPLAY_PLL_9xx_POST1_DIVISOR_MASK;
			}
			if (divisors.post2_high)
				pll |= DISPLAY_PLL_DIVIDE_HIGH;

			pll |= DISPLAY_PLL_MODE_ANALOG;

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

		write32(INTEL_DISPLAY_A_PLL, pll);
		read32(INTEL_DISPLAY_A_PLL);
		spin(150);
		write32(INTEL_DISPLAY_A_PLL, pll);
		read32(INTEL_DISPLAY_A_PLL);
		spin(150);

	#if 0
		// update timing parameters
		write32(INTEL_DISPLAY_A_HTOTAL,
			((uint32)(target.timing.h_total - 1) << 16)
			| ((uint32)target.timing.h_display - 1));
		write32(INTEL_DISPLAY_A_HBLANK,
			((uint32)(target.timing.h_total - 1) << 16)
			| ((uint32)target.timing.h_display - 1));
		write32(INTEL_DISPLAY_A_HSYNC,
			((uint32)(target.timing.h_sync_end - 1) << 16)
			| ((uint32)target.timing.h_sync_start - 1));

		write32(INTEL_DISPLAY_A_VTOTAL,
			((uint32)(target.timing.v_total - 1) << 16)
			| ((uint32)target.timing.v_display - 1));
		write32(INTEL_DISPLAY_A_VBLANK,
			((uint32)(target.timing.v_total - 1) << 16)
			| ((uint32)target.timing.v_display - 1));
		write32(INTEL_DISPLAY_A_VSYNC,
			((uint32)(target.timing.v_sync_end - 1) << 16)
			| ((uint32)target.timing.v_sync_start - 1));

		write32(INTEL_DISPLAY_A_IMAGE_SIZE,
			((uint32)(target.virtual_width - 1) << 16)
			| ((uint32)target.virtual_height - 1));

		write32(INTEL_ANALOG_PORT, (read32(INTEL_ANALOG_PORT)
				& ~(DISPLAY_MONITOR_POLARITY_MASK
					| DISPLAY_MONITOR_VGA_POLARITY))
			| ((target.timing.flags & B_POSITIVE_HSYNC) != 0
				? DISPLAY_MONITOR_POSITIVE_HSYNC : 0)
			| ((target.timing.flags & B_POSITIVE_VSYNC) != 0
				? DISPLAY_MONITOR_POSITIVE_VSYNC : 0));
	#endif
}


void
DisplayPipe::_Enable(bool enable)
{
	uint32 targetRegister = fBaseRegister + INTEL_PIPE_CONTROL;
	write32(targetRegister, (read32(targetRegister) & ~INTEL_PIPE_ENABLED)
		| (enable ? INTEL_PIPE_ENABLED : 0));
	read32(targetRegister);
}
