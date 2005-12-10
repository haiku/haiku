

/* @(#)w_pow.c 5.2 93/10/01 */
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
 * wrapper pow(x,y) return x**y
 */

#include "math.h"
#include "math_private.h"


#ifdef __STDC__
	double __pow(double x, double y)	/* wrapper pow */
#else
	double __pow(x,y)			/* wrapper pow */
	double x,y;
#endif
{
#ifdef _IEEE_LIBM
	return  __ieee754_pow(x,y);
#else
	double z;
	z=__ieee754_pow(x,y);
	if(_LIB_VERSION == _IEEE_|| __isnan(y)) return z;
	if(__isnan(x)) {
	    if(y==0.0)
	        return __kernel_standard(x,y,42); /* pow(NaN,0.0) */
	    else
		return z;
	}
	if(x==0.0) {
	    if(y==0.0)
	        return __kernel_standard(x,y,20); /* pow(0.0,0.0) */
	    if(__finite(y)&&y<0.0) {
	      if (signbit (x) && signbit (z))
	        return __kernel_standard(x,y,23); /* pow(-0.0,negative) */
	      else
	        return __kernel_standard(x,y,43); /* pow(+0.0,negative) */
	    }
	    return z;
	}
	if(!__finite(z)) {
	    if(__finite(x)&&__finite(y)) {
	        if(__isnan(z))
	            return __kernel_standard(x,y,24); /* pow neg**non-int */
	        else
	            return __kernel_standard(x,y,21); /* pow overflow */
	    }
	}
	if(z==0.0&&__finite(x)&&__finite(y))
	    return __kernel_standard(x,y,22); /* pow underflow */
	return z;
#endif
}
weak_alias (__pow, pow)
#ifdef NO_LONG_DOUBLE
strong_alias (__pow, __powl)
weak_alias (__pow, powl)
#endif
