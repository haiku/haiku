/* Internal header for parsing printf format strings.
   Copyright (C) 1995-1999, 2000 Free Software Foundation, Inc.
   This file is part of th GNU C Library.

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
#include <limits.h>
#include <printf.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define NDEBUG 1
#include <assert.h>


struct printf_spec
  {
    /* Information parsed from the format spec.  */
    struct printf_info info;

    /* Pointers into the format string for the end of this format
       spec and the next (or to the end of the string if no more).  */
    const UCHAR_T *end_of_fmt, *next_fmt;

    /* Position of arguments for precision and width, or -1 if `info' has
       the constant value.  */
    int prec_arg, width_arg;

    int data_arg;		/* Position of data argument.  */
    int data_arg_type;		/* Type of first argument.  */
    /* Number of arguments consumed by this format specifier.  */
    size_t ndata_args;
  };


/* The various kinds off arguments that can be passed to printf.  */
union printf_arg
  {
    unsigned char pa_char;
    wchar_t pa_wchar;
    short int pa_short_int;
    int pa_int;
    long int pa_long_int;
    long long int pa_long_long_int;
    unsigned short int pa_u_short_int;
    unsigned int pa_u_int;
    unsigned long int pa_u_long_int;
    unsigned long long int pa_u_long_long_int;
    float pa_float;
    double pa_double;
    long double pa_long_double;
    const char *pa_string;
    const wchar_t *pa_wstring;
    void *pa_pointer;
  };


/* Read a simple integer from a string and update the string pointer.
   It is assumed that the first character is a digit.  */
static inline unsigned int
read_int (const UCHAR_T * *pstr)
{
  unsigned int retval = **pstr - L_('0');

  while (ISDIGIT (*++(*pstr)))
    {
      retval *= 10;
      retval += **pstr - L_('0');
    }

  return retval;
}



/* Find the next spec in FORMAT, or the end of the string.  Returns
   a pointer into FORMAT, to a '%' or a '\0'.  */
static inline const UCHAR_T *
#ifdef COMPILE_WPRINTF
find_spec (const UCHAR_T *format)
#else
find_spec (const UCHAR_T *format, mbstate_t *ps)
#endif
{
#ifdef COMPILE_WPRINTF
  return (const UCHAR_T *) __wcschrnul ((const CHAR_T *) format, L'%');
#else
  while (*format != L_('\0') && *format != L_('%'))
    {
      int len;

      /* Remove any hints of a wrong encoding.  */
      ps->__count = 0;
      if (! isascii (*format) && (len = __mbrlen (format, MB_CUR_MAX, ps)) > 0)
	format += len;
      else
	++format;
    }
  return format;
#endif
}


/* These are defined in reg-printf.c.  */
extern printf_arginfo_function *__printf_arginfo_table[];
extern printf_function **__printf_function_table;


/* FORMAT must point to a '%' at the beginning of a spec.  Fills in *SPEC
   with the parsed details.  POSN is the number of arguments already
   consumed.  At most MAXTYPES - POSN types are filled in TYPES.  Return
   the number of args consumed by this spec; *MAX_REF_ARG is updated so it
   remains the highest argument index used.  */
static inline size_t
#ifdef COMPILE_WPRINTF
parse_one_spec (const UCHAR_T *format, size_t posn, struct printf_spec *spec,
		size_t *max_ref_arg)
#else
parse_one_spec (const UCHAR_T *format, size_t posn, struct printf_spec *spec,
		size_t *max_ref_arg, mbstate_t *ps)
