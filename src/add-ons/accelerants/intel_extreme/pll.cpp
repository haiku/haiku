/*
 * Copyright 2006-2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Alexander von Gluck IV, kallisti5@unixzen.com
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


void
get_pll_limits(pll_limits* limits, bool isLVDS)
{
	// Note, the limits are taken from the X driver; they have not yet been
	// tested

	if (gInfo->shared_info->device_type.InFamily(INTEL_FAMILY_SER5)
		|| gInfo->shared_info->device_type.InFamily(INTEL_FAMILY_SOC0)) {
		// TODO: support LVDS output limits as well
		pll_limits kLimits = {
			// p, p1, p2, high,   n,   m, m1, m2
			{  5,  1, 10, false,  1,  79, 12,  5},	// min
			{ 80,  8,  5, true,   5, 127, 22,  9},	// max
			225000, 1760000, 3510000
		};
		memcpy(limits, &kLimits, sizeof(pll_limits));
	} else if (gInfo->shared_info->device_type.InFamily(INTEL_FAMILY_9xx)) {
		// (Update: Output limits are adjusted in the computation (post2=7/14))
		// Should move them here!
		pll_limits kLimits = {
			// p, p1, p2, high,   n,   m, m1, m2
			{  5,  1, 10, false,  1,  70, 8,  3},	// min
			{ 80,  8,  5, true,   6, 120, 18, 7},	// max
			200000, 1400000, 2800000
		};
		if (isLVDS) {
			kLimits.min.post = 7;
			kLimits.max.post = 98;
			kLimits.min.post2 = 14;
			kLimits.max.post2 = 7;
		}
		memcpy(limits, &kLimits, sizeof(pll_limits));
	} else if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_G4x)) {
		// TODO: support LVDS output limits as well
		pll_limits kLimits = {
			// p, p1, p2, high,   n,   m, m1, m2
			{ 10,  1, 10, false,  1, 104, 17,  5},	// min
			{ 30,  3, 10, true,   4, 138, 23, 11},	// max
			270000, 1750000, 3500000
		};
		memcpy(limits, &kLimits, sizeof(pll_limits));
	} else if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_IGD)) {
		// TODO: support LVDS output limits as well
		// m1 is reserved and must be 0
		pll_limits kLimits = {
			// p, p1, p2, high,   n,   m, m1,  m2
			{  5,  1, 10, false,  3,   2,  0,   2},	// min
			{ 80,  8,  5, true,   6, 256,  0, 256},	// max
			200000, 1700000, 3500000
		};
		memcpy(limits, &kLimits, sizeof(pll_limits));
	} else {
		// TODO: support LVDS output limits as well
		static pll_limits kLimits = {
			// p, p1, p2, high,   n,   m, m1, m2
			{  4,  2,  4, false,  5,  96, 20,  8},
			{128, 33,  2, true,  18, 140, 28, 18},
			165000, 930000, 1400000
		};
		memcpy(limits, &kLimits, sizeof(pll_limits));
	}

	TRACE("PLL limits, min: p %" B_PRId32 " (p1 %" B_PRId32 ", "
		"p2 %" B_PRId32 "), n %" B_PRId32 ", m %" B_PRId32 " "
		"(m1 %" B_PRId32 ", m2 %" B_PRId32 ")\n", limits->min.post,
		limits->min.post1, limits->min.post2, limits->min.n, limits->min.m,
		limits->min.m1, limits->min.m2);
	TRACE("PLL limits, max: p %" B_PRId32 " (p1 %" B_PRId32 ", "
		"p2 %" B_PRId32 "), n %" B_PRId32 ", m %" B_PRId32 " "
		"(m1 %" B_PRId32 ", m2 %" B_PRId32 ")\n", limits->max.post,
		limits->max.post1, limits->max.post2, limits->max.n, limits->max.m,
		limits->max.m1, limits->max.m2);
}


bool
valid_pll_divisors(pll_divisors* divisors, pll_limits* limits)
{
	pll_info &info = gInfo->shared_info->pll_info;
	uint32 vco = info.reference_frequency * divisors->m / divisors->n;
	uint32 frequency = vco / divisors->post;

	if (divisors->post < limits->min.post || divisors->post > limits->max.post
		|| divisors->m < limits->min.m || divisors->m > limits->max.m
		|| vco < limits->min_vco || vco > limits->max_vco
		|| frequency < info.min_frequency || frequency > info.max_frequency)
		return false;

	return true;
}


void
compute_pll_divisors(display_mode* current, pll_divisors* divisors,
	bool isLVDS)
{
	float requestedPixelClock = current->timing.pixel_clock / 1000.0f;
	float referenceClock
		= gInfo->shared_info->pll_info.reference_frequency / 1000.0f;
	pll_limits limits;
	get_pll_limits(&limits, isLVDS);

	TRACE("%s: required MHz: %g\n", __func__, requestedPixelClock);

	if (isLVDS) {
		if (requestedPixelClock > 112.999
			|| (read32(INTEL_DIGITAL_LVDS_PORT) & LVDS_CLKB_POWER_MASK)
				== LVDS_CLKB_POWER_UP)
			divisors->post2 = LVDS_POST2_RATE_FAST;
		else
			divisors->post2 = LVDS_POST2_RATE_SLOW;
	} else {
		if (current->timing.pixel_clock < limits.min_post2_frequency) {
			// slow DAC timing
			divisors->post2 = limits.min.post2;
			divisors->post2_high = limits.min.post2_high;
		} else {
			// fast DAC timing
			divisors->post2 = limits.max.post2;
			divisors->post2_high = limits.max.post2_high;
		}
	}

	float best = requestedPixelClock;
	pll_divisors bestDivisors;

	bool is_igd = gInfo->shared_info->device_type.InGroup(INTEL_GROUP_IGD);
	for (divisors->m1 = limits.min.m1; divisors->m1 <= limits.max.m1;
			divisors->m1++) {
		for (divisors->m2 = limits.min.m2; divisors->m2 <= limits.max.m2
				&& ((divisors->m2 < divisors->m1) || is_igd); divisors->m2++) {
			for (divisors->n = limits.min.n; divisors->n <= limits.max.n;
					divisors->n++) {
				for (divisors->post1 = limits.min.post1;
						divisors->post1 <= limits.max.post1; divisors->post1++) {
					divisors->m = 5 * divisors->m1 + divisors->m2;
					divisors->post = divisors->post1 * divisors->post2;

					if (!valid_pll_divisors(divisors, &limits))
						continue;

					float error = fabs(requestedPixelClock
						- ((referenceClock * divisors->m) / divisors->n)
						/ divisors->post);
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

	TRACE("%s: found: %g MHz, p = %" B_PRId32 " (p1 = %" B_PRId32 ", "
		"p2 = %" B_PRId32 "), n = %" B_PRId32 ", m = %" B_PRId32 " "
		"(m1 = %" B_PRId32 ", m2 = %" B_PRId32 ")\n", __func__,
		((referenceClock * divisors->m) / divisors->n) / divisors->post,
		divisors->post, divisors->post1, divisors->post2, divisors->n,
		divisors->m, divisors->m1, divisors->m2);
}
