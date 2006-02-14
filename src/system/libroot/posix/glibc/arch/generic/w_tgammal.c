/* w_gammal.c -- long double version of w_gamma.c.
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

/* long double gammal(double x)
 * Return the Gamma function of x.
 */

#include "math.h"
#include "math_private.h"

#ifdef __STDC__
	long double __tgammal(long double x)
#else
	long double __tgammal(x)
	long double x;
#endif
{
        long double y;
	int local_signgam;
	y = __ieee754_gammal_r(x,&local_signgam);
	if (local_signgam < 0) y = -y;
#ifdef _IEEE_LIBM
	return y;
#else
	if(_LIB_VERSION == _IEEE_) return y;

	if(!__finitel(y)&&__finitel(x)) {
	  if(x==0.0)
	    return __kernel_standard(x,x,250); /* tgamma pole */
	  else if(__floorl(x)==x&&x<0.0)
	    return __kernel_standard(x,x,241); /* tgamma domain */
	  else
	    return __kernel_standard(x,x,240); /* tgamma overflow */
	}
	return y;
#endif
}
weak_alias (__tgammal, tgammal)
