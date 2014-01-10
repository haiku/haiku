/*
 * dremf() wrapper for remainderf().
 * 
 * Written by J.T. Conklin, <jtc@wimsey.com>
 * Placed into the Public Domain, 1994.
 */

#include "math.h"
#include "math_private.h"

float
__dremf(float x, float y)
{
	return __remainderf(x, y);
}

float dremf(float, float) __attribute__((weak, alias("__dremf")));
