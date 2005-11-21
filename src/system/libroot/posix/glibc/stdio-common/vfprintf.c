/* Copyright (C) 1991-1999, 2000, 2001, 2002 Free Software Foundation, Inc.
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

#include <ctype.h>
#include <limits.h>
#include <printf.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <wchar.h>
#include <bits/libc-lock.h>
#include <sys/param.h>
#include "_itoa.h"
#include <locale/localeinfo.h>
#include <stdio.h>

/* This code is shared between the standard stdio implementation found
   in GNU C library and the libio implementation originally found in
   GNU libg++.

   Beside this it is also shared between the normal and wide character
   implementation as defined in ISO/IEC 9899:1990/Amendment 1:1995.  */


#ifdef USE_IN_LIBIO
/* This code is for use in libio.  */
# include <libioP.h>
# define FILE		_IO_FILE
# undef va_list
# define va_list	_IO_va_list
# undef BUFSIZ
# define BUFSIZ		_IO_BUFSIZ
# define ARGCHECK(S, Format) \
  do									      \
    {									      \
      /* Check file argument for consistence.  */			      \
      CHECK_FILE (S, -1);						      \
      if (S->_flags & _IO_NO_WRITES)					      \
	{								      \
	  __set_errno (EBADF);						      \
	  return -1;							      \
	}								      \
      if (Format == NULL)						      \
	{								      \
	  MAYBE_SET_EINVAL;						      \
	  return -1;							      \
	}								      \
    } while (0)
# define UNBUFFERED_P(S) ((S)->_IO_file_flags & _IO_UNBUFFERED)

# ifndef COMPILE_WPRINTF
#  define vfprintf	_IO_vfprintf
#  define CHAR_T	char
#  define UCHAR_T	unsigned char
#  define INT_T		int
#  define L_(Str)	Str
#  define ISDIGIT(Ch)	((unsigned int) ((Ch) - '0') < 10)

#  define PUT(F, S, N)	_IO_sputn ((F), (S), (N))
#  define PAD(Padchar) \
  if (width > 0)							      \
    done += INTUSE(_IO_padn) (s, (Padchar), width)
#  define PUTC(C, F)	_IO_putc_unlocked (C, F)
#  define ORIENT	if (s->_vtable_offset == 0 && _IO_fwide (s, -1) != -1)\
			  return -1
# else
#  define vfprintf	_IO_vfwprintf
#  define CHAR_T	wchar_t
/* This is a hack!!!  There should be a type uwchar_t.  */
#  define UCHAR_T	unsigned int /* uwchar_t */
#  define INT_T		wint_t
#  define L_(Str)	L##Str
#  define ISDIGIT(Ch)	((unsigned int) ((Ch) - L'0') < 10)

#  include "_itowa.h"

#  define PUT(F, S, N)	_IO_sputn ((F), (S), (N))
#  define PAD(Padchar) \
  if (width > 0)							      \
    done += _IO_wpadn (s, (Padchar), width)
#  define PUTC(C, F)	_IO_putwc_unlocked (C, F)
#  define ORIENT	if (_IO_fwide (s, 1) != 1) return -1

#  define _itoa(Val, Buf, Base, Case) _itowa (Val, Buf, Base, Case)
#  define _itoa_word(Val, Buf, Base, Case) _itowa_word (Val, Buf, Base, Case)
#  undef EOF
#  define EOF WEOF
# endif
#else /* ! USE_IN_LIBIO */
/* This code is for use in the GNU C library.  */
# define ARGCHECK(S, Format) \
  do									      \
    {									      \
      /* Check file argument for consistence.  */			      \
      if (!__validfp (S) || !S->__mode.__write)				      \
	{								      \
	  __set_errno (EBADF);						      \
	  return -1;							      \
	}								      \
      if (Format == NULL)						      \
	{								      \
	  __set_errno (EINVAL);						      \
	  return -1;							      \
	}								      \
      if (!S->__seen)							      \
	{								      \
	  if (__flshfp (S, EOF) == EOF)					      \
	    return -1;							      \
	}								      \
    }									      \
   while (0)
# define UNBUFFERED_P(s) ((s)->__buffer == NULL)

# define CHAR_T         char
# define UCHAR_T        unsigned char
# define INT_T		int
# define L_(Str)	Str
# define ISDIGIT(Ch)	isdigit (Ch)

# define PUT(F, S, N)	fwrite (S, 1, N, F)
ssize_t __printf_pad __P ((FILE *, char pad, size_t n));
# define PAD(Padchar)                                                         \
  if (width > 0)                                                              \
    { ssize_t __res = __printf_pad (s, (Padchar), width);                     \
      if (__res == -1)                                                        \
        {                                                                     \
          done = -1;                                                          \
          goto all_done;                                                      \
        }                                                                     \
      done += __res; }
# define PUTC(C, F)    putc (C, F)

/* XXX These declarations should go as soon as the stdio header files
   have these prototypes.   */
extern void __flockfile (FILE *);
extern void __funlockfile (FILE *);
#endif /* USE_IN_LIBIO */

#include "_i18n_number.h"

/* Include the shared code for parsing the format string.  */
#include "printf-parse.h"


#define	outchar(Ch)							      \
  do									      \
    {									      \
      register const INT_T outc = (Ch);					      \
      if (PUTC (outc, s) == EOF)					      \
	{								      \
	  done = -1;							      \
	  goto all_done;						      \
	}								      \
      else								      \
	++done;								      \
    }									      \
  while (0)

#define outstring(String, Len)						      \
  do									      \
    {									      \
      if ((size_t) PUT (s, (String), (Len)) != (size_t) (Len))		      \
	{								      \
	  done = -1;							      \
	  goto all_done;						      \
	}								      \
      done += (Len);							      \
    }									      \
  while (0)

/* For handling long_double and longlong we use the same flag.  If
   `long' and `long long' are effectively the same type define it to
   zero.  */
#if LONG_MAX == LONG_LONG_MAX
# define is_longlong 0
#else
# define is_longlong is_long_double
#endif

/* If `long' and `int' is effectively the same type we don't have to
   handle `long separately.  */
#if INT_MAX == LONG_MAX
# define is_long_num	0
#else
# define is_long_num	is_long
#endif


/* Global variables.  */
static const CHAR_T null[] = L_("(null)");


/* Helper function to provide temporary buffering for unbuffered streams.  */
static int buffered_vfprintf __P ((FILE *stream, const CHAR_T *fmt, va_list))
     internal_function;

/* Handle unknown format specifier.  */
static int printf_unknown __P ((FILE *, const struct printf_info *,
				const void *const *));

/* Group digits of number string.  */
#ifdef COMPILE_WPRINTF
static CHAR_T *group_number __P ((CHAR_T *, CHAR_T *, const char *, wchar_t))
     internal_function;
#else
static CHAR_T *group_number __P ((CHAR_T *, CHAR_T *, const char *,
				  const char *)) internal_function;
#endif


