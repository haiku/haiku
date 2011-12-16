/* Copyright (C) 1991-1999, 2000, 2001 Free Software Foundation, Inc.
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

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>
#include <bits/libc-lock.h>
#include <locale/localeinfo.h>

#ifdef	__GNUC__
# define HAVE_LONGLONG
# define LONGLONG	long long
#else
# define LONGLONG	long
#endif

/* Determine whether we have to handle `long long' at all.  */
#if LONG_MAX == LONG_LONG_MAX
# define need_longlong	0
#else
# define need_longlong	1
#endif

/* Determine whether we have to handle `long'.  */
#if INT_MAX == LONG_MAX
# define need_long	0
#else
# define need_long 	1
#endif

/* Those are flags in the conversion format. */
#define LONG		0x001	/* l: long or double */
#define LONGDBL		0x002	/* L: long long or long double */
#define SHORT		0x004	/* h: short */
#define SUPPRESS	0x008	/* *: suppress assignment */
#define POINTER		0x010	/* weird %p pointer (`fake hex') */
#define NOSKIP		0x020	/* do not skip blanks */
#define WIDTH		0x040	/* width was given */
#define GROUP		0x080	/* ': group numbers */
#define MALLOC		0x100	/* a: malloc strings */
#define CHAR		0x200	/* hh: char */
#define I18N		0x400	/* I: use locale's digits */


#ifdef USE_IN_LIBIO
# include <libioP.h>
# include <libio.h>

# undef va_list
# define va_list	_IO_va_list

# ifdef COMPILE_WSCANF
#  define ungetc(c, s)	((void) (c == WEOF				      \
				 || (--read_in,				      \
				     _IO_sputbackwc (s, c))))
#  define ungetc_not_eof(c, s)	((void) (--read_in,			      \
					 _IO_sputbackwc (s, c)))
#  define inchar()	(c == WEOF ? WEOF				      \
			 : ((c = _IO_getwc_unlocked (s)),		      \
			    (void) (c != WEOF && ++read_in), c))

#  define MEMCPY(d, s, n) __wmemcpy (d, s, n)
#  define ISSPACE(Ch)	  iswspace (Ch)
#  define ISDIGIT(Ch)	  iswdigit (Ch)
#  define ISXDIGIT(Ch)	  iswxdigit (Ch)
#  define TOLOWER(Ch)	  towlower (Ch)
#  define ORIENT	  if (_IO_fwide (s, 1) != 1) return WEOF
#  define __strtoll_internal	__wcstoll_internal
#  define __strtoull_internal	__wcstoull_internal
#  define __strtol_internal	__wcstol_internal
#  define __strtoul_internal	__wcstoul_internal
#  define __strtold_internal	__wcstold_internal
#  define __strtod_internal	__wcstod_internal
#  define __strtof_internal	__wcstof_internal

#  define L_(Str)	  L##Str
#  define CHAR_T	  wchar_t
#  define UCHAR_T	  unsigned int
#  define WINT_T	  wint_t
#  undef EOF
#  define EOF		  WEOF
# else
#  define ungetc(c, s)	((void) ((int) c == EOF				      \
				 || (--read_in,				      \
				     _IO_sputbackc (s, (unsigned char) c))))
#  define ungetc_not_eof(c, s)	((void) (--read_in,			      \
					 _IO_sputbackc (s, (unsigned char) c)))
#  define inchar()	(c == EOF ? EOF					      \
			 : ((c = _IO_getc_unlocked (s)),		      \
			    (void) (c != EOF && ++read_in), c))
#  define MEMCPY(d, s, n) memcpy (d, s, n)
#  define ISSPACE(Ch)	  isspace (Ch)
#  define ISDIGIT(Ch)	  isdigit (Ch)
#  define ISXDIGIT(Ch)	  isxdigit (Ch)
#  define TOLOWER(Ch)	  tolower (Ch)
#  define ORIENT	  if (s->_vtable_offset == 0			      \
			      && _IO_fwide (s, -1) != -1)		      \
			    return EOF

#  define L_(Str)	  Str
#  define CHAR_T	  char
#  define UCHAR_T	  unsigned char
#  define WINT_T	  int
# endif

# define encode_error() do {						      \
			  if (errp != NULL) *errp |= 4;			      \
			  _IO_funlockfile (s);				      \
			  __libc_cleanup_end (0);			      \
			  __set_errno (EILSEQ);				      \
			  return done;					      \
			} while (0)
# define conv_error()	do {						      \
			  if (errp != NULL) *errp |= 2;			      \
			  _IO_funlockfile (s);				      \
			  __libc_cleanup_end (0);			      \
			  return done;					      \
			} while (0)
# define input_error()	do {						      \
			  _IO_funlockfile (s);				      \
			  if (errp != NULL) *errp |= 1;			      \
			  __libc_cleanup_end (0);			      \
			  return done ?: EOF;				      \
			} while (0)
# define memory_error() do {						      \
			  _IO_funlockfile (s);				      \
			  __set_errno (ENOMEM);				      \
			  __libc_cleanup_end (0);			      \
			  return EOF;					      \
			} while (0)
# define ARGCHECK(s, format)						      \
  do									      \
    {									      \
      /* Check file argument for consistence.  */			      \
      CHECK_FILE (s, EOF);						      \
      if (s->_flags & _IO_NO_READS)					      \
	{								      \
	  __set_errno (EBADF);						      \
	  return EOF;							      \
	}								      \
      else if (format == NULL)						      \
	{								      \
	  MAYBE_SET_EINVAL;						      \
	  return EOF;							      \
	}								      \
    } while (0)
# define LOCK_STREAM(S)							      \
  __libc_cleanup_region_start (1, (void (*) (void *)) &_IO_funlockfile, (S)); \
  _IO_flockfile (S)
# define UNLOCK_STREAM(S)						      \
  _IO_funlockfile (S);							      \
  __libc_cleanup_region_end (0)
#else
# define ungetc(c, s)	((void) (c != EOF && --read_in), ungetc (c, s))
# define ungetc_not_eof(c, s)	(--read_in, (ungetc) (c, s))
# define inchar()	(c == EOF ? EOF					      \
			 : ((c = getc (s)), (void) (c != EOF && ++read_in), c))
# define MEMCPY(d, s, n)  memcpy (d, s, n)
# define ISSPACE(Ch)      isspace (Ch)
# define ISDIGIT(Ch)      isdigit (Ch)
# define ISXDIGIT(Ch)     isxdigit (Ch)
# define TOLOWER(Ch)      tolower (Ch)

# define L_(Str)          Str
# define CHAR_T           char
# define UCHAR_T          unsigned char
# define WINT_T           int

# define encode_error()	do {						      \
			  funlockfile (s);				      \
			  __set_errno (EILSEQ);				      \
			  return done;					      \
			} while (0)
# define conv_error()	do {						      \
			  funlockfile (s);				      \
			  return done;					      \
			} while (0)
# define input_error()	do {						      \
			  funlockfile (s);				      \
			  return done ?: EOF;				      \
			} while (0)
# define memory_error()	do {						      \
			  funlockfile (s);				      \
			  __set_errno (ENOMEM);				      \
			  return EOF;					      \
			} while (0)
# define ARGCHECK(s, format)						      \
  do									      \
    {									      \
      /* Check file argument for consistence.  */			      \
      if (!__validfp (s) || !s->__mode.__read)				      \
	{								      \
	  __set_errno (EBADF);						      \
	  return EOF;							      \
	}								      \
      else if (format == NULL)						      \
	{								      \
	  __set_errno (EINVAL);						      \
	  return EOF;							      \
	}								      \
    } while (0)
#if 1
      /* XXX For now !!! */
# define flockfile(S) /* nothing */
# define funlockfile(S) /* nothing */
# define LOCK_STREAM(S)
# define UNLOCK_STREAM(S)
#else
# define LOCK_STREAM(S)							      \
  __libc_cleanup_region_start (&__funlockfile, (S));			      \
  __flockfile (S)
# define UNLOCK_STREAM(S)						      \
  __funlockfile (S);							      \
  __libc_cleanup_region_end (0)
#endif
#endif


/* Read formatted input from S according to the format string
   FORMAT, using the argument list in ARG.
   Return the number of assignments made, or -1 for an input error.  */
