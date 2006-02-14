/* w_exp10f.c -- float version of w_exp10.c.
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
 * wrapper expf10(x)
 */

#include "math.h"
#include "math_private.h"

#ifdef __STDC__
	float __exp10f(float x)		/* wrapper exp10f */
#else
	float __exp10f(x)			/* wrapper exp10f */
	float x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_exp10f(x);
#else
	float z;
	z = __ieee754_exp10f(x);
	if(_LIB_VERSION == _IEEE_) return z;
	if(!__finitef(z) && __finitef(x)) {
	    /* exp10f overflow (146) if x > 0, underflow (147) if x < 0.  */
	    return (float)__kernel_standard((double) x, (double) x,
					    146+!!__signbitf(x));
	}
	return z;
#endif
}
weak_alias (__exp10f, exp10f)
strong_alias (__exp10f, __pow10f)
weak_alias (__pow10f, pow10f)
