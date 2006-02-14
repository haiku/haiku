/* w_lgammal_r.c -- long double version of w_lgamma_r.c.
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
 * wrapper long double lgammal_r(long double x, int *signgamp)
 */

#include "math.h"
#include "math_private.h"


#ifdef __STDC__
	long double __lgammal_r(long double x, int *signgamp)
		/* wrapper lgamma_r */
#else
	long double __lgammal_r(x,signgamp)             /* wrapper lgamma_r */
        long double x; int *signgamp;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_lgammal_r(x,signgamp);
#else
        long double y;
        y = __ieee754_lgammal_r(x,signgamp);
        if(_LIB_VERSION == _IEEE_) return y;
        if(!__finitel(y)&&__finitel(x)) {
            if(__floorl(x)==x&&x<=0.0)
                return __kernel_standard(x,x,215); /* lgamma pole */
            else
                return __kernel_standard(x,x,214); /* lgamma overflow */
        } else
            return y;
#endif
}
weak_alias (__lgammal_r, lgammal_r)
