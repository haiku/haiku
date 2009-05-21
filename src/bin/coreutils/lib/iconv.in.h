/* -*- buffer-read-only: t -*- vi: set ro: */
/* DO NOT EDIT! GENERATED AUTOMATICALLY! */
#line 1
/* A GNU-like <iconv.h>.

   Copyright (C) 2007-2008 Free Software Foundation, Inc.

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

#ifndef _GL_ICONV_H

#if __GNUC__ >= 3
@PRAGMA_SYSTEM_HEADER@
#endif

/* The include_next requires a split double-inclusion guard.  */
#@INCLUDE_NEXT@ @NEXT_ICONV_H@

#ifndef _GL_ICONV_H
#define _GL_ICONV_H

#ifdef __cplusplus
extern "C" {
#endif


#if @REPLACE_ICONV_OPEN@
/* An iconv_open wrapper that supports the IANA standardized encoding names
   ("ISO-8859-1" etc.) as far as possible.  */
# define iconv_open rpl_iconv_open
extern iconv_t iconv_open (const char *tocode, const char *fromcode);
#endif

#if @REPLACE_ICONV_UTF@
/* Special constants for supporting UTF-{16,32}{BE,LE} encodings.
   Not public.  */
# define _ICONV_UTF8_UTF16BE (iconv_t)(-161)
# define _ICONV_UTF8_UTF16LE (iconv_t)(-162)
# define _ICONV_UTF8_UTF32BE (iconv_t)(-163)
# define _ICONV_UTF8_UTF32LE (iconv_t)(-164)
# define _ICONV_UTF16BE_UTF8 (iconv_t)(-165)
# define _ICONV_UTF16LE_UTF8 (iconv_t)(-166)
# define _ICONV_UTF32BE_UTF8 (iconv_t)(-167)
# define _ICONV_UTF32LE_UTF8 (iconv_t)(-168)
#endif

#if @REPLACE_ICONV@
# define iconv rpl_iconv
extern size_t iconv (iconv_t cd,
		     @ICONV_CONST@ char **inbuf, size_t *inbytesleft,
		     char **outbuf, size_t *outbytesleft);
# define iconv_close rpl_iconv_close
extern int iconv_close (iconv_t cd);
#endif


#ifdef __cplusplus
}
#endif

#endif /* _GL_ICONV_H */
#endif /* _GL_ICONV_H */
