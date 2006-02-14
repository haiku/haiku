/* w_scalbl.c -- long double version of w_scalb.c.
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
 * wrapper scalbl(long double x, long double fn) is provide for
 * passing various standard test suite. One
 * should use scalbnl() instead.
 */

#include "math.h"
#include "math_private.h"

#include <errno.h>

#ifdef __STDC__
#ifdef _SCALB_INT
	long double __scalbl(long double x, int fn)	/* wrapper scalbl */
#else
	long double __scalbl(long double x, long double fn)/* wrapper scalbl */
#endif
#else
	long double __scalbl(x,fn)			/* wrapper scalbl */
#ifdef _SCALB_INT
	long double x; int fn;
#else
	long double x,fn;
#endif
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_scalbl(x,fn);
#else
	long double z;
	z = __ieee754_scalbl(x,fn);
	if(_LIB_VERSION != _SVID_) return z;
	if(!(__finitel(z)||__isnanl(z))&&__finitel(x)) {
	    return __kernel_standard(x,(double)fn,232); /* scalb overflow */
	}
	if(z==0.0&&z!=x) {
	    return __kernel_standard(x,(double)fn,233); /* scalb underflow */
	}
#ifndef _SCALB_INT
	if(!__finitel(fn)) __set_errno (ERANGE);
#endif
	return z;
#endif
}
weak_alias (__scalbl, scalbl)