/* The function itself.  */
int
vfprintf (FILE *s, const CHAR_T *format, va_list ap)
{
  /* The character used as thousands separator.  */
#ifdef COMPILE_WPRINTF
  wchar_t thousands_sep = L'\0';
#else
  const char *thousands_sep = NULL;
#endif

  /* The string describing the size of groups of digits.  */
  const char *grouping;

  /* Place to accumulate the result.  */
  int done;

  /* Current character in format string.  */
  const UCHAR_T *f;

  /* End of leading constant string.  */
  const UCHAR_T *lead_str_end;

  /* Points to next format specifier.  */
  const UCHAR_T *end_of_spec;

  /* Buffer intermediate results.  */
  CHAR_T work_buffer[1000];
  CHAR_T *workstart = NULL;
  CHAR_T *workend;

  /* State for restartable multibyte character handling functions.  */
#ifndef COMPILE_WPRINTF
  mbstate_t mbstate;
#endif

  /* We have to save the original argument pointer.  */
  va_list ap_save;

  /* Count number of specifiers we already processed.  */
  int nspecs_done;

  /* For the %m format we may need the current `errno' value.  */
  int save_errno = errno;


  /* This table maps a character into a number representing a
     class.  In each step there is a destination label for each
     class.  */
  static const int jump_table[] =
  {
    /* ' ' */  1,            0,            0, /* '#' */  4,
	       0, /* '%' */ 14,            0, /* '\''*/  6,
	       0,            0, /* '*' */  7, /* '+' */  2,
	       0, /* '-' */  3, /* '.' */  9,            0,
    /* '0' */  5, /* '1' */  8, /* '2' */  8, /* '3' */  8,
    /* '4' */  8, /* '5' */  8, /* '6' */  8, /* '7' */  8,
    /* '8' */  8, /* '9' */  8,            0,            0,
	       0,            0,            0,            0,
	       0, /* 'A' */ 26,            0, /* 'C' */ 25,
	       0, /* 'E' */ 19, /* F */   19, /* 'G' */ 19,
	       0, /* 'I' */ 29,            0,            0,
    /* 'L' */ 12,            0,            0,            0,
	       0,            0,            0, /* 'S' */ 21,
	       0,            0,            0,            0,
    /* 'X' */ 18,            0, /* 'Z' */ 13,            0,
	       0,            0,            0,            0,
	       0, /* 'a' */ 26,            0, /* 'c' */ 20,
    /* 'd' */ 15, /* 'e' */ 19, /* 'f' */ 19, /* 'g' */ 19,
    /* 'h' */ 10, /* 'i' */ 15, /* 'j' */ 28,            0,
    /* 'l' */ 11, /* 'm' */ 24, /* 'n' */ 23, /* 'o' */ 17,
    /* 'p' */ 22, /* 'q' */ 12,            0, /* 's' */ 21,
    /* 't' */ 27, /* 'u' */ 16,            0,            0,
    /* 'x' */ 18,            0, /* 'z' */ 13
  };

#define NOT_IN_JUMP_RANGE(Ch) ((Ch) < L_(' ') || (Ch) > L_('z'))
#define CHAR_CLASS(Ch) (jump_table[(INT_T) (Ch) - L_(' ')])
#if defined HAVE_SUBTRACT_LOCAL_LABELS && defined SHARED
  /* 'int' is enough and it saves some space on 64 bit systems.  */
# define JUMP_TABLE_TYPE const int
# define JUMP(ChExpr, table)						      \
      do								      \
	{								      \
	  int offset;							      \
	  void *__unbounded ptr;					      \
	  spec = (ChExpr);						      \
	  offset = NOT_IN_JUMP_RANGE (spec) ? REF (form_unknown)	      \
	    : table[CHAR_CLASS (spec)];					      \
	  ptr = &&do_form_unknown + offset;				      \
	  goto *ptr;							      \
	}								      \
      while (0)
#else
# define JUMP_TABLE_TYPE const void *const
# define JUMP(ChExpr, table)						      \
      do								      \
	{								      \
	  const void *__unbounded ptr;					      \
	  spec = (ChExpr);						      \
	  ptr = NOT_IN_JUMP_RANGE (spec) ? REF (form_unknown)		      \
	    : table[CHAR_CLASS (spec)];					      \
	  goto *ptr;							      \
	}								      \
      while (0)
#endif

#define STEP0_3_TABLE							      \
    /* Step 0: at the beginning.  */					      \
    static JUMP_TABLE_TYPE step0_jumps[30] =				      \
    {									      \
      REF (form_unknown),						      \
      REF (flag_space),		/* for ' ' */				      \
      REF (flag_plus),		/* for '+' */				      \
      REF (flag_minus),		/* for '-' */				      \
      REF (flag_hash),		/* for '<hash>' */			      \
      REF (flag_zero),		/* for '0' */				      \
      REF (flag_quote),		/* for '\'' */				      \
      REF (width_asterics),	/* for '*' */				      \
      REF (width),		/* for '1'...'9' */			      \
      REF (precision),		/* for '.' */				      \
      REF (mod_half),		/* for 'h' */				      \
      REF (mod_long),		/* for 'l' */				      \
      REF (mod_longlong),	/* for 'L', 'q' */			      \
      REF (mod_size_t),		/* for 'z', 'Z' */			      \
      REF (form_percent),	/* for '%' */				      \
      REF (form_integer),	/* for 'd', 'i' */			      \
      REF (form_unsigned),	/* for 'u' */				      \
      REF (form_octal),		/* for 'o' */				      \
      REF (form_hexa),		/* for 'X', 'x' */			      \
      REF (form_float),		/* for 'E', 'e', 'F', 'f', 'G', 'g' */	      \
      REF (form_character),	/* for 'c' */				      \
      REF (form_string),	/* for 's', 'S' */			      \
      REF (form_pointer),	/* for 'p' */				      \
      REF (form_number),	/* for 'n' */				      \
      REF (form_strerror),	/* for 'm' */				      \
      REF (form_wcharacter),	/* for 'C' */				      \
      REF (form_floathex),	/* for 'A', 'a' */			      \
      REF (mod_ptrdiff_t),      /* for 't' */				      \
      REF (mod_intmax_t),       /* for 'j' */				      \
      REF (flag_i18n),	        /* for 'I' */				      \
    };									      \
    /* Step 1: after processing width.  */				      \
    static JUMP_TABLE_TYPE step1_jumps[30] =				      \
    {									      \
      REF (form_unknown),						      \
      REF (form_unknown),	/* for ' ' */				      \
      REF (form_unknown),	/* for '+' */				      \
      REF (form_unknown),	/* for '-' */				      \
      REF (form_unknown),	/* for '<hash>' */			      \
      REF (form_unknown),	/* for '0' */				      \
      REF (form_unknown),	/* for '\'' */				      \
      REF (form_unknown),	/* for '*' */				      \
      REF (form_unknown),	/* for '1'...'9' */			      \
      REF (precision),		/* for '.' */				      \
      REF (mod_half),		/* for 'h' */				      \
      REF (mod_long),		/* for 'l' */				      \
      REF (mod_longlong),	/* for 'L', 'q' */			      \
      REF (mod_size_t),		/* for 'z', 'Z' */			      \
      REF (form_percent),	/* for '%' */				      \
      REF (form_integer),	/* for 'd', 'i' */			      \
      REF (form_unsigned),	/* for 'u' */				      \
      REF (form_octal),		/* for 'o' */				      \
      REF (form_hexa),		/* for 'X', 'x' */			      \
      REF (form_float),		/* for 'E', 'e', 'F', 'f', 'G', 'g' */	      \
      REF (form_character),	/* for 'c' */				      \
      REF (form_string),	/* for 's', 'S' */			      \
      REF (form_pointer),	/* for 'p' */				      \
      REF (form_number),	/* for 'n' */				      \
      REF (form_strerror),	/* for 'm' */				      \
      REF (form_wcharacter),	/* for 'C' */				      \
      REF (form_floathex),	/* for 'A', 'a' */			      \
      REF (mod_ptrdiff_t),      /* for 't' */				      \
      REF (mod_intmax_t),       /* for 'j' */				      \
      REF (flag_i18n)	        /* for 'I' */				      \
    };									      \
    /* Step 2: after processing precision.  */				      \
    static JUMP_TABLE_TYPE step2_jumps[30] =				      \
    {									      \
      REF (form_unknown),						      \
      REF (form_unknown),	/* for ' ' */				      \
      REF (form_unknown),	/* for '+' */				      \
      REF (form_unknown),	/* for '-' */				      \
      REF (form_unknown),	/* for '<hash>' */			      \
      REF (form_unknown),	/* for '0' */				      \
      REF (form_unknown),	/* for '\'' */				      \
      REF (form_unknown),	/* for '*' */				      \
      REF (form_unknown),	/* for '1'...'9' */			      \
      REF (form_unknown),	/* for '.' */				      \
      REF (mod_half),		/* for 'h' */				      \
      REF (mod_long),		/* for 'l' */				      \
      REF (mod_longlong),	/* for 'L', 'q' */			      \
      REF (mod_size_t),		/* for 'z', 'Z' */			      \
      REF (form_percent),	/* for '%' */				      \
      REF (form_integer),	/* for 'd', 'i' */			      \
      REF (form_unsigned),	/* for 'u' */				      \
      REF (form_octal),		/* for 'o' */				      \
      REF (form_hexa),		/* for 'X', 'x' */			      \
      REF (form_float),		/* for 'E', 'e', 'F', 'f', 'G', 'g' */	      \
      REF (form_character),	/* for 'c' */				      \
      REF (form_string),	/* for 's', 'S' */			      \
      REF (form_pointer),	/* for 'p' */				      \
      REF (form_number),	/* for 'n' */				      \
      REF (form_strerror),	/* for 'm' */				      \
      REF (form_wcharacter),	/* for 'C' */				      \
      REF (form_floathex),	/* for 'A', 'a' */			      \
      REF (mod_ptrdiff_t),      /* for 't' */				      \
      REF (mod_intmax_t),       /* for 'j' */				      \
      REF (flag_i18n)	        /* for 'I' */				      \
    };									      \
    /* Step 3a: after processing first 'h' modifier.  */		      \
    static JUMP_TABLE_TYPE step3a_jumps[30] =				      \
    {									      \
      REF (form_unknown),						      \
      REF (form_unknown),	/* for ' ' */				      \
      REF (form_unknown),	/* for '+' */				      \
      REF (form_unknown),	/* for '-' */				      \
      REF (form_unknown),	/* for '<hash>' */			      \
      REF (form_unknown),	/* for '0' */				      \
      REF (form_unknown),	/* for '\'' */				      \
      REF (form_unknown),	/* for '*' */				      \
      REF (form_unknown),	/* for '1'...'9' */			      \
      REF (form_unknown),	/* for '.' */				      \
      REF (mod_halfhalf),	/* for 'h' */				      \
      REF (form_unknown),	/* for 'l' */				      \
      REF (form_unknown),	/* for 'L', 'q' */			      \
      REF (form_unknown),	/* for 'z', 'Z' */			      \
      REF (form_percent),	/* for '%' */				      \
      REF (form_integer),	/* for 'd', 'i' */			      \
      REF (form_unsigned),	/* for 'u' */				      \
      REF (form_octal),		/* for 'o' */				      \
      REF (form_hexa),		/* for 'X', 'x' */			      \
      REF (form_unknown),	/* for 'E', 'e', 'F', 'f', 'G', 'g' */	      \
      REF (form_unknown),	/* for 'c' */				      \
      REF (form_unknown),	/* for 's', 'S' */			      \
      REF (form_unknown),	/* for 'p' */				      \
      REF (form_number),	/* for 'n' */				      \
      REF (form_unknown),	/* for 'm' */				      \
      REF (form_unknown),	/* for 'C' */				      \
      REF (form_unknown),	/* for 'A', 'a' */			      \
      REF (form_unknown),       /* for 't' */				      \
      REF (form_unknown),       /* for 'j' */				      \
      REF (form_unknown)        /* for 'I' */				      \
    };									      \
    /* Step 3b: after processing first 'l' modifier.  */		      \
    static JUMP_TABLE_TYPE step3b_jumps[30] =				      \
    {									      \
      REF (form_unknown),						      \
      REF (form_unknown),	/* for ' ' */				      \
      REF (form_unknown),	/* for '+' */				      \
      REF (form_unknown),	/* for '-' */				      \
      REF (form_unknown),	/* for '<hash>' */			      \
      REF (form_unknown),	/* for '0' */				      \
      REF (form_unknown),	/* for '\'' */				      \
      REF (form_unknown),	/* for '*' */				      \
      REF (form_unknown),	/* for '1'...'9' */			      \
      REF (form_unknown),	/* for '.' */				      \
      REF (form_unknown),	/* for 'h' */				      \
      REF (mod_longlong),	/* for 'l' */				      \
      REF (form_unknown),	/* for 'L', 'q' */			      \
      REF (form_unknown),	/* for 'z', 'Z' */			      \
      REF (form_percent),	/* for '%' */				      \
      REF (form_integer),	/* for 'd', 'i' */			      \
      REF (form_unsigned),	/* for 'u' */				      \
      REF (form_octal),		/* for 'o' */				      \
      REF (form_hexa),		/* for 'X', 'x' */			      \
      REF (form_float),		/* for 'E', 'e', 'F', 'f', 'G', 'g' */	      \
      REF (form_character),	/* for 'c' */				      \
      REF (form_string),	/* for 's', 'S' */			      \
      REF (form_pointer),	/* for 'p' */				      \
      REF (form_number),	/* for 'n' */				      \
      REF (form_strerror),	/* for 'm' */				      \
      REF (form_wcharacter),	/* for 'C' */				      \
      REF (form_floathex),	/* for 'A', 'a' */			      \
      REF (form_unknown),       /* for 't' */				      \
      REF (form_unknown),       /* for 'j' */				      \
      REF (form_unknown)        /* for 'I' */				      \
    }

#define STEP4_TABLE							      \
    /* Step 4: processing format specifier.  */				      \
    static JUMP_TABLE_TYPE step4_jumps[30] =				      \
    {									      \
      REF (form_unknown),						      \
      REF (form_unknown),	/* for ' ' */				      \
      REF (form_unknown),	/* for '+' */				      \
      REF (form_unknown),	/* for '-' */				      \
      REF (form_unknown),	/* for '<hash>' */			      \
      REF (form_unknown),	/* for '0' */				      \
      REF (form_unknown),	/* for '\'' */				      \
      REF (form_unknown),	/* for '*' */				      \
      REF (form_unknown),	/* for '1'...'9' */			      \
      REF (form_unknown),	/* for '.' */				      \
      REF (form_unknown),	/* for 'h' */				      \
      REF (form_unknown),	/* for 'l' */				      \
      REF (form_unknown),	/* for 'L', 'q' */			      \
      REF (form_unknown),	/* for 'z', 'Z' */			      \
      REF (form_percent),	/* for '%' */				      \
      REF (form_integer),	/* for 'd', 'i' */			      \
      REF (form_unsigned),	/* for 'u' */				      \
      REF (form_octal),		/* for 'o' */				      \
      REF (form_hexa),		/* for 'X', 'x' */			      \
      REF (form_float),		/* for 'E', 'e', 'F', 'f', 'G', 'g' */	      \
      REF (form_character),	/* for 'c' */				      \
      REF (form_string),	/* for 's', 'S' */			      \
      REF (form_pointer),	/* for 'p' */				      \
      REF (form_number),	/* for 'n' */				      \
      REF (form_strerror),	/* for 'm' */				      \
      REF (form_wcharacter),	/* for 'C' */				      \
      REF (form_floathex),	/* for 'A', 'a' */			      \
      REF (form_unknown),       /* for 't' */				      \
      REF (form_unknown),       /* for 'j' */				      \
      REF (form_unknown)        /* for 'I' */				      \
    }


#define process_arg(fspec)						      \
      /* Start real work.  We know about all flags and modifiers and	      \
	 now process the wanted format specifier.  */			      \
    LABEL (form_percent):						      \
      /* Write a literal "%".  */					      \
      outchar (L_('%'));						      \
      break;								      \
									      \
    LABEL (form_integer):						      \
      /* Signed decimal integer.  */					      \
      base = 10;							      \
									      \
      if (is_longlong)							      \
	{								      \
	  long long int signed_number;					      \
									      \
	  if (fspec == NULL)						      \
	    signed_number = va_arg (ap, long long int);			      \
	  else								      \
	    signed_number = args_value[fspec->data_arg].pa_long_long_int;     \
									      \
	  is_negative = signed_number < 0;				      \
	  number.longlong = is_negative ? (- signed_number) : signed_number;  \
									      \
	  goto LABEL (longlong_number);					      \
	}								      \
      else								      \
	{								      \
	  long int signed_number;					      \
									      \
	  if (fspec == NULL)						      \
	    {								      \
	      if (is_long_num)						      \
		signed_number = va_arg (ap, long int);			      \
	      else  /* `char' and `short int' will be promoted to `int'.  */  \
		signed_number = va_arg (ap, int);			      \
	    }								      \
	  else								      \
	    if (is_long_num)						      \
	      signed_number = args_value[fspec->data_arg].pa_long_int;	      \
	    else if (!is_short)						      \
	      signed_number = args_value[fspec->data_arg].pa_int;	      \
	    else	      						      \
	      signed_number = args_value[fspec->data_arg].pa_short_int;	      \
									      \
	  is_negative = signed_number < 0;				      \
	  number.word = is_negative ? (- signed_number) : signed_number;      \
									      \
	  goto LABEL (number);						      \
	}								      \
      /* NOTREACHED */							      \
									      \
    LABEL (form_unsigned):						      \
      /* Unsigned decimal integer.  */					      \
      base = 10;							      \
      goto LABEL (unsigned_number);					      \
      /* NOTREACHED */							      \
									      \
    LABEL (form_octal):							      \
      /* Unsigned octal integer.  */					      \
      base = 8;								      \
      goto LABEL (unsigned_number);					      \
      /* NOTREACHED */							      \
									      \
    LABEL (form_hexa):							      \
      /* Unsigned hexadecimal integer.  */				      \
      base = 16;							      \
									      \
    LABEL (unsigned_number):	  /* Unsigned number of base BASE.  */	      \
									      \
      /* ISO specifies the `+' and ` ' flags only for signed		      \
	 conversions.  */						      \
      is_negative = 0;							      \
      showsign = 0;							      \
      space = 0;							      \
									      \
      if (is_longlong)							      \
	{								      \
	  if (fspec == NULL)						      \
	    number.longlong = va_arg (ap, unsigned long long int);	      \
	  else								      \
	    number.longlong = args_value[fspec->data_arg].pa_u_long_long_int; \
									      \
	LABEL (longlong_number):					      \
	  if (prec < 0)							      \
	    /* Supply a default precision if none was given.  */	      \
	    prec = 1;							      \
	  else								      \
	    /* We have to take care for the '0' flag.  If a precision	      \
	       is given it must be ignored.  */				      \
	    pad = L_(' ');						      \
									      \
	  /* If the precision is 0 and the number is 0 nothing has to	      \
	     be written for the number, except for the 'o' format in	      \
	     alternate form.  */					      \
	  if (prec == 0 && number.longlong == 0)			      \
	    {								      \
	      string = workend;						      \
	      if (base == 8 && alt)					      \
		*--string = L_('0');					      \
	    }								      \
	  else								      \
	    {								      \
	      /* Put the number in WORK.  */				      \
	      string = _itoa (number.longlong, workend, base,		      \
			      spec == L_('X'));				      \
	      if (group && grouping)					      \
		string = group_number (string, workend, grouping,	      \
				       thousands_sep);			      \
									      \
	      if (use_outdigits && base == 10)				      \
		string = _i18n_number_rewrite (string, workend);	      \
	    }								      \
	  /* Simplify further test for num != 0.  */			      \
	  number.word = number.longlong != 0;				      \
	}								      \
      else								      \
	{								      \
	  if (fspec == NULL)						      \
	    {								      \
	      if (is_long_num)						      \
		number.word = va_arg (ap, unsigned long int);		      \
	      else if (is_char)						      \
	        number.word = (unsigned char) va_arg (ap, unsigned int);      \
	      else if (!is_short)					      \
		number.word = va_arg (ap, unsigned int);		      \
	      else							      \
		number.word = (unsigned short int) va_arg (ap, unsigned int); \
	    }								      \
	  else								      \
	    if (is_long_num)						      \
	      number.word = args_value[fspec->data_arg].pa_u_long_int;	      \
	    else if (is_char)						      \
	      number.word = (unsigned char)				      \
		args_value[fspec->data_arg].pa_char;			      \
	    else if (!is_short)						      \
	      number.word = args_value[fspec->data_arg].pa_u_int;	      \
	    else							      \
	      number.word = (unsigned short int)			      \
		args_value[fspec->data_arg].pa_u_short_int;		      \
									      \
	LABEL (number):							      \
	  if (prec < 0)							      \
	    /* Supply a default precision if none was given.  */	      \
	    prec = 1;							      \
	  else								      \
	    /* We have to take care for the '0' flag.  If a precision	      \
	       is given it must be ignored.  */				      \
	    pad = L_(' ');						      \
									      \
	  /* If the precision is 0 and the number is 0 nothing has to	      \
	     be written for the number, except for the 'o' format in	      \
	     alternate form.  */					      \
	  if (prec == 0 && number.word == 0)				      \
	    {								      \
	      string = workend;						      \
	      if (base == 8 && alt)					      \
		*--string = L_('0');					      \
	    }								      \
	  else								      \
	    {								      \
	      /* Put the number in WORK.  */				      \
	      string = _itoa_word (number.word, workend, base,		      \
				   spec == L_('X'));			      \
	      if (group && grouping)					      \
		string = group_number (string, workend, grouping,	      \
				       thousands_sep);			      \
									      \
	      if (use_outdigits && base == 10)				      \
		string = _i18n_number_rewrite (string, workend);	      \
	    }								      \
	}								      \
									      \
      if (prec <= workend - string && number.word != 0 && alt && base == 8)   \
	/* Add octal marker.  */					      \
	*--string = L_('0');						      \
									      \
      prec = MAX (0, prec - (workend - string));			      \
									      \
      if (!left)							      \
	{								      \
	  width -= workend - string + prec;				      \
									      \
	  if (number.word != 0 && alt && base == 16)			      \
	    /* Account for 0X hex marker.  */				      \
	    width -= 2;							      \
									      \
	  if (is_negative || showsign || space)				      \
	    --width;							      \
									      \
	  if (pad == L_(' '))						      \
	    {								      \
	      PAD (L_(' '));						      \
	      width = 0;						      \
	    }								      \
									      \
	  if (is_negative)						      \
	    outchar (L_('-'));						      \
	  else if (showsign)						      \
	    outchar (L_('+'));						      \
	  else if (space)						      \
	    outchar (L_(' '));						      \
									      \
	  if (number.word != 0 && alt && base == 16)			      \
	    {								      \
	      outchar (L_('0'));					      \
	      outchar (spec);						      \
	    }								      \
									      \
	  width += prec;						      \
	  PAD (L_('0'));						      \
									      \
	  outstring (string, workend - string);				      \
									      \
	  break;							      \
	}								      \
      else								      \
	{								      \
	  if (is_negative)						      \
	    {								      \
	      outchar (L_('-'));					      \
	      --width;							      \
	    }								      \
	  else if (showsign)						      \
	    {								      \
	      outchar (L_('+'));					      \
	      --width;							      \
	    }								      \
	  else if (space)						      \
	    {								      \
	      outchar (L_(' '));					      \
	      --width;							      \
	    }								      \
									      \
	  if (number.word != 0 && alt && base == 16)			      \
	    {								      \
	      outchar (L_('0'));					      \
	      outchar (spec);						      \
	      width -= 2;						      \
	    }								      \
									      \
	  width -= workend - string + prec;				      \
									      \
	  if (prec > 0)							      \
	    {								      \
	      int temp = width;						      \
	      width = prec;						      \
	      PAD (L_('0'));;						      \
	      width = temp;						      \
	    }								      \
									      \
	  outstring (string, workend - string);				      \
									      \
	  PAD (L_(' '));						      \
	  break;							      \
	}								      \
									      \
    LABEL (form_float):							      \
      {									      \
	/* Floating-point number.  This is handled by printf_fp.c.  */	      \
	const void *ptr;						      \
	int function_done;						      \
									      \
	if (fspec == NULL)						      \
	  {								      \
	    struct printf_info info = { prec: prec,			      \
					width: width,			      \
					spec: spec,			      \
					is_long_double: is_long_double,	      \
					is_short: is_short,		      \
					is_long: is_long,		      \
					alt: alt,			      \
					space: space,			      \
					left: left,			      \
					showsign: showsign,		      \
					group: group,			      \
					pad: pad,			      \
					extra: 0,			      \
					wide: sizeof (CHAR_T) != 1 };	      \
									      \
	    if (is_long_double)						      \
	      the_arg.pa_long_double = va_arg (ap, long double);	      \
	    else							      \
	      the_arg.pa_double = va_arg (ap, double);			      \
	    ptr = (const void *) &the_arg;				      \
									      \
	    function_done = __printf_fp (s, &info, &ptr);		      \
	  }								      \
	else								      \
	  {								      \
	    ptr = (const void *) &args_value[fspec->data_arg];		      \
									      \
	    function_done = __printf_fp (s, &fspec->info, &ptr);	      \
	  }								      \
									      \
	if (function_done < 0)						      \
	  {								      \
	    /* Error in print handler.  */				      \
	    done = -1;							      \
	    goto all_done;						      \
	  }								      \
									      \
	done += function_done;						      \
      }									      \
      break;								      \
									      \
    LABEL (form_floathex):						      \
      {									      \
        /* Floating point number printed as hexadecimal number.  */	      \
	const void *ptr;						      \
	int function_done;						      \
									      \
	if (fspec == NULL)						      \
	  {								      \
	    struct printf_info info = { prec: prec,			      \
					width: width,			      \
					spec: spec,			      \
					is_long_double: is_long_double,	      \
					is_short: is_short,		      \
					is_long: is_long,		      \
					alt: alt,			      \
					space: space,			      \
					left: left,			      \
					showsign: showsign,		      \
					group: group,			      \
					pad: pad,			      \
					extra: 0,			      \
					wide: sizeof (CHAR_T) != 1 };	      \
									      \
	    if (is_long_double)						      \
	      the_arg.pa_long_double = va_arg (ap, long double);	      \
	    else							      \
	      the_arg.pa_double = va_arg (ap, double);			      \
	    ptr = (const void *) &the_arg;				      \
									      \
	    function_done = __printf_fphex (s, &info, &ptr);		      \
	  }								      \
	else								      \
	  {								      \
	    ptr = (const void *) &args_value[fspec->data_arg];		      \
									      \
	    function_done = __printf_fphex (s, &fspec->info, &ptr);	      \
	  }								      \
									      \
	if (function_done < 0)						      \
	  {								      \
	    /* Error in print handler.  */				      \
	    done = -1;							      \
	    goto all_done;						      \
	  }								      \
									      \
	done += function_done;						      \
      }									      \
      break;								      \
									      \
    LABEL (form_pointer):						      \
      /* Generic pointer.  */						      \
      {									      \
	const void *ptr;						      \
	if (fspec == NULL)						      \
	  ptr = va_arg (ap, void *);					      \
	else								      \
	  ptr = args_value[fspec->data_arg].pa_pointer;			      \
	if (ptr != NULL)						      \
	  {								      \
	    /* If the pointer is not NULL, write it as a %#x spec.  */	      \
	    base = 16;							      \
	    number.word = (unsigned long int) ptr;			      \
	    is_negative = 0;						      \
	    alt = 1;							      \
	    group = 0;							      \
	    spec = L_('x');						      \
	    goto LABEL (number);					      \
	  }								      \
	else								      \
	  {								      \
	    /* Write "(nil)" for a nil pointer.  */			      \
	    string = (CHAR_T *) L_("(nil)");				      \
	    /* Make sure the full string "(nil)" is printed.  */	      \
	    if (prec < 5)						      \
	      prec = 5;							      \
	    is_long = 0;	/* This is no wide-char string.  */	      \
	    goto LABEL (print_string);					      \
	  }								      \
      }									      \
      /* NOTREACHED */							      \
									      \
    LABEL (form_number):						      \
      /* Answer the count of characters written.  */			      \
      if (fspec == NULL)						      \
	{								      \
	  if (is_longlong)						      \
	    *(long long int *) va_arg (ap, void *) = done;		      \
	  else if (is_long_num)						      \
	    *(long int *) va_arg (ap, void *) = done;			      \
	  else if (is_char)						      \
	    *(char *) va_arg (ap, void *) = done;			      \
	  else if (!is_short)						      \
	    *(int *) va_arg (ap, void *) = done;			      \
	  else								      \
	    *(short int *) va_arg (ap, void *) = done;			      \
	}								      \
      else								      \
	if (is_longlong)						      \
	  *(long long int *) args_value[fspec->data_arg].pa_pointer = done;   \
	else if (is_long_num)						      \
	  *(long int *) args_value[fspec->data_arg].pa_pointer = done;	      \
	else if (is_char)						      \
	  *(char *) args_value[fspec->data_arg].pa_pointer = done;	      \
	else if (!is_short)						      \
	  *(int *) args_value[fspec->data_arg].pa_pointer = done;	      \
	else								      \
	  *(short int *) args_value[fspec->data_arg].pa_pointer = done;	      \
      break;								      \
									      \
    LABEL (form_strerror):						      \
      /* Print description of error ERRNO.  */				      \
      string =								      \
	(CHAR_T *) __strerror_r (save_errno, (char *) work_buffer,	      \
				 sizeof work_buffer);			      \
      is_long = 0;		/* This is no wide-char string.  */	      \
      goto LABEL (print_string)

#ifdef COMPILE_WPRINTF
# define process_string_arg(fspec) \
    LABEL (form_character):						      \
      /* Character.  */							      \
      if (is_long)							      \
	goto LABEL (form_wcharacter);					      \
      --width;	/* Account for the character itself.  */		      \
      if (!left)							      \
	PAD (L' ');							      \
      if (fspec == NULL)						      \
	outchar (__btowc ((unsigned char) va_arg (ap, int))); /* Promoted. */ \
      else								      \
	outchar (__btowc ((unsigned char)				      \
			  args_value[fspec->data_arg].pa_char));	      \
      if (left)								      \
	PAD (L' ');							      \
      break;								      \
									      \
    LABEL (form_wcharacter):						      \
      {									      \
	/* Wide character.  */						      \
	--width;							      \
	if (!left)							      \
	  PAD (L' ');							      \
        if (fspec == NULL)						      \
	  outchar (va_arg (ap, wchar_t));				      \
	else								      \
	  outchar (args_value[fspec->data_arg].pa_wchar);		      \
	if (left)							      \
	  PAD (L' ');							      \
      }									      \
      break;								      \
									      \
    LABEL (form_string):						      \
      {									      \
	size_t len;							      \
	int string_malloced;						      \
									      \
	/* The string argument could in fact be `char *' or `wchar_t *'.      \
	   But this should not make a difference here.  */		      \
	if (fspec == NULL)						      \
	  string = (CHAR_T *) va_arg (ap, const wchar_t *);		      \
	else								      \
	  string = (CHAR_T *) args_value[fspec->data_arg].pa_wstring;	      \
									      \
	/* Entry point for printing other strings.  */			      \
      LABEL (print_string):						      \
									      \
	string_malloced = 0;						      \
	if (string == NULL)						      \
	  {								      \
	    /* Write "(null)" if there's space.  */			      \
	    if (prec == -1						      \
		|| prec >= (int) (sizeof (null) / sizeof (null[0])) - 1)      \
	      {								      \
		string = (CHAR_T *) null;				      \
		len = (sizeof (null) / sizeof (null[0])) - 1;		      \
	      }								      \
	    else							      \
	      {								      \
		string = (CHAR_T *) L"";				      \
		len = 0;						      \
	      }								      \
	  }								      \
	else if (!is_long && spec != L_('S'))				      \
	  {								      \
	    /* This is complicated.  We have to transform the multibyte	      \
	       string into a wide character string.  */			      \
	    const char *mbs = (const char *) string;			      \
	    mbstate_t mbstate;						      \
									      \
	    len = prec != -1 ? (size_t) prec : strlen (mbs);		      \
									      \
	    /* Allocate dynamically an array which definitely is long	      \
	       enough for the wide character version.  */		      \
	    if (len < 8192)						      \
	      string = (CHAR_T *) alloca (len * sizeof (wchar_t));	      \
	    else if ((string = (CHAR_T *) malloc (len * sizeof (wchar_t)))    \
		     == NULL)						      \
	      {								      \
		done = -1;						      \
		goto all_done;						      \
	      }								      \
	    else							      \
	      string_malloced = 1;					      \
									      \
	    memset (&mbstate, '\0', sizeof (mbstate_t));		      \
	    len = __mbsrtowcs (string, &mbs, len, &mbstate);		      \
	    if (len == (size_t) -1)					      \
	      {								      \
		/* Illegal multibyte character.  */			      \
		done = -1;						      \
		goto all_done;						      \
	      }								      \
	  }								      \
	else								      \
	  {								      \
	    if (prec != -1)						      \
	      /* Search for the end of the string, but don't search past      \
		 the length specified by the precision.  */		      \
	      len = __wcsnlen (string, prec);				      \
	    else							      \
	      len = __wcslen (string);					      \
	  }								      \
									      \
	if ((width -= len) < 0)						      \
	  {								      \
	    outstring (string, len);					      \
	    break;							      \
	  }								      \
									      \
	if (!left)							      \
	  PAD (L' ');							      \
	outstring (string, len);					      \
	if (left)							      \
	  PAD (L' ');							      \
	if (__builtin_expect (string_malloced, 0))			      \
	  free (string);						      \
      }									      \
      break;
#else
# define process_string_arg(fspec) \
    LABEL (form_character):						      \
      /* Character.  */							      \
      if (is_long)							      \
	goto LABEL (form_wcharacter);					      \
      --width;	/* Account for the character itself.  */		      \
      if (!left)							      \
	PAD (' ');							      \
      if (fspec == NULL)						      \
	outchar ((unsigned char) va_arg (ap, int)); /* Promoted.  */	      \
      else								      \
	outchar ((unsigned char) args_value[fspec->data_arg].pa_char);	      \
      if (left)								      \
	PAD (' ');							      \
      break;								      \
									      \
    LABEL (form_wcharacter):						      \
      {									      \
	/* Wide character.  */						      \
	char buf[MB_CUR_MAX];						      \
	mbstate_t mbstate;						      \
	size_t len;							      \
									      \
	memset (&mbstate, '\0', sizeof (mbstate_t));			      \
	len = __wcrtomb (buf, (fspec == NULL ? va_arg (ap, wchar_t)	      \
			       : args_value[fspec->data_arg].pa_wchar),	      \
			 &mbstate);					      \
	if (len == (size_t) -1)						      \
	  {								      \
	    /* Something went wron gduring the conversion.  Bail out.  */     \
	    done = -1;							      \
	    goto all_done;						      \
	  }								      \
	width -= len;							      \
	if (!left)							      \
	  PAD (' ');							      \
	outstring (buf, len);						      \
	if (left)							      \
	  PAD (' ');							      \
      }									      \
      break;								      \
									      \
    LABEL (form_string):						      \
      {									      \
	size_t len;							      \
	int string_malloced;						      \
									      \
	/* The string argument could in fact be `char *' or `wchar_t *'.      \
	   But this should not make a difference here.  */		      \
	if (fspec == NULL)						      \
	  string = (char *) va_arg (ap, const char *);			      \
	else								      \
	  string = (char *) args_value[fspec->data_arg].pa_string;	      \
									      \
	/* Entry point for printing other strings.  */			      \
      LABEL (print_string):						      \
									      \
	string_malloced = 0;						      \
	if (string == NULL)						      \
	  {								      \
	    /* Write "(null)" if there's space.  */			      \
	    if (prec == -1 || prec >= (int) sizeof (null) - 1)		      \
	      {								      \
		string = (char *) null;					      \
		len = sizeof (null) - 1;				      \
	      }								      \
	    else							      \
	      {								      \
		string = (char *) "";					      \
		len = 0;						      \
	      }								      \
	  }								      \
	else if (!is_long && spec != L_('S'))				      \
	  {								      \
	    if (prec != -1)						      \
	      {								      \
		/* Search for the end of the string, but don't search past    \
		   the length (in bytes) specified by the precision.  Also    \
		   don't use incomplete characters.  */			      \
		if (_NL_CURRENT_WORD (LC_CTYPE, _NL_CTYPE_MB_CUR_MAX) == 1)   \
		  len = __strnlen (string, prec);			      \
		else							      \
		  {							      \
		    /* In case we have a multibyte character set the	      \
		       situation is more compilcated.  We must not copy	      \
		       bytes at the end which form an incomplete character. */\
		    wchar_t ignore[prec];				      \
		    const char *str2 = string;				      \
		    mbstate_t ps;					      \
									      \
		    memset (&ps, '\0', sizeof (ps));			      \
		    if (__mbsnrtowcs (ignore, &str2, prec, prec, &ps)	      \
			== (size_t) -1)					      \
		      {							      \
			done = -1;					      \
			goto all_done;					      \
		      }							      \
		    if (str2 == NULL)					      \
		      len = strlen (string);				      \
		    else						      \
		      len = str2 - string - (ps.__count);		      \
		  }							      \
	      }								      \
	    else							      \
	      len = strlen (string);					      \
	  }								      \
	else								      \
	  {								      \
	    const wchar_t *s2 = (const wchar_t *) string;		      \
	    mbstate_t mbstate;						      \
									      \
	    memset (&mbstate, '\0', sizeof (mbstate_t));		      \
									      \
	    if (prec >= 0)						      \
	      {								      \
		/* The string `s2' might not be NUL terminated.  */	      \
		if (prec < 32768)					      \
		  string = (char *) alloca (prec);			      \
		else if ((string = (char *) malloc (prec)) == NULL)	      \
		  {							      \
		    done = -1;						      \
		    goto all_done;					      \
		  }							      \
		else							      \
		  string_malloced = 1;					      \
		len = __wcsrtombs (string, &s2, prec, &mbstate);	      \
	      }								      \
	    else							      \
	      {								      \
		len = __wcsrtombs (NULL, &s2, 0, &mbstate);		      \
		if (len != (size_t) -1)					      \
		  {							      \
		    assert (__mbsinit (&mbstate));			      \
		    s2 = (const wchar_t *) string;			      \
		    if (len + 1 < 32768)				      \
		      string = (char *) alloca (len + 1);		      \
		    else if ((string = (char *) malloc (len + 1)) == NULL)    \
		      {							      \
			done = -1;					      \
			goto all_done;					      \
		      }							      \
		    else						      \
		      string_malloced = 1;				      \
		    (void) __wcsrtombs (string, &s2, len + 1, &mbstate);      \
		  }							      \
	      }								      \
									      \
	    if (len == (size_t) -1)					      \
	      {								      \
	        /* Illegal wide-character string.  */			      \
		done = -1;						      \
		goto all_done;						      \
	      }								      \
	  }								      \
									      \
	if ((width -= len) < 0)						      \
	  {								      \
	    outstring (string, len);					      \
	    break;							      \
	  }								      \
									      \
	if (!left)							      \
	  PAD (' ');							      \
	outstring (string, len);					      \
	if (left)							      \
	  PAD (' ');							      \
	if (__builtin_expect (string_malloced, 0))			      \
	  free (string);						      \
      }									      \
      break;
#endif

  /* Orient the stream.  */
#ifdef ORIENT
  ORIENT;
#endif

  /* Sanity check of arguments.  */
  ARGCHECK (s, format);

#ifdef ORIENT
  /* Check for correct orientation.  */
  if (
# ifdef USE_IN_LIBIO
      s->_vtable_offset == 0 &&
# endif
      _IO_fwide (s, sizeof (CHAR_T) == 1 ? -1 : 1)
      != (sizeof (CHAR_T) == 1 ? -1 : 1))
    /* The stream is already oriented otherwise.  */
    return EOF;
#endif

  if (UNBUFFERED_P (s))
    /* Use a helper function which will allocate a local temporary buffer
       for the stream and then call us again.  */
    return buffered_vfprintf (s, format, ap);

  /* Initialize local variables.  */
  done = 0;
  grouping = (const char *) -1;
#ifdef __va_copy
  /* This macro will be available soon in gcc's <stdarg.h>.  We need it
     since on some systems `va_list' is not an integral type.  */
  __va_copy (ap_save, ap);
#else
  ap_save = ap;
#endif
  nspecs_done = 0;

#ifdef COMPILE_WPRINTF
  /* Find the first format specifier.  */
  f = lead_str_end = find_spec ((const UCHAR_T *) format);
#else
  /* Put state for processing format string in initial state.  */
  memset (&mbstate, '\0', sizeof (mbstate_t));

  /* Find the first format specifier.  */
  f = lead_str_end = find_spec (format, &mbstate);
#endif

  /* Lock stream.  */
#ifdef USE_IN_LIBIO
  __libc_cleanup_region_start (1, (void (*) (void *)) &_IO_funlockfile, s);
  _IO_flockfile (s);
#else
  __libc_cleanup_region_start (1, (void (*) (void *)) &__funlockfile, s);
  __flockfile (s);
#endif

  /* Write the literal text before the first format.  */
  outstring ((const UCHAR_T *) format,
	     lead_str_end - (const UCHAR_T *) format);

  /* If we only have to print a simple string, return now.  */
  if (*f == L_('\0'))
    goto all_done;

  /* Process whole format string.  */
  do
    {
#if defined HAVE_SUBTRACT_LOCAL_LABELS && defined SHARED
# define REF(Name) &&do_##Name - &&do_form_unknown
#else
# define REF(Name) &&do_##Name
#endif
#define LABEL(Name) do_##Name
      STEP0_3_TABLE;
      STEP4_TABLE;

      union printf_arg *args_value;	/* This is not used here but ... */
      int is_negative;	/* Flag for negative number.  */
      union
      {
	unsigned long long int longlong;
	unsigned long int word;
      } number;
      int base;
      union printf_arg the_arg;
      CHAR_T *string;	/* Pointer to argument string.  */
      int alt = 0;	/* Alternate format.  */
      int space = 0;	/* Use space prefix if no sign is needed.  */
      int left = 0;	/* Left-justify output.  */
      int showsign = 0;	/* Always begin with plus or minus sign.  */
      int group = 0;	/* Print numbers according grouping rules.  */
      int is_long_double = 0; /* Argument is long double/ long long int.  */
      int is_short = 0;	/* Argument is short int.  */
      int is_long = 0;	/* Argument is long int.  */
      int is_char = 0;	/* Argument is promoted (unsigned) char.  */
      int width = 0;	/* Width of output; 0 means none specified.  */
      int prec = -1;	/* Precision of output; -1 means none specified.  */
      /* This flag is set by the 'I' modifier and selects the use of the
	 `outdigits' as determined by the current locale.  */
      int use_outdigits = 0;
      UCHAR_T pad = L_(' ');/* Padding character.  */
      CHAR_T spec;

      workstart = NULL;
      workend = &work_buffer[sizeof (work_buffer) / sizeof (CHAR_T)];

      /* Get current character in format string.  */
      JUMP (*++f, step0_jumps);

      /* ' ' flag.  */
    LABEL (flag_space):
      space = 1;
      JUMP (*++f, step0_jumps);

      /* '+' flag.  */
    LABEL (flag_plus):
      showsign = 1;
      JUMP (*++f, step0_jumps);

      /* The '-' flag.  */
    LABEL (flag_minus):
      left = 1;
      pad = L_(' ');
      JUMP (*++f, step0_jumps);

      /* The '#' flag.  */
    LABEL (flag_hash):
      alt = 1;
      JUMP (*++f, step0_jumps);

      /* The '0' flag.  */
    LABEL (flag_zero):
      if (!left)
	pad = L_('0');
      JUMP (*++f, step0_jumps);

      /* The '\'' flag.  */
    LABEL (flag_quote):
      group = 1;

      if (grouping == (const char *) -1)
	{
#ifdef COMPILE_WPRINTF
	  thousands_sep = _NL_CURRENT_WORD (LC_NUMERIC,
					    _NL_NUMERIC_THOUSANDS_SEP_WC);
#else
	  thousands_sep = _NL_CURRENT (LC_NUMERIC, THOUSANDS_SEP);
#endif

	  grouping = _NL_CURRENT (LC_NUMERIC, GROUPING);
	  if (*grouping == '\0' || *grouping == CHAR_MAX
#ifdef COMPILE_WPRINTF
	      || thousands_sep == L'\0'
#else
	      || *thousands_sep == '\0'
#endif
	      )
	    grouping = NULL;
	}
      JUMP (*++f, step0_jumps);

    LABEL (flag_i18n):
      use_outdigits = 1;
      JUMP (*++f, step0_jumps);

      /* Get width from argument.  */
    LABEL (width_asterics):
      {
	const UCHAR_T *tmp;	/* Temporary value.  */

	tmp = ++f;
	if (ISDIGIT (*tmp) && read_int (&tmp) && *tmp == L_('$'))
	  /* The width comes from a positional parameter.  */
	  goto do_positional;

	width = va_arg (ap, int);

	/* Negative width means left justified.  */
	if (width < 0)
	  {
	    width = -width;
	    pad = L_(' ');
	    left = 1;
	  }

	if (width + 32 >= (int) (sizeof (work_buffer)
				 / sizeof (work_buffer[0])))
	  {
	    /* We have to use a special buffer.  The "32" is just a safe
	       bet for all the output which is not counted in the width.  */
	    if (width < (int) (32768 / sizeof (CHAR_T)))
	      workend = ((CHAR_T *) alloca ((width + 32) * sizeof (CHAR_T))
			 + (width + 32));
	    else
	      {
		workstart = (CHAR_T *) malloc ((width + 32) * sizeof (CHAR_T));
		if (workstart == NULL)
		  {
		    done = -1;
		    goto all_done;
		  }
		workend = workstart + (width + 32);
	      }
	  }
      }
      JUMP (*f, step1_jumps);

      /* Given width in format string.  */
    LABEL (width):
      width = read_int (&f);

      if (width + 32 >= (int) (sizeof (work_buffer) / sizeof (work_buffer[0])))
	{
	  /* We have to use a special buffer.  The "32" is just a safe
	     bet for all the output which is not counted in the width.  */
	  if (width < (int) (32768 / sizeof (CHAR_T)))
	    workend = ((CHAR_T *) alloca ((width + 32) * sizeof (CHAR_T))
		       + (width + 32));
	  else
	    {
	      workstart = (CHAR_T *) malloc ((width + 32) * sizeof (CHAR_T));
	      if (workstart == NULL)
		{
		  done = -1;
		  goto all_done;
		}
	      workend = workstart + (width + 32);
	    }
	}
      if (*f == L_('$'))
	/* Oh, oh.  The argument comes from a positional parameter.  */
	goto do_positional;
      JUMP (*f, step1_jumps);

    LABEL (precision):
      ++f;
      if (*f == L_('*'))
	{
	  const UCHAR_T *tmp;	/* Temporary value.  */

	  tmp = ++f;
	  if (ISDIGIT (*tmp) && read_int (&tmp) > 0 && *tmp == L_('$'))
	    /* The precision comes from a positional parameter.  */
	    goto do_positional;

	  prec = va_arg (ap, int);

	  /* If the precision is negative the precision is omitted.  */
	  if (prec < 0)
	    prec = -1;
	}
      else if (ISDIGIT (*f))
	prec = read_int (&f);
      else
	prec = 0;
      if (prec > width
	  && prec + 32 > (int)(sizeof (work_buffer) / sizeof (work_buffer[0])))
	{
	  if (prec < (int) (32768 / sizeof (CHAR_T)))
	    workend = alloca (prec + 32) + (prec + 32);
	  else
	    {
	      workstart = (CHAR_T *) malloc ((prec + 32) * sizeof (CHAR_T));
	      if (workstart == NULL)
		{
		  done = -1;
		  goto all_done;
		}
	      workend = workstart + (prec + 32);
	    }
	}
      JUMP (*f, step2_jumps);

      /* Process 'h' modifier.  There might another 'h' following.  */
    LABEL (mod_half):
      is_short = 1;
      JUMP (*++f, step3a_jumps);

      /* Process 'hh' modifier.  */
    LABEL (mod_halfhalf):
      is_short = 0;
      is_char = 1;
      JUMP (*++f, step4_jumps);

      /* Process 'l' modifier.  There might another 'l' following.  */
    LABEL (mod_long):
      is_long = 1;
      JUMP (*++f, step3b_jumps);

      /* Process 'L', 'q', or 'll' modifier.  No other modifier is
	 allowed to follow.  */
    LABEL (mod_longlong):
      is_long_double = 1;
      is_long = 1;
      JUMP (*++f, step4_jumps);

    LABEL (mod_size_t):
      is_long_double = sizeof (size_t) > sizeof (unsigned long int);
      is_long = sizeof (size_t) > sizeof (unsigned int);
      JUMP (*++f, step4_jumps);

    LABEL (mod_ptrdiff_t):
      is_long_double = sizeof (ptrdiff_t) > sizeof (unsigned long int);
      is_long = sizeof (ptrdiff_t) > sizeof (unsigned int);
      JUMP (*++f, step4_jumps);

    LABEL (mod_intmax_t):
      is_long_double = sizeof (intmax_t) > sizeof (unsigned long int);
      is_long = sizeof (intmax_t) > sizeof (unsigned int);
      JUMP (*++f, step4_jumps);

      /* Process current format.  */
      while (1)
	{
	  process_arg (((struct printf_spec *) NULL));
	  process_string_arg (((struct printf_spec *) NULL));

	LABEL (form_unknown):
	  if (spec == L_('\0'))
	    {
	      /* The format string ended before the specifier is complete.  */
	      done = -1;
	      goto all_done;
	    }

	  /* If we are in the fast loop force entering the complicated
	     one.  */
	  goto do_positional;
	}

      /* The format is correctly handled.  */
      ++nspecs_done;

      if (__builtin_expect (workstart != NULL, 0))
	free (workstart);
      workstart = NULL;

      /* Look for next format specifier.  */
#ifdef COMPILE_WPRINTF
      f = find_spec ((end_of_spec = ++f));
#else
      f = find_spec ((end_of_spec = ++f), &mbstate);
#endif

      /* Write the following constant string.  */
      outstring (end_of_spec, f - end_of_spec);
    }
  while (*f != L_('\0'));

  /* Unlock stream and return.  */
  goto all_done;

  /* Here starts the more complex loop to handle positional parameters.  */
do_positional:
  {
    /* Array with information about the needed arguments.  This has to
       be dynamically extensible.  */
    size_t nspecs = 0;
    size_t nspecs_max = 32;	/* A more or less arbitrary start value.  */
    struct printf_spec *specs
      = alloca (nspecs_max * sizeof (struct printf_spec));

    /* The number of arguments the format string requests.  This will
       determine the size of the array needed to store the argument
       attributes.  */
    size_t nargs = 0;
    int *args_type;
    union printf_arg *args_value = NULL;

    /* Positional parameters refer to arguments directly.  This could
       also determine the maximum number of arguments.  Track the
       maximum number.  */
    size_t max_ref_arg = 0;

    /* Just a counter.  */
    size_t cnt;


    if (grouping == (const char *) -1)
      {
#ifdef COMPILE_WPRINTF
	thousands_sep = _NL_CURRENT_WORD (LC_NUMERIC,
					  _NL_NUMERIC_THOUSANDS_SEP_WC);
#else
	thousands_sep = _NL_CURRENT (LC_NUMERIC, THOUSANDS_SEP);
#endif

	grouping = _NL_CURRENT (LC_NUMERIC, GROUPING);
	if (*grouping == '\0' || *grouping == CHAR_MAX)
	  grouping = NULL;
      }

    for (f = lead_str_end; *f != L_('\0'); f = specs[nspecs++].next_fmt)
      {
	if (nspecs >= nspecs_max)
	  {
	    /* Extend the array of format specifiers.  */
	    struct printf_spec *old = specs;

	    nspecs_max *= 2;
	    specs = alloca (nspecs_max * sizeof (struct printf_spec));

	    if (specs == &old[nspecs])
	      /* Stack grows up, OLD was the last thing allocated;
		 extend it.  */
	      nspecs_max += nspecs_max / 2;
	    else
	      {
		/* Copy the old array's elements to the new space.  */
		memcpy (specs, old, nspecs * sizeof (struct printf_spec));
		if (old == &specs[nspecs])
		  /* Stack grows down, OLD was just below the new
		     SPECS.  We can use that space when the new space
		     runs out.  */
		  nspecs_max += nspecs_max / 2;
	      }
	  }

	/* Parse the format specifier.  */
#ifdef COMPILE_WPRINTF
	nargs += parse_one_spec (f, nargs, &specs[nspecs], &max_ref_arg);
#else
	nargs += parse_one_spec (f, nargs, &specs[nspecs], &max_ref_arg,
				 &mbstate);
#endif
      }

    /* Determine the number of arguments the format string consumes.  */
    nargs = MAX (nargs, max_ref_arg);

    /* Allocate memory for the argument descriptions.  */
    args_type = alloca (nargs * sizeof (int));
    memset (args_type, 0, nargs * sizeof (int));
    args_value = alloca (nargs * sizeof (union printf_arg));

    /* XXX Could do sanity check here: If any element in ARGS_TYPE is
       still zero after this loop, format is invalid.  For now we
       simply use 0 as the value.  */

    /* Fill in the types of all the arguments.  */
    for (cnt = 0; cnt < nspecs; ++cnt)
      {
	/* If the width is determined by an argument this is an int.  */
	if (specs[cnt].width_arg != -1)
	  args_type[specs[cnt].width_arg] = PA_INT;

	/* If the precision is determined by an argument this is an int.  */
	if (specs[cnt].prec_arg != -1)
	  args_type[specs[cnt].prec_arg] = PA_INT;

	switch (specs[cnt].ndata_args)
	  {
	  case 0:		/* No arguments.  */
	    break;
	  case 1:		/* One argument; we already have the type.  */
	    args_type[specs[cnt].data_arg] = specs[cnt].data_arg_type;
	    break;
	  default:
	    /* We have more than one argument for this format spec.
	       We must call the arginfo function again to determine
	       all the types.  */
	    (void) (*__printf_arginfo_table[specs[cnt].info.spec])
	      (&specs[cnt].info,
	       specs[cnt].ndata_args, &args_type[specs[cnt].data_arg]);
	    break;
	  }
      }

    /* Now we know all the types and the order.  Fill in the argument
       values.  */
    for (cnt = 0; cnt < nargs; ++cnt)
      switch (args_type[cnt])
	{
#define T(tag, mem, type)						      \
	case tag:							      \
	  args_value[cnt].mem = va_arg (ap_save, type);			      \
	  break

	T (PA_CHAR, pa_char, int); /* Promoted.  */
	T (PA_WCHAR, pa_wchar, wint_t);
	T (PA_INT|PA_FLAG_SHORT, pa_short_int, int); /* Promoted.  */
	T (PA_INT, pa_int, int);
	T (PA_INT|PA_FLAG_LONG, pa_long_int, long int);
	T (PA_INT|PA_FLAG_LONG_LONG, pa_long_long_int, long long int);
	T (PA_FLOAT, pa_float, double);	/* Promoted.  */
	T (PA_DOUBLE, pa_double, double);
	T (PA_DOUBLE|PA_FLAG_LONG_DOUBLE, pa_long_double, long double);
	T (PA_STRING, pa_string, const char *);
	T (PA_WSTRING, pa_wstring, const wchar_t *);
	T (PA_POINTER, pa_pointer, void *);
#undef T
	default:
	  if ((args_type[cnt] & PA_FLAG_PTR) != 0)
	    args_value[cnt].pa_pointer = va_arg (ap_save, void *);
	  else
	    args_value[cnt].pa_long_double = 0.0;
	  break;
	}

    /* Now walk through all format specifiers and process them.  */
    for (; (size_t) nspecs_done < nspecs; ++nspecs_done)
      {
#undef REF
#if defined HAVE_SUBTRACT_LOCAL_LABELS && defined SHARED
# define REF(Name) &&do2_##Name - &&do_form_unknown
#else
# define REF(Name) &&do2_##Name
#endif
#undef LABEL
#define LABEL(Name) do2_##Name
	STEP4_TABLE;

	int is_negative;
	union
	{
	  unsigned long long int longlong;
	  unsigned long int word;
	} number;
	int base;
	union printf_arg the_arg;
	CHAR_T *string;		/* Pointer to argument string.  */

	/* Fill variables from values in struct.  */
	int alt = specs[nspecs_done].info.alt;
	int space = specs[nspecs_done].info.space;
	int left = specs[nspecs_done].info.left;
	int showsign = specs[nspecs_done].info.showsign;
	int group = specs[nspecs_done].info.group;
	int is_long_double = specs[nspecs_done].info.is_long_double;
	int is_short = specs[nspecs_done].info.is_short;
	int is_char = specs[nspecs_done].info.is_char;
	int is_long = specs[nspecs_done].info.is_long;
	int width = specs[nspecs_done].info.width;
	int prec = specs[nspecs_done].info.prec;
	int use_outdigits = specs[nspecs_done].info.i18n;
	char pad = specs[nspecs_done].info.pad;
	CHAR_T spec = specs[nspecs_done].info.spec;
	CHAR_T *workstart = NULL;

	/* Fill in last information.  */
	if (specs[nspecs_done].width_arg != -1)
	  {
	    /* Extract the field width from an argument.  */
	    specs[nspecs_done].info.width =
	      args_value[specs[nspecs_done].width_arg].pa_int;

	    if (specs[nspecs_done].info.width < 0)
	      /* If the width value is negative left justification is
		 selected and the value is taken as being positive.  */
	      {
		specs[nspecs_done].info.width *= -1;
		left = specs[nspecs_done].info.left = 1;
	      }
	    width = specs[nspecs_done].info.width;
	  }

	if (specs[nspecs_done].prec_arg != -1)
	  {
	    /* Extract the precision from an argument.  */
	    specs[nspecs_done].info.prec =
	      args_value[specs[nspecs_done].prec_arg].pa_int;

	    if (specs[nspecs_done].info.prec < 0)
	      /* If the precision is negative the precision is
		 omitted.  */
	      specs[nspecs_done].info.prec = -1;

	    prec = specs[nspecs_done].info.prec;
	  }

	/* Maybe the buffer is too small.  */
	if (MAX (prec, width) + 32 > (int) (sizeof (work_buffer)
					    / sizeof (CHAR_T)))
	  {
	    if (MAX (prec, width) < (int) (32768 / sizeof (CHAR_T)))
	      workend = ((CHAR_T *) alloca ((MAX (prec, width) + 32)
					    * sizeof (CHAR_T))
			 + (MAX (prec, width) + 32));
	    else
	      {
		workstart = (CHAR_T *) malloc ((MAX (prec, width) + 32)
					       * sizeof (CHAR_T));
		workend = workstart + (MAX (prec, width) + 32);
	      }
	  }

	/* Process format specifiers.  */
	while (1)
	  {
	    JUMP (spec, step4_jumps);

	    process_arg ((&specs[nspecs_done]));
	    process_string_arg ((&specs[nspecs_done]));

	  LABEL (form_unknown):
	    {
	      extern printf_function **__printf_function_table;
	      int function_done;
	      printf_function *function;
	      unsigned int i;
	      const void **ptr;

	      function =
		(__printf_function_table == NULL ? NULL :
		 __printf_function_table[specs[nspecs_done].info.spec]);

	      if (function == NULL)
		function = &printf_unknown;

	      ptr = alloca (specs[nspecs_done].ndata_args
			    * sizeof (const void *));

	      /* Fill in an array of pointers to the argument values.  */
	      for (i = 0; i < specs[nspecs_done].ndata_args; ++i)
		ptr[i] = &args_value[specs[nspecs_done].data_arg + i];

	      /* Call the function.  */
	      function_done = (*function) (s, &specs[nspecs_done].info, ptr);

	      /* If an error occurred we don't have information about #
		 of chars.  */
	      if (function_done < 0)
		{
		  done = -1;
		  goto all_done;
		}

	      done += function_done;
	    }
	    break;
	  }

	if (__builtin_expect (workstart != NULL, 0))
	  free (workstart);
	workstart = NULL;

	/* Write the following constant string.  */
	outstring (specs[nspecs_done].end_of_fmt,
		   specs[nspecs_done].next_fmt
		   - specs[nspecs_done].end_of_fmt);
      }
  }

all_done:
  if (__builtin_expect (workstart != NULL, 0))
    free (workstart);
  /* Unlock the stream.  */
#ifdef USE_IN_LIBIO
  _IO_funlockfile (s);
#else
  __funlockfile (s);
#endif
  __libc_cleanup_region_end (0);

  return done;
}

