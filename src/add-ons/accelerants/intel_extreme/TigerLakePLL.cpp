/*
 * Copyright 2024, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */

#include "TigerLakePLL.h"

#include "accelerant.h"

#include <math.h>


#undef TRACE
#define TRACE_PLL
#ifdef TRACE_PLL
#   define TRACE(x...) _sPrintf("intel_extreme: " x)
#else
#   define TRACE(x...)
#endif

#define ERROR(x...) _sPrintf("intel_extreme: " x)
#define CALLED(x...) TRACE("CALLED %s\n", __PRETTY_FUNCTION__)


/**
 * Compute the best PLL parameters for a given symbol clock frequency for a DVI or HDMI port.
 *
 * This is the algorithm documented in Intel Documentation: IHD-OS-TGL-Vol 12-12.21, page 182
 *
 * The clock generation on Tiger Lake is in two steps: first, a DCO generates a fractional
 * multiplication of the reference clock (in the GHz range). Then, 3 dividers bring this back into
 * the symbol clock frequency range.
 *
 * Reference clock (24 or 19.2MHz, as defined in DSSM Reference Frequency register)
 *             ||
 *             vv
 * DCO (multiply by non-integer value defined in DPLL_CFGCR0 register)
 *             ||
 *             vv
 * "DCO frequency" in the range 7998 - 10000 MHz
 *             ||
 *             vv
 * Divide by P, Q, and K
 *             ||
 *             vv
 * AFE clock (PLL output)
 *             ||
 *             vv
 * Divide by 5 (fixed)
 *             ||
 *             vv
 * Symbol clock (same as Pixel clock for 24-bit RGB)
 *
 * The algorithm to configure this is:
 * - Iterate over all allowed values for the divider obtained by P, Q and K
 * - Determine the one that results in the DCO frequency being as close as possible to 8999MHz
 * - Compute the corresponding values for P, Q and K and the DCO multiplier
 *
 * Since the DCO is a fractional multiplier (it can multiply by non-integer values), it will always
 * be possible to set the DCO to a "close enough" value in its available range. The only constraint
 * is getting it as close as possible to the midpoint (8999MHz), and at least have it in the
 * allowed range (7998 to 10000MHz). If this is not possible (too low or too high pixel clock), a
 * different video mode or setup will be needed (for example, enable dual link DVI to divide the
 * clock by two).
 *
 * This also means that this algorithm is independant of the initial reference frequency: there
 * will always be a way to setup the DCO so that it outputs the frequency computed here, no matter
 * what the input clock is.
 *
 * Unlinke in previous hardware generations, there is no need to satisfy multiple constraints at
 * the same time because of several stages of dividers and multipliers each with their own
 * frequency range.
 *
 * DCO multiplier = DCO integer + DCO fraction / 2^15
 * Symbol clock frequency = DCO multiplier * RefFreq in MHz / (5 * Pdiv * Qdiv * Kdiv)
 *
 * The symbol clock is the clock of the DVI/HDMI port. It defines how much time is needed to send
 * one "symbol", which corresponds to 8 bits of useful data for each channel (Red, Green and Blue).
 *
 * In our case (8 bit RGB videomode), the symbol clock is equal to the pixel rate. It would need
 * to be adjusted for 10 and 12-bit modes (more bits per pixel) as well as for YUV420 modes (the U
 * and V parts are sent only for some pixels, reducing the total bandwidth).
 *
 * @param[in] freq Desired symbol clock frequency in kHz
 * @param[out] Pdiv, Qdiv, Kdiv: dividers for the PLL
 * @param[out] bestdco Required DCO frequency, in the range 7998 to 10000, in MHz
 */
