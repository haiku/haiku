/* w_sinhl.c -- long double version of w_sinh.c.
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
 * wrapper sinhl(x)
 */

#include "math.h"
#include "math_private.h"

#ifdef __STDC__
	long double __sinhl(long double x)	/* wrapper sinhl */
#else
	long double __sinhl(x)			/* wrapper sinhl */
	long double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_sinhl(x);
#else
	long double z;
	z = __ieee754_sinhl(x);
	if(_LIB_VERSION == _IEEE_) return z;
	if(!__finitel(z)&&__finitel(x)) {
	    return __kernel_standard(x,x,225); /* sinh overflow */
	} else
	    return z;
#endif
}
weak_alias (__sinhl, sinhl)
