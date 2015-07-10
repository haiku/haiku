/* Return arc hyperbole sine for float value, with the imaginary part
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

__complex__ float
__kernel_casinhf (__complex__ float x, int adj)
{
  __complex__ float res;
  float rx, ix;
  __complex__ float y;

  /* Avoid cancellation by reducing to the first quadrant.  */
  rx = fabsf (__real__ x);
  ix = fabsf (__imag__ x);

  if (rx >= 1.0f / FLT_EPSILON || ix >= 1.0f / FLT_EPSILON)
    {
      /* For large x in the first quadrant, x + csqrt (1 + x * x)
	 is sufficiently close to 2 * x to make no significant
	 difference to the result; avoid possible overflow from
	 the squaring and addition.  */
      __real__ y = rx;
      __imag__ y = ix;

      if (adj)
	{
	  float t = __real__ y;
	  __real__ y = __copysignf (__imag__ y, __imag__ x);
	  __imag__ y = t;
	}

      res = __clogf (y);
      __real__ res += (float) M_LN2;
    }
  else if (rx >= 0.5f && ix < FLT_EPSILON / 8.0f)
    {
      float s = __ieee754_hypotf (1.0f, rx);

      __real__ res = __ieee754_logf (rx + s);
      if (adj)
	__imag__ res = __ieee754_atan2f (s, __imag__ x);
      else
	__imag__ res = __ieee754_atan2f (ix, s);
    }
  else if (rx < FLT_EPSILON / 8.0f && ix >= 1.5f)
    {
      float s = __ieee754_sqrtf ((ix + 1.0f) * (ix - 1.0f));

      __real__ res = __ieee754_logf (ix + s);
      if (adj)
	__imag__ res = __ieee754_atan2f (rx, __copysignf (s, __imag__ x));
      else
	__imag__ res = __ieee754_atan2f (s, rx);
    }
  else if (ix > 1.0f && ix < 1.5f && rx < 0.5f)
    {
      if (rx < FLT_EPSILON * FLT_EPSILON)
	{
	  float ix2m1 = (ix + 1.0f) * (ix - 1.0f);
	  float s = __ieee754_sqrtf (ix2m1);

	  __real__ res = __log1pf (2.0f * (ix2m1 + ix * s)) / 2.0f;
	  if (adj)
	    __imag__ res = __ieee754_atan2f (rx, __copysignf (s, __imag__ x));
	  else
	    __imag__ res = __ieee754_atan2f (s, rx);
	}
      else
	{
	  float ix2m1 = (ix + 1.0f) * (ix - 1.0f);
	  float rx2 = rx * rx;
	  float f = rx2 * (2.0f + rx2 + 2.0f * ix * ix);
	  float d = __ieee754_sqrtf (ix2m1 * ix2m1 + f);
	  float dp = d + ix2m1;
	  float dm = f / dp;
	  float r1 = __ieee754_sqrtf ((dm + rx2) / 2.0f);
	  float r2 = rx * ix / r1;

	  __real__ res
	    = __log1pf (rx2 + dp + 2.0f * (rx * r1 + ix * r2)) / 2.0f;
	  if (adj)
	    __imag__ res = __ieee754_atan2f (rx + r1, __copysignf (ix + r2,
								   __imag__ x));
	  else
	    __imag__ res = __ieee754_atan2f (ix + r2, rx + r1);
	}
    }
  else if (ix == 1.0f && rx < 0.5f)
    {
      if (rx < FLT_EPSILON / 8.0f)
	{
	  __real__ res = __log1pf (2.0f * (rx + __ieee754_sqrtf (rx))) / 2.0f;
	  if (adj)
	    __imag__ res = __ieee754_atan2f (__ieee754_sqrtf (rx),
					     __copysignf (1.0f, __imag__ x));
	  else
	    __imag__ res = __ieee754_atan2f (1.0f, __ieee754_sqrtf (rx));
	}
      else
	{
	  float d = rx * __ieee754_sqrtf (4.0f + rx * rx);
	  float s1 = __ieee754_sqrtf ((d + rx * rx) / 2.0f);
	  float s2 = __ieee754_sqrtf ((d - rx * rx) / 2.0f);

	  __real__ res = __log1pf (rx * rx + d + 2.0f * (rx * s1 + s2)) / 2.0f;
	  if (adj)
	    __imag__ res = __ieee754_atan2f (rx + s1,
					     __copysignf (1.0f + s2,
							  __imag__ x));
	  else
	    __imag__ res = __ieee754_atan2f (1.0f + s2, rx + s1);
	}
    }
  else if (ix < 1.0f && rx < 0.5f)
    {
      if (ix >= FLT_EPSILON)
	{
	  if (rx < FLT_EPSILON * FLT_EPSILON)
	    {
	      float onemix2 = (1.0f + ix) * (1.0f - ix);
	      float s = __ieee754_sqrtf (onemix2);

	      __real__ res = __log1pf (2.0f * rx / s) / 2.0f;
	      if (adj)
		__imag__ res = __ieee754_atan2f (s, __imag__ x);
	      else
		__imag__ res = __ieee754_atan2f (ix, s);
	    }
	  else
	    {
	      float onemix2 = (1.0f + ix) * (1.0f - ix);
	      float rx2 = rx * rx;
	      float f = rx2 * (2.0f + rx2 + 2.0f * ix * ix);
	      float d = __ieee754_sqrtf (onemix2 * onemix2 + f);
	      float dp = d + onemix2;
	      float dm = f / dp;
	      float r1 = __ieee754_sqrtf ((dp + rx2) / 2.0f);
	      float r2 = rx * ix / r1;

	      __real__ res
		= __log1pf (rx2 + dm + 2.0f * (rx * r1 + ix * r2)) / 2.0f;
	      if (adj)
		__imag__ res = __ieee754_atan2f (rx + r1,
						 __copysignf (ix + r2,
							      __imag__ x));
	      else
		__imag__ res = __ieee754_atan2f (ix + r2, rx + r1);
	    }
	}
      else
	{
	  float s = __ieee754_hypotf (1.0f, rx);

	  __real__ res = __log1pf (2.0f * rx * (rx + s)) / 2.0f;
	  if (adj)
	    __imag__ res = __ieee754_atan2f (s, __imag__ x);
	  else
	    __imag__ res = __ieee754_atan2f (ix, s);
	}
      if (__real__ res < FLT_MIN)
	{
	  volatile float force_underflow = __real__ res * __real__ res;
	  (void) force_underflow;
	}
    }
  else
    {
      __real__ y = (rx - ix) * (rx + ix) + 1.0f;
      __imag__ y = 2.0f * rx * ix;

      y = __csqrtf (y);

      __real__ y += rx;
      __imag__ y += ix;

      if (adj)
	{
	  float t = __real__ y;
	  __real__ y = __copysignf (__imag__ y, __imag__ x);
	  __imag__ y = t;
	}

      res = __clogf (y);
    }

  /* Give results the correct sign for the original argument.  */
  __real__ res = __copysignf (__real__ res, __real__ x);
  __imag__ res = __copysignf (__imag__ res, (adj ? 1.0f : __imag__ x));

  return res;
}
