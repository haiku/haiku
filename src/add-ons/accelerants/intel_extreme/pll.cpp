/*
 * Copyright 2006-2018, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */


#include "pll.h"

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


// PLL limits, taken from i915 DRM driver. However, note that we use the values of
// N+2, M1+2 and M2+2 here, the - 2 being applied when we write the values to the registers.

static pll_limits kLimits85x = {
	// p, p1, p2,  n,   m, m1, m2
	{  4,  2,  2,  4,  96, 20,  8},
	{128, 33,  4, 18, 140, 28, 18},
	165000, 908000, 1512000
};

// For Iron Lake, a new set of timings is introduced along with the FDI system,
// and carried on to later cards with just one further change (to the P2 cutoff
// frequency) in Sandy Bridge.

static pll_limits kLimits9xxSdvo = {
	// p, p1, p2,  n,   m, m1, m2
	{  5,  1,  5,  3,  70, 10,  5},	// min
	{ 80,  8, 10,  8, 120, 20,  9},	// max
	200000, 1400000, 2800000
};

static pll_limits kLimits9xxLvds = {
	// p, p1, p2,  n,   m, m1, m2
	{  7,  1,  7,  3,  70, 10,  5},	// min
	{ 98,  8, 14,  8, 120, 20,  9},	// max
	112000, 1400000, 2800000
};

// Limits for G45 cards taken from i915 DRM driver, mixed with old setup
// plus tests to accomodate lower resolutions with still correct refresh.
// Note that n here is actually n+2, same applies to m1 and m2.

static pll_limits kLimitsG4xSdvo = {
	// p, p1, p2,  n,   m, m1, m2
	{ 10,  1, 10,  3, 104, 19,  7},	// min
	{ 80,  8, 10,  8, 138, 25, 13},	// max
	270000, 1750000, 3500000
};

#if 0
static pll_limits kLimitsG4xHdmi = {
	// p, p1, p2,  n,   m, m1, m2
	{  5,  1,  5,  3, 104, 18,  7},	// min
	{ 80,  8, 10,  8, 138, 25, 13},	// max
	165000, 1750000, 3500000
};
#endif

static pll_limits kLimitsG4xLvdsSingle = {
	// p, p1, p2,  n,   m, m1, m2
	{ 28,  2, 14,  3, 104, 19,  7},	// min
	{112,  8, 14,  8, 138, 25, 13},	// max
	0, 1750000, 3500000
};

static pll_limits kLimitsG4xLvdsDual = {
	// p, p1, p2,  n,   m, m1, m2
	{ 14,  2,  7,  3, 104, 19,  7},	// min
	{ 42,  6,  7,  8, 138, 25, 13},	// max
	0, 1750000, 3500000
};

static pll_limits kLimitsIlkDac = {
	// p, p1, p2, n,   m, m1, m2
	{  5,  1,  5, 3,  79, 14,  7}, // min
	{ 80,  8, 10, 7, 127, 24, 11}, // max
	225000, 1760000, 3510000
};

static pll_limits kLimitsIlkLvdsSingle = {
	// p, p1, p2, n,   m, m1, m2
	{ 28,  2, 14, 3,  79, 14,  7}, // min
	{112,  8, 14, 5, 118, 24, 11}, // max
	225000, 1760000, 3510000
};

static pll_limits kLimitsIlkLvdsDual = {
	// p, p1, p2, n,   m, m1, m2
	{ 14,  2,  7, 3,  79, 14,  7}, // min
	{ 56,  8,  7, 5, 127, 24, 11}, // max
	225000, 1760000, 3510000
};

// 100Mhz RefClock
static pll_limits kLimitsIlkLvdsSingle100 = {
	// p, p1, p2, n,   m, m1, m2
	{ 28,  2, 14, 3,  79, 14,  7}, // min
	{112,  8, 14, 4, 126, 24, 11}, // max
	225000, 1760000, 3510000
};

static pll_limits kLimitsIlkLvdsDual100 = {
	// p, p1, p2, n,   m, m1, m2
	{ 14,  2,  7, 3,  79, 14,  7}, // min
	{ 42,  6,  7, 5, 126, 24, 11}, // max
	225000, 1760000, 3510000
};

// TODO From haswell onwards, a completely different PLL design is used
// (intel_gfx-prm-osrc-hsw-display_0.pdf, page 268 for VGA). It uses a "virtual
// root frequency" and one just has to set a single divider (integer and
// fractional parts), so it makes no sense to reuse the same code and limit
// structures there.
//
// For other display connections, the clock is handled differently, as there is
// no need for a precise timing to send things in sync with the display.
#if 0
static pll_limits kLimitsChv = {
	// p, p1, p2, n,   m, m1, m2
	{  0,  2,  1, 1,  79, 2,   24 << 22}, // min
	{  0,  4, 14, 1, 127, 2,  175 << 22}, // max
	0, 4800000, 6480000
};

