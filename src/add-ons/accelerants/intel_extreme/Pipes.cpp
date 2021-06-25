/*
 * Copyright 2011-2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz, mmlr@mlotz.ch
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */
#include "Pipes.h"

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


// #pragma mark - Pipe


Pipe::Pipe(pipe_index pipeIndex)
	:
	fHasTranscoder(false),
	fFDILink(NULL),
	fPanelFitter(NULL),
	fPipeIndex(pipeIndex),
	fPipeOffset(0),
	fPlaneOffset(0)
{
	if (pipeIndex == INTEL_PIPE_B) {
		fPipeOffset = INTEL_DISPLAY_OFFSET;
		fPlaneOffset = INTEL_PLANE_OFFSET;
	}

	// IvyBridge: Analog + Digital Ports behind FDI (on northbridge)
	// Haswell: Only VGA behind FDI (on northbridge)
	// SkyLake: FDI gone. No more northbridge video.
	if (gInfo->shared_info->pch_info != INTEL_PCH_NONE) {
		TRACE("%s: Pipe %s routed through FDI\n", __func__,
			(pipeIndex == INTEL_PIPE_A) ? "A" : "B");

		fHasTranscoder = true;

		// Program FDILink if PCH
		fFDILink = new(std::nothrow) FDILink(pipeIndex);
		// Program gen5(+) style panelfitter as well
		fPanelFitter = new(std::nothrow) PanelFitter(pipeIndex);
	}

	TRACE("Pipe %s. Pipe Base: 0x%" B_PRIxADDR
		" Plane Base: 0x% " B_PRIxADDR "\n", (pipeIndex == INTEL_PIPE_A)
			? "A" : "B", fPipeOffset, fPlaneOffset);
}


Pipe::~Pipe()
{
}


bool
Pipe::IsEnabled()
{
	CALLED();

	return (read32(INTEL_DISPLAY_A_PIPE_CONTROL + fPipeOffset)
		& INTEL_PIPE_ENABLED) != 0;
}


void
Pipe::Configure(display_mode* mode)
{
	uint32 pipeControl = read32(INTEL_DISPLAY_A_PIPE_CONTROL + fPipeOffset);

	// TODO: Haswell+ dithering changes.
	//if (gInfo->shared_info->device_type.Generation() >= 4) {
	//	pipeControl |= (INTEL_PIPE_DITHER_EN | INTEL_PIPE_DITHER_TYPE_SP);

	//Link bit depth: this should be globally known per FDI link (i.e. laptop panel 3x6, rest 3x8)
	//currently using BIOS preconfigured setup
	//pipeControl = (pipeControl & ~INTEL_PIPE_BPC_MASK) | INTEL_PIPE_BPC(INTEL_PIPE_8BPC);

	// TODO: CxSR downclocking?

	// TODO: Interlaced modes
	pipeControl = (pipeControl & ~(0x7 << 21)) | INTEL_PIPE_PROGRESSIVE;

	write32(INTEL_DISPLAY_A_PIPE_CONTROL + fPipeOffset, pipeControl);
	read32(INTEL_DISPLAY_A_PIPE_CONTROL + fPipeOffset);

	if (gInfo->shared_info->device_type.Generation() >= 6) {
		// According to SandyBridge modesetting sequence, pipe must be enabled
		// before PLL are configured.
		addr_t pipeReg = INTEL_DISPLAY_A_PIPE_CONTROL + fPipeOffset;
		write32(pipeReg, read32(pipeReg) | INTEL_PIPE_ENABLED);
	}
}


