/* w_powl.c -- long double version of w_pow.c.
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

/*
 * wrapper powl(x,y) return x**y
 */

#include "math.h"
#include "math_private.h"


#ifdef __STDC__
	long double __powl(long double x, long double y)/* wrapper powl */
#else
	long double __powl(x,y)			/* wrapper powl */
	long double x,y;
#endif
{
#ifdef _IEEE_LIBM
	return  __ieee754_powl(x,y);
#else
	long double z;
	z=__ieee754_powl(x,y);
	if(_LIB_VERSION == _IEEE_|| __isnanl(y)) return z;
	if(__isnanl(x)) {
	    if(y==0.0)
	        return __kernel_standard(x,y,242); /* pow(NaN,0.0) */
	    else
		return z;
	}
	if(x==0.0) {
	    if(y==0.0)
	        return __kernel_standard(x,y,220); /* pow(0.0,0.0) */
	    if(__finitel(y)&&y<0.0) {
	      if (signbit (x) && signbit (z))
	        return __kernel_standard(x,y,223); /* pow(-0.0,negative) */
	      else
	        return __kernel_standard(x,y,243); /* pow(+0.0,negative) */
	    }
	    return z;
	}
	if(!__finitel(z)) {
	    if(__finitel(x)&&__finitel(y)) {
	        if(__isnanl(z))
	            return __kernel_standard(x,y,224); /* pow neg**non-int */
	        else
	            return __kernel_standard(x,y,221); /* pow overflow */
	    }
	}
	if(z==0.0&&__finitel(x)&&__finitel(y))
	    return __kernel_standard(x,y,222); /* pow underflow */
	return z;
#endif
}
weak_alias (__powl, powl)