static pll_limits kLimitsVlv = {
	// p, p1, p2, n,   m, m1, m2
	{  0,  2,  2, 1,  79, 2,   11},	// min
	{  0,  3, 20, 7, 127, 3,  156},	// max
	0, 4000000, 6000000
};

static pll_limits kLimitsBxt = {
	// p, p1, p2, n,  m, m1, m2
	{  0,  2,  1, 1,  0,  2,   2 << 22}, // min
	{  0,  4, 20, 1,  0,  2, 255 << 22}, // max
	0, 4800000, 6700000
};
#endif

static pll_limits kLimitsPinSdvo = {
	// p, p1, p2, n,   m, m1,  m2
	{  5,  1,  5, 3,   2,  0,   0},	// min
	{ 80,  8, 10, 6, 256,  0, 254},	// max
	200000, 1700000, 3500000
};

static pll_limits kLimitsPinLvds = {
	// p, p1, p2, n,   m, m1,  m2
	{  7,  1, 14, 3,   2,  0,   0},	// min
	{112,  8, 14, 6, 256,  0, 254},	// max
	112000, 1700000, 3500000
};


static bool
lvds_dual_link(display_timing* current)
{
	float requestedPixelClock = current->pixel_clock / 1000.0f;
	if (requestedPixelClock > 112.999)
		return true;

	// TODO: Force dual link on MacBookPro6,2  MacBookPro8,2  MacBookPro9,1

	return ((read32(INTEL_DIGITAL_LVDS_PORT) & LVDS_CLKB_POWER_MASK)
		== LVDS_CLKB_POWER_UP);
}


bool
valid_pll_divisors(pll_divisors* divisors, pll_limits* limits)
{
	pll_info &info = gInfo->shared_info->pll_info;
	uint32 vco = info.reference_frequency * divisors->m / divisors->n;
	uint32 frequency = vco / divisors->p;

	if (divisors->p < limits->min.p || divisors->p > limits->max.p
		|| divisors->m < limits->min.m || divisors->m > limits->max.m
		|| vco < limits->min_vco || vco > limits->max_vco
		|| frequency < info.min_frequency || frequency > info.max_frequency)
		return false;

	return true;
}


static void
compute_pll_p2(display_timing* current, pll_divisors* divisors,
	pll_limits* limits, bool isLVDS)
{
	if (isLVDS) {
		if (lvds_dual_link(current)) {
			// fast DAC timing via 2 channels (dual link LVDS)
			divisors->p2 = limits->min.p2;
		} else {
			// slow DAC timing
			divisors->p2 = limits->max.p2;
		}
	} else {
		if (current->pixel_clock < limits->dot_limit) {
			// slow DAC timing
			divisors->p2 = limits->max.p2;
		} else {
			// fast DAC timing
			divisors->p2 = limits->min.p2;
		}
	}
}


// TODO we can simplify this computation, with the way the dividers are set, we
// know that all values in the valid range for M are reachable. M1 allows to
// generate any multiple of 5 in the range and M2 allows to reach the 4 next
// values. Therefore, we don't need to loop over the range of values for M1 and
// M2 separately, we could instead just loop over possible values for M.
// For this to work, the logic of this function must be reversed: for a given M,
// it should give the resulting M1 and M2 values for programming the registers.
static uint32
compute_pll_m(pll_divisors* divisors)
{
	if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_CHV)
		|| gInfo->shared_info->device_type.InGroup(INTEL_GROUP_VLV)) {
		return divisors->m1 * divisors->m2;
	}

	// Pineview, m1 is reserved
	if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_PIN))
		return divisors->m2;

	return 5 * divisors->m1 + divisors->m2;
}


static uint32
compute_pll_p(pll_divisors* divisors)
{
	return divisors->p1 * divisors->p2;
}