#ifdef USE_IN_LIBIO
# ifdef COMPILE_WSCANF
int
_IO_vfwscanf (s, format, argptr, errp)
     _IO_FILE *s;
     const wchar_t *format;
     _IO_va_list argptr;
     int *errp;
# else
int
_IO_vfscanf (s, format, argptr, errp)
     _IO_FILE *s;
     const char *format;
     _IO_va_list argptr;
     int *errp;
# endif
#else
int
__vfscanf (FILE *s, const char *format, va_list argptr)
#endif
{
  va_list arg;
  register const CHAR_T *f = format;
  register UCHAR_T fc;	/* Current character of the format.  */
  register WINT_T done = 0;	/* Assignments done.  */
  register size_t read_in = 0;	/* Chars read in.  */
  register WINT_T c = 0;	/* Last char read.  */
  register int width;		/* Maximum field width.  */
  register int flags;		/* Modifiers for current format element.  */

  /* Status for reading F-P nums.  */
  char got_dot, got_e, negative;
  /* If a [...] is a [^...].  */
  CHAR_T not_in;
#define exp_char not_in
  /* Base for integral numbers.  */
  int base;
  /* Signedness for integral numbers.  */
  int number_signed;
#define is_hexa number_signed
  /* Decimal point character.  */
#ifdef COMPILE_WSCANF
  wchar_t decimal;
#else
  const char *decimal;
#endif
  /* The thousands character of the current locale.  */
#ifdef COMPILE_WSCANF
  wchar_t thousands;
#else
  const char *thousands;
#endif
  /* State for the conversions.  */
  mbstate_t state;
  /* Integral holding variables.  */
  union
    {
      long long int q;
      unsigned long long int uq;
      long int l;
      unsigned long int ul;
    } num;
  /* Character-buffer pointer.  */
  char *str = NULL;
  wchar_t *wstr = NULL;
  char **strptr = NULL;
  ssize_t strsize = 0;
  /* We must not react on white spaces immediately because they can
     possibly be matched even if in the input stream no character is
     available anymore.  */
  int skip_space = 0;
  /* Nonzero if we are reading a pointer.  */
  int read_pointer;
  /* Workspace.  */
  CHAR_T *tw;			/* Temporary pointer.  */
  CHAR_T *wp = NULL;		/* Workspace.  */
  size_t wpmax = 0;		/* Maximal size of workspace.  */
  size_t wpsize;		/* Currently used bytes in workspace.  */
#define ADDW(Ch)							    \
  do									    \
    {									    \
      if (wpsize == wpmax)						    \
	{								    \
	  CHAR_T *old = wp;						    \
	  wpmax = (UCHAR_MAX + 1 > 2 * wpmax ? UCHAR_MAX + 1 : 2 * wpmax);  \
	  wp = (CHAR_T *) alloca (wpmax * sizeof (wchar_t));		    \
	  if (old != NULL)						    \
	    MEMCPY (wp, old, wpsize);					    \
	}								    \
      wp[wpsize++] = (Ch);						    \
    }									    \
  while (0)

#ifdef __va_copy
  __va_copy (arg, argptr);
#else
  arg = (va_list) argptr;
#endif

#ifdef ORIENT
  ORIENT;
#endif

  ARGCHECK (s, format);

  /* Figure out the decimal point character.  */
#ifdef COMPILE_WSCANF
  decimal = _NL_CURRENT_WORD (LC_NUMERIC, _NL_NUMERIC_DECIMAL_POINT_WC);
#else
  decimal = _NL_CURRENT (LC_NUMERIC, DECIMAL_POINT);
#endif
  /* Figure out the thousands separator character.  */
#ifdef COMPILE_WSCANF
  thousands = _NL_CURRENT_WORD (LC_NUMERIC, _NL_NUMERIC_THOUSANDS_SEP_WC);
#else
  thousands = _NL_CURRENT (LC_NUMERIC, THOUSANDS_SEP);
  if (*thousands == '\0')
    thousands = NULL;
#endif

  /* Lock the stream.  */
  LOCK_STREAM (s);


#ifndef COMPILE_WSCANF
  /* From now on we use `state' to convert the format string.  */
  memset (&state, '\0', sizeof (state));
#endif

  /* Run through the format string.  */
  while (*f != '\0')
    {
      unsigned int argpos;
      /* Extract the next argument, which is of type TYPE.
	 For a %N$... spec, this is the Nth argument from the beginning;
	 otherwise it is the next argument after the state now in ARG.  */
#ifdef __va_copy
# define ARG(type)	(argpos == 0 ? va_arg (arg, type) :		      \
			 ({ unsigned int pos = argpos;			      \
			    va_list arg;				      \
			    __va_copy (arg, argptr);			      \
			    while (--pos > 0)				      \
			      (void) va_arg (arg, void *);		      \
			    va_arg (arg, type);				      \
			  }))
#else
# if 0
      /* XXX Possible optimization.  */
#  define ARG(type)	(argpos == 0 ? va_arg (arg, type) :		      \
			 ({ va_list arg = (va_list) argptr;		      \
			    arg = (va_list) ((char *) arg		      \
					     + (argpos - 1)		      \
					     * __va_rounded_size (void *));   \
			    va_arg (arg, type);				      \
			 }))
# else
#  define ARG(type)	(argpos == 0 ? va_arg (arg, type) :		      \
			 ({ unsigned int pos = argpos;			      \
			    va_list arg = (va_list) argptr;		      \
			    while (--pos > 0)				      \
			      (void) va_arg (arg, void *);		      \
			    va_arg (arg, type);				      \
			  }))
# endif
#endif

#ifndef COMPILE_WSCANF
      if (!isascii ((unsigned char) *f))
	{
	  /* Non-ASCII, may be a multibyte.  */
	  int len = __mbrlen (f, strlen (f), &state);
	  if (len > 0)
	    {
	      do
		{
		  c = inchar ();
		  if (c == EOF)
		    input_error ();
		  else if (c != (unsigned char) *f++)
		    {
		      ungetc_not_eof (c, s);
		      conv_error ();
		    }
		}
	      while (--len > 0);
	      continue;
	    }
	}
