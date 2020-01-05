/* Return arc hyperbole sine for double value, with the imaginary part
   of the result possibly adjusted for use in computing other
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

/* Return the complex inverse hyperbolic sine of finite nonzero Z,
   with the imaginary part of the result subtracted from pi/2 if ADJ
   is nonzero.  */

__complex__ double
__kernel_casinh (__complex__ double x, int adj)
{
  __complex__ double res;
  double rx, ix;
  __complex__ double y;

  /* Avoid cancellation by reducing to the first quadrant.  */
  rx = fabs (__real__ x);
  ix = fabs (__imag__ x);

  if (rx >= 1.0 / DBL_EPSILON || ix >= 1.0 / DBL_EPSILON)
    {
      /* For large x in the first quadrant, x + csqrt (1 + x * x)
	 is sufficiently close to 2 * x to make no significant
	 difference to the result; avoid possible overflow from
	 the squaring and addition.  */
      __real__ y = rx;
      __imag__ y = ix;

      if (adj)
	{
	  double t = __real__ y;
	  __real__ y = copysign (__imag__ y, __imag__ x);
	  __imag__ y = t;
	}

      res = clog (y);
      __real__ res += M_LN2;
    }
  else if (rx >= 0.5 && ix < DBL_EPSILON / 8.0)
    {
      double s = hypot (1.0, rx);

      __real__ res = log (rx + s);
      if (adj)
	__imag__ res = atan2 (s, __imag__ x);
      else
	__imag__ res = atan2 (ix, s);
    }
  else if (rx < DBL_EPSILON / 8.0 && ix >= 1.5)
    {
      double s = sqrt ((ix + 1.0) * (ix - 1.0));

      __real__ res = log (ix + s);
      if (adj)
	__imag__ res = atan2 (rx, copysign (s, __imag__ x));
      else
	__imag__ res = atan2 (s, rx);
    }
  else if (ix > 1.0 && ix < 1.5 && rx < 0.5)
    {
      if (rx < DBL_EPSILON * DBL_EPSILON)
	{
	  double ix2m1 = (ix + 1.0) * (ix - 1.0);
	  double s = sqrt (ix2m1);

	  __real__ res = log1p (2.0 * (ix2m1 + ix * s)) / 2.0;
	  if (adj)
	    __imag__ res = atan2 (rx, copysign (s, __imag__ x));
	  else
	    __imag__ res = atan2 (s, rx);
	}
      else
	{
	  double ix2m1 = (ix + 1.0) * (ix - 1.0);
	  double rx2 = rx * rx;
	  double f = rx2 * (2.0 + rx2 + 2.0 * ix * ix);
	  double d = sqrt (ix2m1 * ix2m1 + f);
	  double dp = d + ix2m1;
	  double dm = f / dp;
	  double r1 = sqrt ((dm + rx2) / 2.0);
	  double r2 = rx * ix / r1;

	  __real__ res = log1p (rx2 + dp + 2.0 * (rx * r1 + ix * r2)) / 2.0;
	  if (adj)
	    __imag__ res = atan2 (rx + r1, copysign (ix + r2,
								 __imag__ x));
	  else
	    __imag__ res = atan2 (ix + r2, rx + r1);
	}
    }
  else if (ix == 1.0 && rx < 0.5)
    {
      if (rx < DBL_EPSILON / 8.0)
	{
	  __real__ res = log1p (2.0 * (rx + sqrt (rx))) / 2.0;
	  if (adj)
	    __imag__ res = atan2 (sqrt (rx),
					    copysign (1.0, __imag__ x));
	  else
	    __imag__ res = atan2 (1.0, sqrt (rx));
	}
      else
	{
	  double d = rx * sqrt (4.0 + rx * rx);
	  double s1 = sqrt ((d + rx * rx) / 2.0);
	  double s2 = sqrt ((d - rx * rx) / 2.0);

	  __real__ res = log1p (rx * rx + d + 2.0 * (rx * s1 + s2)) / 2.0;
	  if (adj)
	    __imag__ res = atan2 (rx + s1, copysign (1.0 + s2,
								 __imag__ x));
	  else
	    __imag__ res = atan2 (1.0 + s2, rx + s1);
	}
    }
  else if (ix < 1.0 && rx < 0.5)
    {
      if (ix >= DBL_EPSILON)
	{
	  if (rx < DBL_EPSILON * DBL_EPSILON)
	    {
	      double onemix2 = (1.0 + ix) * (1.0 - ix);
	      double s = sqrt (onemix2);

	      __real__ res = log1p (2.0 * rx / s) / 2.0;
	      if (adj)
		__imag__ res = atan2 (s, __imag__ x);
	      else
		__imag__ res = atan2 (ix, s);
	    }
	  else
	    {
	      double onemix2 = (1.0 + ix) * (1.0 - ix);
	      double rx2 = rx * rx;
	      double f = rx2 * (2.0 + rx2 + 2.0 * ix * ix);
	      double d = sqrt (onemix2 * onemix2 + f);
	      double dp = d + onemix2;
	      double dm = f / dp;
	      double r1 = sqrt ((dp + rx2) / 2.0);
	      double r2 = rx * ix / r1;

	      __real__ res
		= log1p (rx2 + dm + 2.0 * (rx * r1 + ix * r2)) / 2.0;
	      if (adj)
		__imag__ res = atan2 (rx + r1,
						copysign (ix + r2,
							    __imag__ x));
	      else
		__imag__ res = atan2 (ix + r2, rx + r1);
	    }
	}
      else
	{
	  double s = hypot (1.0, rx);

	  __real__ res = log1p (2.0 * rx * (rx + s)) / 2.0;
	  if (adj)
	    __imag__ res = atan2 (s, __imag__ x);
	  else
	    __imag__ res = atan2 (ix, s);
	}
      if (__real__ res < DBL_MIN)
	{
	  volatile double force_underflow = __real__ res * __real__ res;
	  (void) force_underflow;
	}
    }
  else
    {
      __real__ y = (rx - ix) * (rx + ix) + 1.0;
      __imag__ y = 2.0 * rx * ix;

      y = csqrt (y);

      __real__ y += rx;
      __imag__ y += ix;

      if (adj)
	{
	  double t = __real__ y;
	  __real__ y = copysign (__imag__ y, __imag__ x);
	  __imag__ y = t;
	}

      res = clog (y);
    }

  /* Give results the correct sign for the original argument.  */
  __real__ res = copysign (__real__ res, __real__ x);
  __imag__ res = copysign (__imag__ res, (adj ? 1.0 : __imag__ x));

  return res;
}