/* Handle an unknown format specifier.  This prints out a canonicalized
   representation of the format spec itself.  */
static int
printf_unknown (FILE *s, const struct printf_info *info,
		const void *const *args)

{
  int done = 0;
  CHAR_T work_buffer[MAX (info->width, info->spec) + 32];
  CHAR_T *const workend
    = &work_buffer[sizeof (work_buffer) / sizeof (CHAR_T)];
  register CHAR_T *w;

  outchar (L_('%'));

  if (info->alt)
    outchar (L_('#'));
  if (info->group)
    outchar (L_('\''));
  if (info->showsign)
    outchar (L_('+'));
  else if (info->space)
    outchar (L_(' '));
  if (info->left)
    outchar (L_('-'));
  if (info->pad == L_('0'))
    outchar (L_('0'));
  if (info->i18n)
    outchar (L_('I'));

  if (info->width != 0)
    {
      w = _itoa_word (info->width, workend, 10, 0);
      while (w < workend)
	outchar (*w++);
    }

  if (info->prec != -1)
    {
      outchar (L_('.'));
      w = _itoa_word (info->prec, workend, 10, 0);
      while (w < workend)
	outchar (*w++);
    }

  if (info->spec != L_('\0'))
    outchar (info->spec);

 all_done:
  return done;
}

