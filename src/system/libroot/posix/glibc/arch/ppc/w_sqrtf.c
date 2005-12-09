/* Single-precision floating point square root.
   Copyright (C) 1997 Free Software Foundation, Inc.
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
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include <math.h>
#include <math_private.h>
#include <fenv_libc.h>
#include <inttypes.h>

static const float almost_half = 0.50000006;  /* 0.5 + 2^-24 */
static const uint32_t a_nan = 0x7fc00000;
static const uint32_t a_inf = 0x7f800000;
static const float two48 = 281474976710656.0;
static const float twom24 = 5.9604644775390625e-8;
extern const float __t_sqrt[1024];

/* The method is based on a description in
   Computation of elementary functions on the IBM RISC System/6000 processor,
   P. W. Markstein, IBM J. Res. Develop, 34(1) 1990.
   Basically, it consists of two interleaved Newton-Rhapson approximations,
   one to find the actual square root, and one to find its reciprocal
   without the expense of a division operation.   The tricky bit here
   is the use of the POWER/PowerPC multiply-add operation to get the
   required accuracy with high speed.

   The argument reduction works by a combination of table lookup to
   obtain the initial guesses, and some careful modification of the
   generated guesses (which mostly runs on the integer unit, while the
   Newton-Rhapson is running on the FPU).  */
float
__sqrtf(float x)
{
  const float inf = *(const float *)&a_inf;
  /* x = f_washf(x); *//* This ensures only one exception for SNaN. */
  if (x > 0)
    {
      if (x != inf)
	{
	  /* Variables named starting with 's' exist in the
	     argument-reduced space, so that 2 > sx >= 0.5,
	     1.41... > sg >= 0.70.., 0.70.. >= sy > 0.35... .
	     Variables named ending with 'i' are integer versions of
	     floating-point values.  */
	  float sx;   /* The value of which we're trying to find the square
			 root.  */
	  float sg,g; /* Guess of the square root of x.  */
	  float sd,d; /* Difference between the square of the guess and x.  */
	  float sy;   /* Estimate of 1/2g (overestimated by 1ulp).  */
	  float sy2;  /* 2*sy */
	  float e;    /* Difference between y*g and 1/2 (note that e==se).  */
	  float shx;  /* == sx * fsg */
	  float fsg;  /* sg*fsg == g.  */
	  fenv_t fe;  /* Saved floating-point environment (stores rounding
			 mode and whether the inexact exception is
			 enabled).  */
	  uint32_t xi, sxi, fsgi;
	  const float *t_sqrt;

	  GET_FLOAT_WORD (xi, x);
	  fe = fegetenv_register ();
	  relax_fenv_state ();
	  sxi = (xi & 0x3fffffff) | 0x3f000000;
	  SET_FLOAT_WORD (sx, sxi);
	  t_sqrt = __t_sqrt + (xi >> (23-8-1)  & 0x3fe);
	  sg = t_sqrt[0];
	  sy = t_sqrt[1];
	  
	  /* Here we have three Newton-Rhapson iterations each of a
	     division and a square root and the remainder of the
	     argument reduction, all interleaved.   */
	  sd  = -(sg*sg - sx);
	  fsgi = (xi + 0x40000000) >> 1 & 0x7f800000;
	  sy2 = sy + sy;
	  sg  = sy*sd + sg;  /* 16-bit approximation to sqrt(sx). */
	  e   = -(sy*sg - almost_half);
	  SET_FLOAT_WORD (fsg, fsgi);
	  sd  = -(sg*sg - sx);
	  sy  = sy + e*sy2;
	  if ((xi & 0x7f800000) == 0)
	    goto denorm;
	  shx = sx * fsg;
	  sg  = sg + sy*sd;  /* 32-bit approximation to sqrt(sx),
				but perhaps rounded incorrectly.  */
	  sy2 = sy + sy;
	  g   = sg * fsg;
	  e   = -(sy*sg - almost_half);
	  d   = -(g*sg - shx);
	  sy  = sy + e*sy2;
	  fesetenv_register (fe);
	  return g + sy*d;
	denorm:
	  /* For denormalised numbers, we normalise, calculate the
	     square root, and return an adjusted result.  */
	  fesetenv_register (fe);
	  return __sqrtf(x * two48) * twom24;
	}
    }
  else if (x < 0)
    {
#ifdef FE_INVALID_SQRT
      feraiseexcept (FE_INVALID_SQRT);
      /* For some reason, some PowerPC processors don't implement
	 FE_INVALID_SQRT.  I guess no-one ever thought they'd be
	 used for square roots... :-) */
      if (!fetestexcept (FE_INVALID))
#endif
	feraiseexcept (FE_INVALID);
#ifndef _IEEE_LIBM
      if (_LIB_VERSION != _IEEE_)
	x = __kernel_standard(x,x,126);
      else
#endif
      x = *(const float*)&a_nan;
    }
  return f_washf(x);
}

weak_alias (__sqrtf, sqrtf)
/* Strictly, this is wrong, but the only places where _ieee754_sqrt is
   used will not pass in a negative result.  */
strong_alias(__sqrtf,__ieee754_sqrtf)
