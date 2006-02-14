/* w_gammaf.c -- float version of w_gamma.c.
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
static char rcsid[] = "$NetBSD: w_gammaf.c,v 1.4 1995/11/20 22:06:48 jtc Exp $";
#endif

#include "math.h"
#include "math_private.h"

#ifdef __STDC__
	float __tgammaf(float x)
#else
	float __tgammaf(x)
	float x;
#endif
{
        float y;
	int local_signgam;
	y = __ieee754_gammaf_r(x,&local_signgam);
	if (local_signgam < 0) y = -y;
#ifdef _IEEE_LIBM
	return y;
#else
	if(_LIB_VERSION == _IEEE_) return y;

	if(!__finitef(y)&&__finitef(x)) {
	  if (x == (float)0.0)
	    /* tgammaf pole */
	    return (float)__kernel_standard((double)x,(double)x,150);
	  else if(__floorf(x)==x&&x<(float)0.0)
	    /* tgammaf domain */
	    return (float)__kernel_standard((double)x,(double)x,141);
	  else
	    /* tgammaf overflow */
	    return (float)__kernel_standard((double)x,(double)x,140);
	}
	return y;
#endif
}
weak_alias (__tgammaf, tgammaf)