/* Group the digits according to the grouping rules of the current locale.
   The interpretation of GROUPING is as in `struct lconv' from <locale.h>.  */
static CHAR_T *
internal_function
group_number (CHAR_T *w, CHAR_T *rear_ptr, const char *grouping,
#ifdef COMPILE_WPRINTF
	      wchar_t thousands_sep
#else
	      const char *thousands_sep
#endif
	      )
{
  int len;
  CHAR_T *src, *s;
#ifndef COMPILE_WPRINTF
  int tlen = strlen (thousands_sep);
#endif

  /* We treat all negative values like CHAR_MAX.  */

  if (*grouping == CHAR_MAX || *grouping <= 0)
    /* No grouping should be done.  */
    return w;

  len = *grouping;

  /* Copy existing string so that nothing gets overwritten.  */
  src = (CHAR_T *) alloca ((rear_ptr - w) * sizeof (CHAR_T));
  s = (CHAR_T *) __mempcpy (src, w,
			    (rear_ptr - w) * sizeof (CHAR_T));
  w = rear_ptr;

  /* Process all characters in the string.  */
  while (s > src)
    {
      *--w = *--s;

      if (--len == 0 && s > src)
	{
	  /* A new group begins.  */
#ifdef COMPILE_WPRINTF
	  *--w = thousands_sep;
#else
	  int cnt = tlen;
	  do
	    *--w = thousands_sep[--cnt];
	  while (cnt > 0);
#endif

	  len = *grouping++;
	  if (*grouping == '\0')
	    /* The previous grouping repeats ad infinitum.  */
	    --grouping;
	  else if (*grouping == CHAR_MAX
#if CHAR_MIN < 0
		   || *grouping < 0
#endif
		   )
	    {
	      /* No further grouping to be done.
		 Copy the rest of the number.  */
	      do
		*--w = *--s;
	      while (s > src);
	      break;
	    }
	}
    }
  return w;
}