bool
ComputeHdmiDpll(int freq, int* Pdiv, int* Qdiv, int* Kdiv, float* bestdco)
{
	int bestdiv = 0;
	float dco = 0, dcocentrality = 0;
	float bestdcocentrality = 999999;

	// The allowed values for the divider depending on the allowed values for P, Q, and K:
	// - P can be 2, 3, 5 or 7
	// - K can be 1, 2, or 3
	// - Q can be 1 to 255 if K = 2. Otherwise, Q must be 1.
	// Not all possible combinations are listed here, more can be added if needed to reach lower
	// resolutions and refresh rates (probably not so interesting, this already allows to reach
	// frequencies low enough for all practical uses in a standard setup).
	const int dividerlist[] = { 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 24, 28, 30, 32, 36,
		40, 42, 44, 48, 50, 52, 54, 56, 60, 64, 66, 68, 70, 72, 76, 78, 80, 84, 88, 90, 92,
		96, 98, 100, 102, 3, 5, 7, 9, 15, 21 };
	const float dcomin = 7998;
	const float dcomax = 10000;
	const float dcomid = (dcomin + dcomax) / 2;

	float afeclk = (5 * freq) / 1000;

	for (size_t i = 0; i < B_COUNT_OF(dividerlist); i++) {
		int div = dividerlist[i];
		dco = afeclk * div;
		if (dco <= dcomax && dco >= dcomin) {
			dcocentrality = fabs(dco - dcomid);
			if (dcocentrality < bestdcocentrality) {
				bestdcocentrality = dcocentrality;
				bestdiv = div;
				*bestdco = dco;
			}
		}
	}

	if (bestdiv != 0) {
		// Good divider found
		if (bestdiv % 2 == 0) {
			// Divider is even
			if (bestdiv == 2) {
				*Pdiv = 2;
				*Qdiv = 1;
				*Kdiv = 1;
			} else if (bestdiv % 4 == 0) {
				*Pdiv = 2;
				*Qdiv = bestdiv / 4;
				*Kdiv = 2;
			} else if (bestdiv % 6 == 0) {
				*Pdiv = 3;
				*Qdiv = bestdiv / 6;
				*Kdiv = 2;
			} else if (bestdiv % 5 == 0) {
				*Pdiv = 5;
				*Qdiv = bestdiv / 10;
				*Kdiv = 2;
			} else if (bestdiv % 14 == 0) {
				*Pdiv = 7;
				*Qdiv = bestdiv / 14;
				*Kdiv = 2;
			}
		} else {
			// Divider is odd
			if (bestdiv == 3 || bestdiv == 5 || bestdiv == 7) {
				*Pdiv = bestdiv;
				*Qdiv = 1;
				*Kdiv = 1;
			} else {
				// Divider is 9, 15, or 21
				*Pdiv = bestdiv / 3;
				*Qdiv = 1;
				*Kdiv = 3;
			}
		}

		// SUCCESS
		return true;
	} else {
		// No good divider found
		// FAIL, try a different frequency (different video mode)
		return false;
	}
}


/*! In the case of DisplayPort, the interface between the computer and the display is not just a
 * stream of pixels, but instead a packetized link. This means the interface does not need to be
 * running in sync with the pixel clock. Instead, a selection of well-defined frequencies are used.
 *
 * This also would allow to use a "spread spectrum" clock, reducing interferences without degrading
 * picture quality.
 *
 * Here we just set it to the isecond lowest predefined frequency of 2.7GHz, which will be enough
 * for displays up to full HD, and a little more.
 *
 * TODO decide when we have to use one of the higher frequencies. See "DisplayPort Mode PLL values"
 * in IHD-OS-TGL-Vol 12-12.21, page 178. However, my machine uses a slightly different value than
 * what's in Intel datasheets (with Intel values, bestdco should be 8100 and not 8090). I'm not
 * sure why that is so, possibly they shift the fractional value in the CFGR0 register by 9 bits
 * instead of 10? But replicating what my BIOS does here allows me to skip the PLL
 * programming, a good idea, because it seems we don't yet know how to properly disable and
 * re-train displayport once it is switched off.
 */
bool
ComputeDisplayPortDpll(int freq, int* Pdiv, int* Qdiv, int* Kdiv, float* bestdco)
{
	*Pdiv = 3;
	*Qdiv = 1;
	*Kdiv = 2;
	*bestdco = 8090;

	return true;
}


/*! Actually program the computed values (from the functions above) into the PLL, and start it.
 *
 * TODO: detect if the PLL is already running at the right frequency, and in that case, skip
 * reprogramming it altogether. In the case of DisplayPort, most often there is no need to change
 * anything once the clock has been initially set.
 */
