/* w_exp10l.c -- long double version of w_exp10.c.
 * Conversion to long double by Ulrich Drepper,
 * Cygnus Support, drepper@cygnus.com.
 */

/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

/*
 * wrapper exp10l(x)
 */

#include "math.h"
#include "math_private.h"

#ifdef __STDC__
	long double __exp10l(long double x)	/* wrapper exp10 */
#else
	long double __exp10l(x)			/* wrapper exp10 */
	long double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_exp10l(x);
#else
	long double z;
	z = __ieee754_exp10l(x);
	if(_LIB_VERSION == _IEEE_) return z;
	if(!__finitel(z) && __finitel(x)) {
	    /* exp10 overflow (246) if x > 0, underflow (247) if x < 0.  */
	    return __kernel_standard(x,x,246+__signbitl(x));
	}
	return z;
#endif
}
weak_alias (__exp10l, exp10l)
strong_alias (__exp10l, __pow10l)
weak_alias (__pow10l, pow10l)
