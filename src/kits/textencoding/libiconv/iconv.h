/* Copyright (C) 1999-2003 Free Software Foundation, Inc.
   This file is part of the GNU LIBICONV Library.

   The GNU LIBICONV Library is free software; you can redistribute it
   and/or modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   The GNU LIBICONV Library is distributed in the hope that it will be
   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU LIBICONV Library; see the file COPYING.LIB.
   If not, write to the Free Software Foundation, Inc., 59 Temple Place -
   Suite 330, Boston, MA 02111-1307, USA.  */

/* 
   This file has been selectively butchered to serve our purposes:
   1. it exports the functions without the "lib" prefix.
   2. it includes iconvctl and related constans.
   3. it does not use const in iconv.
   4. it includes our iconv.h as well, so we won't incompatibly change it
   - Andrew Bachmann
*/

#ifndef _LIBICONV_H
#define _LIBICONV_H

#define ICONV_CONST

#define _LIBICONV_VERSION 0x0109    /* version number: (major<<8) + minor */
extern int _libiconv_version;       /* Likewise */

#include_next <iconv.h>

/* Get size_t declaration. */
#include <stddef.h>

/* Get errno declaration and values. */
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Allocates descriptor for code conversion from encoding `fromcode' to
   encoding `tocode'. */
extern iconv_t iconv_open (const char* tocode, const char* fromcode);

/* Converts, using conversion descriptor `cd', at most `*inbytesleft' bytes
   starting at `*inbuf', writing at most `*outbytesleft' bytes starting at
   `*outbuf'.
   Decrements `*inbytesleft' and increments `*inbuf' by the same amount.
   Decrements `*outbytesleft' and increments `*outbuf' by the same amount. */
extern size_t iconv (iconv_t cd, ICONV_CONST char* * inbuf, size_t *inbytesleft, char* * outbuf, size_t *outbytesleft);

/* Frees resources allocated for conversion descriptor `cd'. */
extern int iconv_close (iconv_t cd);


/* Nonstandard extensions. */

/* Control of attributes. */
extern int iconvctl (iconv_t cd, int request, void* argument);

/* Requests for iconvctl. */
#define ICONV_TRIVIALP            0  /* int *argument */
#define ICONV_GET_TRANSLITERATE   1  /* int *argument */
#define ICONV_SET_TRANSLITERATE   2  /* const int *argument */
#define ICONV_GET_DISCARD_ILSEQ   3  /* int *argument */
#define ICONV_SET_DISCARD_ILSEQ   4  /* const int *argument */

#ifdef __cplusplus
}
#endif



#endif /* _LIBICONV_H */