#endif
{
  unsigned int n;
  size_t nargs = 0;

  /* Skip the '%'.  */
  ++format;

  /* Clear information structure.  */
  spec->data_arg = -1;
  spec->info.alt = 0;
  spec->info.space = 0;
  spec->info.left = 0;
  spec->info.showsign = 0;
  spec->info.group = 0;
  spec->info.i18n = 0;
  spec->info.pad = ' ';
  spec->info.wide = sizeof (UCHAR_T) > 1;

  /* Test for positional argument.  */
  if (ISDIGIT (*format))
    {
      const UCHAR_T *begin = format;

      n = read_int (&format);

      if (n > 0 && *format == L_('$'))
	/* Is positional parameter.  */
	{
	  ++format;		/* Skip the '$'.  */
	  spec->data_arg = n - 1;
	  *max_ref_arg = MAX (*max_ref_arg, n);
	}
      else
	/* Oops; that was actually the width and/or 0 padding flag.
	   Step back and read it again.  */
	format = begin;
    }

  /* Check for spec modifiers.  */
  do
    {
      switch (*format)
	{
	case L_(' '):
	  /* Output a space in place of a sign, when there is no sign.  */
	  spec->info.space = 1;
	  continue;
	case L_('+'):
	  /* Always output + or - for numbers.  */
	  spec->info.showsign = 1;
	  continue;
	case L_('-'):
	  /* Left-justify things.  */
	  spec->info.left = 1;
	  continue;
	case L_('#'):
	  /* Use the "alternate form":
	     Hex has 0x or 0X, FP always has a decimal point.  */
	  spec->info.alt = 1;
	  continue;
	case L_('0'):
	  /* Pad with 0s.  */
	  spec->info.pad = '0';
	  continue;
	case L_('\''):
	  /* Show grouping in numbers if the locale information
	     indicates any.  */
	  spec->info.group = 1;
	  continue;
	case L_('I'):
	  /* Use the internationalized form of the output.  Currently
	     means to use the `outdigits' of the current locale.  */
	  spec->info.i18n = 1;
	  continue;
	default:
	  break;
	}
      break;
    }
  while (*++format);

  if (spec->info.left)
    spec->info.pad = ' ';

  /* Get the field width.  */
  spec->width_arg = -1;
  spec->info.width = 0;
  if (*format == L_('*'))
    {
      /* The field width is given in an argument.
	 A negative field width indicates left justification.  */
      const UCHAR_T *begin = ++format;

      if (ISDIGIT (*format))
	{
	  /* The width argument might be found in a positional parameter.  */
	  n = read_int (&format);

	  if (n > 0 && *format == L_('$'))
	    {
	      spec->width_arg = n - 1;
	      *max_ref_arg = MAX (*max_ref_arg, n);
	      ++format;		/* Skip '$'.  */
	    }
	}

      if (spec->width_arg < 0)
	{
	  /* Not in a positional parameter.  Consume one argument.  */
	  spec->width_arg = posn++;
	  ++nargs;
	  format = begin;	/* Step back and reread.  */
	}
    }
  else if (ISDIGIT (*format))
    /* Constant width specification.  */
    spec->info.width = read_int (&format);

  /* Get the precision.  */
  spec->prec_arg = -1;
  /* -1 means none given; 0 means explicit 0.  */
  spec->info.prec = -1;
  if (*format == L_('.'))
    {
      ++format;
      if (*format == L_('*'))
	{
	  /* The precision is given in an argument.  */
	  const UCHAR_T *begin = ++format;

	  if (ISDIGIT (*format))
	    {
	      n = read_int (&format);

	      if (n > 0 && *format == L_('$'))
		{
		  spec->prec_arg = n - 1;
		  *max_ref_arg = MAX (*max_ref_arg, n);
		  ++format;
		}
	    }

	  if (spec->prec_arg < 0)
	    {
	      /* Not in a positional parameter.  */
	      spec->prec_arg = posn++;
	      ++nargs;
	      format = begin;
	    }
	}
      else if (ISDIGIT (*format))
	spec->info.prec = read_int (&format);
      else
	/* "%.?" is treated like "%.0?".  */
	spec->info.prec = 0;
    }

  /* Check for type modifiers.  */
  spec->info.is_long_double = 0;
  spec->info.is_short = 0;
  spec->info.is_long = 0;
  spec->info.is_char = 0;

  switch (*format++)
    {
    case L_('h'):
      /* ints are short ints or chars.  */
      if (*format != L_('h'))
	spec->info.is_short = 1;
      else
	{
	  ++format;
	  spec->info.is_char = 1;
	}
      break;
    case L_('l'):
      /* ints are long ints.  */
      spec->info.is_long = 1;
      if (*format != L_('l'))
	break;
      ++format;
      /* FALLTHROUGH */
    case L_('L'):
      /* doubles are long doubles, and ints are long long ints.  */
    case L_('q'):
      /* 4.4 uses this for long long.  */
      spec->info.is_long_double = 1;
      break;
    case L_('z'):
    case L_('Z'):
      /* ints are size_ts.  */
      assert (sizeof (size_t) <= sizeof (unsigned long long int));
#if LONG_MAX != LONG_LONG_MAX
      spec->info.is_long_double = sizeof (size_t) > sizeof (unsigned long int);
#endif
      spec->info.is_long = sizeof (size_t) > sizeof (unsigned int);
      break;
    case L_('t'):
      assert (sizeof (ptrdiff_t) <= sizeof (long long int));
#if LONG_MAX != LONG_LONG_MAX
      spec->info.is_long_double = (sizeof (ptrdiff_t) > sizeof (long int));
#endif
      spec->info.is_long = sizeof (ptrdiff_t) > sizeof (int);
      break;
    case L_('j'):
      assert (sizeof (uintmax_t) <= sizeof (unsigned long long int));
#if LONG_MAX != LONG_LONG_MAX
      spec->info.is_long_double = (sizeof (uintmax_t)
				   > sizeof (unsigned long int));
#endif
      spec->info.is_long = sizeof (uintmax_t) > sizeof (unsigned int);
      break;
    default:
      /* Not a recognized modifier.  Backup.  */
      --format;
      break;
    }

  /* Get the format specification.  */
  spec->info.spec = (wchar_t) *format++;
  if (__printf_function_table != NULL
      && spec->info.spec <= UCHAR_MAX
      && __printf_arginfo_table[spec->info.spec] != NULL)
    /* We don't try to get the types for all arguments if the format
       uses more than one.  The normal case is covered though.  */
    spec->ndata_args = (*__printf_arginfo_table[spec->info.spec])
      (&spec->info, 1, &spec->data_arg_type);
  else
    {
      /* Find the data argument types of a built-in spec.  */
      spec->ndata_args = 1;

      switch (spec->info.spec)
	{
	case L'i':
	case L'd':
	case L'u':
	case L'o':
	case L'X':
	case L'x':
#if LONG_MAX != LONG_LONG_MAX
	  if (spec->info.is_long_double)
	    spec->data_arg_type = PA_INT|PA_FLAG_LONG_LONG;
	  else
#endif
	    if (spec->info.is_long)
	      spec->data_arg_type = PA_INT|PA_FLAG_LONG;
	    else if (spec->info.is_short)
	      spec->data_arg_type = PA_INT|PA_FLAG_SHORT;
	    else if (spec->info.is_char)
	      spec->data_arg_type = PA_CHAR;
	    else
	      spec->data_arg_type = PA_INT;
	  break;
	case L'e':
	case L'E':
	case L'f':
	case L'F':
	case L'g':
	case L'G':
	case L'a':
	case L'A':
	  if (spec->info.is_long_double)
	    spec->data_arg_type = PA_DOUBLE|PA_FLAG_LONG_DOUBLE;
	  else
	    spec->data_arg_type = PA_DOUBLE;
	  break;
	case L'c':
	  spec->data_arg_type = PA_CHAR;
	  break;
	case L'C':
	  spec->data_arg_type = PA_WCHAR;
	  break;
	case L's':
	  spec->data_arg_type = PA_STRING;
	  break;
	case L'S':
	  spec->data_arg_type = PA_WSTRING;
	  break;
	case L'p':
	  spec->data_arg_type = PA_POINTER;
	  break;
	case L'n':
	  spec->data_arg_type = PA_INT|PA_FLAG_PTR;
	  break;

	case L'm':
	default:
	  /* An unknown spec will consume no args.  */
	  spec->ndata_args = 0;
	  break;
	}
    }

  if (spec->data_arg == -1 && spec->ndata_args > 0)
    {
      /* There are args consumed, but no positional spec.  Use the
	 next sequential arg position.  */
      spec->data_arg = posn;
      nargs += spec->ndata_args;
    }

  if (spec->info.spec == L'\0')
    /* Format ended before this spec was complete.  */
    spec->end_of_fmt = spec->next_fmt = format - 1;
  else
    {
      /* Find the next format spec.  */
      spec->end_of_fmt = format;
#ifdef COMPILE_WPRINTF
      spec->next_fmt = find_spec (format);
#else
      spec->next_fmt = find_spec (format, ps);
#endif
    }

  return nargs;
}