#ifdef USE_IN_LIBIO
/* Helper "class" for `fprintf to unbuffered': creates a temporary buffer.  */
struct helper_file
  {
    struct _IO_FILE_plus _f;
#ifdef COMPILE_WPRINTF
    struct _IO_wide_data _wide_data;
#endif
    _IO_FILE *_put_stream;
#ifdef _IO_MTSAFE_IO
    _IO_lock_t lock;
#endif
  };

static int
_IO_helper_overflow (_IO_FILE *s, int c)
{
  _IO_FILE *target = ((struct helper_file*) s)->_put_stream;
#ifdef COMPILE_WPRINTF
  int used = s->_wide_data->_IO_write_ptr - s->_wide_data->_IO_write_base;
  if (used)
    {
      _IO_size_t written = _IO_sputn (target, s->_wide_data->_IO_write_base,
				      used);
      s->_wide_data->_IO_write_ptr -= written;
    }
#else
  int used = s->_IO_write_ptr - s->_IO_write_base;
  if (used)
    {
      _IO_size_t written = _IO_sputn (target, s->_IO_write_base, used);
      s->_IO_write_ptr -= written;
    }
#endif
  return PUTC (c, s);
}

#ifdef COMPILE_WPRINTF
static const struct _IO_jump_t _IO_helper_jumps =
{
  JUMP_INIT_DUMMY,
  JUMP_INIT (finish, INTUSE(_IO_wdefault_finish)),
  JUMP_INIT (overflow, _IO_helper_overflow),
  JUMP_INIT (underflow, _IO_default_underflow),
  JUMP_INIT (uflow, INTUSE(_IO_default_uflow)),
  JUMP_INIT (pbackfail, (_IO_pbackfail_t) INTUSE(_IO_wdefault_pbackfail)),
  JUMP_INIT (xsputn, INTUSE(_IO_wdefault_xsputn)),
  JUMP_INIT (xsgetn, INTUSE(_IO_wdefault_xsgetn)),
  JUMP_INIT (seekoff, _IO_default_seekoff),
  JUMP_INIT (seekpos, _IO_default_seekpos),
  JUMP_INIT (setbuf, _IO_default_setbuf),
  JUMP_INIT (sync, _IO_default_sync),
  JUMP_INIT (doallocate, INTUSE(_IO_wdefault_doallocate)),
  JUMP_INIT (read, _IO_default_read),
  JUMP_INIT (write, _IO_default_write),
  JUMP_INIT (seek, _IO_default_seek),
  JUMP_INIT (close, _IO_default_close),
  JUMP_INIT (stat, _IO_default_stat)
};
#else
static const struct _IO_jump_t _IO_helper_jumps =
{
  JUMP_INIT_DUMMY,
  JUMP_INIT (finish, INTUSE(_IO_default_finish)),
  JUMP_INIT (overflow, _IO_helper_overflow),
  JUMP_INIT (underflow, _IO_default_underflow),
  JUMP_INIT (uflow, INTUSE(_IO_default_uflow)),
  JUMP_INIT (pbackfail, INTUSE(_IO_default_pbackfail)),
  JUMP_INIT (xsputn, INTUSE(_IO_default_xsputn)),
  JUMP_INIT (xsgetn, INTUSE(_IO_default_xsgetn)),
  JUMP_INIT (seekoff, _IO_default_seekoff),
  JUMP_INIT (seekpos, _IO_default_seekpos),
  JUMP_INIT (setbuf, _IO_default_setbuf),
  JUMP_INIT (sync, _IO_default_sync),
  JUMP_INIT (doallocate, INTUSE(_IO_default_doallocate)),
  JUMP_INIT (read, _IO_default_read),
  JUMP_INIT (write, _IO_default_write),
  JUMP_INIT (seek, _IO_default_seek),
  JUMP_INIT (close, _IO_default_close),
  JUMP_INIT (stat, _IO_default_stat)
};
#endif