void
Pipe::_ConfigureTranscoder(display_mode* target)
{
	CALLED();

	TRACE("%s: fPipeOffset: 0x%" B_PRIx32"\n", __func__, fPipeOffset);

	// update timing (fPipeOffset bumps the DISPLAY_A to B when needed)
	write32(INTEL_TRANSCODER_A_HTOTAL + fPipeOffset,
		((uint32)(target->timing.h_total - 1) << 16)
		| ((uint32)target->timing.h_display - 1));
	write32(INTEL_TRANSCODER_A_HBLANK + fPipeOffset,
		((uint32)(target->timing.h_total - 1) << 16)
		| ((uint32)target->timing.h_display - 1));
	write32(INTEL_TRANSCODER_A_HSYNC + fPipeOffset,
		((uint32)(target->timing.h_sync_end - 1) << 16)
		| ((uint32)target->timing.h_sync_start - 1));

	write32(INTEL_TRANSCODER_A_VTOTAL + fPipeOffset,
		((uint32)(target->timing.v_total - 1) << 16)
		| ((uint32)target->timing.v_display - 1));
	write32(INTEL_TRANSCODER_A_VBLANK + fPipeOffset,
		((uint32)(target->timing.v_total - 1) << 16)
		| ((uint32)target->timing.v_display - 1));
	write32(INTEL_TRANSCODER_A_VSYNC + fPipeOffset,
		((uint32)(target->timing.v_sync_end - 1) << 16)
		| ((uint32)target->timing.v_sync_start - 1));

	#if 0
	// XXX: Is it ok to do these on non-digital?
	write32(INTEL_TRANSCODER_A_POS + fPipeOffset, 0);
	write32(INTEL_TRANSCODER_A_IMAGE_SIZE + fPipeOffset,
		((uint32)(target->virtual_width - 1) << 16)
			| ((uint32)target->virtual_height - 1));
	#endif
}


void
Pipe::ConfigureTimings(display_mode* target, bool hardware)
{
	CALLED();

	TRACE("%s(%d): fPipeOffset: 0x%" B_PRIx32"\n", __func__, hardware,
		fPipeOffset);

	if (target == NULL) {
		ERROR("%s: Invalid display mode!\n", __func__);
		return;
	}

	/* If using the transcoder, leave the display at its native resolution,
	 * and configure only the transcoder (panel fitting will match them
	 * together). */
	if (!fHasTranscoder || hardware)
	{
		// update timing (fPipeOffset bumps the DISPLAY_A to B when needed)
		write32(INTEL_DISPLAY_A_HTOTAL + fPipeOffset,
			((uint32)(target->timing.h_total - 1) << 16)
			| ((uint32)target->timing.h_display - 1));
		write32(INTEL_DISPLAY_A_HBLANK + fPipeOffset,
			((uint32)(target->timing.h_total - 1) << 16)
			| ((uint32)target->timing.h_display - 1));
		write32(INTEL_DISPLAY_A_HSYNC + fPipeOffset,
			((uint32)(target->timing.h_sync_end - 1) << 16)
			| ((uint32)target->timing.h_sync_start - 1));

		write32(INTEL_DISPLAY_A_VTOTAL + fPipeOffset,
			((uint32)(target->timing.v_total - 1) << 16)
			| ((uint32)target->timing.v_display - 1));
		write32(INTEL_DISPLAY_A_VBLANK + fPipeOffset,
			((uint32)(target->timing.v_total - 1) << 16)
			| ((uint32)target->timing.v_display - 1));
		write32(INTEL_DISPLAY_A_VSYNC + fPipeOffset,
			((uint32)(target->timing.v_sync_end - 1) << 16)
			| ((uint32)target->timing.v_sync_start - 1));
	}

	if (gInfo->shared_info->device_type.Generation() < 6) {
		// FIXME check on which generations this register exists
		// (it appears it would be available only for cursor planes, not
		// display planes)
		// Since we set the plane to be the same size as the display, we can
		// just show it starting at top-left.
		write32(INTEL_DISPLAY_A_POS + fPipeOffset, 0);
	}

	// The only thing that really matters: set the image size and let the
	// panel fitter or the transcoder worry about the rest
	write32(INTEL_DISPLAY_A_PIPE_SIZE + fPipeOffset,
		((uint32)(target->virtual_width - 1) << 16)
			| ((uint32)target->virtual_height - 1));

	// Set the plane size as well while we're at it (this is independant, we
	// could have a larger plane and scroll through it).
	if (gInfo->shared_info->device_type.Generation() <= 4) {
		// This is "reserved" on G35 and GMA965, but needed on 945 (for which
		// there is no public documentation), and I assume earlier devices as
		// well. Note that the height and width are swapped when compared to
		// the other registers.
		write32(INTEL_DISPLAY_A_IMAGE_SIZE + fPipeOffset,
			((uint32)(target->virtual_height - 1) << 16)
			| ((uint32)target->virtual_width - 1));
	}

	if (fHasTranscoder && hardware) {
		_ConfigureTranscoder(target);
	}
}


