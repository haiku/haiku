/* shmbutil.h -- utility functions for multibyte characters. */

/* Copyright (C) 2002 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne Again SHell.

   Bash is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 2, or (at your option) any later
   version.

   Bash is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.

   You should have received a copy of the GNU General Public License along
   with Bash; see the file COPYING.  If not, write to the Free Software
   Foundation, 59 Temple Place, Suite 330, Boston, MA 02111 USA. */
                                 
#if !defined (_SH_MBUTIL_H_)
#define _SH_MBUTIL_H_

#include "stdc.h"

/************************************************/
/* check multibyte capability for I18N code     */
/************************************************/

/* For platforms which support the ISO C amendement 1 functionality we
   support user defined character classes.  */
   /* Solaris 2.5 has a bug: <wchar.h> must be included before <wctype.h>.  */
#if defined (HAVE_WCTYPE_H) && defined (HAVE_WCHAR_H)
#  include <wchar.h>
#  include <wctype.h>
#  if defined (HAVE_MBSRTOWCS) /* system is supposed to support XPG5 */
#    define HANDLE_MULTIBYTE      1
#  endif
#endif /* HAVE_WCTYPE_H && HAVE_WCHAR_H */

/* Some systems, like BeOS, have multibyte encodings but lack mbstate_t.  */
#if HANDLE_MULTIBYTE && !defined (HAVE_MBSTATE_T)
#  define wcsrtombs(dest, src, len, ps) (wcsrtombs) (dest, src, len, 0)
#  define mbsrtowcs(dest, src, len, ps) (mbsrtowcs) (dest, src, len, 0)
#  define wcrtomb(s, wc, ps) (wcrtomb) (s, wc, 0)
#  define mbrtowc(pwc, s, n, ps) (mbrtowc) (pwc, s, n, 0)
#  define mbrlen(s, n, ps) (mbrlen) (s, n, 0)
#  define mbstate_t int
#endif /* HANDLE_MULTIBYTE && !HAVE_MBSTATE_T */

/* Make sure MB_LEN_MAX is at least 16 on systems that claim to be able to
   handle multibyte chars (some systems define MB_LEN_MAX as 1) */
#ifdef HANDLE_MULTIBYTE
#  include <limits.h>
#  if defined(MB_LEN_MAX) && (MB_LEN_MAX < 16)
#    undef MB_LEN_MAX
#  endif
#  if !defined (MB_LEN_MAX)
#    define MB_LEN_MAX 16
#  endif
#endif /* HANDLE_MULTIBYTE */

/************************************************/
/* end of multibyte capability checks for I18N  */
/************************************************/

#if defined (HANDLE_MULTIBYTE)

extern size_t xmbsrtowcs __P((wchar_t *, const char **, size_t, mbstate_t *));

extern char *xstrchr __P((const char *, int));

#else /* !HANDLE_MULTIBYTE */

#undef MB_LEN_MAX
#undef MB_CUR_MAX

#define MB_LEN_MAX	1
#define MB_CUR_MAX	1

#undef xstrchr
#define xstrchr(s, c)	strchr(s, c)

#endif /* !HANDLE_MULTIBYTE */

/* Declare and initialize a multibyte state.  Call must be terminated
   with `;'. */
#if defined (HANDLE_MULTIBYTE)
#  define DECLARE_MBSTATE \
	mbstate_t state; \
	memset (&state, '\0', sizeof (mbstate_t))
#else
#  define DECLARE_MBSTATE
#endif  /* !HANDLE_MULTIBYTE */

/* Initialize or reinitialize a multibyte state named `state'.  Call must be
   terminated with `;'. */
#if defined (HANDLE_MULTIBYTE)
#  define INITIALIZE_MBSTATE memset (&state, '\0', sizeof (mbstate_t))
#else
#  define INITIALIZE_MBSTATE
#endif  /* !HANDLE_MULTIBYTE */

/* Advance one (possibly multi-byte) character in string _STR of length
   _STRSIZE, starting at index _I.  STATE must have already been declared. */
