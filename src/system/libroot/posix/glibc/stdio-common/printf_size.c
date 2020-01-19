/* Print size value using units for orders of magnitude.
   Copyright (C) 1997,1998,1999,2000,2001,2002 Free Software Foundation, Inc.
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
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include <libioP.h>

#include <stdio_private.h>
#include <ctype.h>
#include <ieee754.h>
#include <math.h>
#include <printf.h>


/* This defines make it possible to use the same code for GNU C library and
   the GNU I/O library.	 */
#ifdef USE_IN_LIBIO
# define PUT(f, s, n) _IO_sputn (f, s, n)
# define PAD(f, c, n) (wide ? _IO_wpadn (f, c, n) : INTUSE(_IO_padn) (f, c, n))
/* We use this file GNU C library and GNU I/O library.	So make
   names equal.	 */
# undef putc
# define putc(c, f) (wide \
		     ? (int)_IO_putwc_unlocked (c, f) : _IO_putc_unlocked (c, f))
# define size_t	_IO_size_t
# define FILE	_IO_FILE
#else	/* ! USE_IN_LIBIO */
# define PUT(f, s, n) fwrite (s, 1, n, f)
# define PAD(f, c, n) __printf_pad (f, c, n)
ssize_t __printf_pad __P ((FILE *, char pad, int n)); /* In vfprintf.c.  */
#endif	/* USE_IN_LIBIO */

/* Macros for doing the actual output.  */

#define outchar(ch)							      \
  do									      \
    {									      \
      register const int outc = (ch);					      \
      if (putc (outc, fp) == EOF)					      \
	return -1;							      \
      ++done;								      \
    } while (0)

#define PRINT(ptr, wptr, len)						      \
  do									      \
    {									      \
      register size_t outlen = (len);					      \
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
printf_size (FILE *fp, const struct printf_info *info, const void *const *args)
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
      union ieee854_long_double ldbl;
    }
  fpnum;
  const void *ptr = &fpnum;

  int negative = 0;

  /* "NaN" or "Inf" for the special cases.  */
  const char *special = NULL;
  const wchar_t *wspecial = NULL;

  struct printf_info fp_info;
  int done = 0;
  int wide = info->wide;


  /* Fetch the argument value.	*/
#ifndef __NO_LONG_DOUBLE_MATH
  if (info->is_long_double && sizeof (long double) > sizeof (double))
    {
      fpnum.ldbl.d = *(const long double *) args[0];

      /* Check for special values: not a number or infinity.  */
      if (isnan (fpnum.ldbl.d))
	{
	  special = "nan";
	  wspecial = L"nan";
	  negative = 0;
	}
      else if (isinf (fpnum.ldbl.d))
	{
	  special = "inf";
	  wspecial = L"inf";

	  negative = fpnum.ldbl.d < 0;
	}
      else
	while (fpnum.ldbl.d >= divisor && tag[1] != '\0')
	  {
	    fpnum.ldbl.d /= divisor;
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
	  negative = 0;
	}
      else if (isinf (fpnum.dbl.d))
	{
	  special = "inf";
	  wspecial = L"inf";

	  negative = fpnum.dbl.d < 0;
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

      if (negative || info->showsign || info->space)
	--width;
      width -= 3;

      if (!info->left && width > 0)
	PADN (' ', width);

      if (negative)
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
  fp_info.spec = 'f';
  fp_info.prec = info->prec < 0 ? 3 : info->prec;
  fp_info.is_long_double = info->is_long_double;
  fp_info.is_short = info->is_short;
  fp_info.is_long = info->is_long;
  fp_info.alt = info->alt;
  fp_info.space = info->space;
  fp_info.left = info->left;
  fp_info.showsign = info->showsign;
  fp_info.group = info->group;
  fp_info.extra = info->extra;
  fp_info.pad = info->pad;
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