void
Pipe::ConfigureClocks(const pll_divisors& divisors, uint32 pixelClock,
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

	// Disable DPLL first
	write32(pllControl, read32(pllControl) & ~DISPLAY_PLL_ENABLED);
	spin(150);

	float refFreq = gInfo->shared_info->pll_info.reference_frequency / 1000.0f;

	if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_96x)) {
		float adjusted = ((refFreq * divisors.m) / divisors.n) / divisors.p;
		uint32 pixelMultiply = uint32(adjusted / (pixelClock / 1000.0f));
		write32(pllMD, (0 << 24) | ((pixelMultiply - 1) << 8));
	}

	// XXX: For now we assume no LVDS downclocking and program the same divisor
	// value to both divisor 0 (standard) and 1 (reduced divisor)
	if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_PIN)) {
		write32(pllDivisorA, (((1 << divisors.n) << DISPLAY_PLL_N_DIVISOR_SHIFT)
				& DISPLAY_PLL_IGD_N_DIVISOR_MASK)
			| (((divisors.m2 - 2) << DISPLAY_PLL_M2_DIVISOR_SHIFT)
				& DISPLAY_PLL_IGD_M2_DIVISOR_MASK));
		write32(pllDivisorB, (((1 << divisors.n) << DISPLAY_PLL_N_DIVISOR_SHIFT)
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
		write32(pllDivisorB, (((divisors.n - 2) << DISPLAY_PLL_N_DIVISOR_SHIFT)
				& DISPLAY_PLL_N_DIVISOR_MASK)
			| (((divisors.m1 - 2) << DISPLAY_PLL_M1_DIVISOR_SHIFT)
				& DISPLAY_PLL_M1_DIVISOR_MASK)
			| (((divisors.m2 - 2) << DISPLAY_PLL_M2_DIVISOR_SHIFT)
				& DISPLAY_PLL_M2_DIVISOR_MASK));
	}

	//note: bit DISPLAY_PLL_NO_VGA_CONTROL does not exist on IvyBridge and should be left
	//      zero there. It does not influence it though.
	uint32 pll = DISPLAY_PLL_ENABLED | DISPLAY_PLL_NO_VGA_CONTROL | extraFlags;

	if (gInfo->shared_info->device_type.Generation() >= 3) {
		// p1 divisor << 1 , 1-8
		if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_PIN)) {
			pll |= ((1 << (divisors.p1 - 1))
					<< DISPLAY_PLL_IGD_POST1_DIVISOR_SHIFT)
				& DISPLAY_PLL_IGD_POST1_DIVISOR_MASK;
		} else {
			pll |= ((1 << (divisors.p1 - 1))
					<< DISPLAY_PLL_POST1_DIVISOR_SHIFT)
				& DISPLAY_PLL_9xx_POST1_DIVISOR_MASK;
		//	pll |= ((divisors.p1 - 1) << DISPLAY_PLL_POST1_DIVISOR_SHIFT)
		//		& DISPLAY_PLL_9xx_POST1_DIVISOR_MASK;
		}

		// Also configure the FP0 divisor on SandyBridge
		if (gInfo->shared_info->device_type.Generation() == 6) {
			pll |= ((1 << (divisors.p1 - 1))
					<< DISPLAY_PLL_SNB_FP0_POST1_DIVISOR_SHIFT)
				& DISPLAY_PLL_SNB_FP0_POST1_DIVISOR_MASK;
		}

		if (divisors.p2 == 5 || divisors.p2 == 7)
			pll |= DISPLAY_PLL_DIVIDE_HIGH;

		if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_96x))
			pll |= 6 << DISPLAY_PLL_PULSE_PHASE_SHIFT;
	} else {
		if (divisors.p2 != 5 && divisors.p2 != 7)
			pll |= DISPLAY_PLL_DIVIDE_4X;

		pll |= DISPLAY_PLL_2X_CLOCK;

		// TODO: Is this supposed to be DISPLAY_PLL_IGD_POST1_DIVISOR_MASK??
		if (divisors.p1 > 2) {
			pll |= ((divisors.p1 - 2) << DISPLAY_PLL_POST1_DIVISOR_SHIFT)
				& DISPLAY_PLL_POST1_DIVISOR_MASK;
		} else
			pll |= DISPLAY_PLL_POST1_DIVIDE_2;
	}

	// Configure PLL while -keeping- it disabled
	//note: on older chipsets DISPLAY_PLL_NO_VGA_CONTROL probably enables the PLL and locks regs;
	//      on newer chipsets DISPLAY_PLL_ENABLED does this.
	write32(pllControl, pll & ~DISPLAY_PLL_ENABLED & ~DISPLAY_PLL_NO_VGA_CONTROL);
	read32(pllControl);
	spin(150);

	// enable pre-configured PLL (locks PLL settings directly blocking changes in this write even)
	write32(pllControl, pll);
	read32(pllControl);

	// Allow the PLL to warm up.
	spin(150);

	if (gInfo->shared_info->device_type.Generation() >= 6) {
		// SandyBridge has 3 transcoders, but only 2 PLLs. So there is a new
		// register which routes the PLL output to the transcoder that we need
		// to configure
		uint32 pllSel = read32(SNB_DPLL_SEL);
		TRACE("Old PLL selection: 0x%" B_PRIx32 "\n", pllSel);
		uint32 shift = 0;
		uint32 pllIndex = 0;

		// FIXME we assume that pipe A is used with transcoder A, and pipe B
		// with transcoder B, that may not always be the case
		if (fPipeIndex == INTEL_PIPE_A) {
			shift = 0;
			pllIndex = 0;
			TRACE("Route PLL A to transcoder A\n");
		} else if (fPipeIndex == INTEL_PIPE_B) {
			shift = 4;
			pllIndex = 1;
			TRACE("Route PLL B to transcoder B\n");
		} else {
			ERROR("Attempting to configure PLL for unhandled pipe");
			return;
		}

		// Mask out the previous PLL configuration for this transcoder
		pllSel &= ~(0xF << shift);

		// Set up the new configuration for this transcoder and enable it
		pllSel |= (8 | pllIndex) << shift;

		TRACE("New PLL selection: 0x%" B_PRIx32 "\n", pllSel);
		write32(SNB_DPLL_SEL, pllSel);
	}
}

