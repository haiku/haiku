/* w_fmodl.c -- long double version of w_fmod.c.
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
 * wrapper fmodl(x,y)
 */

#include "math.h"
#include "math_private.h"


#ifdef __STDC__
	long double __fmodl(long double x, long double y)/* wrapper fmodl */
#else
	long double __fmodl(x,y)		/* wrapper fmodl */
	long double x,y;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_fmodl(x,y);
#else
	long double z;
	z = __ieee754_fmodl(x,y);
	if(_LIB_VERSION == _IEEE_ ||__isnanl(y)||__isnanl(x)) return z;
	if(y==0.0) {
	        return __kernel_standard(x,y,227); /* fmod(x,0) */
	} else
	    return z;
#endif
}
weak_alias (__fmodl, fmodl)
