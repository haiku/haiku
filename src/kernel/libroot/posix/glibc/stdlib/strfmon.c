/* Formatting a monetary value according to the current locale.
   Copyright (C) 1996-2001, 2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>
   and Jochen Hein <Jochen.Hein@informatik.TU-Clausthal.de>, 1996.

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

#include <ctype.h>
#include <errno.h>
#include <langinfo.h>
#include <locale.h>
#include <monetary.h>
#ifdef USE_IN_LIBIO
# include "../libio/libioP.h"
# include "../libio/strfile.h"
#endif
#include <printf.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "../locale/localeinfo.h"


#define out_char(Ch)							      \
  do {									      \
    if (dest >= s + maxsize - 1)					      \
      {									      \
	__set_errno (E2BIG);						      \
	va_end (ap);							      \
	return -1;							      \
      }									      \
    *dest++ = (Ch);							      \
  } while (0)

#define out_string(String)						      \
  do {									      \
    const char *_s = (String);						      \
    while (*_s)								      \
      out_char (*_s++);							      \
  } while (0)

#define out_nstring(String, N)						      \
  do {									      \
    int _n = (N);							      \
    const char *_s = (String);						      \
    while (_n-- > 0)							      \
      out_char (*_s++);							      \
  } while (0)

#define to_digit(Ch) ((Ch) - '0')


/* We use this code also for the extended locale handling where the
   function gets as an additional argument the locale which has to be
   used.  To access the values we have to redefine the _NL_CURRENT
   macro.  */
#ifdef USE_IN_EXTENDED_LOCALE_MODEL
# undef _NL_CURRENT
# define _NL_CURRENT(category, item) \
  (current->values[_NL_ITEM_INDEX (item)].string)
#endif

extern int __printf_fp (FILE *, const struct printf_info *,
			const void *const *);
libc_hidden_proto (__printf_fp)
/* This function determines the number of digit groups in the output.
   The definition is in printf_fp.c.  */
extern unsigned int __guess_grouping (unsigned int intdig_max,
				      const char *grouping, wchar_t sepchar);


/* We have to overcome some problems with this implementation.  On the
   one hand the strfmon() function is specified in XPG4 and of course
   it has to follow this.  But on the other hand POSIX.2 specifies
   some information in the LC_MONETARY category which should be used,
   too.  Some of the information contradicts the information which can
   be specified in format string.  */
