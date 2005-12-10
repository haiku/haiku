/* @(#)e_scalb.c 5.1 93/09/24 */
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
static char rcsid[] = "$NetBSD: e_scalb.c,v 1.6 1995/05/10 20:46:09 jtc Exp $";
#endif

/*
 * __ieee754_scalb(x, fn) is provide for
 * passing various standard test suite. One
 * should use scalbn() instead.
 */

#include <fenv.h>
#include "math.h"
#include "math_private.h"

#ifdef _SCALB_INT
#ifdef __STDC__
	double __ieee754_scalb(double x, int fn)
#else
	double __ieee754_scalb(x,fn)
	double x; int fn;
#endif
#else
#ifdef __STDC__
	double __ieee754_scalb(double x, double fn)
#else
	double __ieee754_scalb(x,fn)
	double x, fn;
#endif
#endif
{
#ifdef _SCALB_INT
	return __scalbn(x,fn);
#else
	if (__isnan(x)||__isnan(fn)) return x*fn;
	if (!__finite(fn)) {
	    if(fn>0.0) return x*fn;
	    else if (x == 0)
	      return x;
	    else if (!__finite (x))
	      {
# ifdef FE_INVALID
		feraiseexcept (FE_INVALID);
# endif
		return __nan ("");
	      }
	    else       return x/(-fn);
	}
	if (__rint(fn)!=fn)
	  {
# ifdef FE_INVALID
	    feraiseexcept (FE_INVALID);
# endif
	    return __nan ("");
	  }
	if ( fn > 65000.0) return __scalbn(x, 65000);
	if (-fn > 65000.0) return __scalbn(x,-65000);
	return __scalbn(x,(int)fn);
#endif
}