static int
internal_function
buffered_vfprintf (register _IO_FILE *s, const CHAR_T *format,
		   _IO_va_list args)
{
  CHAR_T buf[_IO_BUFSIZ];
  struct helper_file helper;
  register _IO_FILE *hp = (_IO_FILE *) &helper._f;
  int result, to_flush;

  /* Orient the stream.  */
#ifdef ORIENT
  ORIENT;
#endif

  /* Initialize helper.  */
  helper._put_stream = s;
#ifdef COMPILE_WPRINTF
  hp->_wide_data = &helper._wide_data;
  _IO_wsetp (hp, buf, buf + sizeof buf / sizeof (CHAR_T));
  hp->_mode = 1;
#else
  _IO_setp (hp, buf, buf + sizeof buf);
  hp->_mode = -1;
#endif
  hp->_IO_file_flags = _IO_MAGIC|_IO_NO_READS|_IO_USER_LOCK;
#if _IO_JUMPS_OFFSET
  hp->_vtable_offset = 0;
#endif
#ifdef _IO_MTSAFE_IO
  hp->_lock = NULL;
#endif
  _IO_JUMPS (&helper._f) = (struct _IO_jump_t *) &_IO_helper_jumps;

  /* Now print to helper instead.  */
#if defined USE_IN_LIBIO && !defined COMPILE_WPRINTF
  result = INTUSE(_IO_vfprintf) (hp, format, args);
#else
  result = vfprintf (hp, format, args);
#endif

  /* Lock stream.  */
  __libc_cleanup_region_start (1, (void (*) (void *)) &_IO_funlockfile, s);
  _IO_flockfile (s);

  /* Now flush anything from the helper to the S. */
#ifdef COMPILE_WPRINTF
  if ((to_flush = (hp->_wide_data->_IO_write_ptr
		   - hp->_wide_data->_IO_write_base)) > 0)
    {
      if ((int) _IO_sputn (s, hp->_wide_data->_IO_write_base, to_flush)
	  != to_flush)
	result = -1;
    }
#else
  if ((to_flush = hp->_IO_write_ptr - hp->_IO_write_base) > 0)
    {
      if ((int) _IO_sputn (s, hp->_IO_write_base, to_flush) != to_flush)
	result = -1;
    }
#endif

  /* Unlock the stream.  */
  _IO_funlockfile (s);
  __libc_cleanup_region_end (0);

  return result;
}

