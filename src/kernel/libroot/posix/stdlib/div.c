/*
 *  Copyright (c) 2002, OpenBeOS Project.
 *  All rights reserved.
 *  Distributed under the terms of the OpenBeOS license.
 *
 *
 *  div.c:
 *  implements the standard C library functions:
 *    div, ldiv
 *
 *
 *  Author(s):
 *  Daniel Reinhold (danielre@users.sf.net)
 *
 */

#include <stdlib.h>


div_t
div(int numerator, int denominator)
{
	div_t val;
	
	val.quot = numerator / denominator;
	val.rem  = numerator - denominator * val.quot;
	
	if ((val.rem > 0) && (val.quot < 0)) {
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
	
	if ((val.rem > 0) && (val.quot < 0)) {
		val.rem -= denominator;
		++val.quot;
	}

	return val;
}

