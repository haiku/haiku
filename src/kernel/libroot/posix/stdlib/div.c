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


// ToDo: these are supposed to be declared in <stdlib.h>
// - - - - - - - - - - - - - - -
#include <div_t.h>
div_t	div(int, int);
ldiv_t	ldiv(long, long);
// - - - - - - - - - - - - - - -


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

