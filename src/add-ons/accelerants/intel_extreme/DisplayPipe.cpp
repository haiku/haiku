/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz, mmlr@mlotz.ch
 */


#include "DisplayPipe.h"

#include "accelerant.h"
#include "intel_extreme.h"

#include <stdlib.h>
#include <string.h>


#define TRACE_PIPE
#ifdef TRACE_PIPE
extern "C" void _sPrintf(const char* format, ...);
#	define TRACE(x) _sPrintf x
#else
#	define TRACE(x) ;
#endif


// #pragma mark - DisplayPipe


DisplayPipe::DisplayPipe(int32 pipeIndex)
	:
	fFDILink(NULL),
	fPanelFitter(NULL),
	fBaseRegister(INTEL_PIPE_BASE_REGISTER + pipeIndex * INTEL_PIPE_PIPE_OFFSET)
{
}


bool
DisplayPipe::IsEnabled()
{
	return (read32(fBaseRegister + INTEL_PIPE_CONTROL) & PIPE_ENABLED) != 0;
}


void
DisplayPipe::Enable(const display_mode& mode)
{
	_Enable(true);
}


void
DisplayPipe::Disable()
{
	_Enable(false);
}


void
DisplayPipe::ConfigureTimings(const pll_divisors& divisors)
{
	if (gInfo->shared_info->device_type.InGroup(INTEL_TYPE_IGD)) {
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
		if (gInfo->shared_info->device_type.InFamily(INTEL_TYPE_9xx)) {
			if (gInfo->shared_info->device_type.InGroup(INTEL_TYPE_IGD)) {
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

			if (gInfo->shared_info->device_type.InGroup(INTEL_TYPE_96x))
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

		// TODO: verify the two comments below: the X driver doesn't seem to
		//		care about both of them!

		// These two have to be set for display B, too - this obviously means
		// that the second head always must adopt the color space of the first
		// head.
		write32(INTEL_DISPLAY_A_CONTROL, (read32(INTEL_DISPLAY_A_CONTROL)
				& ~(DISPLAY_CONTROL_COLOR_MASK | DISPLAY_CONTROL_GAMMA))
			| colorMode);

		if ((gInfo->head_mode & HEAD_MODE_B_DIGITAL) != 0) {
			write32(INTEL_DISPLAY_B_IMAGE_SIZE,
				((uint32)(target.virtual_width - 1) << 16)
				| ((uint32)target.virtual_height - 1));

			write32(INTEL_DISPLAY_B_CONTROL, (read32(INTEL_DISPLAY_B_CONTROL)
					& ~(DISPLAY_CONTROL_COLOR_MASK | DISPLAY_CONTROL_GAMMA))
				| colorMode);
		}

}


void
DisplayPipe::_Enable(bool enable)
{
	uint32 targetRegister = fBaseRegister + INTEL_PIPE_CONTROL;
	write32(targetRegister, read32(targetRegister) & ~PIPE_ENABLED
		| (enable ? PIPE_ENABLED | 0));
	read32(targetRegister);
}