static void
compute_dpll_g4x(display_timing* current, pll_divisors* divisors, bool isLVDS)
{
	float requestedPixelClock = current->pixel_clock / 1000.0f;
	float referenceClock
		= gInfo->shared_info->pll_info.reference_frequency / 1000.0f;

	TRACE("%s: required MHz: %g, reference clock: %g\n", __func__,
		requestedPixelClock, referenceClock);

	pll_limits limits;
	if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_G4x)) {
		// TODO: Pass port type via video_configuration
		if (isLVDS) {
			if (lvds_dual_link(current))
				memcpy(&limits, &kLimitsG4xLvdsDual, sizeof(pll_limits));
			else
				memcpy(&limits, &kLimitsG4xLvdsSingle, sizeof(pll_limits));
		//} else if (type == INTEL_PORT_TYPE_HDMI) {
		//	memcpy(&limits, &kLimitsG4xHdmi, sizeof(pll_limits));
		} else
			memcpy(&limits, &kLimitsG4xSdvo, sizeof(pll_limits));
	} else {
		// There must be a PCH, so this is ivy bridge or later
		if (isLVDS) {
			if (lvds_dual_link(current)) {
				if (referenceClock == 100.0)
					memcpy(&limits, &kLimitsIlkLvdsDual100, sizeof(pll_limits));
				else
					memcpy(&limits, &kLimitsIlkLvdsDual, sizeof(pll_limits));
			} else {
				if (referenceClock == 100.0) {
					memcpy(&limits, &kLimitsIlkLvdsSingle100,
						sizeof(pll_limits));
				} else {
					memcpy(&limits, &kLimitsIlkLvdsSingle, sizeof(pll_limits));
				}
			}
		} else {
			memcpy(&limits, &kLimitsIlkDac, sizeof(pll_limits));
		}
	}

	compute_pll_p2(current, divisors, &limits, isLVDS);

	TRACE("PLL limits, min: p %" B_PRId32 " (p1 %" B_PRId32 ", "
		"p2 %" B_PRId32 "), n %" B_PRId32 ", m %" B_PRId32 " "
		"(m1 %" B_PRId32 ", m2 %" B_PRId32 ")\n", limits.min.p,
		limits.min.p1, limits.min.p2, limits.min.n, limits.min.m,
		limits.min.m1, limits.min.m2);
	TRACE("PLL limits, max: p %" B_PRId32 " (p1 %" B_PRId32 ", "
		"p2 %" B_PRId32 "), n %" B_PRId32 ", m %" B_PRId32 " "
		"(m1 %" B_PRId32 ", m2 %" B_PRId32 ")\n", limits.max.p,
		limits.max.p1, limits.max.p2, limits.max.n, limits.max.m,
		limits.max.m1, limits.max.m2);

	float best = requestedPixelClock;
	pll_divisors bestDivisors;

	for (divisors->n = limits.min.n; divisors->n <= limits.max.n;
			divisors->n++) {
		for (divisors->m1 = limits.max.m1; divisors->m1 >= limits.min.m1;
				divisors->m1--) {
			for (divisors->m2 = limits.max.m2; divisors->m2 >= limits.min.m2;
					divisors->m2--) {
				for (divisors->p1 = limits.max.p1;
						divisors->p1 >= limits.min.p1; divisors->p1--) {
					divisors->m = compute_pll_m(divisors);
					divisors->p = compute_pll_p(divisors);

					if (!valid_pll_divisors(divisors, &limits))
						continue;

					float error = fabs(requestedPixelClock
						- (referenceClock * divisors->m)
						/ (divisors->n * divisors->p));
					if (error < best) {
						best = error;
						bestDivisors = *divisors;

						if (error == 0)
							break;
					}
				}
			}
		}
	}
	*divisors = bestDivisors;
	TRACE("%s: best MHz: %g (error: %g)\n", __func__,
		(referenceClock * divisors->m) / (divisors->n * divisors->p),
		best);
}


