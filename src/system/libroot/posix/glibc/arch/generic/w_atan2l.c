/* w_atan2l.c -- long double version of w_atan2.c.
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
 * wrapper atan2l(y,x)
 */

#include "math.h"
#include "math_private.h"


#ifdef __STDC__
	long double __atan2l(long double y, long double x) /* wrapper atan2l */
#else
	long double __atan2l(y,x)			/* wrapper atan2l */
	long double y,x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_atan2l(y,x);
#else
	long double z;
	z = __ieee754_atan2l(y,x);
	if(_LIB_VERSION != _SVID_||__isnanl(x)||__isnanl(y)) return z;
	if(x==0.0&&y==0.0)
	  return __kernel_standard(y,x,203); /* atan2(+-0,+-0) */
	return z;
#endif
}
weak_alias (__atan2l, atan2l)
