/*
 * dreml() wrapper for remainderl().
 *
 * Written by J.T. Conklin, <jtc@wimsey.com>
 * Conversion to long double by Ulrich Drepper,
 * Cygnus Support, drepper@cygnus.com.
 * Placed into the Public Domain, 1994.
 */

#include <math.h>

long double
__dreml(x, y)
	long double x, y;
{
	return __remainderl(x, y);
}
weak_alias (__dreml, dreml)