static void
compute_dpll_9xx(display_timing* current, pll_divisors* divisors, bool isLVDS)
{
	float requestedPixelClock = current->pixel_clock / 1000.0f;
	float referenceClock
		= gInfo->shared_info->pll_info.reference_frequency / 1000.0f;

	TRACE("%s: required MHz: %g\n", __func__, requestedPixelClock);

	pll_limits limits;
	if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_PIN)) {
		if (isLVDS)
			memcpy(&limits, &kLimitsPinLvds, sizeof(pll_limits));
		else
			memcpy(&limits, &kLimitsPinSdvo, sizeof(pll_limits));
	} else if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_85x)) {
		memcpy(&limits, &kLimits85x, sizeof(pll_limits));
	} else {
		if (isLVDS)
			memcpy(&limits, &kLimits9xxLvds, sizeof(pll_limits));
		else
			memcpy(&limits, &kLimits9xxSdvo, sizeof(pll_limits));
	}

	compute_pll_p2(current, divisors, &limits, isLVDS);

	TRACE("PLL limits, min: p %" B_PRId32 " (p1 %" B_PRId32 ", "
		"p2 %" B_PRId32 "), n %" B_PRId32 ", m %" B_PRId32 " "
		"(m1 %" B_PRId32 ", m2 %" B_PRId32 ")\n", limits.min.p,
		limits.min.p1, limits.min.p2, limits.min.n, limits.min.m,
		limits.min.m1, limits.min.m2);
	TRACE("PLL limits, max: p %" B_PRId32 " (p1 %" B_PRId32 ", "
		"p2 %" B_PRId32 "), n %" B_PRId32 ", m %" B_PRId32 " "
		"(m1 %" B_PRId32 ", m2 %" B_PRId32 ")\n", limits.max.p,
		limits.max.p1, limits.max.p2, limits.max.n, limits.max.m,
		limits.max.m1, limits.max.m2);

	bool is_pine = gInfo->shared_info->device_type.InGroup(INTEL_GROUP_PIN);

	float best = requestedPixelClock;
	pll_divisors bestDivisors;
	memset(&bestDivisors, 0, sizeof(bestDivisors));

	for (divisors->m1 = limits.min.m1; divisors->m1 <= limits.max.m1;
			divisors->m1++) {
		for (divisors->m2 = limits.min.m2; divisors->m2 <= limits.max.m2
				&& ((divisors->m2 < divisors->m1) || is_pine); divisors->m2++) {
			for (divisors->n = limits.min.n; divisors->n <= limits.max.n;
					divisors->n++) {
				for (divisors->p1 = limits.min.p1;
						divisors->p1 <= limits.max.p1; divisors->p1++) {
					divisors->m = compute_pll_m(divisors);
					divisors->p = compute_pll_p(divisors);

					if (!valid_pll_divisors(divisors, &limits))
						continue;

					float error = fabs(requestedPixelClock
						- (referenceClock * divisors->m)
						/ (divisors->n * divisors->p));
					if (error < best) {
						best = error;
						bestDivisors = *divisors;

						if (error == 0)
							break;
					}
				}
			}
		}
	}

	*divisors = bestDivisors;

	if (best == requestedPixelClock)
		debugger("No valid PLL configuration found");
	else {
		TRACE("%s: best MHz: %g (error: %g)\n", __func__,
			(referenceClock * divisors->m) / (divisors->n * divisors->p),
			best);
	}
}


void
compute_pll_divisors(display_timing* current, pll_divisors* divisors, bool isLVDS)
{
	if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_G4x)
		|| (gInfo->shared_info->pch_info != INTEL_PCH_NONE)) {
		compute_dpll_g4x(current, divisors, isLVDS);
	} else if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_CHV)) {
		ERROR("%s: TODO: CherryView\n", __func__);
	} else if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_VLV)) {
		ERROR("%s: TODO: VallyView\n", __func__);
	} else
		compute_dpll_9xx(current, divisors, isLVDS);

	TRACE("%s: found: p = %" B_PRId32 " (p1 = %" B_PRId32 ", "
		"p2 = %" B_PRId32 "), n = %" B_PRId32 ", m = %" B_PRId32 " "
		"(m1 = %" B_PRId32 ", m2 = %" B_PRId32 ")\n", __func__,
		divisors->p, divisors->p1, divisors->p2, divisors->n,
		divisors->m, divisors->m1, divisors->m2);
}


