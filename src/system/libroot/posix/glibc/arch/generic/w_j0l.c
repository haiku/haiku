/* w_j0l.c -- long double version of w_j0.c.
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
 * wrapper j0l(long double x), y0l(long double x)
 */

#include "math.h"
#include "math_private.h"

#ifdef __STDC__
	long double j0l(long double x)		/* wrapper j0l */
#else
	long double j0l(x)				/* wrapper j0 */
	long double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_j0l(x);
#else
	long double z = __ieee754_j0l(x);
	if(_LIB_VERSION == _IEEE_ || __isnanl(x)) return z;
	if(fabsl(x)>X_TLOSS) {
	        return __kernel_standard(x,x,234); /* j0(|x|>X_TLOSS) */
	} else
	    return z;
#endif
}

#ifdef __STDC__
	long double y0l(long double x)		/* wrapper y0l */
#else
	long double y0l(x)				/* wrapper y0 */
	long double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_y0l(x);
#else
	long double z;
	z = __ieee754_y0l(x);
	if(_LIB_VERSION == _IEEE_ || __isnanl(x) ) return z;
        if(x <= 0.0){
                if(x==0.0)
                    /* d= -one/(x-x); */
                    return __kernel_standard(x,x,208);
                else
                    /* d = zero/(x-x); */
                    return __kernel_standard(x,x,209);
        }
	if(x>X_TLOSS) {
	        return __kernel_standard(x,x,235); /* y0(x>X_TLOSS) */
	} else
	    return z;
#endif
}
