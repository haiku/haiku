/* Copyright (C) 1991, 1992, 1997, 1999, 2003, 2006, 2008 Free
   Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <config.h>

#include <stdlib.h>

#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>

#include "c-ctype.h"

/* Convert NPTR to a double.  If ENDPTR is not NULL, a pointer to the
   character after the last one used in the number is put in *ENDPTR.  */
double
strtod (const char *nptr, char **endptr)
{
  const unsigned char *s;
  bool negative = false;

  /* The number so far.  */
  double num;

  bool got_dot;			/* Found a decimal point.  */
  bool got_digit;		/* Seen any digits.  */
  bool hex = false;		/* Look for hex float exponent.  */

  /* The exponent of the number.  */
  long int exponent;

  if (nptr == NULL)
    {
      errno = EINVAL;
      goto noconv;
    }

  /* Use unsigned char for the ctype routines.  */
  s = (unsigned char *) nptr;

  /* Eat whitespace.  */
  while (isspace (*s))
    ++s;

  /* Get the sign.  */
  negative = *s == '-';
  if (*s == '-' || *s == '+')
    ++s;

  num = 0.0;
  got_dot = false;
  got_digit = false;
  exponent = 0;

  /* Check for hex float.  */
  if (*s == '0' && c_tolower (s[1]) == 'x'
      && (c_isxdigit (s[2]) || ('.' == s[2] && c_isxdigit (s[3]))))
    {
      hex = true;
      s += 2;
      for (;; ++s)
	{
	  if (c_isxdigit (*s))
	    {
	      got_digit = true;

	      /* Make sure that multiplication by 16 will not overflow.  */
	      if (num > DBL_MAX / 16)
		/* The value of the digit doesn't matter, since we have already
		   gotten as many digits as can be represented in a `double'.
		   This doesn't necessarily mean the result will overflow.
		   The exponent may reduce it to within range.

		   We just need to record that there was another
		   digit so that we can multiply by 16 later.  */
		++exponent;
	      else
		num = ((num * 16.0)
		       + (c_tolower (*s) - (c_isdigit (*s) ? '0' : 'a' - 10)));

	      /* Keep track of the number of digits after the decimal point.
		 If we just divided by 16 here, we would lose precision.  */
	      if (got_dot)
		--exponent;
	    }
	  else if (!got_dot && *s == '.')
	    /* Record that we have found the decimal point.  */
	    got_dot = true;
	  else
	    /* Any other character terminates the number.  */
	    break;
	}
    }

  /* Not a hex float.  */
  else
    {
      for (;; ++s)
	{
	  if (c_isdigit (*s))
	    {
	      got_digit = true;

	      /* Make sure that multiplication by 10 will not overflow.  */
	      if (num > DBL_MAX * 0.1)
		/* The value of the digit doesn't matter, since we have already
		   gotten as many digits as can be represented in a `double'.
		   This doesn't necessarily mean the result will overflow.
		   The exponent may reduce it to within range.

		   We just need to record that there was another
		   digit so that we can multiply by 10 later.  */
		++exponent;
	      else
		num = (num * 10.0) + (*s - '0');

	      /* Keep track of the number of digits after the decimal point.
		 If we just divided by 10 here, we would lose precision.  */
	      if (got_dot)
		--exponent;
	    }
	  else if (!got_dot && *s == '.')
	    /* Record that we have found the decimal point.  */
	    got_dot = true;
	  else
	    /* Any other character terminates the number.  */
	    break;
	}
    }

  if (!got_digit)
    {
      /* Check for infinities and NaNs.  */
      if (c_tolower (*s) == 'i'
	  && c_tolower (s[1]) == 'n'
	  && c_tolower (s[2]) == 'f')
	{
	  s += 3;
	  num = HUGE_VAL;
	  if (c_tolower (*s) == 'i'
	      && c_tolower (s[1]) == 'n'
	      && c_tolower (s[2]) == 'i'
	      && c_tolower (s[3]) == 't'
	      && c_tolower (s[4]) == 'y')
	    s += 5;
	  goto valid;
	}
#ifdef NAN
      else if (c_tolower (*s) == 'n'
	       && c_tolower (s[1]) == 'a'
	       && c_tolower (s[2]) == 'n')
	{
	  s += 3;
	  num = NAN;
	  /* Since nan(<n-char-sequence>) is implementation-defined,
	     we define it by ignoring <n-char-sequence>.  A nicer
	     implementation would populate the bits of the NaN
	     according to interpreting n-char-sequence as a
	     hexadecimal number, but the result is still a NaN.  */
	  if (*s == '(')
	    {
	      const unsigned char *p = s + 1;
	      while (c_isalnum (*p))
		p++;
	      if (*p == ')')
		s = p + 1;
	    }
	  goto valid;
	}
#endif
      goto noconv;
    }

  if (c_tolower (*s) == (hex ? 'p' : 'e') && !isspace (s[1]))
    {
      /* Get the exponent specified after the `e' or `E'.  */
      int save = errno;
      char *end;
      long int value;

      errno = 0;
      ++s;
      value = strtol ((char *) s, &end, 10);
      if (errno == ERANGE && num)
	{
	  /* The exponent overflowed a `long int'.  It is probably a safe
	     assumption that an exponent that cannot be represented by
	     a `long int' exceeds the limits of a `double'.  */
	  if (endptr != NULL)
	    *endptr = end;
	  if (value < 0)
	    goto underflow;
	  else
	    goto overflow;
	}
      else if (end == (char *) s)
	/* There was no exponent.  Reset END to point to
	   the 'e' or 'E', so *ENDPTR will be set there.  */
	end = (char *) s - 1;
      errno = save;
      s = (unsigned char *) end;
      exponent += value;
    }

  if (num == 0.0)
    goto valid;

  if (hex)
    {
      /* ldexp takes care of range errors.  */
      num = ldexp (num, exponent);
      goto valid;
    }

  /* Multiply NUM by 10 to the EXPONENT power,
     checking for overflow and underflow.  */

  if (exponent < 0)
    {
      if (num < DBL_MIN * pow (10.0, (double) -exponent))
	goto underflow;
    }
  else if (exponent > 0)
    {
      if (num > DBL_MAX * pow (10.0, (double) -exponent))
	goto overflow;
    }

  num *= pow (10.0, (double) exponent);

 valid:
  if (endptr != NULL)
    *endptr = (char *) s;
  return negative ? -num : num;

 overflow:
  /* Return an overflow error.  */
  if (endptr != NULL)
    *endptr = (char *) s;
  errno = ERANGE;
  return negative ? -HUGE_VAL : HUGE_VAL;

 underflow:
  /* Return an underflow error.  */
  if (endptr != NULL)
    *endptr = (char *) s;
  errno = ERANGE;
  return negative ? -0.0 : 0.0;

 noconv:
  /* There was no number.  */
  if (endptr != NULL)
    *endptr = (char *) nptr;
  errno = EINVAL;
  return 0.0;
}