#endif

      fc = *f++;
      if (fc != '%')
	{
	  /* Remember to skip spaces.  */
	  if (ISSPACE (fc))
	    {
	      skip_space = 1;
	      continue;
	    }

	  /* Read a character.  */
	  c = inchar ();

	  /* Characters other than format specs must just match.  */
	  if (c == EOF)
	    input_error ();

	  /* We saw white space char as the last character in the format
	     string.  Now it's time to skip all leading white space.  */
	  if (skip_space)
	    {
	      while (ISSPACE (c))
		if (inchar () == EOF && errno == EINTR)
		  conv_error ();
	      skip_space = 0;
	    }

	  if (c != fc)
	    {
	      ungetc (c, s);
	      conv_error ();
	    }

	  continue;
	}

      /* This is the start of the conversion string. */
      flags = 0;

      /* Not yet decided whether we read a pointer or not.  */
      read_pointer = 0;

      /* Initialize state of modifiers.  */
      argpos = 0;

      /* Prepare temporary buffer.  */
      wpsize = 0;

      /* Check for a positional parameter specification.  */
      if (ISDIGIT ((UCHAR_T) *f))
	{
	  argpos = (UCHAR_T) *f++ - L_('0');
	  while (ISDIGIT ((UCHAR_T) *f))
	    argpos = argpos * 10 + ((UCHAR_T) *f++ - L_('0'));
	  if (*f == L_('$'))
	    ++f;
	  else
	    {
	      /* Oops; that was actually the field width.  */
	      width = argpos;
	      flags |= WIDTH;
	      argpos = 0;
	      goto got_width;
	    }
	}

      /* Check for the assignment-suppressing, the number grouping flag,
	 and the signal to use the locale's digit representation.  */
      while (*f == L_('*') || *f == L_('\'') || *f == L_('I'))
	switch (*f++)
	  {
	  case L_('*'):
	    flags |= SUPPRESS;
	    break;
	  case L_('\''):
	    flags |= GROUP;
	    break;
	  case L_('I'):
	    flags |= I18N;
	    break;
	  }

      /* We have seen width. */
      if (ISDIGIT ((UCHAR_T) *f))
	flags |= WIDTH;

      /* Find the maximum field width.  */
      width = 0;
      while (ISDIGIT ((UCHAR_T) *f))
	{
	  width *= 10;
	  width += (UCHAR_T) *f++ - L_('0');
	}
    got_width:
      if (width == 0)
	width = -1;

      /* Check for type modifiers.  */
      switch (*f++)
	{
	case L_('h'):
	  /* ints are short ints or chars.  */
	  if (*f == L_('h'))
	    {
	      ++f;
	      flags |= CHAR;
	    }
	  else
	    flags |= SHORT;
	  break;
	case L_('l'):
	  if (*f == L_('l'))
	    {
	      /* A double `l' is equivalent to an `L'.  */
	      ++f;
	      flags |= LONGDBL | LONG;
	    }
	  else
	    /* ints are long ints.  */
	    flags |= LONG;
	  break;
	case L_('q'):
	case L_('L'):
	  /* doubles are long doubles, and ints are long long ints.  */
	  flags |= LONGDBL | LONG;
	  break;
	case L_('a'):
	  /* The `a' is used as a flag only if followed by `s', `S' or
	     `['.  */
	  if (*f != L_('s') && *f != L_('S') && *f != L_('['))
	    {
	      --f;
	      break;
	    }
	  /* String conversions (%s, %[) take a `char **'
	     arg and fill it in with a malloc'd pointer.  */
	  flags |= MALLOC;
	  break;
	case L_('z'):
	  if (need_longlong && sizeof (size_t) > sizeof (unsigned long int))
	    flags |= LONGDBL;
	  else if (sizeof (size_t) > sizeof (unsigned int))
	    flags |= LONG;
	  break;
	case L_('j'):
	  if (need_longlong && sizeof (uintmax_t) > sizeof (unsigned long int))
	    flags |= LONGDBL;
	  else if (sizeof (uintmax_t) > sizeof (unsigned int))
	    flags |= LONG;
	  break;
	case L_('t'):
	  if (need_longlong && sizeof (ptrdiff_t) > sizeof (long int))
	    flags |= LONGDBL;
	  else if (sizeof (ptrdiff_t) > sizeof (int))
	    flags |= LONG;
	  break;
	default:
	  /* Not a recognized modifier.  Backup.  */
	  --f;
	  break;
	}

      /* End of the format string?  */
      if (*f == L_('\0'))
	conv_error ();

      /* Find the conversion specifier.  */
      fc = *f++;
      if (skip_space || (fc != L_('[') && fc != L_('c')
			 && fc != L_('C') && fc != L_('n')))
	{
	  /* Eat whitespace.  */
	  int save_errno = errno;
	  __set_errno(0);
	  do
	    if (inchar () == EOF && errno == EINTR)
	      input_error ();
	  while (ISSPACE (c));
	  __set_errno(save_errno);
	  ungetc (c, s);
	  skip_space = 0;
	}

      switch (fc)
	{
	case L_('%'):	/* Must match a literal '%'.  */
	  c = inchar ();
	  if (c == EOF)
	    input_error ();
	  if (c != fc)
	    {
	      ungetc_not_eof (c, s);
	      conv_error ();
	    }
	  break;

	case L_('n'):	/* Answer number of assignments done.  */
	  /* Corrigendum 1 to ISO C 1990 describes the allowed flags
	     with the 'n' conversion specifier.  */
	  if (!(flags & SUPPRESS))
	    {
	      /* Don't count the read-ahead.  */
	      if (need_longlong && (flags & LONGDBL))
		*ARG (long long int *) = read_in;
	      else if (need_long && (flags & LONG))
		*ARG (long int *) = read_in;
	      else if (flags & SHORT)
		*ARG (short int *) = read_in;
	      else if (!(flags & CHAR))
		*ARG (int *) = read_in;
	      else
		*ARG (char *) = read_in;

#ifdef NO_BUG_IN_ISO_C_CORRIGENDUM_1
	      /* We have a severe problem here.  The ISO C standard
		 contradicts itself in explaining the effect of the %n
		 format in `scanf'.  While in ISO C:1990 and the ISO C
		 Amendement 1:1995 the result is described as

		   Execution of a %n directive does not effect the
		   assignment count returned at the completion of
		   execution of the f(w)scanf function.

		 in ISO C Corrigendum 1:1994 the following was added:

		   Subclause 7.9.6.2
		   Add the following fourth example:
		     In:
		       #include <stdio.h>
		       int d1, d2, n1, n2, i;
		       i = sscanf("123", "%d%n%n%d", &d1, &n1, &n2, &d2);
		     the value 123 is assigned to d1 and the value3 to n1.
		     Because %n can never get an input failure the value
		     of 3 is also assigned to n2.  The value of d2 is not
		     affected.  The value 3 is assigned to i.

		 We go for now with the historically correct code from ISO C,
		 i.e., we don't count the %n assignments.  When it ever
		 should proof to be wrong just remove the #ifdef above.  */
	      ++done;
#endif
	    }
	  break;

	case L_('c'):	/* Match characters.  */
	  if ((flags & LONG) == 0)
	    {
	      if (!(flags & SUPPRESS))
		{
		  str = ARG (char *);
		  if (str == NULL)
		    conv_error ();
		}

	      c = inchar ();
	      if (c == EOF)
		input_error ();

	      if (width == -1)
		width = 1;

#ifdef COMPILE_WSCANF
	      /* We have to convert the wide character(s) into multibyte
		 characters and store the result.  */
	      memset (&state, '\0', sizeof (state));

	      do
		{
		  size_t n;

		  n = __wcrtomb (!(flags & SUPPRESS) ? str : NULL, c, &state);
		  if (n == (size_t) -1)
		    /* No valid wide character.  */
		    input_error ();

		  /* Increment the output pointer.  Even if we don't
		     write anything.  */
		  str += n;
		}
	      while (--width > 0 && inchar () != EOF);
#else
	      if (!(flags & SUPPRESS))
		{
		  do
		    *str++ = c;
		  while (--width > 0 && inchar () != EOF);
		}
	      else
		while (--width > 0 && inchar () != EOF);
#endif

	      if (!(flags & SUPPRESS))
		++done;

	      break;
	    }
	  /* FALLTHROUGH */
	case L_('C'):
	  if (!(flags & SUPPRESS))
	    {
	      wstr = ARG (wchar_t *);
	      if (wstr == NULL)
		conv_error ();
	    }

	  c = inchar ();
	  if (c == EOF)
	    input_error ();

#ifdef COMPILE_WSCANF
	  /* Just store the incoming wide characters.  */
	  if (!(flags & SUPPRESS))
	    {
	      do
		*wstr++ = c;
	      while (--width > 0 && inchar () != EOF);
	    }
	  else
	    while (--width > 0 && inchar () != EOF);
#else
	  {
	    /* We have to convert the multibyte input sequence to wide
	       characters.  */
	    char buf[1];
	    mbstate_t cstate;

	    memset (&cstate, '\0', sizeof (cstate));

	    do
	      {
		/* This is what we present the mbrtowc function first.  */
		buf[0] = c;

		while (1)
		  {
		    size_t n;

		    n = __mbrtowc (!(flags & SUPPRESS) ? wstr : NULL,
				   buf, 1, &cstate);

		    if (n == (size_t) -2)
		      {
			/* Possibly correct character, just not enough
			   input.  */
			if (inchar () == EOF)
			  encode_error ();

			buf[0] = c;
			continue;
		      }

		    if (n != 1)
		      encode_error ();

		    /* We have a match.  */
		    break;
		  }

		/* Advance the result pointer.  */
		++wstr;
	      }
	    while (--width > 0 && inchar () != EOF);
	  }
#endif

	  if (!(flags & SUPPRESS))
	    ++done;

	  break;

	case L_('s'):		/* Read a string.  */
	  if (!(flags & LONG))
	    {
#define STRING_ARG(Str, Type)						      \
	      do if (!(flags & SUPPRESS))				      \
		{							      \
		  if (flags & MALLOC)					      \
		    {							      \
		      /* The string is to be stored in a malloc'd buffer.  */ \
		      strptr = ARG (char **);				      \
		      if (strptr == NULL)				      \
			conv_error ();					      \
		      /* Allocate an initial buffer.  */		      \
		      strsize = 100;					      \
		      *strptr = (char *) malloc (strsize * sizeof (Type));    \
		      Str = (Type *) *strptr;				      \
		    }							      \
		  else							      \
		    Str = ARG (Type *);					      \
		  if (Str == NULL)					      \
		    conv_error ();					      \
		} while (0)
	      STRING_ARG (str, char);

	      c = inchar ();
	      if (c == EOF)
		input_error ();

#ifdef COMPILE_WSCANF
	      memset (&state, '\0', sizeof (state));
#endif

	      do
		{
		  if (ISSPACE (c))
		    {
		      ungetc_not_eof (c, s);
		      break;
		    }

#ifdef COMPILE_WSCANF
		  /* This is quite complicated.  We have to convert the
		     wide characters into multibyte characters and then
		     store them.  */
		  {
		    size_t n;

		    if (!(flags & SUPPRESS) && (flags & MALLOC)
			&& str + MB_CUR_MAX >= *strptr + strsize)
		      {
			/* We have to enlarge the buffer if the `a' flag
			   was given.  */
			size_t strleng = str - *strptr;
			char *newstr;

			newstr = (char *) realloc (*strptr, strsize * 2);
			if (newstr == NULL)
			  {
			    /* Can't allocate that much.  Last-ditch
			       effort.  */
			    newstr = (char *) realloc (*strptr,
						       strleng + MB_CUR_MAX);
			    if (newstr == NULL)
			      {
				/* We lose.  Oh well.  Terminate the
				   string and stop converting,
				   so at least we don't skip any input.  */
				((char *) (*strptr))[strleng] = '\0';
				++done;
				conv_error ();
			      }
			    else
			      {
				*strptr = newstr;
				str = newstr + strleng;
				strsize = strleng + MB_CUR_MAX;
			      }
			  }
			else
			  {
			    *strptr = newstr;
			    str = newstr + strleng;
			    strsize *= 2;
			  }
		      }

		    n = __wcrtomb (!(flags & SUPPRESS) ? str : NULL, c,
				   &state);
		    if (n == (size_t) -1)
		      encode_error ();

		    assert (n <= MB_CUR_MAX);
		    str += n;
		  }
#else
		  /* This is easy.  */
		  if (!(flags & SUPPRESS))
		    {
		      *str++ = c;
		      if ((flags & MALLOC)
			  && (char *) str == *strptr + strsize)
			{
			  /* Enlarge the buffer.  */
			  str = (char *) realloc (*strptr, 2 * strsize);
			  if (str == NULL)
			    {
			      /* Can't allocate that much.  Last-ditch
				 effort.  */
			      str = (char *) realloc (*strptr, strsize + 1);
			      if (str == NULL)
				{
				  /* We lose.  Oh well.  Terminate the
				     string and stop converting,
				     so at least we don't skip any input.  */
				  ((char *) (*strptr))[strsize - 1] = '\0';
				  ++done;
				  conv_error ();
				}
			      else
				{
				  *strptr = (char *) str;
				  str += strsize;
				  ++strsize;
				}
			    }
			  else
			    {
			      *strptr = (char *) str;
			      str += strsize;
			      strsize *= 2;
			    }
			}
		    }
#endif
		}
	      while ((width <= 0 || --width > 0) && inchar () != EOF);

	      if (!(flags & SUPPRESS))
		{
#ifdef COMPILE_WSCANF
		  /* We have to emit the code to get into the initial
		     state.  */
		  char buf[MB_LEN_MAX];
		  size_t n = __wcrtomb (buf, L'\0', &state);
		  if (n > 0 && (flags & MALLOC)
		      && str + n >= *strptr + strsize)
		    {
		      /* Enlarge the buffer.  */
		      size_t strleng = str - *strptr;
		      char *newstr;

		      newstr = (char *) realloc (*strptr, strleng + n + 1);
		      if (newstr == NULL)
			{
			  /* We lose.  Oh well.  Terminate the string
			     and stop converting, so at least we don't
			     skip any input.  */
			  ((char *) (*strptr))[strleng] = '\0';
			  ++done;
			  conv_error ();
			}
		      else
			{
			  *strptr = newstr;
			  str = newstr + strleng;
			  strsize = strleng + n + 1;
			}
		    }

		  str = __mempcpy (str, buf, n);
#endif
		  *str++ = '\0';

		  if ((flags & MALLOC) && str - *strptr != strsize)
		    {
		      char *cp = (char *) realloc (*strptr, str - *strptr);
		      if (cp != NULL)
			*strptr = cp;
		    }

		  ++done;
		}
	      break;
	    }
	  /* FALLTHROUGH */

	case L_('S'):
	  {
#ifndef COMPILE_WSCANF
	    mbstate_t cstate;
#endif

	    /* Wide character string.  */
	    STRING_ARG (wstr, wchar_t);

	    c = inchar ();
	    if (c == EOF)
	      input_error ();

#ifndef COMPILE_WSCANF
	    memset (&cstate, '\0', sizeof (cstate));
#endif

	    do
	      {
		if (ISSPACE (c))
		  {
		    ungetc_not_eof (c, s);
		    break;
		  }

#ifdef COMPILE_WSCANF
		/* This is easy.  */
		if (!(flags & SUPPRESS))
		  {
		    *wstr++ = c;
		    if ((flags & MALLOC)
			&& wstr == (wchar_t *) *strptr + strsize)
		      {
			/* Enlarge the buffer.  */
			wstr = (wchar_t *) realloc (*strptr,
						    (2 * strsize)
						    * sizeof (wchar_t));
			if (wstr == NULL)
			  {
			    /* Can't allocate that much.  Last-ditch
                               effort.  */
			    wstr = (wchar_t *) realloc (*strptr,
							(strsize + 1)
							* sizeof (wchar_t));
			    if (wstr == NULL)
			      {
				/* We lose.  Oh well.  Terminate the string
				   and stop converting, so at least we don't
				   skip any input.  */
				((wchar_t *) (*strptr))[strsize - 1] = L'\0';
				++done;
				conv_error ();
			      }
			    else
			      {
				*strptr = (char *) wstr;
				wstr += strsize;
				++strsize;
			      }
			  }
			else
			  {
			    *strptr = (char *) wstr;
			    wstr += strsize;
			    strsize *= 2;
			  }
		      }
		  }
#else
		{
		  char buf[1];

		  buf[0] = c;

		  while (1)
		    {
		      size_t n;

		      n = __mbrtowc (!(flags & SUPPRESS) ? wstr : NULL,
				     buf, 1, &cstate);

		      if (n == (size_t) -2)
			{
			  /* Possibly correct character, just not enough
			     input.  */
			  if (inchar () == EOF)
			    encode_error ();

			  buf[0] = c;
			  continue;
			}

		      if (n != 1)
			encode_error ();

		      /* We have a match.  */
		      ++wstr;
		      break;
		    }

		  if (!(flags & SUPPRESS) && (flags & MALLOC)
		      && wstr == (wchar_t *) *strptr + strsize)
		    {
		      /* Enlarge the buffer.  */
		      wstr = (wchar_t *) realloc (*strptr,
						  (2 * strsize
						   * sizeof (wchar_t)));
		      if (wstr == NULL)
			{
			  /* Can't allocate that much.  Last-ditch effort.  */
			  wstr = (wchar_t *) realloc (*strptr,
						      ((strsize + 1)
						       * sizeof (wchar_t)));
			  if (wstr == NULL)
			    {
			      /* We lose.  Oh well.  Terminate the
				 string and stop converting, so at
				 least we don't skip any input.  */
			      ((wchar_t *) (*strptr))[strsize - 1] = L'\0';
			      ++done;
			      conv_error ();
			    }
			  else
			    {
			      *strptr = (char *) wstr;
			      wstr += strsize;
			      ++strsize;
			    }
			}
		      else
			{
			  *strptr = (char *) wstr;
			  wstr += strsize;
			  strsize *= 2;
			}
		    }
		}
#endif
	      }
	    while ((width <= 0 || --width > 0) && inchar () != EOF);

	    if (!(flags & SUPPRESS))
	      {
		*wstr++ = L'\0';

		if ((flags & MALLOC) && wstr - (wchar_t *) *strptr != strsize)
		  {
		    wchar_t *cp = (wchar_t *) realloc (*strptr,
						       ((wstr
							 - (wchar_t *) *strptr)
							* sizeof(wchar_t)));
		    if (cp != NULL)
		      *strptr = (char *) cp;
		  }

		++done;
	      }
	  }
	  break;

	case L_('x'):	/* Hexadecimal integer.  */
	case L_('X'):	/* Ditto.  */
	  base = 16;
	  number_signed = 0;
	  goto number;

	case L_('o'):	/* Octal integer.  */
	  base = 8;
	  number_signed = 0;
	  goto number;

	case L_('u'):	/* Unsigned decimal integer.  */
	  base = 10;
	  number_signed = 0;
	  goto number;

	case L_('d'):	/* Signed decimal integer.  */
	  base = 10;
	  number_signed = 1;
	  goto number;

	case L_('i'):	/* Generic number.  */
	  base = 0;
	  number_signed = 1;

	number:
	  c = inchar ();
	  if (c == EOF)
	    input_error ();

	  /* Check for a sign.  */
	  if (c == L_('-') || c == L_('+'))
	    {
	      ADDW (c);
	      if (width > 0)
		--width;
	      c = inchar ();
	    }

	  /* Look for a leading indication of base.  */
	  if (width != 0 && c == L_('0'))
	    {
	      if (width > 0)
		--width;

	      ADDW (c);
	      c = inchar ();

	      if (width != 0 && TOLOWER (c) == L_('x'))
		{
		  if (base == 0)
		    base = 16;
		  if (base == 16)
		    {
		      if (width > 0)
			--width;
		      c = inchar ();
		    }
		}
	      else if (base == 0)
		base = 8;
	    }

	  if (base == 0)
	    base = 10;

	  if (base == 10 && (flags & I18N) != 0)
	    {
	      int from_level;
	      int to_level;
	      int level;
#ifdef COMPILE_WSCANF
	      const wchar_t *wcdigits[10];
#else
	      const char *mbdigits[10];
#endif
	      int n;

	      from_level = 0;
#ifdef COMPILE_WSCANF
	      to_level = _NL_CURRENT_WORD (LC_CTYPE,
					   _NL_CTYPE_INDIGITS_WC_LEN) - 1;
#else
	      to_level = _NL_CURRENT_WORD (LC_CTYPE,
					   _NL_CTYPE_INDIGITS_MB_LEN) - 1;
#endif

	      /* Read the number into workspace.  */
	      while (c != EOF && width != 0)
		{
		  /* In this round we get the pointer to the digit strings
		     and also perform the first round of comparisons.  */
		  for (n = 0; n < 10; ++n)
		    {
		      /* Get the string for the digits with value N.  */
#ifdef COMPILE_WSCANF
		      wcdigits[n] = (const wchar_t *)
			_NL_CURRENT (LC_CTYPE, _NL_CTYPE_INDIGITS0_WC + n);
		      wcdigits[n] += from_level;

		      if (c == *wcdigits[n])
			{
			  to_level = from_level;
			  break;
			}

		      /* Advance the pointer to the next string.  */
		      ++wcdigits[n];
#else
		      const char *cmpp;
		      int avail = width > 0 ? width : INT_MAX;

		      mbdigits[n] = _NL_CURRENT (LC_CTYPE,
						 _NL_CTYPE_INDIGITS0_MB + n);

		      for (level = 0; level < from_level; level++)
			mbdigits[n] = strchr (mbdigits[n], '\0') + 1;

		      cmpp = mbdigits[n];
		      while ((unsigned char) *cmpp == c && avail > 0)
			{
			  if (*++cmpp == '\0')
			    break;
			  else
			    {
			      if ((c = inchar ()) == EOF)
				break;
			      --avail;
			    }
			}

		      if (*cmpp == '\0')
			{
			  if (width > 0)
			    width = avail;
			  to_level = from_level;
			  break;
			}

		      /* We are pushing all read characters back.  */
		      if (cmpp > mbdigits[n])
			{
			  ungetc (c, s);
			  while (--cmpp > mbdigits[n])
			    ungetc_not_eof ((unsigned char) *cmpp, s);
			  c = (unsigned char) *cmpp;
			}

		      /* Advance the pointer to the next string.  */
		      mbdigits[n] = strchr (mbdigits[n], '\0') + 1;
#endif
		    }

		  if (n == 10)
		    {
		      /* Have not yet found the digit.  */
		      for (level = from_level + 1; level <= to_level; ++level)
			{
			  /* Search all ten digits of this level.  */
			  for (n = 0; n < 10; ++n)
			    {
#ifdef COMPILE_WSCANF
			      if (c == *wcdigits[n])
				break;

			      /* Advance the pointer to the next string.  */
			      ++wcdigits[n];
#else
			      const char *cmpp;
			      int avail = width > 0 ? width : INT_MAX;

			      cmpp = mbdigits[n];
			      while ((unsigned char) *cmpp == c && avail > 0)
				{
				  if (*++cmpp == '\0')
				    break;
				  else
				    {
				      if ((c = inchar ()) == EOF)
					break;
				      --avail;
				    }
				}

			      if (*cmpp == '\0')
				{
				  if (width > 0)
				    width = avail;
				  break;
				}

			      /* We are pushing all read characters back.  */
			      if (cmpp > mbdigits[n])
				{
				  ungetc (c, s);
				  while (--cmpp > mbdigits[n])
				    ungetc_not_eof ((unsigned char) *cmpp, s);
				  c = (unsigned char) *cmpp;
				}

			      /* Advance the pointer to the next string.  */
			      mbdigits[n] = strchr (mbdigits[n], '\0') + 1;
#endif
			    }

			  if (n < 10)
			    {
			      /* Found it.  */
			      from_level = level;
			      to_level = level;
			      break;
			    }
			}
		    }

		  if (n < 10)
		    c = L_('0') + n;
		  else if ((flags & GROUP)
#ifdef COMPILE_WSCANF
			   && thousands != L'\0'
#else
			   && thousands != NULL
#endif
			   )
		    {
		      /* Try matching against the thousands separator.  */
#ifdef COMPILE_WSCANF
		      if (c != thousands)
			  break;
#else
		      const char *cmpp = thousands;
		      int avail = width > 0 ? width : INT_MAX;

		      while ((unsigned char) *cmpp == c && avail > 0)
			{
			  ADDW (c);
			  if (*++cmpp == '\0')
			    break;
			  else
			    {
			      if ((c = inchar ()) == EOF)
				break;
			      --avail;
			    }
			}

		      if (*cmpp != '\0')
			{
			  /* We are pushing all read characters back.  */
			  if (cmpp > thousands)
			    {
			      wpsize -= cmpp - thousands;
			      ungetc (c, s);
			      while (--cmpp > thousands)
				ungetc_not_eof ((unsigned char) *cmpp, s);
			      c = (unsigned char) *cmpp;
			    }
			  break;
			}

		      if (width > 0)
			width = avail;

		      /* The last thousands character will be added back by
			 the ADDW below.  */
			--wpsize;
#endif
		    }
		  else
		    break;

		  ADDW (c);
		  if (width > 0)
		    --width;

		  c = inchar ();
		}
	    }
	  else
	    /* Read the number into workspace.  */
	    while (c != EOF && width != 0)
	      {
		if (base == 16)
		  {
		    if (!ISXDIGIT (c))
		      break;
		  }
		else if (!ISDIGIT (c) || c - L_('0') >= base)
		  {
		    if (base == 10 && (flags & GROUP)
#ifdef COMPILE_WSCANF
			&& thousands != L'\0'
#else
			&& thousands != NULL
#endif
			)
		      {
			/* Try matching against the thousands separator.  */
#ifdef COMPILE_WSCANF
			if (c != thousands)
			  break;
#else
			const char *cmpp = thousands;
			int avail = width > 0 ? width : INT_MAX;

			while ((unsigned char) *cmpp == c && avail > 0)
			  {
			    ADDW (c);
			    if (*++cmpp == '\0')
			      break;
			    else
			      {
				if ((c = inchar ()) == EOF)
				  break;
				--avail;
			      }
			  }

			if (*cmpp != '\0')
			  {
			    /* We are pushing all read characters back.  */
			    if (cmpp > thousands)
			      {
				wpsize -= cmpp - thousands;
				ungetc (c, s);
				while (--cmpp > thousands)
				  ungetc_not_eof ((unsigned char) *cmpp, s);
				c = (unsigned char) *cmpp;
			      }
			    break;
			  }

			if (width > 0)
			  width = avail;

			/* The last thousands character will be added back by
			   the ADDW below.  */
			--wpsize;
#endif
		      }
		    else
		      break;
		  }
		ADDW (c);
		if (width > 0)
		  --width;

		c = inchar ();
	      }

	  if (wpsize == 0
	      || (wpsize == 1 && (wp[0] == L_('+') || wp[0] == L_('-'))))
	    {
	      /* There was no number.  If we are supposed to read a pointer
		 we must recognize "(nil)" as well.  */
	      if (wpsize == 0 && read_pointer && (width < 0 || width >= 0)
		  && c == '('
		  && TOLOWER (inchar ()) == L_('n')
		  && TOLOWER (inchar ()) == L_('i')
		  && TOLOWER (inchar ()) == L_('l')
		  && inchar () == L_(')'))
		/* We must produce the value of a NULL pointer.  A single
		   '0' digit is enough.  */
		ADDW (L_('0'));
	      else
		{
		  /* The last read character is not part of the number
		     anymore.  */
		  ungetc (c, s);

		  conv_error ();
		}
	    }
	  else
	    /* The just read character is not part of the number anymore.  */
	    ungetc (c, s);

	  /* Convert the number.  */
	  ADDW (L_('\0'));
	  if (need_longlong && (flags & LONGDBL))
	    {
	      if (number_signed)
		num.q = __strtoll_internal (wp, &tw, base, flags & GROUP);
	      else
		num.uq = __strtoull_internal (wp, &tw, base, flags & GROUP);
	    }
	  else
	    {
	      if (number_signed)
		num.l = __strtol_internal (wp, &tw, base, flags & GROUP);
	      else
		num.ul = __strtoul_internal (wp, &tw, base, flags & GROUP);
	    }
	  if (wp == tw)
	    conv_error ();

	  if (!(flags & SUPPRESS))
	    {
	      if (! number_signed)
		{
		  if (need_longlong && (flags & LONGDBL))
		    *ARG (unsigned LONGLONG int *) = num.uq;
		  else if (need_long && (flags & LONG))
		    *ARG (unsigned long int *) = num.ul;
		  else if (flags & SHORT)
		    *ARG (unsigned short int *)
		      = (unsigned short int) num.ul;
		  else if (!(flags & CHAR))
		    *ARG (unsigned int *) = (unsigned int) num.ul;
		  else
		    *ARG (unsigned char *) = (unsigned char) num.ul;
		}
	      else
		{
		  if (need_longlong && (flags & LONGDBL))
		    *ARG (LONGLONG int *) = num.q;
		  else if (need_long && (flags & LONG))
		    *ARG (long int *) = num.l;
		  else if (flags & SHORT)
		    *ARG (short int *) = (short int) num.l;
		  else if (!(flags & CHAR))
		    *ARG (int *) = (int) num.l;
		  else
		    *ARG (signed char *) = (signed char) num.ul;
		}
	      ++done;
	    }
	  break;

	case L_('e'):	/* Floating-point numbers.  */
	case L_('E'):
	case L_('f'):
	case L_('F'):
	case L_('g'):
	case L_('G'):
	case L_('a'):
	case L_('A'):
	  c = inchar ();
	  if (c == EOF)
	    input_error ();

	  /* Check for a sign.  */
	  if (c == L_('-') || c == L_('+'))
	    {
	      negative = c == L_('-');
	      if (width == 0 || inchar () == EOF)
		/* EOF is only an input error before we read any chars.  */
		conv_error ();
	      if (! ISDIGIT (c) && TOLOWER (c) != L_('i')
		  && TOLOWER (c) != L_('n'))
		{
#ifdef COMPILE_WSCANF
		  if (c != decimal)
		    {
		      /* This is no valid number.  */
		      ungetc (c, s);
		      conv_error ();
		    }
#else
		  /* Match against the decimal point.  At this point
                     we are taking advantage of the fact that we can
                     push more than one character back.  This is
                     (almost) never necessary since the decimal point
                     string hopefully never contains more than one
                     byte.  */
		  const char *cmpp = decimal;
		  int avail = width > 0 ? width : INT_MAX;

		  while ((unsigned char) *cmpp == c && avail > 0)
		    if (*++cmpp == '\0')
		      break;
		    else
		      {
			if (inchar () == EOF)
			  break;
			--avail;
		      }

		  if (*cmpp != '\0')
		    {
		      /* This is no valid number.  */
		      while (1)
			{
			  ungetc (c, s);
			  if (cmpp == decimal)
			    break;
			  c = (unsigned char) *--cmpp;
			}

		      conv_error ();
		    }
		  if (width > 0)
		    width = avail;
#endif
		}
	      if (width > 0)
		--width;
	    }
	  else
	    negative = 0;

	  /* Take care for the special arguments "nan" and "inf".  */
	  if (TOLOWER (c) == L_('n'))
	    {
	      /* Maybe "nan".  */
	      ADDW (c);
	      if (width == 0 || inchar () == EOF || TOLOWER (c) != L_('a'))
		conv_error ();
	      if (width > 0)
		--width;
	      ADDW (c);
	      if (width == 0 || inchar () == EOF || TOLOWER (c) != L_('n'))
		conv_error ();
	      if (width > 0)
		--width;
	      ADDW (c);
	      /* It is "nan".  */
	      goto scan_float;
	    }
	  else if (TOLOWER (c) == L_('i'))
	    {
	      /* Maybe "inf" or "infinity".  */
	      ADDW (c);
	      if (width == 0 || inchar () == EOF || TOLOWER (c) != L_('n'))
		conv_error ();
	      if (width > 0)
		--width;
	      ADDW (c);
	      if (width == 0 || inchar () == EOF || TOLOWER (c) != L_('f'))
		conv_error ();
	      if (width > 0)
		--width;
	      ADDW (c);
	      /* It is as least "inf".  */
	      if (width != 0 && inchar () != EOF)
		{
		  if (TOLOWER (c) == L_('i'))
		    {
		      if (width > 0)
			--width;
		      /* Now we have to read the rest as well.  */
		      ADDW (c);
		      if (width == 0 || inchar () == EOF
			  || TOLOWER (c) != L_('n'))
			conv_error ();
		      if (width > 0)
			--width;
		      ADDW (c);
		      if (width == 0 || inchar () == EOF
			  || TOLOWER (c) != L_('i'))
			conv_error ();
		      if (width > 0)
			--width;
		      ADDW (c);
		      if (width == 0 || inchar () == EOF
			  || TOLOWER (c) != L_('t'))
			conv_error ();
		      if (width > 0)
			--width;
		      ADDW (c);
		      if (width == 0 || inchar () == EOF
			  || TOLOWER (c) != L_('y'))
			conv_error ();
		      if (width > 0)
			--width;
		      ADDW (c);
		    }
		  else
		    /* Never mind.  */
		    ungetc (c, s);
		}
	      goto scan_float;
	    }

	  is_hexa = 0;
	  exp_char = L_('e');
	  if (width != 0 && c == L_('0'))
	    {
	      ADDW (c);
	      c = inchar ();
	      if (width > 0)
		--width;
	      if (width != 0 && TOLOWER (c) == L_('x'))
		{
		  /* It is a number in hexadecimal format.  */
		  ADDW (c);

		  is_hexa = 1;
		  exp_char = L_('p');

		  /* Grouping is not allowed.  */
		  flags &= ~GROUP;
		  c = inchar ();
		  if (width > 0)
		    --width;
		}
	    }

	  got_dot = got_e = 0;
	  do
	    {
	      if (ISDIGIT (c))
		ADDW (c);
	      else if (!got_e && is_hexa && ISXDIGIT (c))
		ADDW (c);
	      else if (got_e && wp[wpsize - 1] == exp_char
		       && (c == L_('-') || c == L_('+')))
		ADDW (c);
	      else if (wpsize > 0 && !got_e && TOLOWER (c) == exp_char)
		{
		  ADDW (exp_char);
		  got_e = got_dot = 1;
		}
	      else
		{
#ifdef COMPILE_WSCANF
		  if (! got_dot && c == decimal)
		    {
		      ADDW (c);
		      got_dot = 1;
		    }
		  else if (thousands != L'\0' && ! got_dot && c == thousands)
		    ADDW (c);
		  else
		    {
		      /* The last read character is not part of the number
			 anymore.  */
		      ungetc (c, s);
		      break;
		    }
#else
		  const char *cmpp = decimal;
		  int avail = width > 0 ? width : INT_MAX;

		  if (! got_dot)
		    {
		      while ((unsigned char) *cmpp == c && avail > 0)
			if (*++cmpp == '\0')
			  break;
			else
			  {
			    if (inchar () == EOF)
			      break;
			    --avail;
			  }
		    }

		  if (*cmpp == '\0')
		    {
		      /* Add all the characters.  */
		      for (cmpp = decimal; *cmpp != '\0'; ++cmpp)
			ADDW ((unsigned char) *cmpp);
		      if (width > 0)
			width = avail;
		      got_dot = 1;
		    }
		  else
		    {
		      /* Figure out whether it is a thousands separator.
			 There is one problem: we possibly read more than
			 one character.  We cannot push them back but since
			 we know that parts of the `decimal' string matched,
			 we can compare against it.  */
		      const char *cmp2p = thousands;

		      if (thousands != NULL && ! got_dot)
			{
			  while (cmp2p < cmpp
				 && *cmp2p == decimal[cmp2p - thousands])
			    ++cmp2p;
			  if (cmp2p == cmpp)
			    {
			      while ((unsigned char) *cmp2p == c && avail > 0)
				if (*++cmp2p == '\0')
				  break;
				else
				  {
				    if (inchar () == EOF)
				      break;
				    --avail;
				  }
			    }
			}

		      if (cmp2p != NULL && *cmp2p == '\0')
			{
			  /* Add all the characters.  */
			  for (cmpp = thousands; *cmpp != '\0'; ++cmpp)
			    ADDW ((unsigned char) *cmpp);
			  if (width > 0)
			    width = avail;
			}
		      else
			{
			  /* The last read character is not part of the number
			     anymore.  */
			  ungetc (c, s);
			  break;
			}
		    }
#endif
		}
	      if (width > 0)
		--width;
	    }
	  while (width != 0 && inchar () != EOF);

	  /* Have we read any character?  If we try to read a number
	     in hexadecimal notation and we have read only the `0x'
	     prefix or no exponent this is an error.  */
	  if (wpsize == 0 || (is_hexa && (wpsize == 2 || ! got_e)))
	    conv_error ();

	scan_float:
	  /* Convert the number.  */
	  ADDW (L_('\0'));
	  if (flags & LONGDBL)
	    {
	      long double d = __strtold_internal (wp, &tw, flags & GROUP);
	      if (!(flags & SUPPRESS) && tw != wp)
		*ARG (long double *) = negative ? -d : d;
	    }
	  else if (flags & LONG)
	    {
	      double d = __strtod_internal (wp, &tw, flags & GROUP);
	      if (!(flags & SUPPRESS) && tw != wp)
		*ARG (double *) = negative ? -d : d;
	    }
	  else
	    {
	      float d = __strtof_internal (wp, &tw, flags & GROUP);
	      if (!(flags & SUPPRESS) && tw != wp)
		*ARG (float *) = negative ? -d : d;
	    }

	  if (tw == wp)
	    conv_error ();

	  if (!(flags & SUPPRESS))
	    ++done;
	  break;

	case L_('['):	/* Character class.  */
	  if (flags & LONG)
	    STRING_ARG (wstr, wchar_t);
	  else
	    STRING_ARG (str, char);

	  if (*f == L_('^'))
	    {
	      ++f;
	      not_in = 1;
	    }
	  else
	    not_in = 0;

	  if (width < 0)
	    /* There is no width given so there is also no limit on the
	       number of characters we read.  Therefore we set width to
	       a very high value to make the algorithm easier.  */
	    width = INT_MAX;

#ifdef COMPILE_WSCANF
	  /* Find the beginning and the end of the scanlist.  We are not
	     creating a lookup table since it would have to be too large.
	     Instead we search each time through the string.  This is not
	     a constant lookup time but who uses this feature deserves to
	     be punished.  */
	  tw = (wchar_t *) f;	/* Marks the beginning.  */

	  if (*f == L']')
	    ++f;

	  while ((fc = *f++) != L'\0' && fc != L']');

	  if (fc == L'\0')
	    conv_error ();
	  wp = (wchar_t *) f - 1;
#else
	  /* Fill WP with byte flags indexed by character.
	     We will use this flag map for matching input characters.  */
	  if (wpmax < UCHAR_MAX + 1)
	    {
	      wpmax = UCHAR_MAX + 1;
	      wp = (char *) alloca (wpmax);
	    }
	  memset (wp, '\0', UCHAR_MAX + 1);

	  fc = *f;
	  if (fc == ']' || fc == '-')
	    {
	      /* If ] or - appears before any char in the set, it is not
		 the terminator or separator, but the first char in the
		 set.  */
	      wp[fc] = 1;
	      ++f;
	    }

	  while ((fc = *f++) != '\0' && fc != ']')
	    if (fc == '-' && *f != '\0' && *f != ']'
		&& (unsigned char) f[-2] <= (unsigned char) *f)
	      {
		/* Add all characters from the one before the '-'
		   up to (but not including) the next format char.  */
		for (fc = (unsigned char) f[-2]; fc < (unsigned char) *f; ++fc)
		  wp[fc] = 1;
	      }
	    else
	      /* Add the character to the flag map.  */
	      wp[fc] = 1;

	  if (fc == '\0')
	    conv_error();
#endif

	  if (flags & LONG)
	    {
	      size_t now = read_in;
#ifdef COMPILE_WSCANF
	      if (inchar () == WEOF)
		input_error ();

	      do
		{
		  wchar_t *runp;

		  /* Test whether it's in the scanlist.  */
		  runp = tw;
		  while (runp < wp)
		    {
		      if (runp[0] == L'-' && runp[1] != '\0' && runp + 1 != wp
			  && runp != tw
			  && (unsigned int) runp[-1] <= (unsigned int) runp[1])
			{
			  /* Match against all characters in between the
			     first and last character of the sequence.  */
			  wchar_t wc;

			  for (wc = runp[-1] + 1; wc <= runp[1]; ++wc)
			    if (wc == c)
			      break;

			  if (wc <= runp[1] && !not_in)
			    break;
			  if (wc <= runp[1] && not_in)
			    {
			      /* The current character is not in the
                                 scanset.  */
			      ungetc (c, s);
			      goto out;
			    }

			  runp += 2;
			}
		      else
			{
			  if (*runp == c && !not_in)
			    break;
			  if (*runp == c && not_in)
			    {
				  ungetc (c, s);
			      goto out;
			    }

			  ++runp;
			}
		    }

		  if (runp == wp && !not_in)
		    {
		      ungetc (c, s);
		      goto out;
		    }

		  if (!(flags & SUPPRESS))
		    {
		      *wstr++ = c;

		      if ((flags & MALLOC)
			  && wstr == (wchar_t *) *strptr + strsize)
			{
			  /* Enlarge the buffer.  */
			  wstr = (wchar_t *) realloc (*strptr,
						      (2 * strsize)
						      * sizeof (wchar_t));
			  if (wstr == NULL)
			    {
			      /* Can't allocate that much.  Last-ditch
				 effort.  */
			      wstr = (wchar_t *)
				realloc (*strptr, (strsize + 1)
						  * sizeof (wchar_t));
			      if (wstr == NULL)
				{
				  /* We lose.  Oh well.  Terminate the string
				     and stop converting, so at least we don't
				     skip any input.  */
				  ((wchar_t *) (*strptr))[strsize - 1] = L'\0';
				  ++done;
				  conv_error ();
				}
			      else
				{
				  *strptr = (char *) wstr;
				  wstr += strsize;
				  ++strsize;
				}
			    }
			  else
			    {
			      *strptr = (char *) wstr;
			      wstr += strsize;
			      strsize *= 2;
			    }
			}
		    }
		}
	      while (--width > 0 && inchar () != WEOF);
	    out:
#else
	      char buf[MB_LEN_MAX];
	      size_t cnt = 0;
	      mbstate_t cstate;

	      if (inchar () == EOF)
		input_error ();

	      memset (&cstate, '\0', sizeof (cstate));

	      do
		{
		  if (wp[c] == not_in)
		    {
		      ungetc_not_eof (c, s);
		      break;
		    }

		  /* This is easy.  */
		  if (!(flags & SUPPRESS))
		    {
		      size_t n;

		      /* Convert it into a wide character.  */
		      buf[0] = c;
		      n = __mbrtowc (wstr, buf, 1, &cstate);

		      if (n == (size_t) -2)
			{
			  /* Possibly correct character, just not enough
			     input.  */
			  ++cnt;
			  assert (cnt < MB_CUR_MAX);
			  continue;
			}

		      ++wstr;
		      if ((flags & MALLOC)
			  && wstr == (wchar_t *) *strptr + strsize)
			{
			  /* Enlarge the buffer.  */
			  wstr = (wchar_t *) realloc (*strptr,
						      (2 * strsize
						       * sizeof (wchar_t)));
			  if (wstr == NULL)
			    {
			      /* Can't allocate that much.  Last-ditch
				 effort.  */
			      wstr = (wchar_t *)
				realloc (*strptr, ((strsize + 1)
						   * sizeof (wchar_t)));
			      if (wstr == NULL)
				{
				  /* We lose.  Oh well.  Terminate the
				     string and stop converting,
				     so at least we don't skip any input.  */
				  ((wchar_t *) (*strptr))[strsize - 1] = L'\0';
				  ++done;
				  conv_error ();
				}
			      else
				{
				  *strptr = (char *) wstr;
				  wstr += strsize;
				  ++strsize;
				}
			    }
			  else
			    {
			      *strptr = (char *) wstr;
			      wstr += strsize;
			      strsize *= 2;
			    }
			}
		    }

		  if (--width <= 0)
		    break;
		}
	      while (inchar () != EOF);

	      if (cnt != 0)
		/* We stopped in the middle of recognizing another
		   character.  That's a problem.  */
		encode_error ();
#endif

	      if (now == read_in)
		/* We haven't succesfully read any character.  */
		conv_error ();

	      if (!(flags & SUPPRESS))
		{
		  *wstr++ = L'\0';

		  if ((flags & MALLOC)
		      && wstr - (wchar_t *) *strptr != strsize)
		    {
		      wchar_t *cp = (wchar_t *)
			realloc (*strptr, ((wstr - (wchar_t *) *strptr)
					   * sizeof(wchar_t)));
		      if (cp != NULL)
			*strptr = (char *) cp;
		    }

		  ++done;
		}
	    }
	  else
	    {
	      size_t now = read_in;

	      if (inchar () == EOF)
		input_error ();

#ifdef COMPILE_WSCANF

	      memset (&state, '\0', sizeof (state));

	      do
		{
		  wchar_t *runp;
		  size_t n;

		  /* Test whether it's in the scanlist.  */
		  runp = tw;
		  while (runp < wp)
		    {
		      if (runp[0] == L'-' && runp[1] != '\0' && runp + 1 != wp
			  && runp != tw
			  && (unsigned int) runp[-1] <= (unsigned int) runp[1])
			{
			  /* Match against all characters in between the
			     first and last character of the sequence.  */
			  wchar_t wc;

			  for (wc = runp[-1] + 1; wc <= runp[1]; ++wc)
			    if (wc == c)
			      break;

			  if (wc <= runp[1] && !not_in)
			    break;
			  if (wc <= runp[1] && not_in)
			    {
			      /* The current character is not in the
                                 scanset.  */
			      ungetc (c, s);
			      goto out2;
			    }

			  runp += 2;
			}
		      else
			{
			  if (*runp == c && !not_in)
			    break;
			  if (*runp == c && not_in)
			    {
			      ungetc (c, s);
			      goto out2;
			    }

			  ++runp;
			}
		    }

		  if (runp == wp && !not_in)
		    {
		      ungetc (c, s);
		      goto out2;
		    }

		  if (!(flags & SUPPRESS))
		    {
		      if ((flags & MALLOC)
			  && str + MB_CUR_MAX >= *strptr + strsize)
			{
			  /* Enlarge the buffer.  */
			  size_t strleng = str - *strptr;
			  char *newstr;

			  newstr = (char *) realloc (*strptr, 2 * strsize);
			  if (newstr == NULL)
			    {
			      /* Can't allocate that much.  Last-ditch
				 effort.  */
			      newstr = (char *) realloc (*strptr,
							 strleng + MB_CUR_MAX);
			      if (newstr == NULL)
				{
				  /* We lose.  Oh well.  Terminate the string
				     and stop converting, so at least we don't
				     skip any input.  */
				  ((char *) (*strptr))[strleng] = '\0';
				  ++done;
				  conv_error ();
				}
			      else
				{
				  *strptr = newstr;
				  str = newstr + strleng;
				  strsize = strleng + MB_CUR_MAX;
				}
			    }
			  else
			    {
			      *strptr = newstr;
			      str = newstr + strleng;
			      strsize *= 2;
			    }
			}
		    }

		  n = __wcrtomb (!(flags & SUPPRESS) ? str : NULL, c, &state);
		  if (n == (size_t) -1)
		    encode_error ();

		  assert (n <= MB_CUR_MAX);
		  str += n;
		}
	      while (--width > 0 && inchar () != WEOF);
	    out2:
#else
	      do
		{
		  if (wp[c] == not_in)
		    {
		      ungetc_not_eof (c, s);
		      break;
		    }

		  /* This is easy.  */
		  if (!(flags & SUPPRESS))
		    {
		      *str++ = c;
		      if ((flags & MALLOC)
			  && (char *) str == *strptr + strsize)
			{
			  /* Enlarge the buffer.  */
			  str = (char *) realloc (*strptr, 2 * strsize);
			  if (str == NULL)
			    {
			      /* Can't allocate that much.  Last-ditch
				 effort.  */
			      str = (char *) realloc (*strptr, strsize + 1);
			      if (str == NULL)
				{
				  /* We lose.  Oh well.  Terminate the
				     string and stop converting,
				     so at least we don't skip any input.  */
				  ((char *) (*strptr))[strsize - 1] = '\0';
				  ++done;
				  conv_error ();
				}
			      else
				{
				  *strptr = (char *) str;
				  str += strsize;
				  ++strsize;
				}
			    }
			  else
			    {
			      *strptr = (char *) str;
			      str += strsize;
			      strsize *= 2;
			    }
			}
		    }
		}
	      while (--width > 0 && inchar () != EOF);
#endif

	      if (now == read_in)
		/* We haven't succesfully read any character.  */
		conv_error ();

	      if (!(flags & SUPPRESS))
		{
#ifdef COMPILE_WSCANF
		  /* We have to emit the code to get into the initial
		     state.  */
		  char buf[MB_LEN_MAX];
		  size_t n = __wcrtomb (buf, L'\0', &state);
		  if (n > 0 && (flags & MALLOC)
		      && str + n >= *strptr + strsize)
		    {
		      /* Enlarge the buffer.  */
		      size_t strleng = str - *strptr;
		      char *newstr;

		      newstr = (char *) realloc (*strptr, strleng + n + 1);
		      if (newstr == NULL)
			{
			  /* We lose.  Oh well.  Terminate the string
			     and stop converting, so at least we don't
			     skip any input.  */
			  ((char *) (*strptr))[strleng] = '\0';
			  ++done;
			  conv_error ();
			}
		      else
			{
			  *strptr = newstr;
			  str = newstr + strleng;
			  strsize = strleng + n + 1;
			}
		    }

		  str = __mempcpy (str, buf, n);
#endif
		  *str++ = '\0';

		  if ((flags & MALLOC) && str - *strptr != strsize)
		    {
		      char *cp = (char *) realloc (*strptr, str - *strptr);
		      if (cp != NULL)
			*strptr = cp;
		    }

		  ++done;
		}
	    }
	  break;

	case L_('p'):	/* Generic pointer.  */
	  base = 16;
	  /* A PTR must be the same size as a `long int'.  */
	  flags &= ~(SHORT|LONGDBL);
	  if (need_long)
	    flags |= LONG;
	  number_signed = 0;
	  read_pointer = 1;
	  goto number;

	default:
	  /* If this is an unknown format character punt.  */
	  conv_error ();
	}
    }

  /* The last thing we saw int the format string was a white space.
     Consume the last white spaces.  */
  if (skip_space)
    {
      do
	c = inchar ();
      while (ISSPACE (c));
      ungetc (c, s);
    }

  /* Unlock stream.  */
  UNLOCK_STREAM (s);

  return done;
}

#ifdef USE_IN_LIBIO
# ifdef COMPILE_WSCANF
int
__vfwscanf (FILE *s, const wchar_t *format, va_list argptr)
{
  return _IO_vfwscanf (s, format, argptr, NULL);
}
# else
int
__vfscanf (FILE *s, const char *format, va_list argptr)
{
  return _IO_vfscanf (s, format, argptr, NULL);
}
# endif
#endif

#ifdef COMPILE_WSCANF
weak_alias (__vfwscanf, vfwscanf)
#else
weak_alias (__vfscanf, vfscanf)
#endif
