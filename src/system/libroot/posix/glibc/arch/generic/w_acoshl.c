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
 * wrapper acoshl(x)
 */

#include "math.h"
#include "math_private.h"

#ifdef __STDC__
	long double __acoshl(long double x)	/* wrapper acosh */
#else
	long double __acoshl(x)			/* wrapper acosh */
	long double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_acoshl(x);
#else
	long double z;
	z = __ieee754_acoshl(x);
	if(_LIB_VERSION == _IEEE_ || __isnanl(x)) return z;
	if(x<1.0) {
	        return __kernel_standard(x,x,229); /* acoshl(x<1) */
	} else
	    return z;
#endif
}
weak_alias (__acoshl, acoshl)
