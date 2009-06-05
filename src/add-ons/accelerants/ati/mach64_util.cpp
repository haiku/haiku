/*
	Haiku ATI video driver adapted from the X.org ATI driver.

	Copyright 1997 through 2004 by Marc Aurele La France (TSI @ UQV), tsi@xfree86.org

	Copyright 2009 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac 2009
*/

#include "accelerant.h"
#include "mach64.h"


#define MAX_INT	((int)((unsigned int)(-1) >> 2))



void
Mach64_ReduceRatio(int *numerator, int *denominator)
{
	// Reduce a fraction by factoring out the largest common divider of the
	// fraction's numerator and denominator.

	int multiplier = *numerator;
	int divider = *denominator;
	int remainder;

	while ((remainder = multiplier % divider)) {
		multiplier = divider;
		divider = remainder;
	}

	*numerator /= divider;
	*denominator /= divider;
}



int
Mach64_Divide(int numerator, int denominator, int shift, const int roundingKind)
{
	// Using integer arithmetic and avoiding overflows, this function finds the
	// rounded integer that best approximates
	//
	//		numerator      shift
	//		----------- * 2
	//		denominator
	//
	// using the specified rounding (floor (<0), nearest (=0) or ceiling (>0)).

	Mach64_ReduceRatio(&numerator, &denominator);

	// Deal with left shifts but try to keep the denominator even.
	if (denominator & 1) {
		if (denominator <= MAX_INT) {
			denominator <<= 1;
			shift++;
		}
	} else {
		while ((shift > 0) && !(denominator & 3)) {
			denominator >>= 1;
			shift--;
		}
	}

	// Deal with right shifts.
	while (shift < 0) {
		if ((numerator & 1) && (denominator <= MAX_INT))
			denominator <<= 1;
		else
			numerator >>= 1;

		shift++;
	}

	int rounding = 0;					// Default to floor

	if (!roundingKind)					// nearest
		rounding = denominator >> 1;
	else if (roundingKind > 0)			// ceiling
		rounding = denominator - 1;

	return ((numerator / denominator) << shift) +
		   ((((numerator % denominator) << shift) + rounding) / denominator);
}
