/* DO NOT EDIT! GENERATED AUTOMATICALLY! */
/* -*- buffer-read-only: t -*- vi: set ro: */
/* DO NOT EDIT! GENERATED AUTOMATICALLY! */
#line 1
/* A substitute for ISO C99 <wchar.h>, for platforms that have issues.

   Copyright (C) 2007-2010 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* Written by Eric Blake.  */

/*
 * ISO C 99 <wchar.h> for platforms that have issues.
 * <http://www.opengroup.org/susv3xbd/wchar.h.html>
 *
 * For now, this just ensures proper prerequisite inclusion order and
 * the declaration of wcwidth().
 */

#if __GNUC__ >= 3
#pragma GCC system_header
#endif

#if defined __need_mbstate_t || defined __need_wint_t || (defined __hpux && ((defined _INTTYPES_INCLUDED && !defined strtoimax) || defined _GL_JUST_INCLUDE_SYSTEM_WCHAR_H)) || defined _GL_ALREADY_INCLUDING_WCHAR_H
/* Special invocation convention:
   - Inside glibc and uClibc header files.
   - On HP-UX 11.00 we have a sequence of nested includes
     <wchar.h> -> <stdlib.h> -> <stdint.h>, and the latter includes <wchar.h>,
     once indirectly <stdint.h> -> <sys/types.h> -> <inttypes.h> -> <wchar.h>
     and once directly.  In both situations 'wint_t' is not yet defined,
     therefore we cannot provide the function overrides; instead include only
     the system's <wchar.h>.
   - On IRIX 6.5, similarly, we have an include <wchar.h> -> <wctype.h>, and
     the latter includes <wchar.h>.  But here, we have no way to detect whether
     <wctype.h> is completely included or is still being included.  */

#include_next <wchar.h>

#else
/* Normal invocation convention.  */

#ifndef _GL_WCHAR_H

#define _GL_ALREADY_INCLUDING_WCHAR_H

/* Tru64 with Desktop Toolkit C has a bug: <stdio.h> must be included before
   <wchar.h>.
   BSD/OS 4.0.1 has a bug: <stddef.h>, <stdio.h> and <time.h> must be
   included before <wchar.h>.
   But avoid namespace pollution on glibc systems.  */
#ifndef __GLIBC__
# include <stddef.h>
# include <stdio.h>
# include <time.h>
#endif

/* Include the original <wchar.h> if it exists.
   Some builds of uClibc lack it.  */
/* The include_next requires a split double-inclusion guard.  */
#if 1
# include_next <wchar.h>
#endif

#undef _GL_ALREADY_INCLUDING_WCHAR_H

#ifndef _GL_WCHAR_H
#define _GL_WCHAR_H

/* The definition of GL_LINK_WARNING is copied here.  */
/* GL_LINK_WARNING("literal string") arranges to emit the literal string as
   a linker warning on most glibc systems.
   We use a linker warning rather than a preprocessor warning, because
   #warning cannot be used inside macros.  */
#ifndef GL_LINK_WARNING
  /* This works on platforms with GNU ld and ELF object format.
     Testing __GLIBC__ is sufficient for asserting that GNU ld is in use.
     Testing __ELF__ guarantees the ELF object format.
     Testing __GNUC__ is necessary for the compound expression syntax.  */
# if defined __GLIBC__ && defined __ELF__ && defined __GNUC__
#  define GL_LINK_WARNING(message) \
     GL_LINK_WARNING1 (__FILE__, __LINE__, message)
#  define GL_LINK_WARNING1(file, line, message) \
     GL_LINK_WARNING2 (file, line, message)  /* macroexpand file and line */