#if defined (HANDLE_MULTIBYTE)
#  define ADVANCE_CHAR(_str, _strsize, _i) \
    do \
      { \
	if (MB_CUR_MAX > 1) \
	  { \
	    mbstate_t state_bak; \
	    size_t mblength; \
\
	    state_bak = state; \
	    mblength = mbrlen ((_str) + (_i), (_strsize) - (_i), &state); \
\
	    if (mblength == (size_t)-2 || mblength == (size_t)-1) \
	      { \
		state = state_bak; \
		(_i)++; \
	      } \
	    else \
	      (_i) += mblength; \
	  } \
	else \
	  (_i)++; \
      } \
    while (0)
#else
#  define ADVANCE_CHAR(_str, _strsize, _i)	(_i)++
#endif  /* !HANDLE_MULTIBYTE */

/* Advance one (possibly multibyte) character in the string _STR of length
   _STRSIZE.
   SPECIAL:  assume that _STR will be incremented by 1 after this call. */
#if defined (HANDLE_MULTIBYTE)
#  define ADVANCE_CHAR_P(_str, _strsize) \
    do \
      { \
	if (MB_CUR_MAX > 1) \
	  { \
	    mbstate_t state_bak; \
	    size_t mblength; \
\
	    state_bak = state; \
	    mblength = mbrlen ((_str), (_strsize), &state); \
\
	    if (mblength == (size_t)-2 || mblength == (size_t)-1) \
	      { \
		state = state_bak; \
		mblength = 1; \
	      } \
	    else \
	      (_str) += (mblength < 1) ? 0 : (mblength - 1); \
	  } \
      } \
    while (0)
#else
#  define ADVANCE_CHAR_P(_str, _strsize)
#endif  /* !HANDLE_MULTIBYTE */

/* Copy a single character from the string _SRC to the string _DST.
   _SRCEND is a pointer to the end of _SRC. */
#if defined (HANDLE_MULTIBYTE)
#  define COPY_CHAR_P(_dst, _src, _srcend) \
    do \
      { \
	if (MB_CUR_MAX > 1) \
	  { \
	    mbstate_t state_bak; \
	    size_t mblength; \
	    int _k; \
\
	    state_bak = state; \
	    mblength = mbrlen ((_src), (_srcend) - (_src), &state); \
	    if (mblength == (size_t)-2 || mblength == (size_t)-1) \
	      { \
		state = state_bak; \
		mblength = 1; \
	      } \
	    else \
	      mblength = (mblength < 1) ? 1 : mblength; \
\
	    for (_k = 0; _k < mblength; _k++) \
	      *(_dst)++ = *(_src)++; \
	  } \
	else \
	  *(_dst)++ = *(_src)++; \
      } \
    while (0)
#else
#  define COPY_CHAR_P(_dst, _src, _srcend)	*(_dst)++ = *(_src)++
#endif  /* !HANDLE_MULTIBYTE */

/* Copy a single character from the string _SRC at index _SI to the string
   _DST at index _DI.  _SRCEND is a pointer to the end of _SRC. */
#if defined (HANDLE_MULTIBYTE)
#  define COPY_CHAR_I(_dst, _di, _src, _srcend, _si) \
    do \
      { \
	if (MB_CUR_MAX > 1) \
	  { \
	    mbstate_t state_bak; \
	    size_t mblength; \
	    int _k; \
\
	    state_bak = state; \
	    mblength = mbrlen ((_src) + (_si), (_srcend) - ((_src)+(_si)), &state); \
	    if (mblength == (size_t)-2 || mblength == (size_t)-1) \
	      { \
		state = state_bak; \
		mblength = 1; \
	      } \
	    else \
	      mblength = (mblength < 1) ? 1 : mblength; \
\
	    for (_k = 0; _k < mblength; _k++) \
	      _dst[_di++] = _src[_si++]; \
	  } \
	else \
	  _dst[_di++] = _src[_si++]; \
      } \
    while (0)
