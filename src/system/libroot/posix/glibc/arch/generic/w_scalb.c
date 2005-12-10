/* @(#)w_scalb.c 5.1 93/09/24 */
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
static char rcsid[] = "$NetBSD: w_scalb.c,v 1.6 1995/05/10 20:49:48 jtc Exp $";
#endif

/*
 * wrapper scalb(double x, double fn) is provide for
 * passing various standard test suite. One
 * should use scalbn() instead.
 */

#include "math.h"
#include "math_private.h"

#include <errno.h>

#ifdef __STDC__
#ifdef _SCALB_INT
	double __scalb(double x, int fn)		/* wrapper scalb */
#else
	double __scalb(double x, double fn)	/* wrapper scalb */
#endif
#else
	double __scalb(x,fn)			/* wrapper scalb */
#ifdef _SCALB_INT
	double x; int fn;
#else
	double x,fn;
#endif
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_scalb(x,fn);
#else
	double z;
	z = __ieee754_scalb(x,fn);
	if(_LIB_VERSION != _SVID_) return z;
	if(!(__finite(z)||__isnan(z))&&__finite(x)) {
	    return __kernel_standard(x,(double)fn,32); /* scalb overflow */
	}
	if(z==0.0&&z!=x) {
	    return __kernel_standard(x,(double)fn,33); /* scalb underflow */
	}
#ifndef _SCALB_INT
	if(!__finite(fn)) __set_errno (ERANGE);
#endif
	return z;
#endif
}
weak_alias (__scalb, scalb)
#ifdef NO_LONG_DOUBLE
strong_alias (__scalb, __scalbl)
weak_alias (__scalb, scalbl)
#endif