void
Pipe::Enable(bool enable)
{
	CALLED();

	addr_t pipeReg = INTEL_DISPLAY_A_PIPE_CONTROL + fPipeOffset;
	addr_t planeReg = INTEL_DISPLAY_A_CONTROL + fPlaneOffset;

	// Planes always have to operate on an enabled pipe

	if (enable) {
		write32(pipeReg, read32(pipeReg) | INTEL_PIPE_ENABLED);
		wait_for_vblank();
		write32(planeReg, read32(planeReg) | DISPLAY_CONTROL_ENABLED);

		//Enable default display main watermarks
		if (gInfo->shared_info->pch_info == INTEL_PCH_CPT) {
			if (fPipeOffset == 0)
				write32(INTEL_DISPLAY_A_PIPE_WATERMARK, 0x0783818);
			else
				write32(INTEL_DISPLAY_B_PIPE_WATERMARK, 0x0783818);
		}
	} else {
		write32(planeReg, read32(planeReg) & ~DISPLAY_CONTROL_ENABLED);
		wait_for_vblank();
		//Sandy+: when link training is to be done re-enable this line but otherwise don't touch!
		//GMA(Q45): must disable PIPE or DPLL programming fails.
		if (gInfo->shared_info->device_type.Generation() <= 5) {
			write32(pipeReg, read32(pipeReg) & ~INTEL_PIPE_ENABLED);
		}
	}

	// flush the eventually cached PCI bus writes
	read32(INTEL_DISPLAY_A_BASE);
}
