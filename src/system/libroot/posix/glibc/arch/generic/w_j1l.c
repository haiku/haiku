/* w_j1l.c -- long double version of w_j1.c.
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
 * wrapper of j1l,y1l
 */

#include "math.h"
#include "math_private.h"

#ifdef __STDC__
	long double j1l(long double x)	/* wrapper j1l */
#else
	long double j1l(x)			/* wrapper j1l */
	long double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_j1l(x);
#else
	long double z;
	z = __ieee754_j1l(x);
	if(_LIB_VERSION == _IEEE_ || __isnanl(x) ) return z;
	if(fabsl(x)>X_TLOSS) {
	        return __kernel_standard(x,x,236); /* j1(|x|>X_TLOSS) */
	} else
	    return z;
#endif
}

#ifdef __STDC__
	long double y1l(long double x)	/* wrapper y1l */
#else
	long double y1l(x)			/* wrapper y1l */
	long double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_y1l(x);
#else
	long double z;
	z = __ieee754_y1l(x);
	if(_LIB_VERSION == _IEEE_ || __isnanl(x) ) return z;
        if(x <= 0.0){
                if(x==0.0)
                    /* d= -one/(x-x); */
                    return __kernel_standard(x,x,210);
                else
                    /* d = zero/(x-x); */
                    return __kernel_standard(x,x,211);
        }
	if(x>X_TLOSS) {
	        return __kernel_standard(x,x,237); /* y1(x>X_TLOSS) */
	} else
	    return z;
#endif
}
