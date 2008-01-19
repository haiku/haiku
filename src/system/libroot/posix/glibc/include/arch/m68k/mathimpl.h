/* Definitions of libc internal inline math functions implemented
   by the m68881/2.
   Copyright (C) 1991,92,93,94,96,97,98,99 Free Software Foundation, Inc.
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

/* This file contains the definitions of the inline math functions that
   are only used internally inside libm, not visible to the user.  */

__inline_mathop	(__ieee754_acos, acos)
__inline_mathop	(__ieee754_asin, asin)
__inline_mathop	(__ieee754_cosh, cosh)
__inline_mathop	(__ieee754_sinh, sinh)
__inline_mathop	(__ieee754_exp, etox)
__inline_mathop	(__ieee754_exp2, twotox)
__inline_mathop	(__ieee754_exp10, tentox)
__inline_mathop	(__ieee754_log10, log10)
__inline_mathop	(__ieee754_log2, log2)
__inline_mathop	(__ieee754_log, logn)
__inline_mathop	(__ieee754_sqrt, sqrt)
__inline_mathop	(__ieee754_atanh, atanh)

__m81_defun (double, __ieee754_remainder, (double __x, double __y))
{
  double __result;
  __asm ("frem%.x %1, %0" : "=f" (__result) : "f" (__y), "0" (__x));
  return __result;
}

__m81_defun (float, __ieee754_remainderf, (float __x, float __y))
{
  float __result;
  __asm ("frem%.x %1, %0" : "=f" (__result) : "f" (__y), "0" (__x));
  return __result;
}

__m81_defun (long double,
	     __ieee754_remainderl, (long double __x, long double __y))
{
  long double __result;
  __asm ("frem%.x %1, %0" : "=f" (__result) : "f" (__y), "0" (__x));
  return __result;
}

__m81_defun (double, __ieee754_fmod, (double __x, double __y))
{
  double __result;
  __asm ("fmod%.x %1, %0" : "=f" (__result) : "f" (__y), "0" (__x));
  return __result;
}

__m81_defun (float, __ieee754_fmodf, (float __x, float __y))
{
  float __result;
  __asm ("fmod%.x %1, %0" : "=f" (__result) : "f" (__y), "0" (__x));
  return __result;
}

__m81_defun (long double,
	     __ieee754_fmodl, (long double __x, long double __y))
{
  long double __result;
  __asm ("fmod%.x %1, %0" : "=f" (__result) : "f" (__y), "0" (__x));
  return __result;
}

/* Get the m68881 condition codes, to quickly check multiple conditions.  */
static __inline__ unsigned long
__m81_test (long double __val)
{
  unsigned long __fpsr;
  __asm ("ftst%.x %1; fmove%.l %/fpsr,%0" : "=dm" (__fpsr) : "f" (__val));
  return __fpsr;
}

/* Bit values returned by __m81_test.  */
#define __M81_COND_NAN  (1 << 24)
#define __M81_COND_INF  (2 << 24)
#define __M81_COND_ZERO (4 << 24)
#define __M81_COND_NEG  (8 << 24)