#else
#  define COPY_CHAR_I(_dst, _di, _src, _srcend, _si)	_dst[_di++] = _src[_si++]
#endif  /* !HANDLE_MULTIBYTE */

/****************************************************************
 *								*
 * The following are only guaranteed to work in subst.c		*
 *								*
 ****************************************************************/

#if defined (HANDLE_MULTIBYTE)
#  define SCOPY_CHAR_I(_dst, _escchar, _sc, _src, _si, _slen) \
    do \
      { \
	if (MB_CUR_MAX > 1) \
	  { \
	    mbstate_t state_bak; \
	    size_t mblength; \
	    int _i; \
\
	    state_bak = state; \
	    mblength = mbrlen ((_src) + (_si), (_slen) - (_si), &state); \
	    if (mblength == (size_t)-2 || mblength == (size_t)-1) \
	      { \
		state = state_bak; \
		mblength = 1; \
	      } \
	    else \
	      mblength = (mblength < 1) ? 1 : mblength; \
\
	    temp = xmalloc (mblength + 2); \
	    temp[0] = _escchar; \
	    for (_i = 0; _i < mblength; _i++) \
	      temp[_i + 1] = _src[_si++]; \
	    temp[mblength + 1] = '\0'; \
\
	    goto add_string; \
	  } \
	else \
	  { \
	    _dst[0] = _escchar; \
	    _dst[1] = _sc; \
	  } \
      } \
    while (0)
#else
#  define SCOPY_CHAR_I(_dst, _escchar, _sc, _src, _si, _slen) \
    _dst[0] = _escchar; \
    _dst[1] = _sc
#endif  /* !HANDLE_MULTIBYTE */

#if defined (HANDLE_MULTIBYTE)
#  define SCOPY_CHAR_M(_dst, _src, _srcend, _si) \
    do \
      { \
	if (MB_CUR_MAX > 1) \
	  { \
	    mbstate_t state_bak; \
	    size_t mblength; \
\
	    state_bak = state; \
	    mblength = mbrlen ((_src) + (_si), (_srcend) - ((_src) + (_si)), &state); \
	    if (mblength == (size_t)-2 || mblength == (size_t)-1) \
	      { \
		state = state_bak; \
		mblength = 1; \
	      } \
	    else \
	      mblength = (mblength < 1) ? 1 : mblength; \
\
	    FASTCOPY(((_src) + (_si)), (_dst), mblength); \
\
	    (_dst) += mblength; \
	    (_si) += mblength; \
	  } \
	else \
	  { \
	    *(_dst)++ = _src[(_si)]; \
	    (_si)++; \
	  } \
      } \
    while (0)
#else
#  define SCOPY_CHAR_M(_dst, _src, _srcend, _si) \
	*(_dst)++ = _src[(_si)]; \
	(_si)++
#endif  /* !HANDLE_MULTIBYTE */

#if HANDLE_MULTIBYTE
#  define SADD_MBCHAR(_dst, _src, _si, _srcsize) \
    do \
      { \
	if (MB_CUR_MAX > 1) \
	  { \
	    int i; \
	    mbstate_t state_bak; \
	    size_t mblength; \
\
	    state_bak = state; \
	    mblength = mbrlen ((_src) + (_si), (_srcsize) - (_si), &state); \
	    if (mblength == (size_t)-1 || mblength == (size_t)-2) \
	      { \
		state = state_bak; \
		mblength = 1; \
	      } \
	    if (mblength < 1) \
	      mblength = 1; \
\
	    _dst = (char *)xmalloc (mblength + 1); \
	    for (i = 0; i < mblength; i++) \
	      (_dst)[i] = (_src)[(_si)++]; \
	    (_dst)[mblength] = '\0'; \
\
	    goto add_string; \
	  } \
      } \
    while (0)

#else
#  define SADD_MBCHAR(_dst, _src, _si, _srcsize)
#endif

#endif /* _SH_MBUTIL_H_ */
