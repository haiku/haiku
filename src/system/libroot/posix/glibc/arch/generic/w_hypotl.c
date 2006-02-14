/* w_hypotl.c -- long double version of w_hypot.c.
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
 * wrapper hypotl(x,y)
 */

#include "math.h"
#include "math_private.h"


#ifdef __STDC__
	long double __hypotl(long double x, long double y)/* wrapper hypotl */
#else
	long double __hypotl(x,y)			 /* wrapper hypotl */
	long double x,y;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_hypotl(x,y);
#else
	long double z;
	z = __ieee754_hypotl(x,y);
	if(_LIB_VERSION == _IEEE_) return z;
	if((!__finitel(z))&&__finitel(x)&&__finitel(y))
	    return __kernel_standard(x,y,204); /* hypot overflow */
	else
	    return z;
#endif
}
weak_alias (__hypotl, hypotl)
