/* Copyright (C) 1995-1999, 2000, 2001 Free Software Foundation, Inc.
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

/*
 *      ISO C99 Standard: 7.24
 *	Extended multibyte and wide character utilities	<wchar.h>
 */

#ifndef _WCHAR_H

#ifndef __need_mbstate_t
# define _WCHAR_H 1
# include <features.h>
#endif

#ifdef _WCHAR_H
/* Get FILE definition.  */
# define __need___FILE
# ifdef __USE_UNIX98
#  define __need_FILE
# endif
# include <libio/stdio.h>
/* Get va_list definition.  */
# define __need___va_list
# include <stdarg.h>

/* Get size_t, wchar_t, wint_t and NULL from <stddef.h>.  */
# define __need_size_t
# define __need_wchar_t
# define __need_NULL
#endif
#define __need_wint_t
#include <stddef.h>

#include <bits/wchar.h>

/* We try to get wint_t from <stddef.h>, but not all GCC versions define it
   there.  So define it ourselves if it remains undefined.  */
#ifndef _WINT_T
/* Integral type unchanged by default argument promotions that can
   hold any value corresponding to members of the extended character
   set, as well as at least one value that does not correspond to any
   member of the extended character set.  */
# define _WINT_T
typedef unsigned int wint_t;
#endif


#ifndef __mbstate_t_defined
# define __mbstate_t_defined	1
/* Conversion state information - for now a mixture between glibc's own idea of
   mbstate_t and our own.  */
typedef struct
{
  void* converter;
  char charset[64];
  union
  {
    unsigned int count;
    int __count;
  };
  union
  {
	char data[1024 + 8];	// 1024 bytes for data, 8 for alignment space
	union
	{
	  wint_t __wch;
	  char __wchb[4];
	} __value;		/* Value so far.  */
  };
} __mbstate_t;
#endif
#undef __need_mbstate_t


/* The rest of the file is only used if used if __need_mbstate_t is not
   defined.  */
#ifdef _WCHAR_H

/* Public type.  */
typedef __mbstate_t mbstate_t;

#ifndef WCHAR_MIN
/* These constants might also be defined in <inttypes.h>.  */
# define WCHAR_MIN __WCHAR_MIN
# define WCHAR_MAX __WCHAR_MAX
#endif

#ifndef WEOF
# define WEOF (0xffffffffu)
#endif

/* For XPG4 compliance we have to define the stuff from <wctype.h> here
   as well.  */
#if defined __USE_XOPEN && !defined __USE_UNIX98
# include <wctype.h>
#endif

/* This incomplete type is defined in <time.h> but needed here because
   of `wcsftime'.  */
struct tm;


__BEGIN_DECLS



/* Convert initial portion of the wide string NPTR to `double'
   representation.  */
extern double wcstod (__const wchar_t *__restrict __nptr,
		      wchar_t **__restrict __endptr) __THROW;

#ifdef __USE_ISOC99
/* Likewise for `float' and `long double' sizes of floating-point numbers.  */
extern float wcstof (__const wchar_t *__restrict __nptr,
		     wchar_t **__restrict __endptr) __THROW;
extern long double wcstold (__const wchar_t *__restrict __nptr,
			    wchar_t **__restrict __endptr) __THROW;
#endif /* C99 */


/* Convert initial portion of wide string NPTR to `long int'
   representation.  */
extern long int wcstol (__const wchar_t *__restrict __nptr,
			wchar_t **__restrict __endptr, int __base) __THROW;

/* Convert initial portion of wide string NPTR to `unsigned long int'
   representation.  */
extern unsigned long int wcstoul (__const wchar_t *__restrict __nptr,
				  wchar_t **__restrict __endptr, int __base)
     __THROW;

#if defined __GNUC__ && defined __USE_GNU
/* Convert initial portion of wide string NPTR to `long int'
   representation.  */
__extension__
extern long long int wcstoq (__const wchar_t *__restrict __nptr,
			     wchar_t **__restrict __endptr, int __base)
     __THROW;

/* Convert initial portion of wide string NPTR to `unsigned long long int'
   representation.  */
__extension__
extern unsigned long long int wcstouq (__const wchar_t *__restrict __nptr,
				       wchar_t **__restrict __endptr,
				       int __base) __THROW;
#endif /* GCC and use GNU.  */

#if defined __USE_ISOC99 || (defined __GNUC__ && defined __USE_GNU)
/* Convert initial portion of wide string NPTR to `long int'
   representation.  */
__extension__
extern long long int wcstoll (__const wchar_t *__restrict __nptr,
			      wchar_t **__restrict __endptr, int __base)
     __THROW;

/* Convert initial portion of wide string NPTR to `unsigned long long int'
   representation.  */
