/* w_logl.c -- long double version of w_log.c.
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
 * wrapper logl(x)
 */

#include "math.h"
#include "math_private.h"


#ifdef __STDC__
	long double __logl(long double x)	/* wrapper logl */
#else
	long double __logl(x)			/* wrapper logl */
	long double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_logl(x);
#else
	long double z;
	z = __ieee754_logl(x);
	if(_LIB_VERSION == _IEEE_ || __isnanl(x) || x > 0.0) return z;
	if(x==0.0)
	    return __kernel_standard(x,x,216); /* log(0) */
	else
	    return __kernel_standard(x,x,217); /* log(x<0) */
#endif
}
weak_alias (__logl, logl)