void
refclk_activate_ilk(bool hasPanel)
{
	CALLED();

	bool wantsSSC;
	bool hasCK505;
	if (gInfo->shared_info->pch_info == INTEL_PCH_IBX) {
		TRACE("%s: Generation 5 graphics\n", __func__);
		//XXX: This should be == vbt display_clock_mode
		hasCK505 = false;
		wantsSSC = hasCK505;
	} else {
		if (gInfo->shared_info->device_type.Generation() == 6) {
			TRACE("%s: Generation 6 graphics\n", __func__);
		} else {
			TRACE("%s: Generation 7 graphics\n", __func__);
		}
		hasCK505 = false;
		wantsSSC = true;
	}

	uint32 clkRef = read32(PCH_DREF_CONTROL);
	uint32 newRef = clkRef;
	TRACE("%s: PCH_DREF_CONTROL before: 0x%" B_PRIx32 "\n", __func__, clkRef);

	newRef &= ~DREF_NONSPREAD_SOURCE_MASK;

	if (hasCK505)
		newRef |= DREF_NONSPREAD_CK505_ENABLE;
	else
		newRef |= DREF_NONSPREAD_SOURCE_ENABLE;

	newRef &= ~DREF_SSC_SOURCE_MASK;
	newRef &= ~DREF_CPU_SOURCE_OUTPUT_MASK;
	newRef &= ~DREF_SSC1_ENABLE;

	if (newRef == clkRef) {
		TRACE("%s: No changes to reference clock.\n", __func__);
		return;
	}

	if (hasPanel) {
		newRef &= ~DREF_SSC_SOURCE_MASK;
		newRef |= DREF_SSC_SOURCE_ENABLE;

		if (wantsSSC)
			newRef |= DREF_SSC1_ENABLE;
		else
			newRef &= ~DREF_SSC1_ENABLE;

		// Power up SSC before enabling outputs
		write32(PCH_DREF_CONTROL, newRef);
		read32(PCH_DREF_CONTROL);
		TRACE("%s: PCH_DREF_CONTROL after SSC on/off: 0x%" B_PRIx32 "\n",
				__func__, read32(PCH_DREF_CONTROL));
		spin(200);

		newRef &= ~DREF_CPU_SOURCE_OUTPUT_MASK;

		bool hasEDP = true;
		if (hasEDP) {
			if (wantsSSC)
				newRef |= DREF_CPU_SOURCE_OUTPUT_DOWNSPREAD;
			else
				newRef |= DREF_CPU_SOURCE_OUTPUT_NONSPREAD;
		} else
			newRef |= DREF_CPU_SOURCE_OUTPUT_DISABLE;

		write32(PCH_DREF_CONTROL, newRef);
		read32(PCH_DREF_CONTROL);
		TRACE("%s: PCH_DREF_CONTROL after done: 0x%" B_PRIx32 "\n",
				__func__, read32(PCH_DREF_CONTROL));
		spin(200);
	} else {
		newRef &= ~DREF_CPU_SOURCE_OUTPUT_MASK;
		newRef |= DREF_CPU_SOURCE_OUTPUT_DISABLE;

		write32(PCH_DREF_CONTROL, newRef);
		read32(PCH_DREF_CONTROL);
		TRACE("%s: PCH_DREF_CONTROL after disable CPU output: 0x%" B_PRIx32 "\n",
				__func__, read32(PCH_DREF_CONTROL));
		spin(200);

		if (!wantsSSC) {
			newRef &= ~DREF_SSC_SOURCE_MASK;
			newRef |= DREF_SSC_SOURCE_DISABLE;
			newRef &= ~DREF_SSC1_ENABLE;

			write32(PCH_DREF_CONTROL, newRef);
			read32(PCH_DREF_CONTROL);
			TRACE("%s: PCH_DREF_CONTROL after disable SSC: 0x%" B_PRIx32 "\n",
					__func__, read32(PCH_DREF_CONTROL));
			spin(200);
		}
	}
}


//excerpt (plus modifications) from intel_dpll_mgr.c:

/*
 * Copyright © 2006-2016 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#define LC_FREQ 2700
#define LC_FREQ_2K (uint64)(LC_FREQ * 2000)

#define P_MIN 2
#define P_MAX 64
#define P_INC 2

/* Constraints for PLL good behavior */
#define REF_MIN 48
#define REF_MAX 400
#define VCO_MIN 2400
#define VCO_MAX 4800

static uint64 AbsSubtr64(uint64 nr1, uint64 nr2)
{
	if (nr1 >= nr2) {
		return nr1 - nr2;
	} else {
		return nr2 - nr1;
	}
}

struct hsw_wrpll_rnp {
	unsigned p, n2, r2;
};

static unsigned hsw_wrpll_get_budget_for_freq(int clock)
{
	unsigned budget;

	switch (clock) {
	case 25175000:
	case 25200000:
	case 27000000:
	case 27027000:
	case 37762500:
	case 37800000:
	case 40500000:
	case 40541000:
	case 54000000:
	case 54054000:
	case 59341000:
	case 59400000:
	case 72000000:
	case 74176000:
	case 74250000:
	case 81000000:
	case 81081000:
	case 89012000:
	case 89100000:
	case 108000000:
	case 108108000:
	case 111264000:
	case 111375000:
	case 148352000:
	case 148500000:
	case 162000000:
	case 162162000:
	case 222525000:
	case 222750000:
	case 296703000:
	case 297000000:
		budget = 0;
		break;
	case 233500000:
	case 245250000:
	case 247750000:
	case 253250000:
	case 298000000:
		budget = 1500;
		break;
	case 169128000:
	case 169500000:
	case 179500000:
	case 202000000:
		budget = 2000;
		break;
	case 256250000:
	case 262500000:
	case 270000000:
	case 272500000:
	case 273750000:
	case 280750000:
	case 281250000:
	case 286000000:
	case 291750000:
		budget = 4000;
		break;
	case 267250000:
	case 268500000:
		budget = 5000;
		break;
	default:
		budget = 1000;
		break;
	}

	return budget;
}

