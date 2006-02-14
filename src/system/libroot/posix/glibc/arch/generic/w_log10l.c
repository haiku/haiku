/* w_log10l.c -- long double version of w_log10.c.
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
 * wrapper log10l(X)
 */

#include "math.h"
#include "math_private.h"


#ifdef __STDC__
	long double __log10l(long double x)	/* wrapper log10l */
#else
	long double __log10l(x)			/* wrapper log10l */
	long double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_log10l(x);
#else
	long double z;
	z = __ieee754_log10l(x);
	if(_LIB_VERSION == _IEEE_ || __isnanl(x)) return z;
	if(x<=0.0) {
	    if(x==0.0)
	        return __kernel_standard(x,x,218); /* log10(0) */
	    else
	        return __kernel_standard(x,x,219); /* log10(x<0) */
	} else
	    return z;
#endif
}
weak_alias (__log10l, log10l)