#else /* !USE_IN_LIBIO */

static int
internal_function
buffered_vfprintf (register FILE *s, const CHAR_T *format, va_list args)
{
  char buf[BUFSIZ];
  int result;

  /* Orient the stream.  */
#ifdef ORIENT
  ORIENT;
#endif

  s->__bufp = s->__buffer = buf;
  s->__bufsize = sizeof buf;
  s->__put_limit = s->__buffer + s->__bufsize;
  s->__get_limit = s->__buffer;

  /* Now use buffer to print.  */
  result = vfprintf (s, format, args);

  if (fflush (s) == EOF)
    result = -1;
  s->__buffer = s->__bufp = s->__get_limit = s->__put_limit = NULL;
  s->__bufsize = 0;

  return result;
}

/* Pads string with given number of a specified character.
   This code is taken from iopadn.c of the GNU I/O library.  */
#define PADSIZE 16
static const CHAR_T blanks[PADSIZE] =
{ L_(' '), L_(' '), L_(' '), L_(' '), L_(' '), L_(' '), L_(' '), L_(' '),
  L_(' '), L_(' '), L_(' '), L_(' '), L_(' '), L_(' '), L_(' '), L_(' ') };
static const CHAR_T zeroes[PADSIZE] =
{ L_('0'), L_('0'), L_('0'), L_('0'), L_('0'), L_('0'), L_('0'), L_('0'),
  L_('0'), L_('0'), L_('0'), L_('0'), L_('0'), L_('0'), L_('0'), L_('0') };