static void hsw_wrpll_update_rnp(uint64 freq2k, unsigned int budget,
				 unsigned int r2, unsigned int n2,
				 unsigned int p,
				 struct hsw_wrpll_rnp *best)
{
	uint64 a, b, c, d, diff, diff_best;

	/* No best (r,n,p) yet */
	if (best->p == 0) {
		best->p = p;
		best->n2 = n2;
		best->r2 = r2;
		return;
	}

	/*
	 * Output clock is (LC_FREQ_2K / 2000) * N / (P * R), which compares to
	 * freq2k.
	 *
	 * delta = 1e6 *
	 *	   abs(freq2k - (LC_FREQ_2K * n2/(p * r2))) /
	 *	   freq2k;
	 *
	 * and we would like delta <= budget.
	 *
	 * If the discrepancy is above the PPM-based budget, always prefer to
	 * improve upon the previous solution.  However, if you're within the
	 * budget, try to maximize Ref * VCO, that is N / (P * R^2).
	 */
	a = freq2k * budget * p * r2;
	b = freq2k * budget * best->p * best->r2;
	diff = AbsSubtr64((uint64)freq2k * p * r2, LC_FREQ_2K * n2);
	diff_best = AbsSubtr64((uint64)freq2k * best->p * best->r2,
			     LC_FREQ_2K * best->n2);
	c = 1000000 * diff;
	d = 1000000 * diff_best;

	if (a < c && b < d) {
		/* If both are above the budget, pick the closer */
		if (best->p * best->r2 * diff < p * r2 * diff_best) {
			best->p = p;
			best->n2 = n2;
			best->r2 = r2;
		}
	} else if (a >= c && b < d) {
		/* If A is below the threshold but B is above it?  Update. */
		best->p = p;
		best->n2 = n2;
		best->r2 = r2;
	} else if (a >= c && b >= d) {
		/* Both are below the limit, so pick the higher n2/(r2*r2) */
		if (n2 * best->r2 * best->r2 > best->n2 * r2 * r2) {
			best->p = p;
			best->n2 = n2;
			best->r2 = r2;
		}
	}
	/* Otherwise a < c && b >= d, do nothing */
}

void
hsw_ddi_calculate_wrpll(int clock /* in Hz */,
			unsigned *r2_out, unsigned *n2_out, unsigned *p_out)
{
	uint64 freq2k;
	unsigned p, n2, r2;
	struct hsw_wrpll_rnp best = { 0, 0, 0 };
	unsigned budget;

	freq2k = clock / 100;

	budget = hsw_wrpll_get_budget_for_freq(clock);

	/* Special case handling for 540 pixel clock: bypass WR PLL entirely
	 * and directly pass the LC PLL to it. */
	if (freq2k == 5400000) {
		*n2_out = 2;
		*p_out = 1;
		*r2_out = 2;
		return;
	}

	/*
	 * Ref = LC_FREQ / R, where Ref is the actual reference input seen by
	 * the WR PLL.
	 *
	 * We want R so that REF_MIN <= Ref <= REF_MAX.
	 * Injecting R2 = 2 * R gives:
	 *   REF_MAX * r2 > LC_FREQ * 2 and
	 *   REF_MIN * r2 < LC_FREQ * 2
	 *
	 * Which means the desired boundaries for r2 are:
	 *  LC_FREQ * 2 / REF_MAX < r2 < LC_FREQ * 2 / REF_MIN
	 *
	 */
	for (r2 = LC_FREQ * 2 / REF_MAX + 1;
	     r2 <= LC_FREQ * 2 / REF_MIN;
	     r2++) {

		/*
		 * VCO = N * Ref, that is: VCO = N * LC_FREQ / R
		 *
		 * Once again we want VCO_MIN <= VCO <= VCO_MAX.
		 * Injecting R2 = 2 * R and N2 = 2 * N, we get:
		 *   VCO_MAX * r2 > n2 * LC_FREQ and
		 *   VCO_MIN * r2 < n2 * LC_FREQ)
		 *
		 * Which means the desired boundaries for n2 are:
		 * VCO_MIN * r2 / LC_FREQ < n2 < VCO_MAX * r2 / LC_FREQ
		 */
		for (n2 = VCO_MIN * r2 / LC_FREQ + 1;
		     n2 <= VCO_MAX * r2 / LC_FREQ;
		     n2++) {

			for (p = P_MIN; p <= P_MAX; p += P_INC)
				hsw_wrpll_update_rnp(freq2k, budget,
						     r2, n2, p, &best);
		}
	}

	*n2_out = best.n2;
	*p_out = best.p;
	*r2_out = best.r2;
}