__extension__
extern unsigned long long int wcstoull (__const wchar_t *__restrict __nptr,
					wchar_t **__restrict __endptr,
					int __base) __THROW;
#endif /* ISO C99 or GCC and GNU.  */


/* The internal entry points for `wcstoX' take an extra flag argument
   saying whether or not to parse locale-dependent number grouping.  */
extern double __wcstod_internal (__const wchar_t *__restrict __nptr,
				 wchar_t **__restrict __endptr, int __group)
     __THROW;
extern float __wcstof_internal (__const wchar_t *__restrict __nptr,
				wchar_t **__restrict __endptr, int __group)
     __THROW;
extern long double __wcstold_internal (__const wchar_t *__restrict __nptr,
				       wchar_t **__restrict __endptr,
				       int __group) __THROW;

#ifndef __wcstol_internal_defined
extern long int __wcstol_internal (__const wchar_t *__restrict __nptr,
				   wchar_t **__restrict __endptr,
				   int __base, int __group) __THROW;
# define __wcstol_internal_defined	1
#endif
#ifndef __wcstoul_internal_defined
extern unsigned long int __wcstoul_internal (__const wchar_t *__restrict __npt,
					     wchar_t **__restrict __endptr,
					     int __base, int __group) __THROW;
# define __wcstoul_internal_defined	1
#endif
#ifndef __wcstoll_internal_defined
__extension__
extern long long int __wcstoll_internal (__const wchar_t *__restrict __nptr,
					 wchar_t **__restrict __endptr,
					 int __base, int __group) __THROW;
# define __wcstoll_internal_defined	1
#endif
#ifndef __wcstoull_internal_defined
__extension__
extern unsigned long long int __wcstoull_internal (__const wchar_t *
						   __restrict __nptr,
						   wchar_t **
						   __restrict __endptr,
						   int __base,
						   int __group) __THROW;
# define __wcstoull_internal_defined	1
#endif


#if defined __OPTIMIZE__ && __GNUC__ >= 2
/* Define inline functions which call the internal entry points.  */

extern __inline double wcstod (__const wchar_t *__restrict __nptr,
			       wchar_t **__restrict __endptr) __THROW
{ return __wcstod_internal (__nptr, __endptr, 0); }
extern __inline long int wcstol (__const wchar_t *__restrict __nptr,
                                 wchar_t **__restrict __endptr,
				 int __base) __THROW
{ return __wcstol_internal (__nptr, __endptr, __base, 0); }
extern __inline unsigned long int wcstoul (__const wchar_t *__restrict __nptr,
                                           wchar_t **__restrict __endptr,
					   int __base) __THROW
{ return __wcstoul_internal (__nptr, __endptr, __base, 0); }

# ifdef __USE_GNU
extern __inline float wcstof (__const wchar_t *__restrict __nptr,
			      wchar_t **__restrict __endptr) __THROW
{ return __wcstof_internal (__nptr, __endptr, 0); }
extern __inline long double wcstold (__const wchar_t *__restrict __nptr,
				     wchar_t **__restrict __endptr) __THROW
{ return __wcstold_internal (__nptr, __endptr, 0); }


__extension__
extern __inline long long int wcstoq (__const wchar_t *__restrict __nptr,
				      wchar_t **__restrict __endptr,
				      int __base) __THROW
{ return __wcstoll_internal (__nptr, __endptr, __base, 0); }
__extension__
extern __inline unsigned long long int wcstouq (__const wchar_t *
						__restrict __nptr,
						wchar_t **__restrict __endptr,
						int __base) __THROW
{ return __wcstoull_internal (__nptr, __endptr, __base, 0); }
# endif /* Use GNU.  */
#endif /* Optimizing GCC >=2.  */


/* Wide character I/O functions.  */
#if defined __USE_ISOC99 || defined __USE_UNIX98

/* Select orientation for stream.  */
extern int fwide (__FILE *__fp, int __mode) __THROW;


/* Write formatted output to STREAM.  */
extern int fwprintf (__FILE *__restrict __stream,
		     __const wchar_t *__restrict __format, ...)
     __THROW /* __attribute__ ((__format__ (__wprintf__, 2, 3))) */;
/* Write formatted output to stdout.  */
extern int wprintf (__const wchar_t *__restrict __format, ...)
     __THROW /* __attribute__ ((__format__ (__wprintf__, 1, 2))) */;
/* Write formatted output of at most N characters to S.  */
extern int swprintf (wchar_t *__restrict __s, size_t __n,
		     __const wchar_t *__restrict __format, ...)
     __THROW /* __attribute__ ((__format__ (__wprintf__, 3, 4))) */;

/* Write formatted output to S from argument list ARG.  */
extern int vfwprintf (__FILE *__restrict __s,
		      __const wchar_t *__restrict __format,
		      __gnuc_va_list __arg)
     __THROW /* __attribute__ ((__format__ (__wprintf__, 2, 0))) */;
