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

/* Copy SRC to DEST.  */
extern wchar_t *wcscpy (wchar_t *__restrict __dest,
			__const wchar_t *__restrict __src) __THROW;
/* Copy no more than N wide-characters of SRC to DEST.  */
extern wchar_t *wcsncpy (wchar_t *__restrict __dest,
			 __const wchar_t *__restrict __src, size_t __n)
     __THROW;

/* Append SRC onto DEST.  */
extern wchar_t *wcscat (wchar_t *__restrict __dest,
			__const wchar_t *__restrict __src) __THROW;
/* Append no more than N wide-characters of SRC onto DEST.  */
extern wchar_t *wcsncat (wchar_t *__restrict __dest,
			 __const wchar_t *__restrict __src, size_t __n)
     __THROW;

/* Compare S1 and S2.  */
extern int wcscmp (__const wchar_t *__s1, __const wchar_t *__s2)
     __THROW __attribute_pure__;
/* Compare N wide-characters of S1 and S2.  */
extern int wcsncmp (__const wchar_t *__s1, __const wchar_t *__s2, size_t __n)
     __THROW __attribute_pure__;

#ifdef __USE_GNU
/* Compare S1 and S2, ignoring case.  */
extern int wcscasecmp (__const wchar_t *__s1, __const wchar_t *__s2) __THROW;

/* Compare no more than N chars of S1 and S2, ignoring case.  */
extern int wcsncasecmp (__const wchar_t *__s1, __const wchar_t *__s2,
			size_t __n) __THROW;

/* Similar to the two functions above but take the information from
   the provided locale and not the global locale.  */
# include <xlocale.h>

extern int __wcscasecmp_l (__const wchar_t *__s1, __const wchar_t *__s2,
			   __locale_t __loc) __THROW;

extern int __wcsncasecmp_l (__const wchar_t *__s1, __const wchar_t *__s2,
			    size_t __n, __locale_t __loc) __THROW;
#endif

/* Compare S1 and S2, both interpreted as appropriate to the
   LC_COLLATE category of the current locale.  */
extern int wcscoll (__const wchar_t *__s1, __const wchar_t *__s2) __THROW;
/* Transform S2 into array pointed to by S1 such that if wcscmp is
   applied to two transformed strings the result is the as applying
   `wcscoll' to the original strings.  */
extern size_t wcsxfrm (wchar_t *__restrict __s1,
		       __const wchar_t *__restrict __s2, size_t __n) __THROW;

#ifdef __USE_GNU
/* Similar to the two functions above but take the information from
   the provided locale and not the global locale.  */

/* Compare S1 and S2, both interpreted as appropriate to the
   LC_COLLATE category of the given locale.  */
extern int __wcscoll_l (__const wchar_t *__s1, __const wchar_t *__s2,
			__locale_t __loc) __THROW;
/* Transform S2 into array pointed to by S1 such that if wcscmp is
   applied to two transformed strings the result is the as applying
   `wcscoll' to the original strings.  */
extern size_t __wcsxfrm_l (wchar_t *__s1, __const wchar_t *__s2,
			   size_t __n, __locale_t __loc) __THROW;

/* Duplicate S, returning an identical malloc'd string.  */
extern wchar_t *wcsdup (__const wchar_t *__s) __THROW __attribute_malloc__;
#endif

/* Find the first occurrence of WC in WCS.  */
extern wchar_t *wcschr (__const wchar_t *__wcs, wchar_t __wc)
     __THROW __attribute_pure__;
/* Find the last occurrence of WC in WCS.  */
extern wchar_t *wcsrchr (__const wchar_t *__wcs, wchar_t __wc)
     __THROW __attribute_pure__;

#ifdef __USE_GNU
/* This function is similar to `wcschr'.  But it returns a pointer to
   the closing NUL wide character in case C is not found in S.  */
extern wchar_t *wcschrnul (__const wchar_t *__s, wchar_t __wc)
     __THROW __attribute_pure__;
#endif

/* Return the length of the initial segmet of WCS which
   consists entirely of wide characters not in REJECT.  */
extern size_t wcscspn (__const wchar_t *__wcs, __const wchar_t *__reject)
     __THROW __attribute_pure__;
/* Return the length of the initial segmet of WCS which
   consists entirely of wide characters in  ACCEPT.  */
extern size_t wcsspn (__const wchar_t *__wcs, __const wchar_t *__accept)
     __THROW __attribute_pure__;