#ifndef USE_IN_EXTENDED_LOCALE_MODEL
ssize_t
strfmon (char *s, size_t maxsize, const char *format, ...)
#else
ssize_t
__strfmon_l (char *s, size_t maxsize, __locale_t loc, const char *format, ...)
#endif
{
#ifdef USE_IN_EXTENDED_LOCALE_MODEL
  struct locale_data *current = loc->__locales[LC_MONETARY];
#endif
#ifdef USE_IN_LIBIO
  _IO_strfile f;
# ifdef _IO_MTSAFE_IO
  _IO_lock_t lock;
# endif
#else
  FILE f;
#endif
  struct printf_info info;
  va_list ap;			/* Scan through the varargs.  */
  char *dest;			/* Pointer so copy the output.  */
  const char *fmt;		/* Pointer that walks through format.  */

  va_start (ap, format);

  dest = s;
  fmt = format;

  /* Loop through the format-string.  */
  while (*fmt != '\0')
    {
      /* The floating-point value to output.  */
      union
      {
	double dbl;
	__long_double_t ldbl;
      }
      fpnum;
      int print_curr_symbol;
      int left_prec;
      int left_pad;
      int right_prec;
      int group;
      char pad;
      int is_long_double;
      int p_sign_posn;
      int n_sign_posn;
      int sign_posn;
      int other_sign_posn;
      int left;
      int is_negative;
      int sep_by_space;
      int other_sep_by_space;
      int cs_precedes;
      int other_cs_precedes;
      const char *sign_string;
      const char *other_sign_string;
      int done;
      const char *currency_symbol;
      size_t currency_symbol_len;
      int width;
      char *startp;
      const void *ptr;
      char space_char;

      /* Process all character which do not introduce a format
	 specification.  */
      if (*fmt != '%')
	{
	  out_char (*fmt++);
	  continue;
	}

      /* "%%" means a single '%' character.  */
      if (fmt[1] == '%')
	{
	  out_char (*++fmt);
	  ++fmt;
	  continue;
	}

      /* Defaults for formatting.  */
      print_curr_symbol = 1;		/* Print the currency symbol.  */
      left_prec = -1;			/* No left precision specified.  */
      right_prec = -1;			/* No right precision specified.  */
      group = 1;			/* Print digits grouped.  */
      pad = ' ';			/* Fill character is <SP>.  */
      is_long_double = 0;		/* Double argument by default.  */
      p_sign_posn = -1;			/* This indicates whether the */
      n_sign_posn = -1;			/* '(' flag is given.  */
      width = -1;			/* No width specified so far.  */
      left = 0;				/* Right justified by default.  */

      /* Parse group characters.  */
      while (1)
	{
	  switch (*++fmt)
	    {
	    case '=':			/* Set fill character.  */
	      pad = *++fmt;
	      if (pad == '\0')
		{
		  /* Premature EOS.  */
		  __set_errno (EINVAL);
		  va_end (ap);
		  return -1;
		}
	      continue;
	    case '^':			/* Don't group digits.  */
	      group = 0;
	      continue;
	    case '+':			/* Use +/- for sign of number.  */
	      if (n_sign_posn != -1)
		{
		  __set_errno (EINVAL);
		  va_end (ap);
		  return -1;
		}
	      p_sign_posn = *_NL_CURRENT (LC_MONETARY, P_SIGN_POSN);
	      n_sign_posn = *_NL_CURRENT (LC_MONETARY, N_SIGN_POSN);
	      continue;
	    case '(':			/* Use ( ) for negative sign.  */
	      if (n_sign_posn != -1)
		{
		  __set_errno (EINVAL);
		  va_end (ap);
		  return -1;
		}
	      p_sign_posn = 0;
	      n_sign_posn = 0;
	      continue;
	    case '!':			/* Don't print the currency symbol.  */
	      print_curr_symbol = 0;
	      continue;
	    case '-':			/* Print left justified.  */
	      left = 1;
	      continue;
	    default:
	      /* Will stop the loop.  */;
	    }
	  break;
	}

      /* If not specified by the format string now find the values for
	 the format specification.  */
      if (p_sign_posn == -1)
	p_sign_posn = *_NL_CURRENT (LC_MONETARY, P_SIGN_POSN);
      if (n_sign_posn == -1)
	n_sign_posn = *_NL_CURRENT (LC_MONETARY, N_SIGN_POSN);

      if (isdigit (*fmt))
	{
	  /* Parse field width.  */
	  width = to_digit (*fmt);

	  while (isdigit (*++fmt))
	    {
	      width *= 10;
	      width += to_digit (*fmt);
	    }

	  /* If we don't have enough room for the demanded width we
	     can stop now and return an error.  */
	  if (dest + width >= s + maxsize)
	    {
	      __set_errno (E2BIG);
	      va_end (ap);
	      return -1;
	    }
	}

      /* Recognize left precision.  */
      if (*fmt == '#')
	{
	  if (!isdigit (*++fmt))
	    {
	      __set_errno (EINVAL);
	      va_end (ap);
	      return -1;
	    }
	  left_prec = to_digit (*fmt);

	  while (isdigit (*++fmt))
	    {
	      left_prec *= 10;
	      left_prec += to_digit (*fmt);
	    }
	}

      /* Recognize right precision.  */
      if (*fmt == '.')
	{
	  if (!isdigit (*++fmt))
	    {
	      __set_errno (EINVAL);
	      va_end (ap);
	      return -1;
	    }
	  right_prec = to_digit (*fmt);

	  while (isdigit (*++fmt))
	    {
	      right_prec *= 10;
	      right_prec += to_digit (*fmt);
	    }
	}

      /* Handle modifier.  This is an extension.  */
      if (*fmt == 'L')
	{
	  ++fmt;
	  is_long_double = 1;
	}

      /* Handle format specifier.  */
      switch (*fmt++)
	{
	case 'i':		/* Use international currency symbol.  */
	  currency_symbol = _NL_CURRENT (LC_MONETARY, INT_CURR_SYMBOL);
	  currency_symbol_len = 3;
	  space_char = currency_symbol[3];
	  if (right_prec == -1)
	    {
	      if (*_NL_CURRENT (LC_MONETARY, INT_FRAC_DIGITS) == CHAR_MAX)
		right_prec = 2;
	      else
		right_prec = *_NL_CURRENT (LC_MONETARY, INT_FRAC_DIGITS);
	    }
	  break;
	case 'n':		/* Use national currency symbol.  */
	  currency_symbol = _NL_CURRENT (LC_MONETARY, CURRENCY_SYMBOL);
	  currency_symbol_len = strlen (currency_symbol);
	  space_char = ' ';
	  if (right_prec == -1)
	    {
	      if (*_NL_CURRENT (LC_MONETARY, FRAC_DIGITS) == CHAR_MAX)
		right_prec = 2;
	      else
		right_prec = *_NL_CURRENT (LC_MONETARY, FRAC_DIGITS);
	    }
	  break;
	default:		/* Any unrecognized format is an error.  */
	  __set_errno (EINVAL);
	  va_end (ap);
	  return -1;
	}

      /* If we have to print the digits grouped determine how many
	 extra characters this means.  */
      if (group && left_prec != -1)
	left_prec += __guess_grouping (left_prec,
				       _NL_CURRENT (LC_MONETARY, MON_GROUPING),
				       *_NL_CURRENT (LC_MONETARY,
						     MON_THOUSANDS_SEP));

      /* Now it's time to get the value.  */
      if (is_long_double == 1)
	{
	  fpnum.ldbl = va_arg (ap, long double);
	  is_negative = fpnum.ldbl < 0;
	  if (is_negative)
	    fpnum.ldbl = -fpnum.ldbl;
	}
      else
	{
	  fpnum.dbl = va_arg (ap, double);
	  is_negative = fpnum.dbl < 0;
	  if (is_negative)
	    fpnum.dbl = -fpnum.dbl;
	}

      /* We now know the sign of the value and can determine the format.  */
      if (is_negative)
	{
	  sign_string = _NL_CURRENT (LC_MONETARY, NEGATIVE_SIGN);
	  /* If the locale does not specify a character for the
	     negative sign we use a '-'.  */
	  if (*sign_string == '\0')
	    sign_string = (const char *) "-";
	  cs_precedes = *_NL_CURRENT (LC_MONETARY, N_CS_PRECEDES);
	  sep_by_space = *_NL_CURRENT (LC_MONETARY, N_SEP_BY_SPACE);
	  sign_posn = n_sign_posn;

	  other_sign_string = _NL_CURRENT (LC_MONETARY, POSITIVE_SIGN);
	  other_cs_precedes = *_NL_CURRENT (LC_MONETARY, P_CS_PRECEDES);
	  other_sep_by_space = *_NL_CURRENT (LC_MONETARY, P_SEP_BY_SPACE);
	  other_sign_posn = p_sign_posn;
	}
      else
	{
	  sign_string = _NL_CURRENT (LC_MONETARY, POSITIVE_SIGN);
	  cs_precedes = *_NL_CURRENT (LC_MONETARY, P_CS_PRECEDES);
	  sep_by_space = *_NL_CURRENT (LC_MONETARY, P_SEP_BY_SPACE);
	  sign_posn = p_sign_posn;

	  other_sign_string = _NL_CURRENT (LC_MONETARY, NEGATIVE_SIGN);
	  if (*other_sign_string == '\0')
	    other_sign_string = (const char *) "-";
	  other_cs_precedes = *_NL_CURRENT (LC_MONETARY, N_CS_PRECEDES);
	  other_sep_by_space = *_NL_CURRENT (LC_MONETARY, N_SEP_BY_SPACE);
	  other_sign_posn = n_sign_posn;
	}

      /* Set default values for unspecified information.  */
      if (cs_precedes != 0)
	cs_precedes = 1;
      if (other_cs_precedes != 0)
	other_cs_precedes = 1;
      if (sep_by_space == CHAR_MAX)
	sep_by_space = 0;
      if (other_sep_by_space == CHAR_MAX)
	other_sep_by_space = 0;
      if (sign_posn == CHAR_MAX)
	sign_posn = 1;
      if (other_sign_posn == CHAR_MAX)
	other_sign_posn = 1;

      /* Check for degenerate cases */
      if (sep_by_space == 2)
	{
	  if (sign_posn == 0 ||
	      (sign_posn == 1 && !cs_precedes) ||
	      (sign_posn == 2 && cs_precedes))
	    /* sign and symbol are not adjacent, so no separator */
	    sep_by_space = 0;
	}
      if (other_sep_by_space == 2)
	{
	  if (other_sign_posn == 0 ||
	      (other_sign_posn == 1 && !other_cs_precedes) ||
	      (other_sign_posn == 2 && other_cs_precedes))
	    /* sign and symbol are not adjacent, so no separator */
	    other_sep_by_space = 0;
	}

      /* Set the left precision and padding needed for alignment */
      if (left_prec == -1)
	{
	  left_prec = 0;
	  left_pad = 0;
	}
      else
	{
	  /* Set left_pad to number of spaces needed to align positive
	     and negative formats */

	  int left_bytes = 0;
	  int other_left_bytes = 0;

	  /* Work out number of bytes for currency string and separator
	     preceding the value */
	  if (cs_precedes)
	    {
	      left_bytes += currency_symbol_len;
	      if (sep_by_space != 0)
		++left_bytes;
	    }

	  if (other_cs_precedes)
	    {
	      other_left_bytes += currency_symbol_len;
	      if (other_sep_by_space != 0)
		++other_left_bytes;
	    }

	  /* Work out number of bytes for the sign (or left parenthesis)
	     preceding the value */
	  if (sign_posn == 0 && is_negative)
	    ++left_bytes;
	  else if (sign_posn == 1)
	    left_bytes += strlen (sign_string);
	  else if (cs_precedes && (sign_posn == 3 || sign_posn == 4))
	    left_bytes += strlen (sign_string);

	  if (other_sign_posn == 0 && !is_negative)
	    ++other_left_bytes;
	  else if (other_sign_posn == 1)
	    other_left_bytes += strlen (other_sign_string);
	  else if (other_cs_precedes &&
		   (other_sign_posn == 3 || other_sign_posn == 4))
	    other_left_bytes += strlen (other_sign_string);

	  /* Compare the number of bytes preceding the value for
	     each format, and set the padding accordingly */
	  if (other_left_bytes > left_bytes)
	    left_pad = other_left_bytes - left_bytes;
	  else
	    left_pad = 0;
	}

      /* Perhaps we'll someday make these things configurable so
	 better start using symbolic names now.  */
#define left_paren '('
#define right_paren ')'

      startp = dest;		/* Remember start so we can compute length.  */

      while (left_pad-- > 0)
	out_char (' ');

      if (sign_posn == 0 && is_negative)
	out_char (left_paren);

      if (cs_precedes)
	{
	  if (sign_posn != 0 && sign_posn != 2 && sign_posn != 4
	      && sign_posn != 5)
	    {
	      out_string (sign_string);
	      if (sep_by_space == 2)
		out_char (' ');
	    }

	  if (print_curr_symbol)
	    {
	      out_string (currency_symbol);

	      if (sign_posn == 4)
		{
		  if (sep_by_space == 2)
		    out_char (space_char);
		  out_string (sign_string);
		  if (sep_by_space == 1)
		    /* POSIX.2 and SUS are not clear on this case, but C99
		       says a space follows the adjacent-symbol-and-sign */
		    out_char (' ');
		}
	      else
		if (sep_by_space == 1)
		  out_char (space_char);
	    }
	}
      else
	if (sign_posn != 0 && sign_posn != 2 && sign_posn != 3
	    && sign_posn != 4 && sign_posn != 5)
	  out_string (sign_string);

      /* Print the number.  */
#ifdef USE_IN_LIBIO
# ifdef _IO_MTSAFE_IO
      f._sbf._f._lock = &lock;
# endif
      INTUSE(_IO_init) ((_IO_FILE *) &f, 0);
      _IO_JUMPS ((struct _IO_FILE_plus *) &f) = &_IO_str_jumps;
      INTUSE(_IO_str_init_static) ((_IO_strfile *) &f, dest,
				   (s + maxsize) - dest, dest);
#else
      memset ((void *) &f, 0, sizeof (f));
      f.__magic = _IOMAGIC;
      f.__mode.__write = 1;
      /* The buffer size is one less than MAXLEN
	 so we have space for the null terminator.  */
      f.__bufp = f.__buffer = (char *) dest;
      f.__bufsize = (s + maxsize) - dest;
      f.__put_limit = f.__buffer + f.__bufsize;
      f.__get_limit = f.__buffer;
      /* After the buffer is full (MAXLEN characters have been written),
	 any more characters written will go to the bit bucket.  */
      f.__room_funcs = __default_room_functions;
      f.__io_funcs.__write = NULL;
      f.__seen = 1;
#endif
      /* We clear the last available byte so we can find out whether
	 the numeric representation is too long.  */
      s[maxsize - 1] = '\0';

      info.prec = right_prec;
      info.width = left_prec + (right_prec ? (right_prec + 1) : 0);
      info.spec = 'f';
      info.is_long_double = is_long_double;
      info.is_short = 0;
      info.is_long = 0;
      info.alt = 0;
      info.space = 0;
      info.left = left;
      info.showsign = 0;
      info.group = group;
      info.pad = pad;
      info.extra = 1;		/* This means use values from LC_MONETARY.  */
      info.wide = 0;

      ptr = &fpnum;
      done = __printf_fp ((FILE *) &f, &info, &ptr);
      if (done < 0)
	{
	  va_end (ap);
	  return -1;
	}

      if (s[maxsize - 1] != '\0')
	{
	  __set_errno (E2BIG);
	  return -1;
	}

      dest += done;

      if (!cs_precedes)
	{
	  if (sign_posn == 3)
	    {
	      if (sep_by_space == 1)
		out_char (' ');
	      out_string (sign_string);
	    }

	  if (print_curr_symbol)
	    {
	      if ((sign_posn == 3 && sep_by_space == 2)
		  || (sign_posn == 4 && sep_by_space == 1)
		  || (sign_posn == 2 && sep_by_space == 1)
		  || (sign_posn == 1 && sep_by_space == 1)
		  || (sign_posn == 0 && sep_by_space == 1))
		out_char (space_char);
	      out_nstring (currency_symbol, currency_symbol_len);
	      if (sign_posn == 4)
		{
		  if (sep_by_space == 2)
		    out_char (' ');
		  out_string (sign_string);
		}
	    }
	}

      if (sign_posn == 2)
	{
	  if (sep_by_space == 2)
	    out_char (' ');
	  out_string (sign_string);
	}

      if (sign_posn == 0 && is_negative)
	out_char (right_paren);

      /* Now test whether the output width is filled.  */
      if (dest - startp < width)
	{
	  if (left)
	    /* We simply have to fill using spaces.  */
	    do
	      out_char (' ');
	    while (dest - startp < width);
	  else
	    {
	      int dist = width - (dest - startp);
	      char *cp;
	      for (cp = dest - 1; cp >= startp; --cp)
		cp[dist] = cp[0];

	      dest += dist;

	      do
		startp[--dist] = ' ';
	      while (dist > 0);
	    }
	}
    }

  /* Terminate the string.  */
  *dest = '\0';

  va_end (ap);

  return dest - s;
}
