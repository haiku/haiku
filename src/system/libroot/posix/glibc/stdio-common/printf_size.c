/* Print size value using units for orders of magnitude.
   Copyright (C) 1997-2014 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1997.
   Based on a proposal by Larry McVoy <lm@sgi.com>.

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

#include <ctype.h>
#include <ieee754.h>
#include <math.h>
#include <printf.h>
#include <libioP.h>


/* This defines make it possible to use the same code for GNU C library and
   the GNU I/O library.	 */
#define PUT(f, s, n) _IO_sputn (f, s, n)
#define PAD(f, c, n) (wide ? _IO_wpadn (f, c, n) : _IO_padn (f, c, n))
/* We use this file GNU C library and GNU I/O library.	So make
   names equal.	 */
#undef putc
#define putc(c, f) (wide \
		    ? (int)_IO_putwc_unlocked (c, f) : _IO_putc_unlocked (c, f))
#define size_t	_IO_size_t
#define FILE	_IO_FILE

/* Macros for doing the actual output.  */

#define outchar(ch)							      \
  do									      \
    {									      \
      const int outc = (ch);						      \
      if (putc (outc, fp) == EOF)					      \
	return -1;							      \
      ++done;								      \
    } while (0)

#define PRINT(ptr, wptr, len)						      \
  do									      \
    {									      \
      size_t outlen = (len);						      \
      if (len > 20)							      \
	{								      \
	  if (PUT (fp, wide ? (const char *) wptr : ptr, outlen) != outlen)   \
	    return -1;							      \
	  ptr += outlen;						      \
	  done += outlen;						      \
	}								      \
      else								      \
	{								      \
	  if (wide)							      \
	    while (outlen-- > 0)					      \
	      outchar (*wptr++);					      \
	  else								      \
	    while (outlen-- > 0)					      \
	      outchar (*ptr++);						      \
	}								      \
    } while (0)

#define PADN(ch, len)							      \
  do									      \
    {									      \
      if (PAD (fp, ch, len) != len)					      \
	return -1;							      \
      done += len;							      \
    }									      \
  while (0)

/* Prototype for helper functions.  */
extern int __printf_fp (FILE *fp, const struct printf_info *info,
			const void *const *args);


int
__printf_size (FILE *fp, const struct printf_info *info,
	       const void *const *args)
{
  /* Units for the both formats.  */
#define BINARY_UNITS	" kmgtpezy"
#define DECIMAL_UNITS	" KMGTPEZY"
  static const char units[2][sizeof (BINARY_UNITS)] =
  {
    BINARY_UNITS,	/* For binary format.  */
    DECIMAL_UNITS	/* For decimal format.  */
  };
  const char *tag = units[isupper (info->spec) != 0];
  int divisor = isupper (info->spec) ? 1000 : 1024;

  /* The floating-point value to output.  */
  union
    {
      union ieee754_double dbl;
      long double ldbl;
    }
  fpnum;
  const void *ptr = &fpnum;

  int fpnum_sign = 0;

  /* "NaN" or "Inf" for the special cases.  */
  const char *special = NULL;
  const wchar_t *wspecial = NULL;

  struct printf_info fp_info;
  int done = 0;
  int wide = info->wide;
  int res;

  /* Fetch the argument value.	*/
#ifndef __NO_LONG_DOUBLE_MATH
  if (info->is_long_double && sizeof (long double) > sizeof (double))
    {
      fpnum.ldbl = *(const long double *) args[0];

      /* Check for special values: not a number or infinity.  */
      if (isnan (fpnum.ldbl))
	{
	  special = "nan";
	  wspecial = L"nan";
	  // fpnum_sign = 0;	Already zero
	}
      else if ((res = isinf (fpnum.ldbl)))
	{
	  fpnum_sign = res;
	  special = "inf";
	  wspecial = L"inf";
	}
      else
	while (fpnum.ldbl >= divisor && tag[1] != '\0')
	  {
	    fpnum.ldbl /= divisor;
	    ++tag;
	  }
    }
  else
#endif	/* no long double */
    {
      fpnum.dbl.d = *(const double *) args[0];

      /* Check for special values: not a number or infinity.  */
      if (isnan (fpnum.dbl.d))
	{
	  special = "nan";
	  wspecial = L"nan";
	  // fpnum_sign = 0;	Already zero
	}
      else if ((res = isinf (fpnum.dbl.d)))
	{
	  fpnum_sign = res;
	  special = "inf";
	  wspecial = L"inf";
	}
      else
	while (fpnum.dbl.d >= divisor && tag[1] != '\0')
	  {
	    fpnum.dbl.d /= divisor;
	    ++tag;
	  }
    }

  if (special)
    {
      int width = info->prec > info->width ? info->prec : info->width;

      if (fpnum_sign < 0 || info->showsign || info->space)
	--width;
      width -= 3;

      if (!info->left && width > 0)
	PADN (' ', width);

      if (fpnum_sign < 0)
	outchar ('-');
      else if (info->showsign)
	outchar ('+');
      else if (info->space)
	outchar (' ');

      PRINT (special, wspecial, 3);

      if (info->left && width > 0)
	PADN (' ', width);

      return done;
    }

  /* Prepare to print the number.  We want to use `__printf_fp' so we
     have to prepare a `printf_info' structure.  */
  fp_info = *info;
  fp_info.spec = 'f';
  fp_info.prec = info->prec < 0 ? 3 : info->prec;
  fp_info.wide = wide;

  if (fp_info.left && fp_info.pad == L' ')
    {
      /* We must do the padding ourself since the unit character must
	 be placed before the padding spaces.  */
      fp_info.width = 0;

      done = __printf_fp (fp, &fp_info, &ptr);
      if (done > 0)
	{
	  outchar (*tag);
	  if (info->width > done)
	    PADN (' ', info->width - done);
	}
    }
  else
    {
      /* We can let __printf_fp do all the printing and just add our
	 unit character afterwards.  */
      fp_info.width = info->width - 1;

      done = __printf_fp (fp, &fp_info, &ptr);
      if (done > 0)
	outchar (*tag);
    }

  return done;
}
strong_alias (__printf_size, printf_size);

/* This is the function used by `vfprintf' to determine number and
   type of the arguments.  */
int
printf_size_info (const struct printf_info *info, size_t n, int *argtypes)
{
  /* We need only one double or long double argument.  */
  if (n >= 1)
    argtypes[0] = PA_DOUBLE | (info->is_long_double ? PA_FLAG_LONG_DOUBLE : 0);

  return 1;
}