ssize_t
#ifndef COMPILE_WPRINTF
__printf_pad (FILE *s, char pad, size_t count)
#else
__wprintf_pad (FILE *s, wchar_t pad, size_t count)
#endif
{
  const CHAR_T *padptr;
  register size_t i;

  padptr = pad == L_(' ') ? blanks : zeroes;

  for (i = count; i >= PADSIZE; i -= PADSIZE)
    if (PUT (s, padptr, PADSIZE) != PADSIZE)
      return -1;
  if (i > 0)
    if (PUT (s, padptr, i) != i)
      return -1;

  return count;
}
#undef PADSIZE
#endif /* USE_IN_LIBIO */

#ifdef USE_IN_LIBIO
# undef vfprintf
# ifdef strong_alias
/* This is for glibc.  */
#  ifdef COMPILE_WPRINTF
strong_alias (_IO_vfwprintf, __vfwprintf);
weak_alias (_IO_vfwprintf, vfwprintf);
#  else
strong_alias (_IO_vfprintf, vfprintf);
libc_hidden_def (vfprintf)
INTDEF(_IO_vfprintf)
#  endif
# else
#  if defined __ELF__ || defined __GNU_LIBRARY__
#   include <gnu-stabs.h>
#   ifdef weak_alias
#    ifdef COMPILE_WPRINTF
weak_alias (_IO_vfwprintf, vfwprintf);
#    else
weak_alias (_IO_vfprintf, vfprintf);
#    endif
#   endif
#  endif
# endif
#endif
