/*
 * drem() wrapper for remainder().
 *
 * Written by J.T. Conklin, <jtc@wimsey.com>
 * Placed into the Public Domain, 1994.
 */

#include <math.h>

double
__drem(x, y)
	double x, y;
{
	return __remainder(x, y);
}
weak_alias (__drem, drem)
#ifdef NO_LONG_DOUBLE
strong_alias (__drem, __dreml)
weak_alias (__drem, dreml)
#endif
