/* Charset handling for GNU tar.

   Copyright (C) 2004, 2006, 2007 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 3, or (at your option) any later
   version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
   Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#include <system.h>
#include <quotearg.h>
#include <localcharset.h>
#include "common.h"
#ifdef HAVE_ICONV_H
# include <iconv.h>
#endif

#ifndef ICONV_CONST
# define ICONV_CONST
#endif

#ifndef HAVE_ICONV

# undef iconv_open
# define iconv_open(tocode, fromcode) ((iconv_t) -1)

# undef iconv
# define iconv(cd, inbuf, inbytesleft, outbuf, outbytesleft) ((size_t) 0)

# undef iconv_close
# define iconv_close(cd) 0

#endif




static iconv_t conv_desc[2] = { (iconv_t) -1, (iconv_t) -1 };

static iconv_t
utf8_init (bool to_utf)
{
  if (conv_desc[(int) to_utf] == (iconv_t) -1)
    {
      if (to_utf)
	conv_desc[(int) to_utf] = iconv_open ("UTF-8", locale_charset ());
      else
	conv_desc[(int) to_utf] = iconv_open (locale_charset (), "UTF-8");
    }
  return conv_desc[(int) to_utf];
}

bool
utf8_convert (bool to_utf, char const *input, char **output)
{
  char ICONV_CONST *ib;
  char *ob;
  size_t inlen;
  size_t outlen;
  size_t rc;
  iconv_t cd = utf8_init (to_utf);

  if (cd == 0)
    {
      *output = xstrdup (input);
      return true;
    }
  else if (cd == (iconv_t)-1)
    return false;

  inlen = strlen (input) + 1;
  outlen = inlen * MB_LEN_MAX + 1;
  ob = *output = xmalloc (outlen);
  ib = (char ICONV_CONST *) input;
  rc = iconv (cd, &ib, &inlen, &ob, &outlen);
  *ob = 0;
  return rc != -1;
}


bool
string_ascii_p (char const *p)
{
  for (; *p; p++)
    if (! (0 <= *p && *p <= 127))
      return false;
  return true;
}