/* Find the first occurrence in WCS of any character in ACCEPT.  */
extern wchar_t *wcspbrk (__const wchar_t *__wcs, __const wchar_t *__accept)
     __THROW __attribute_pure__;
/* Find the first occurrence of NEEDLE in HAYSTACK.  */
extern wchar_t *wcsstr (__const wchar_t *__haystack, __const wchar_t *__needle)
     __THROW __attribute_pure__;

#ifdef __USE_XOPEN
/* Another name for `wcsstr' from XPG4.  */
extern wchar_t *wcswcs (__const wchar_t *__haystack, __const wchar_t *__needle)
     __THROW __attribute_pure__;
#endif

/* Divide WCS into tokens separated by characters in DELIM.  */
extern wchar_t *wcstok (wchar_t *__restrict __s,
			__const wchar_t *__restrict __delim,
			wchar_t **__restrict __ptr) __THROW;

/* Return the number of wide characters in S.  */
extern size_t wcslen (__const wchar_t *__s) __THROW __attribute_pure__;

#ifdef __USE_GNU
/* Return the number of wide characters in S, but at most MAXLEN.  */
extern size_t wcsnlen (__const wchar_t *__s, size_t __maxlen)
     __THROW __attribute_pure__;
#endif


/* Search N wide characters of S for C.  */
extern wchar_t *wmemchr (__const wchar_t *__s, wchar_t __c, size_t __n)
     __THROW __attribute_pure__;

/* Compare N wide characters of S1 and S2.  */
extern int wmemcmp (__const wchar_t *__restrict __s1,
		    __const wchar_t *__restrict __s2, size_t __n)
     __THROW __attribute_pure__;

/* Copy N wide characters of SRC to DEST.  */
extern wchar_t *wmemcpy (wchar_t *__restrict __s1,
			 __const wchar_t *__restrict __s2, size_t __n) __THROW;

/* Copy N wide characters of SRC to DEST, guaranteeing
   correct behavior for overlapping strings.  */
extern wchar_t *wmemmove (wchar_t *__s1, __const wchar_t *__s2, size_t __n)
     __THROW;

/* Set N wide characters of S to C.  */
extern wchar_t *wmemset (wchar_t *__s, wchar_t __c, size_t __n) __THROW;

#ifdef __USE_GNU
/* Copy N wide characters of SRC to DEST and return pointer to following
   wide character.  */
extern wchar_t *wmempcpy (wchar_t *__restrict __s1,
			  __const wchar_t *__restrict __s2, size_t __n)
     __THROW;
#endif


/* Determine whether C constitutes a valid (one-byte) multibyte
   character.  */
extern wint_t btowc (int __c) __THROW;

/* Determine whether C corresponds to a member of the extended
   character set whose multibyte representation is a single byte.  */
extern int wctob (wint_t __c) __THROW;

/* Determine whether PS points to an object representing the initial
   state.  */
extern int mbsinit (__const mbstate_t *__ps) __THROW __attribute_pure__;

/* Write wide character representation of multibyte character pointed
   to by S to PWC.  */
extern size_t mbrtowc (wchar_t *__restrict __pwc,
		       __const char *__restrict __s, size_t __n,
		       mbstate_t *__p) __THROW;

/* Write multibyte representation of wide character WC to S.  */
extern size_t wcrtomb (char *__restrict __s, wchar_t __wc,
		       mbstate_t *__restrict __ps) __THROW;

/* Return number of bytes in multibyte character pointed to by S.  */
extern size_t __mbrlen (__const char *__restrict __s, size_t __n,
			mbstate_t *__restrict __ps) __THROW;
extern size_t mbrlen (__const char *__restrict __s, size_t __n,
		      mbstate_t *__restrict __ps) __THROW;

#ifdef __USE_EXTERN_INLINES
/* Define inline function as optimization.  */
extern __inline size_t mbrlen (__const char *__restrict __s, size_t __n,
			       mbstate_t *__restrict __ps) __THROW
{ return (__ps != NULL
	  ? mbrtowc (NULL, __s, __n, __ps) : __mbrlen (__s, __n, NULL)); }
#endif

/* Write wide character representation of multibyte character string
   SRC to DST.  */
extern size_t mbsrtowcs (wchar_t *__restrict __dst,
			 __const char **__restrict __src, size_t __len,
			 mbstate_t *__restrict __ps) __THROW;

/* Write multibyte character representation of wide character string
   SRC to DST.  */
