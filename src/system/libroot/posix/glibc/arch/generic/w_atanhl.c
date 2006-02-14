/* w_atanhl.c -- long double version of w_atanh.c.
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
 * wrapper atanhl(x)
 */

#include "math.h"
#include "math_private.h"


#ifdef __STDC__
	long double __atanhl(long double x)	/* wrapper atanhl */
#else
	long double __atanhl(x)			/* wrapper atanhl */
	long double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_atanhl(x);
#else
	long double z,y;
	z = __ieee754_atanhl(x);
	if(_LIB_VERSION == _IEEE_ || __isnanl(x)) return z;
	y = fabsl(x);
	if(y>=1.0) {
	    if(y>1.0)
	        return __kernel_standard(x,x,230); /* atanhl(|x|>1) */
	    else
	        return __kernel_standard(x,x,231); /* atanhl(|x|==1) */
	} else
	    return z;
#endif
}
weak_alias (__atanhl, atanhl)
