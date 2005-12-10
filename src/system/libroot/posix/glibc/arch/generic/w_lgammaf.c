/* w_lgammaf.c -- float version of w_lgamma.c.
 * Conversion to float by Ian Lance Taylor, Cygnus Support, ian@cygnus.com.
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
static char rcsid[] = "$NetBSD: w_lgammaf.c,v 1.3 1995/05/10 20:49:30 jtc Exp $";
#endif

#include "math.h"
#include "math_private.h"

#ifdef __STDC__
	float __lgammaf(float x)
#else
	float __lgammaf(x)
	float x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_lgammaf_r(x,&signgam);
#else
        float y;
	int local_signgam = 0;
        y = __ieee754_lgammaf_r(x,&local_signgam);
	if (_LIB_VERSION != _ISOC_)
	  /* ISO C99 does not define the global variable.  */
	  signgam = local_signgam;
        if(_LIB_VERSION == _IEEE_) return y;
        if(!__finitef(y)&&__finitef(x)) {
            if(__floorf(x)==x&&x<=(float)0.0)
	        /* lgamma pole */
                return (float)__kernel_standard((double)x,(double)x,115);
            else
	        /* lgamma overflow */
                return (float)__kernel_standard((double)x,(double)x,114);
        } else
            return y;
#endif
}
weak_alias (__lgammaf, lgammaf)
strong_alias (__lgammaf, __gammaf)
weak_alias (__gammaf, gammaf)
