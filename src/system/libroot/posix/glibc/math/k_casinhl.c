/* Return arc hyperbole sine for long double value, with the imaginary
   part of the result possibly adjusted for use in computing other
   functions.
   Copyright (C) 1997-2015 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#include <complex.h>
#include <math.h>
#include <math_private.h>
#include <float.h>

/* To avoid spurious overflows, use this definition to treat IBM long
   double as approximating an IEEE-style format.  */
#if LDBL_MANT_DIG == 106
# undef LDBL_EPSILON
# define LDBL_EPSILON 0x1p-106L
#endif

/* Return the complex inverse hyperbolic sine of finite nonzero Z,
   with the imaginary part of the result subtracted from pi/2 if ADJ
   is nonzero.  */

__complex__ long double
__kernel_casinhl (__complex__ long double x, int adj)
{
  __complex__ long double res;
  long double rx, ix;
  __complex__ long double y;

  /* Avoid cancellation by reducing to the first quadrant.  */
  rx = fabsl (__real__ x);
  ix = fabsl (__imag__ x);

  if (rx >= 1.0L / LDBL_EPSILON || ix >= 1.0L / LDBL_EPSILON)
    {
      /* For large x in the first quadrant, x + csqrt (1 + x * x)
	 is sufficiently close to 2 * x to make no significant
	 difference to the result; avoid possible overflow from
	 the squaring and addition.  */
      __real__ y = rx;
      __imag__ y = ix;

      if (adj)
	{
	  long double t = __real__ y;
	  __real__ y = __copysignl (__imag__ y, __imag__ x);
	  __imag__ y = t;
	}

      res = __clogl (y);
      __real__ res += M_LN2l;
    }
  else if (rx >= 0.5L && ix < LDBL_EPSILON / 8.0L)
    {
      long double s = __ieee754_hypotl (1.0L, rx);

      __real__ res = __ieee754_logl (rx + s);
      if (adj)
	__imag__ res = __ieee754_atan2l (s, __imag__ x);
      else
	__imag__ res = __ieee754_atan2l (ix, s);
    }
  else if (rx < LDBL_EPSILON / 8.0L && ix >= 1.5L)
    {
      long double s = __ieee754_sqrtl ((ix + 1.0L) * (ix - 1.0L));

      __real__ res = __ieee754_logl (ix + s);
      if (adj)
	__imag__ res = __ieee754_atan2l (rx, __copysignl (s, __imag__ x));
      else
	__imag__ res = __ieee754_atan2l (s, rx);
    }
  else if (ix > 1.0L && ix < 1.5L && rx < 0.5L)
    {
      if (rx < LDBL_EPSILON * LDBL_EPSILON)
	{
	  long double ix2m1 = (ix + 1.0L) * (ix - 1.0L);
	  long double s = __ieee754_sqrtl (ix2m1);

	  __real__ res = __log1pl (2.0L * (ix2m1 + ix * s)) / 2.0L;
	  if (adj)
	    __imag__ res = __ieee754_atan2l (rx, __copysignl (s, __imag__ x));
	  else
	    __imag__ res = __ieee754_atan2l (s, rx);
	}
      else
	{
	  long double ix2m1 = (ix + 1.0L) * (ix - 1.0L);
	  long double rx2 = rx * rx;
	  long double f = rx2 * (2.0L + rx2 + 2.0L * ix * ix);
	  long double d = __ieee754_sqrtl (ix2m1 * ix2m1 + f);
	  long double dp = d + ix2m1;
	  long double dm = f / dp;
	  long double r1 = __ieee754_sqrtl ((dm + rx2) / 2.0L);
	  long double r2 = rx * ix / r1;

	  __real__ res
	    = __log1pl (rx2 + dp + 2.0L * (rx * r1 + ix * r2)) / 2.0L;
	  if (adj)
	    __imag__ res = __ieee754_atan2l (rx + r1, __copysignl (ix + r2,
								   __imag__ x));
	  else
	    __imag__ res = __ieee754_atan2l (ix + r2, rx + r1);
	}
    }
  else if (ix == 1.0L && rx < 0.5L)
    {
      if (rx < LDBL_EPSILON / 8.0L)
	{
	  __real__ res = __log1pl (2.0L * (rx + __ieee754_sqrtl (rx))) / 2.0L;
	  if (adj)
	    __imag__ res = __ieee754_atan2l (__ieee754_sqrtl (rx),
					     __copysignl (1.0L, __imag__ x));
	  else
	    __imag__ res = __ieee754_atan2l (1.0L, __ieee754_sqrtl (rx));
	}
      else
	{
	  long double d = rx * __ieee754_sqrtl (4.0L + rx * rx);
	  long double s1 = __ieee754_sqrtl ((d + rx * rx) / 2.0L);
	  long double s2 = __ieee754_sqrtl ((d - rx * rx) / 2.0L);

	  __real__ res = __log1pl (rx * rx + d + 2.0L * (rx * s1 + s2)) / 2.0L;
	  if (adj)
	    __imag__ res = __ieee754_atan2l (rx + s1,
					     __copysignl (1.0L + s2,
							  __imag__ x));
	  else
	    __imag__ res = __ieee754_atan2l (1.0L + s2, rx + s1);
	}
    }
  else if (ix < 1.0L && rx < 0.5L)
    {
      if (ix >= LDBL_EPSILON)
	{
	  if (rx < LDBL_EPSILON * LDBL_EPSILON)
	    {
	      long double onemix2 = (1.0L + ix) * (1.0L - ix);
	      long double s = __ieee754_sqrtl (onemix2);

	      __real__ res = __log1pl (2.0L * rx / s) / 2.0L;
	      if (adj)
		__imag__ res = __ieee754_atan2l (s, __imag__ x);
	      else
		__imag__ res = __ieee754_atan2l (ix, s);
	    }
	  else
	    {
	      long double onemix2 = (1.0L + ix) * (1.0L - ix);
	      long double rx2 = rx * rx;
	      long double f = rx2 * (2.0L + rx2 + 2.0L * ix * ix);
	      long double d = __ieee754_sqrtl (onemix2 * onemix2 + f);
	      long double dp = d + onemix2;
	      long double dm = f / dp;
	      long double r1 = __ieee754_sqrtl ((dp + rx2) / 2.0L);
	      long double r2 = rx * ix / r1;

	      __real__ res
		= __log1pl (rx2 + dm + 2.0L * (rx * r1 + ix * r2)) / 2.0L;
	      if (adj)
		__imag__ res = __ieee754_atan2l (rx + r1,
						 __copysignl (ix + r2,
							      __imag__ x));
	      else
		__imag__ res = __ieee754_atan2l (ix + r2, rx + r1);
	    }
	}
      else
	{
	  long double s = __ieee754_hypotl (1.0L, rx);

	  __real__ res = __log1pl (2.0L * rx * (rx + s)) / 2.0L;
	  if (adj)
	    __imag__ res = __ieee754_atan2l (s, __imag__ x);
	  else
	    __imag__ res = __ieee754_atan2l (ix, s);
	}
      if (__real__ res < LDBL_MIN)
	{
	  volatile long double force_underflow = __real__ res * __real__ res;
	  (void) force_underflow;
	}
    }
  else
    {
      __real__ y = (rx - ix) * (rx + ix) + 1.0L;
      __imag__ y = 2.0L * rx * ix;

      y = __csqrtl (y);

      __real__ y += rx;
      __imag__ y += ix;

      if (adj)
	{
	  long double t = __real__ y;
	  __real__ y = __copysignl (__imag__ y, __imag__ x);
	  __imag__ y = t;
	}

      res = __clogl (y);
    }

  /* Give results the correct sign for the original argument.  */
  __real__ res = __copysignl (__real__ res, __real__ x);
  __imag__ res = __copysignl (__imag__ res, (adj ? 1.0L : __imag__ x));

  return res;
}
