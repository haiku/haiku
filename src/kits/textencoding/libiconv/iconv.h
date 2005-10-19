/* Copyright (C) 1999-2003, 2005 Free Software Foundation, Inc.
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
   If not, write to the Free Software Foundation, Inc., 51 Franklin Street,
   Fifth Floor, Boston, MA 02110-1301, USA.  */

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

#define _LIBICONV_VERSION 0x010A    /* version number: (major<<8) + minor */
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


#ifndef LIBICONV_PLUG

/* Nonstandard extensions. */

/* Control of attributes. */
#define iconvctl libiconvctl
extern int iconvctl (iconv_t cd, int request, void* argument);

/* Hook performed after every successful conversion of a Unicode character. */
typedef void (*iconv_unicode_char_hook) (unsigned int uc, void* data);
/* Hook performed after every successful conversion of a wide character. */
typedef void (*iconv_wide_char_hook) (wchar_t wc, void* data);
/* Set of hooks. */
struct iconv_hooks {
  iconv_unicode_char_hook uc_hook;
  iconv_wide_char_hook wc_hook;
  void* data;
};

/* Requests for iconvctl. */
#define ICONV_TRIVIALP            0  /* int *argument */
#define ICONV_GET_TRANSLITERATE   1  /* int *argument */
#define ICONV_SET_TRANSLITERATE   2  /* const int *argument */
#define ICONV_GET_DISCARD_ILSEQ   3  /* int *argument */
#define ICONV_SET_DISCARD_ILSEQ   4  /* const int *argument */
#define ICONV_SET_HOOKS           5  /* const struct iconv_hooks *argument */

/* Listing of locale independent encodings. */
#define iconvlist libiconvlist
extern void iconvlist (int (*do_one) (unsigned int namescount,
                                      const char * const * names,
                                      void* data),
                       void* data);

/* Canonicalize an encoding name.
   The result is either a canonical encoding name, or name itself. */
extern const char * iconv_canonicalize (const char * name);

/* Support for relocatable packages.  */

/* Sets the original and the current installation prefix of the package.
   Relocation simply replaces a pathname starting with the original prefix
   by the corresponding pathname with the current prefix instead.  Both
   prefixes should be directory names without trailing slash (i.e. use ""
   instead of "/").  */
extern void libiconv_set_relocation_prefix (const char *orig_prefix,
					    const char *curr_prefix);

#endif


#ifdef __cplusplus
}
#endif


#endif /* _LIBICONV_H */
