/*
 * Copyright 2002-2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 * 		Daniel Reinhold, danielre@users.sf.net
 */


#include <stdlib.h>


div_t
div(int numerator, int denominator)
{
	div_t val;

	val.quot = numerator / denominator;
	val.rem  = numerator - denominator * val.quot;

	if (val.rem > 0 && val.quot < 0) {
		val.rem -= denominator;
		++val.quot;
	}

	return val;
}


ldiv_t
ldiv(long numerator, long denominator)
{
	ldiv_t val;

	val.quot = numerator / denominator;
	val.rem  = numerator - denominator * val.quot;

	if (val.rem > 0 && val.quot < 0) {
		val.rem -= denominator;
		++val.quot;
	}

	return val;
}


lldiv_t
lldiv(long long numerator, long long denominator)
{
	lldiv_t val;

	val.quot = numerator / denominator;
	val.rem  = numerator - denominator * val.quot;

	if (val.rem > 0 && val.quot < 0) {
		val.rem -= denominator;
		++val.quot;
	}

	return val;
}
