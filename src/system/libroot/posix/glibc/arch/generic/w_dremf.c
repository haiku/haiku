/*
 * dremf() wrapper for remainderf().
 * 
 * Written by J.T. Conklin, <jtc@wimsey.com>
 * Placed into the Public Domain, 1994.
 */

#include "math.h"
#include "math_private.h"

float
__dremf(x, y)
	float x, y;
{
	return __remainderf(x, y);
}
weak_alias (__dremf, dremf)
