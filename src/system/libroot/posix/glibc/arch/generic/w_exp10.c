/* @(#)w_exp10.c
 * Conversion to exp10 by Ulrich Drepper <drepper@cygnus.com>.
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
 * wrapper exp10(x)
 */

#include "math.h"
#include "math_private.h"

#ifdef __STDC__
	double __exp10(double x)		/* wrapper exp10 */
#else
	double __exp10(x)			/* wrapper exp10 */
	double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_exp10(x);
#else
	double z;
	z = __ieee754_exp10(x);
	if(_LIB_VERSION == _IEEE_) return z;
	if(!__finite(z) && __finite(x)) {
	    /* exp10 overflow (46) if x > 0, underflow (47) if x < 0.  */
	    return __kernel_standard(x,x,46+!!__signbit(x));
	}
	return z;
#endif
}
weak_alias (__exp10, exp10)
strong_alias (__exp10, __pow10)
weak_alias (__pow10, pow10)
#ifdef NO_LONG_DOUBLE
strong_alias (__exp10, __exp10l)
weak_alias (__exp10, exp10l)
strong_alias (__exp10l, __pow10l)
weak_alias (__pow10l, pow10l)
#endif