status_t
ProgramPLL(int which, int Pdiv, int Qdiv, int Kdiv, float dco)
{
	// Set up the registers for PLL access for the requested PLL
	uint32 DPLL_CFGCR0;
	uint32 DPLL_CFGCR1;
	uint32 DPLL_ENABLE;
	uint32 DPLL_SPREAD_SPECTRUM;

	switch (which) {
		case 0:
			DPLL_ENABLE = TGL_DPLL0_ENABLE;
			DPLL_SPREAD_SPECTRUM = TGL_DPLL0_SPREAD_SPECTRUM;
			DPLL_CFGCR0 = TGL_DPLL0_CFGCR0;
			DPLL_CFGCR1 = TGL_DPLL0_CFGCR1;
			break;
		case 1:
			DPLL_ENABLE = TGL_DPLL1_ENABLE;
			DPLL_SPREAD_SPECTRUM = TGL_DPLL1_SPREAD_SPECTRUM;
			DPLL_CFGCR0 = TGL_DPLL1_CFGCR0;
			DPLL_CFGCR1 = TGL_DPLL1_CFGCR1;
			break;
		case 4:
			DPLL_ENABLE = TGL_DPLL4_ENABLE;
			DPLL_SPREAD_SPECTRUM = TGL_DPLL4_SPREAD_SPECTRUM;
			DPLL_CFGCR0 = TGL_DPLL4_CFGCR0;
			DPLL_CFGCR1 = TGL_DPLL4_CFGCR1;
			break;
		default:
			return B_BAD_VALUE;
	}

	// Find the reference frequency (24 or 19.2MHz)
	int ref_khz = gInfo->shared_info->pll_info.reference_frequency;

	// There is an automatic divide-by-two in this case
	if (ref_khz == 38400)
		ref_khz = 19200;

	float ref = ref_khz / 1000.0f;

	// Compute the DCO divider integer and fractional parts
	uint32 dco_int = (uint32)floorf(dco / ref);
	uint32 dco_frac = (uint32)ceilf((dco / ref - dco_int) * (1 << 15));

	int32 dco_reg = dco_int | (dco_frac << TGL_DPLL_DCO_FRACTION_SHIFT);

	int32 dividers = 0;
	switch (Pdiv) {
		case 2:
			dividers |= TGL_DPLL_PDIV_2;
			break;
		case 3:
			dividers |= TGL_DPLL_PDIV_3;
			break;
		case 5:
			dividers |= TGL_DPLL_PDIV_5;
			break;
		case 7:
			dividers |= TGL_DPLL_PDIV_7;
			break;
		default:
			return B_BAD_VALUE;
	}
	switch (Kdiv) {
		case 1:
			dividers |= TGL_DPLL_KDIV_1;
			break;
		case 2:
			dividers |= TGL_DPLL_KDIV_2;
			break;
		case 3:
			dividers |= TGL_DPLL_KDIV_3;
			break;
		default:
			return B_BAD_VALUE;
	}
	if (Qdiv != 1)
		dividers |= (Qdiv << TGL_DPLL_QDIV_RATIO_SHIFT) | TGL_DPLL_QDIV_ENABLE;

	int32 initialState = read32(DPLL_ENABLE);
	TRACE("DPLL_ENABLE(%" B_PRIx32 ") initial value = %" B_PRIx32 "\n", DPLL_ENABLE, initialState);

	if (initialState & TGL_DPLL_LOCK) {
		int32 oldDCO = read32(DPLL_CFGCR0);
		int32 oldDividers = read32(DPLL_CFGCR1);
		TRACE("DPLL already locked, checking current settings: DCO %" B_PRIx32 " -> %" B_PRIx32
				", dividers %" B_PRIx32 " -> %" B_PRIx32 "\n",
			oldDCO, dco_reg, oldDividers, dividers);

		if ((oldDCO == dco_reg) && (oldDividers == dividers)) {
			TRACE("DPLL already configured at the right frequency, no changes needed\n");
			return B_OK;
		}
	}

	// Before we start, disable the PLL
	write32(DPLL_ENABLE, read32(DPLL_ENABLE) & ~TGL_DPLL_ENABLE);
	while ((read32(DPLL_ENABLE) & TGL_DPLL_LOCK) != 0);
	TRACE("PLL is unlocked\n");

	// Enable PLL power
	write32(DPLL_ENABLE, read32(DPLL_ENABLE) | TGL_DPLL_POWER_ENABLE);

	// Wait for PLL to be powered up
	while ((read32(DPLL_ENABLE) & TGL_DPLL_POWER_STATE) == 0);
	TRACE("PLL is powered on\n");

	// Deactivate spread spectrum
	write32(DPLL_SPREAD_SPECTRUM, read32(DPLL_SPREAD_SPECTRUM) & ~TGL_DPLL_SSC_ENABLE);

	// Configure DCO
	write32(DPLL_CFGCR0, dco_reg);

	// Configure dividers
	write32(DPLL_CFGCR1, dividers);
	TRACE("DFGCR0(%" B_PRIx32 ") = %" B_PRIx32 ", CFGCR1(%" B_PRIx32 ") = %" B_PRIx32
			" (int = %" B_PRId32 ", frac = %" B_PRId32 ")\n", DPLL_CFGCR0, dco_reg,
		DPLL_CFGCR1, dividers, dco_int, dco_frac);

	// Read to make sure all writes are flushed to the hardware
	read32(DPLL_CFGCR1);

	// TODO Display voltage frequency switching?

	// Enable PLL
	write32(DPLL_ENABLE, read32(DPLL_ENABLE) | TGL_DPLL_ENABLE);
	TRACE("DPLL_ENABLE(%" B_PRIx32 ") = %" B_PRIx32 "\n", DPLL_ENABLE, read32(DPLL_ENABLE));

	// Wait for PLL to be enabled
	while ((read32(DPLL_ENABLE) & TGL_DPLL_LOCK) == 0);
	TRACE("PLL is locked\n");

	// TODO Display voltage frequency switching?

	return B_OK;
}
