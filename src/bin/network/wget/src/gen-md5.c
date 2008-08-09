/* General MD5 support.
   Copyright (C) 2001, 2002, 2003, 2004, 2005, 2006, 2007,
   2008 Free Software Foundation, Inc.

This file is part of GNU Wget.

GNU Wget is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

GNU Wget is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wget.  If not, see <http://www.gnu.org/licenses/>.

Additional permission under GNU GPL version 3 section 7

If you modify this program, or any covered work, by linking or
combining it with the OpenSSL project's OpenSSL library (or a
modified version of that library), containing parts covered by the
terms of the OpenSSL or SSLeay licenses, the Free Software Foundation
grants you additional permission to convey the resulting work.
Corresponding Source for a non-source form of such a combination
shall include the source code for the parts of OpenSSL used as well
as that of the covered work.  */

#include <config.h>
#include "wget.h"

#include "gen-md5.h"

#ifdef HAVE_BUILTIN_MD5
# include <gnu-md5.h>
typedef struct md5_ctx gen_md5_context_imp;
#endif

#ifdef HAVE_SOLARIS_MD5
# include <md5.h>
typedef MD5_CTX gen_md5_context_imp;
#endif

#ifdef HAVE_OPENSSL_MD5
# include <openssl/md5.h>
typedef MD5_CTX gen_md5_context_imp;
#endif

struct gen_md5_context {
  gen_md5_context_imp imp;
};

/* Originally I planned for these to be macros, but that's very hard
   because some of these MD5 implementations use the same names for
   their types.  For example, it is impossible to include <md5.h> and
   <openssl/ssl.h> on Solaris, because the latter includes its own MD5
   implementation, which clashes with <md5.h>.  */

int
gen_md5_context_size (void)
{
  return sizeof (struct gen_md5_context);
}

void
gen_md5_init (gen_md5_context *ctx)
{
  gen_md5_context_imp *ctx_imp = &ctx->imp;

#ifdef HAVE_BUILTIN_MD5
  md5_init_ctx (ctx_imp);
#endif

#ifdef HAVE_SOLARIS_MD5
  MD5Init (ctx_imp);
#endif

#ifdef HAVE_OPENSSL_MD5
  MD5_Init (ctx_imp);
#endif
}

void
gen_md5_update (unsigned const char *buffer, int len, gen_md5_context *ctx)
{
  gen_md5_context_imp *ctx_imp = &ctx->imp;

#ifdef HAVE_BUILTIN_MD5
  md5_process_bytes (buffer, len, ctx_imp);
#endif

#ifdef HAVE_SOLARIS_MD5
  MD5Update (ctx_imp, (unsigned char *)buffer, len);
#endif

#ifdef HAVE_OPENSSL_MD5
  MD5_Update (ctx_imp, buffer, len);
#endif
}

void
gen_md5_finish (gen_md5_context *ctx, unsigned char *result)
{
  gen_md5_context_imp *ctx_imp = &ctx->imp;

#ifdef HAVE_BUILTIN_MD5
  md5_finish_ctx (ctx_imp, result);
#endif

#ifdef HAVE_SOLARIS_MD5
  MD5Final (result, ctx_imp);
#endif

#ifdef HAVE_OPENSSL_MD5
  MD5_Final (result, ctx_imp);
#endif
}

#if 0
/* A debugging function for checking whether an MD5 library works. */

#include "gen-md5.h"

char *
debug_test_md5 (char *buf)
{
  unsigned char raw[16];
  static char res[33];
  unsigned char *p1;
  char *p2;
  int cnt;
  ALLOCA_MD5_CONTEXT (ctx);

  gen_md5_init (ctx);
  gen_md5_update ((unsigned char *)buf, strlen (buf), ctx);
  gen_md5_finish (ctx, raw);

  p1 = raw;
  p2 = res;
  cnt = 16;
  while (cnt--)
    {
      *p2++ = XNUM_TO_digit (*p1 >> 4);
      *p2++ = XNUM_TO_digit (*p1 & 0xf);
      ++p1;
    }
  *p2 = '\0';

  return res;
}
#endif