/* Write formatted output to stdout from argument list ARG.  */
extern int vwprintf (__const wchar_t *__restrict __format,
		     __gnuc_va_list __arg)
     __THROW /* __attribute__ ((__format__ (__wprintf__, 1, 0))) */;
/* Write formatted output of at most N character to S from argument
   list ARG.  */
extern int vswprintf (wchar_t *__restrict __s, size_t __n,
		      __const wchar_t *__restrict __format,
		      __gnuc_va_list __arg)
     __THROW /* __attribute__ ((__format__ (__wprintf__, 3, 0))) */;


/* Read formatted input from STREAM.  */
extern int fwscanf (__FILE *__restrict __stream,
		    __const wchar_t *__restrict __format, ...)
     __THROW /* __attribute__ ((__format__ (__wscanf__, 2, 3))) */;
/* Read formatted input from stdin.  */
extern int wscanf (__const wchar_t *__restrict __format, ...)
     __THROW /* __attribute__ ((__format__ (__wscanf__, 1, 2))) */;
/* Read formatted input from S.  */
extern int swscanf (__const wchar_t *__restrict __s,
		    __const wchar_t *__restrict __format, ...)
     __THROW /* __attribute__ ((__format__ (__wscanf__, 2, 3))) */;
#endif /* Use ISO C99 and Unix98. */

#ifdef __USE_ISOC99
/* Read formatted input from S into argument list ARG.  */
extern int vfwscanf (__FILE *__restrict __s,
		     __const wchar_t *__restrict __format,
		     __gnuc_va_list __arg)
     __THROW /* __attribute__ ((__format__ (__wscanf__, 2, 0))) */;
/* Read formatted input from stdin into argument list ARG.  */
extern int vwscanf (__const wchar_t *__restrict __format,
		    __gnuc_va_list __arg)
     __THROW /* __attribute__ ((__format__ (__wscanf__, 1, 0))) */;
/* Read formatted input from S into argument list ARG.  */
extern int vswscanf (__const wchar_t *__restrict __s,
		     __const wchar_t *__restrict __format,
		     __gnuc_va_list __arg)
     __THROW /* __attribute__ ((__format__ (__wscanf__, 2, 0))) */;
#endif /* Use ISO C99. */


/* Read a character from STREAM.  */
extern wint_t fgetwc (__FILE *__stream) __THROW;
extern wint_t getwc (__FILE *__stream) __THROW;

/* Read a character from stdin.  */
extern wint_t getwchar (void) __THROW;


/* Write a character to STREAM.  */
extern wint_t fputwc (wchar_t __wc, __FILE *__stream) __THROW;
extern wint_t putwc (wchar_t __wc, __FILE *__stream) __THROW;

/* Write a character to stdout.  */
extern wint_t putwchar (wchar_t __wc) __THROW;


/* Get a newline-terminated wide character string of finite length
   from STREAM.  */
extern wchar_t *fgetws (wchar_t *__restrict __ws, int __n,
			__FILE *__restrict __stream) __THROW;

/* Write a string to STREAM.  */
extern int fputws (__const wchar_t *__restrict __ws,
		   __FILE *__restrict __stream) __THROW;


/* Push a character back onto the input buffer of STREAM.  */
extern wint_t ungetwc (wint_t __wc, __FILE *__stream) __THROW;


#ifdef __USE_GNU
/* These are defined to be equivalent to the `char' functions defined
   in POSIX.1:1996.  */
extern wint_t getwc_unlocked (__FILE *__stream) __THROW;
extern wint_t getwchar_unlocked (void) __THROW;

/* This is the wide character version of a GNU extension.  */
extern wint_t fgetwc_unlocked (__FILE *__stream) __THROW;

/* Faster version when locking is not necessary.  */
extern wint_t fputwc_unlocked (wchar_t __wc, __FILE *__stream) __THROW;

/* These are defined to be equivalent to the `char' functions defined
   in POSIX.1:1996.  */
extern wint_t putwc_unlocked (wchar_t __wc, __FILE *__stream) __THROW;
extern wint_t putwchar_unlocked (wchar_t __wc) __THROW;


/* This function does the same as `fgetws' but does not lock the stream.  */
extern wchar_t *fgetws_unlocked (wchar_t *__restrict __ws, int __n,
				 __FILE *__restrict __stream) __THROW;

/* This function does the same as `fputws' but does not lock the stream.  */
extern int fputws_unlocked (__const wchar_t *__restrict __ws,
			    __FILE *__restrict __stream) __THROW;
#endif


__END_DECLS

#endif	/* _WCHAR_H defined */

#endif /* wchar.h  */