struct skl_wrpll_context {
	uint64 min_deviation;		/* current minimal deviation */
	uint64 central_freq;		/* chosen central freq */
	uint64 dco_freq;			/* chosen dco freq */
	unsigned int p;				/* chosen divider */
};

/* DCO freq must be within +1%/-6%  of the DCO central freq */
#define SKL_DCO_MAX_PDEVIATION	100
#define SKL_DCO_MAX_NDEVIATION	600

static void skl_wrpll_try_divider(struct skl_wrpll_context *ctx,
				  uint64 central_freq,
				  uint64 dco_freq,
				  unsigned int divider)
{
	uint64 deviation;

	deviation = ((uint64)10000 * AbsSubtr64(dco_freq, central_freq)
			      / central_freq);

	/* positive deviation */
	if (dco_freq >= central_freq) {
		if (deviation < SKL_DCO_MAX_PDEVIATION &&
		    deviation < ctx->min_deviation) {
			ctx->min_deviation = deviation;
			ctx->central_freq = central_freq;
			ctx->dco_freq = dco_freq;
			ctx->p = divider;

			TRACE("%s: DCO central frequency %" B_PRIu64 "Hz\n", __func__, central_freq);
			TRACE("%s: DCO frequency %" B_PRIu64 "Hz\n", __func__, dco_freq);
			TRACE("%s: positive offset accepted, deviation %" B_PRIu64 "\n",
				__func__, deviation);
		}
	/* negative deviation */
	} else if (deviation < SKL_DCO_MAX_NDEVIATION &&
		   deviation < ctx->min_deviation) {
		ctx->min_deviation = deviation;
		ctx->central_freq = central_freq;
		ctx->dco_freq = dco_freq;
		ctx->p = divider;

		TRACE("%s: DCO central frequency %" B_PRIu64 "Hz\n", __func__, central_freq);
		TRACE("%s: DCO frequency %" B_PRIu64 "Hz\n", __func__, dco_freq);
		TRACE("%s: negative offset accepted, deviation %" B_PRIu64 "\n",
			__func__, deviation);
	}
}

static void skl_wrpll_get_multipliers(unsigned int p,
				      unsigned int *p0 /* out */,
				      unsigned int *p1 /* out */,
				      unsigned int *p2 /* out */)
{
	/* even dividers */
	if (p % 2 == 0) {
		unsigned int half = p / 2;

		if (half == 1 || half == 2 || half == 3 || half == 5) {
			*p0 = 2;
			*p1 = 1;
			*p2 = half;
		} else if (half % 2 == 0) {
			*p0 = 2;
			*p1 = half / 2;
			*p2 = 2;
		} else if (half % 3 == 0) {
			*p0 = 3;
			*p1 = half / 3;
			*p2 = 2;
		} else if (half % 7 == 0) {
			*p0 = 7;
			*p1 = half / 7;
			*p2 = 2;
		}
	} else if (p == 3 || p == 9) {  /* 3, 5, 7, 9, 15, 21, 35 */
		*p0 = 3;
		*p1 = 1;
		*p2 = p / 3;
	} else if (p == 5 || p == 7) {
		*p0 = p;
		*p1 = 1;
		*p2 = 1;
	} else if (p == 15) {
		*p0 = 3;
		*p1 = 1;
		*p2 = 5;
	} else if (p == 21) {
		*p0 = 7;
		*p1 = 1;
		*p2 = 3;
	} else if (p == 35) {
		*p0 = 7;
		*p1 = 1;
		*p2 = 5;
	}
}

static void skl_wrpll_context_init(struct skl_wrpll_context *ctx)
{
	memset(ctx, 0, sizeof(*ctx));
	ctx->min_deviation = UINT64_MAX;
}