extern size_t wcsrtombs (char *__restrict __dst,
			 __const wchar_t **__restrict __src, size_t __len,
			 mbstate_t *__restrict __ps) __THROW;


#ifdef	__USE_GNU
/* Write wide character representation of at most NMC bytes of the
   multibyte character string SRC to DST.  */
extern size_t mbsnrtowcs (wchar_t *__restrict __dst,
			  __const char **__restrict __src, size_t __nmc,
			  size_t __len, mbstate_t *__restrict __ps) __THROW;

/* Write multibyte character representation of at most NWC characters
   from the wide character string SRC to DST.  */
extern size_t wcsnrtombs (char *__restrict __dst,
			  __const wchar_t **__restrict __src,
			  size_t __nwc, size_t __len,
			  mbstate_t *__restrict __ps) __THROW;
#endif	/* use GNU */


/* The following functions are extensions found in X/Open CAE.  */
#ifdef __USE_XOPEN
/* Determine number of column positions required for C.  */
extern int wcwidth (wchar_t __c) __THROW;

/* Determine number of column positions required for first N wide
   characters (or fewer if S ends before this) in S.  */
extern int wcswidth (__const wchar_t *__s, size_t __n) __THROW;
#endif	/* Use X/Open.  */


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

#ifdef __USE_GNU
/* The concept of one static locale per category is not very well
   thought out.  Many applications will need to process its data using
   information from several different locales.  Another application is
   the implementation of the internationalization handling in the
   upcoming ISO C++ standard library.  To support this another set of
   the functions using locale data exist which have an additional
   argument.

   Attention: all these functions are *not* standardized in any form.
   This is a proof-of-concept implementation.  */

/* Structure for reentrant locale using functions.  This is an
   (almost) opaque type for the user level programs.  */
# include <xlocale.h>

/* Special versions of the functions above which take the locale to
   use as an additional parameter.  */
extern long int __wcstol_l (__const wchar_t *__restrict __nptr,
			    wchar_t **__restrict __endptr, int __base,
			    __locale_t __loc) __THROW;

extern unsigned long int __wcstoul_l (__const wchar_t *__restrict __nptr,
				      wchar_t **__restrict __endptr,
				      int __base, __locale_t __loc) __THROW;

__extension__
extern long long int __wcstoll_l (__const wchar_t *__restrict __nptr,
				  wchar_t **__restrict __endptr,
				  int __base, __locale_t __loc) __THROW;

__extension__
extern unsigned long long int __wcstoull_l (__const wchar_t *__restrict __nptr,
					    wchar_t **__restrict __endptr,
					    int __base, __locale_t __loc)
     __THROW;

extern double __wcstod_l (__const wchar_t *__restrict __nptr,
			  wchar_t **__restrict __endptr, __locale_t __loc)
     __THROW;

extern float __wcstof_l (__const wchar_t *__restrict __nptr,
			 wchar_t **__restrict __endptr, __locale_t __loc)
     __THROW;

extern long double __wcstold_l (__const wchar_t *__restrict __nptr,
				wchar_t **__restrict __endptr,
				__locale_t __loc) __THROW;
#endif /* GNU */


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


#ifdef	__USE_GNU
/* Copy SRC to DEST, returning the address of the terminating L'\0' in
   DEST.  */
extern wchar_t *wcpcpy (wchar_t *__dest, __const wchar_t *__src) __THROW;

/* Copy no more than N characters of SRC to DEST, returning the address of
   the last character written into DEST.  */
extern wchar_t *wcpncpy (wchar_t *__dest, __const wchar_t *__src, size_t __n)
     __THROW;
#endif	/* use GNU */


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


/* Format TP into S according to FORMAT.
   Write no more than MAXSIZE wide characters and return the number
   of wide characters written, or 0 if it would exceed MAXSIZE.  */
extern size_t wcsftime (wchar_t *__restrict __s, size_t __maxsize,
			__const wchar_t *__restrict __format,
			__const struct tm *__restrict __tp) __THROW;

/* The X/Open standard demands that most of the functions defined in
   the <wctype.h> header must also appear here.  This is probably
   because some X/Open members wrote their implementation before the
   ISO C standard was published and introduced the better solution.
   We have to provide these definitions for compliance reasons but we
   do this nonsense only if really necessary.  */
#if defined __USE_UNIX98 && !defined __USE_GNU
# define __need_iswxxx
# include <wctype.h>
#endif

__END_DECLS

#endif	/* _WCHAR_H defined */

#endif /* wchar.h  */
