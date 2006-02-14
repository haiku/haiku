/* w_acosl.c -- long double version of w_acos.c.
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
 * wrap_acosl(x)
 */

#include "math.h"
#include "math_private.h"


#ifdef __STDC__
	long double __acosl(long double x)	/* wrapper acos */
#else
	long double __acosl(x)			/* wrapper acos */
	long double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_acosl(x);
#else
	long double z;
	z = __ieee754_acosl(x);
	if(_LIB_VERSION == _IEEE_ || __isnanl(x)) return z;
	if(fabsl(x)>1.0) {
	        return __kernel_standard(x,x,201); /* acosl(|x|>1) */
	} else
	    return z;
#endif
}
weak_alias (__acosl, acosl)