static void skl_wrpll_params_populate(struct skl_wrpll_params *params,
				      uint64 afe_clock,
				      int ref_clock,
				      uint64 central_freq,
				      uint32 p0, uint32 p1, uint32 p2)
{
	uint64 dco_freq;

	switch (central_freq) {
	case 9600000000ULL:
		params->central_freq = 0;
		break;
	case 9000000000ULL:
		params->central_freq = 1;
		break;
	case 8400000000ULL:
		params->central_freq = 3;
	}

	switch (p0) {
	case 1:
		params->pdiv = 0;
		break;
	case 2:
		params->pdiv = 1;
		break;
	case 3:
		params->pdiv = 2;
		break;
	case 7:
		params->pdiv = 4;
		break;
	default:
		TRACE("%s: Incorrect PDiv\n", __func__);
	}

	switch (p2) {
	case 5:
		params->kdiv = 0;
		break;
	case 2:
		params->kdiv = 1;
		break;
	case 3:
		params->kdiv = 2;
		break;
	case 1:
		params->kdiv = 3;
		break;
	default:
		TRACE("%s: Incorrect KDiv\n", __func__);
	}

	params->qdiv_ratio = p1;
	params->qdiv_mode = (params->qdiv_ratio == 1) ? 0 : 1;

	dco_freq = p0 * p1 * p2 * afe_clock;
	TRACE("%s: AFE frequency %" B_PRIu64 "Hz\n", __func__, afe_clock);
	TRACE("%s: p0: %" B_PRIu32 ", p1: %" B_PRIu32 ", p2: %" B_PRIu32 "\n",
		__func__, p0,p1,p2);
	TRACE("%s: DCO frequency %" B_PRIu64 "Hz\n", __func__, dco_freq);

	/*
	 * Intermediate values are in Hz.
	 * Divide by MHz to match bsepc
	 */
	params->dco_integer = (uint64)dco_freq / ((uint64)ref_clock * 1000);
	params->dco_fraction = (
			(uint64)dco_freq / ((uint64)ref_clock / 1000) -
			(uint64)params->dco_integer * 1000000) * 0x8000 /
			1000000;

	TRACE("%s: Reference clock: %gMhz\n", __func__, ref_clock / 1000.0f);
	TRACE("%s: DCO integer %" B_PRIu32 "\n", __func__, params->dco_integer);
	TRACE("%s: DCO fraction 0x%" B_PRIx32 "\n", __func__, params->dco_fraction);
}

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

bool
skl_ddi_calculate_wrpll(int clock /* in Hz */,
			int ref_clock,
			struct skl_wrpll_params *wrpll_params)
{
	uint64 afe_clock = (uint64) clock * 5; /* AFE Clock is 5x Pixel clock */
	uint64 dco_central_freq[3] = { 8400000000ULL,
				    9000000000ULL,
				    9600000000ULL };
	static const int even_dividers[] = {  4,  6,  8, 10, 12, 14, 16, 18, 20,
					     24, 28, 30, 32, 36, 40, 42, 44,
					     48, 52, 54, 56, 60, 64, 66, 68,
					     70, 72, 76, 78, 80, 84, 88, 90,
					     92, 96, 98 };
	static const int odd_dividers[] = { 3, 5, 7, 9, 15, 21, 35 };
	static const struct {
		const int *list;
		unsigned int n_dividers;
	} dividers[] = {
		{ even_dividers, ARRAY_SIZE(even_dividers) },
		{ odd_dividers, ARRAY_SIZE(odd_dividers) },
	};
	struct skl_wrpll_context ctx;
	unsigned int dco, d, i;
	unsigned int p0, p1, p2;

	skl_wrpll_context_init(&ctx);

	for (d = 0; d < ARRAY_SIZE(dividers); d++) {
		for (dco = 0; dco < ARRAY_SIZE(dco_central_freq); dco++) {
			for (i = 0; i < dividers[d].n_dividers; i++) {
				unsigned int p = dividers[d].list[i];
				uint64 dco_freq = p * afe_clock;

				skl_wrpll_try_divider(&ctx,
						      dco_central_freq[dco],
						      dco_freq,
						      p);
				/*
				 * Skip the remaining dividers if we're sure to
				 * have found the definitive divider, we can't
				 * improve a 0 deviation.
				 */
				if (ctx.min_deviation == 0)
					goto skip_remaining_dividers;
			}
		}

skip_remaining_dividers:
		/*
		 * If a solution is found with an even divider, prefer
		 * this one.
		 */
		if (d == 0 && ctx.p)
			break;
	}

	if (!ctx.p) {
		TRACE("%s: No valid divider found for %dHz\n", __func__, clock);
		return false;
	}
	TRACE("%s: Full divider (p) found is %d\n", __func__, ctx.p);

	/*
	 * gcc incorrectly analyses that these can be used without being
	 * initialized. To be fair, it's hard to guess.
	 */
	p0 = p1 = p2 = 0;
	skl_wrpll_get_multipliers(ctx.p, &p0, &p1, &p2);
	skl_wrpll_params_populate(wrpll_params, afe_clock, ref_clock,
				  ctx.central_freq, p0, p1, p2);

	return true;
}

