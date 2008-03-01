/* Compatibility functions for floating point formatting, reentrant versions.
   Copyright (C) 1995,96,97,98,99,2000,01,02 Free Software Foundation, Inc.
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

#include <errno.h>
#include <float.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <sys/param.h>

#ifndef FLOAT_TYPE
# define FLOAT_TYPE double
# define FUNC_PREFIX
# define FLOAT_FMT_FLAG
# define FLOAT_NAME_EXT
# if DBL_MANT_DIG == 53
#  define NDIGIT_MAX 17
# elif DBL_MANT_DIG == 24
#  define NDIGIT_MAX 9
# elif DBL_MANT_DIG == 56
#  define NDIGIT_MAX 18
# else
/* See IEEE 854 5.6, table 2 for this formula.  Unfortunately we need a
   compile time constant here, so we cannot use it.  */
#  error "NDIGIT_MAX must be precomputed"
#  define NDIGIT_MAX (lrint (ceil (M_LN2 / M_LN10 * DBL_MANT_DIG + 1.0)))
# endif
#endif

#define APPEND(a, b) APPEND2 (a, b)
#define APPEND2(a, b) a##b

#define FLOOR APPEND(floor, FLOAT_NAME_EXT)
#define FABS APPEND(fabs, FLOAT_NAME_EXT)
#define LOG10 APPEND(log10, FLOAT_NAME_EXT)
#define EXP APPEND(exp, FLOAT_NAME_EXT)


int
APPEND (FUNC_PREFIX, fcvt_r) (value, ndigit, decpt, sign, buf, len)
     FLOAT_TYPE value;
     int ndigit, *decpt, *sign;
     char *buf;
     size_t len;
{
  ssize_t n;
  ssize_t i;
  int left;

  if (buf == NULL)
    {
      __set_errno (EINVAL);
      return -1;
    }

  left = 0;
  if (isfinite (value))
    {
      *sign = signbit (value) != 0;
      if (*sign)
	value = -value;

      if (ndigit < 0)
	{
	  /* Rounding to the left of the decimal point.  */
	  while (ndigit < 0)
	    {
	      FLOAT_TYPE new_value = value * 0.1;

	      if (new_value < 1.0)
		{
		  ndigit = 0;
		  break;
		}

	      value = new_value;
	      ++left;
	      ++ndigit;
	    }
	}
    }
  else
    /* Value is Inf or NaN.  */
    *sign = 0;

  n = __snprintf (buf, len, "%.*" FLOAT_FMT_FLAG "f", MIN (ndigit, NDIGIT_MAX),
		  value);
  /* Check for a too small buffer.  */
  if (n >= (ssize_t) len)
    return -1;

  i = 0;
  while (i < n && isdigit (buf[i]))
    ++i;
  *decpt = i;

  if (i == 0)
    /* Value is Inf or NaN.  */
    return 0;

  if (i < n)
    {
      do
	++i;
      while (i < n && !isdigit (buf[i]));

      if (*decpt == 1 && buf[0] == '0' && value != 0.0)
	{
	  /* We must not have leading zeroes.  Strip them all out and
	     adjust *DECPT if necessary.  */
	  --*decpt;
	  while (i < n && buf[i] == '0')
	    {
	      --*decpt;
	      ++i;
	    }
	}

      memmove (&buf[MAX (*decpt, 0)], &buf[i], n - i);
      buf[n - (i - MAX (*decpt, 0))] = '\0';
    }

  if (left)
    {
      *decpt += left;
      if ((ssize_t) --len > n)
	{
	  while (left-- > 0 && n < (ssize_t) len)
	    buf[n++] = '0';
	  buf[n] = '\0';
	}
    }

  return 0;
}
libc_hidden_def (APPEND (FUNC_PREFIX, fcvt_r))

int
APPEND (FUNC_PREFIX, ecvt_r) (value, ndigit, decpt, sign, buf, len)
     FLOAT_TYPE value;
     int ndigit, *decpt, *sign;
     char *buf;
     size_t len;
{
  int exponent = 0;

  if (isfinite (value) && value != 0.0)
    {
      /* Slow code that doesn't require -lm functions.  */
      FLOAT_TYPE d;
      FLOAT_TYPE f = 1.0;
      if (value < 0.0)
	d = -value;
      else
	d = value;
      if (d < 1.0)
	{
	  do
	    {
	      f *= 10.0;
	      --exponent;
	    }
	  while (d * f < 1.0);

	  value *= f;
	}
      else if (d >= 10.0)
	{
	  do
	    {
	      f *= 10;
	      ++exponent;
	    }
	  while (d >= f * 10.0);

	  value /= f;
	}
    }
  else if (value == 0.0)
    /* SUSv2 leaves it unspecified whether *DECPT is 0 or 1 for 0.0.
       This could be changed to -1 if we want to return 0.  */
    exponent = 0;

  if (ndigit <= 0 && len > 0)
    {
      buf[0] = '\0';
      *decpt = 1;
      *sign = isfinite (value) ? signbit (value) != 0 : 0;
    }
  else
    if (APPEND (FUNC_PREFIX, fcvt_r) (value, MIN (ndigit, NDIGIT_MAX) - 1,
				      decpt, sign, buf, len))
      return -1;

  *decpt += exponent;
  return 0;
}
libc_hidden_def (APPEND (FUNC_PREFIX, ecvt_r))