#  define GL_LINK_WARNING2(file, line, message) \
     GL_LINK_WARNING3 (file ":" #line ": warning: " message)
#  define GL_LINK_WARNING3(message) \
     ({ static const char warning[sizeof (message)]             \
          __attribute__ ((__unused__,                           \
                          __section__ (".gnu.warning"),         \
                          __aligned__ (1)))                     \
          = message "\n";                                       \
        (void)0;                                                \
     })
# else
#  define GL_LINK_WARNING(message) ((void) 0)
# endif
#endif

/* The definition of _GL_ARG_NONNULL is copied here.  */
/* _GL_ARG_NONNULL((n,...,m)) tells the compiler and static analyzer tools
   that the values passed as arguments n, ..., m must be non-NULL pointers.
   n = 1 stands for the first argument, n = 2 for the second argument etc.  */
#ifndef _GL_ARG_NONNULL
# if (__GNUC__ == 3 && __GNUC_MINOR__ >= 3) || __GNUC__ > 3
#  define _GL_ARG_NONNULL(params) __attribute__ ((__nonnull__ params))
# else
#  define _GL_ARG_NONNULL(params)
# endif
#endif

#ifdef __cplusplus
extern "C" {
#endif


/* Define wint_t.  (Also done in wctype.in.h.)  */
#if !1 && !defined wint_t
# define wint_t int
# ifndef WEOF
#  define WEOF -1
# endif
#endif


/* Override mbstate_t if it is too small.
   On IRIX 6.5, sizeof (mbstate_t) == 1, which is not sufficient for
   implementing mbrtowc for encodings like UTF-8.  */
#if !(1 && 1) || 0
typedef int rpl_mbstate_t;
# undef mbstate_t
# define mbstate_t rpl_mbstate_t
# define GNULIB_defined_mbstate_t 1
#endif


/* Convert a single-byte character to a wide character.  */
#if 1
# if 0
#  undef btowc
#  define btowc rpl_btowc
# endif
# if !1 || 0
extern wint_t btowc (int c);
# endif
#elif defined GNULIB_POSIXCHECK
# undef btowc
# define btowc(c) \
    (GL_LINK_WARNING ("btowc is unportable - " \
                      "use gnulib module btowc for portability"), \
     btowc (c))
#endif


/* Convert a wide character to a single-byte character.  */
#if 1
# if 0
#  undef wctob
#  define wctob rpl_wctob
# endif
# if (!defined wctob && !1) || 0
/* wctob is provided by gnulib, or wctob exists but is not declared.  */
extern int wctob (wint_t wc);
# endif
#elif defined GNULIB_POSIXCHECK
# undef wctob
# define wctob(w) \
    (GL_LINK_WARNING ("wctob is unportable - " \
                      "use gnulib module wctob for portability"), \
     wctob (w))
#endif


/* Test whether *PS is in the initial state.  */
#if 1
# if 0
#  undef mbsinit
#  define mbsinit rpl_mbsinit
# endif
# if !1 || 0
extern int mbsinit (const mbstate_t *ps);
# endif
#elif defined GNULIB_POSIXCHECK
# undef mbsinit
# define mbsinit(p) \
    (GL_LINK_WARNING ("mbsinit is unportable - " \
                      "use gnulib module mbsinit for portability"), \
     mbsinit (p))
#endif


/* Convert a multibyte character to a wide character.  */
#if 1
# if 0
#  undef mbrtowc
#  define mbrtowc rpl_mbrtowc
# endif
# if !1 || 0
extern size_t mbrtowc (wchar_t *pwc, const char *s, size_t n, mbstate_t *ps);
# endif
#elif defined GNULIB_POSIXCHECK
# undef mbrtowc
# define mbrtowc(w,s,n,p) \
    (GL_LINK_WARNING ("mbrtowc is unportable - " \
                      "use gnulib module mbrtowc for portability"), \
     mbrtowc (w, s, n, p))
#endif


/* Recognize a multibyte character.  */
#if 1
# if 0
#  undef mbrlen
#  define mbrlen rpl_mbrlen
# endif
# if !1 || 0
extern size_t mbrlen (const char *s, size_t n, mbstate_t *ps);
# endif
#elif defined GNULIB_POSIXCHECK
# undef mbrlen
# define mbrlen(s,n,p) \
    (GL_LINK_WARNING ("mbrlen is unportable - " \
                      "use gnulib module mbrlen for portability"), \
     mbrlen (s, n, p))
#endif


/* Convert a string to a wide string.  */
#if 1
# if 0
#  undef mbsrtowcs
#  define mbsrtowcs rpl_mbsrtowcs
# endif
# if !1 || 0
extern size_t mbsrtowcs (wchar_t *dest, const char **srcp, size_t len, mbstate_t *ps)
     _GL_ARG_NONNULL ((2));
# endif
#elif defined GNULIB_POSIXCHECK
# undef mbsrtowcs
# define mbsrtowcs(d,s,l,p) \
    (GL_LINK_WARNING ("mbsrtowcs is unportable - " \
                      "use gnulib module mbsrtowcs for portability"), \
     mbsrtowcs (d, s, l, p))
#endif


/* Convert a string to a wide string.  */
#if 0
# if 0
#  undef mbsnrtowcs
#  define mbsnrtowcs rpl_mbsnrtowcs
# endif
# if !1 || 0
extern size_t mbsnrtowcs (wchar_t *dest, const char **srcp, size_t srclen, size_t len, mbstate_t *ps)
     _GL_ARG_NONNULL ((2));
# endif
#elif defined GNULIB_POSIXCHECK
# undef mbsnrtowcs
# define mbsnrtowcs(d,s,n,l,p) \
    (GL_LINK_WARNING ("mbsnrtowcs is unportable - " \
                      "use gnulib module mbsnrtowcs for portability"), \
     mbsnrtowcs (d, s, n, l, p))
#endif


/* Convert a wide character to a multibyte character.  */
#if 1
# if 0
#  undef wcrtomb
#  define wcrtomb rpl_wcrtomb
# endif
# if !1 || 0
extern size_t wcrtomb (char *s, wchar_t wc, mbstate_t *ps);
# endif
#elif defined GNULIB_POSIXCHECK
# undef wcrtomb
# define wcrtomb(s,w,p) \
    (GL_LINK_WARNING ("wcrtomb is unportable - " \
                      "use gnulib module wcrtomb for portability"), \
     wcrtomb (s, w, p))
#endif


/* Convert a wide string to a string.  */
#if 0
# if 0
#  undef wcsrtombs
#  define wcsrtombs rpl_wcsrtombs
# endif
# if !1 || 0
extern size_t wcsrtombs (char *dest, const wchar_t **srcp, size_t len, mbstate_t *ps)
     _GL_ARG_NONNULL ((2));
# endif
#elif defined GNULIB_POSIXCHECK
# undef wcsrtombs
# define wcsrtombs(d,s,l,p) \
    (GL_LINK_WARNING ("wcsrtombs is unportable - " \
                      "use gnulib module wcsrtombs for portability"), \
     wcsrtombs (d, s, l, p))
#endif


/* Convert a wide string to a string.  */
#if 0
# if 0
#  undef wcsnrtombs
#  define wcsnrtombs rpl_wcsnrtombs
# endif
# if !1 || 0
extern size_t wcsnrtombs (char *dest, const wchar_t **srcp, size_t srclen, size_t len, mbstate_t *ps)
     _GL_ARG_NONNULL ((2));
# endif
#elif defined GNULIB_POSIXCHECK
# undef wcsnrtombs
# define wcsnrtombs(d,s,n,l,p) \
    (GL_LINK_WARNING ("wcsnrtombs is unportable - " \
                      "use gnulib module wcsnrtombs for portability"), \
     wcsnrtombs (d, s, n, l, p))
#endif


/* Return the number of screen columns needed for WC.  */
#if 1
# if 0
#  undef wcwidth
#  define wcwidth rpl_wcwidth
extern int wcwidth (wchar_t);
# else
#  if !defined wcwidth && !1
/* wcwidth exists but is not declared.  */
extern int wcwidth (int /* actually wchar_t */);
#  endif
# endif
#elif defined GNULIB_POSIXCHECK
# undef wcwidth
# define wcwidth(w) \
    (GL_LINK_WARNING ("wcwidth is unportable - " \
                      "use gnulib module wcwidth for portability"), \
     wcwidth (w))
#endif


#ifdef __cplusplus
}
#endif

#endif /* _GL_WCHAR_H */
#endif /* _GL_WCHAR_H */
#endif
