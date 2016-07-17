/*
 * Copyright 2006-2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Axel Dörfler, axeld@pinc-software.de
 *      Alexander von Gluck IV, kallisti5@unixzen.com
 */
#ifndef INTEL_EXTREME_PLL_H
#define INTEL_EXTREME_PLL_H


#include "intel_extreme.h"


struct pll_divisors {
	uint32  p;
	uint32  p1;
	uint32  p2;
	uint32  n;
	uint32  m;
	uint32  m1;
	uint32  m2;
};

struct pll_limits {
	pll_divisors    min;
	pll_divisors    max;
	uint32			dot_limit;
	uint32          min_vco;
	uint32          max_vco;
};


bool valid_pll_divisors(pll_divisors* divisors, pll_limits* limits);
void compute_pll_divisors(display_mode* current, pll_divisors* divisors,
	bool isLVDS);

void refclk_activate_ilk(bool hasPanel);


#endif /* INTEL_EXTREME_PLL_H */
