/* w_acoshl.c -- long double version of w_acosh.c.
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

#if defined(LIBM_SCCS) && !defined(lint)
static char rcsid[] = "$NetBSD: $";
#endif

/*
 * wrapper coshl(x)
 */

#include "math.h"
#include "math_private.h"

#ifdef __STDC__
	long double __coshl(long double x)	/* wrapper coshl */
#else
	long double __coshl(x)			/* wrapper coshl */
	long double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_coshl(x);
#else
	long double z;
	z = __ieee754_coshl(x);
	if(_LIB_VERSION == _IEEE_ || __isnanl(x)) return z;
	if(!__finite(z) && __finite(x)) {
	        return __kernel_standard(x,x,205); /* cosh overflow */
	} else
	    return z;
#endif
}
weak_alias (__coshl, coshl)
