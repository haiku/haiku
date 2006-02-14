/* w_sqrtl.c -- long double version of w_sqrt.c.
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
 * wrapper sqrtl(x)
 */

#include "math.h"
#include "math_private.h"

#ifdef __STDC__
	long double __sqrtl(long double x)	/* wrapper sqrtl */
#else
	long double __sqrtl(x)			/* wrapper sqrtl */
	long double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_sqrtl(x);
#else
	long double z;
	z = __ieee754_sqrtl(x);
	if(_LIB_VERSION == _IEEE_ || __isnanl(x)) return z;
	if(x<0.0) {
	    return __kernel_standard(x,x,226); /* sqrt(negative) */
	} else
	    return z;
#endif
}
weak_alias (__sqrtl, sqrtl)
