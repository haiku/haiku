/* General MD5 header file.
   Copyright (C) 2001 Free Software Foundation, Inc.

This file is part of GNU Wget.

GNU Wget is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GNU Wget is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wget; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

In addition, as a special exception, the Free Software Foundation
gives permission to link the code of its release of Wget with the
OpenSSL project's "OpenSSL" library (or with modified versions of it
that use the same license as the "OpenSSL" library), and distribute
the linked executables.  You must obey the GNU General Public License
in all respects for all of the code used other than "OpenSSL".  If you
modify this file, you may extend this exception to your version of the
file, but you are not obligated to do so.  If you do not wish to do
so, delete this exception statement from your version.  */

#ifndef GEN_MD5_H
#define GEN_MD5_H

typedef struct gen_md5_context gen_md5_context;

/* Use a forward declaration so we don't have to include any of the
   includes.  */
struct gen_md5_context;

#define MD5_HASHLEN 16

#define ALLOCA_MD5_CONTEXT(var_name)			\
  gen_md5_context *var_name =				\
  (gen_md5_context *) alloca (gen_md5_context_size ())

int gen_md5_context_size PARAMS ((void));
void gen_md5_init PARAMS ((gen_md5_context *));
void gen_md5_update PARAMS ((const unsigned char *, int, gen_md5_context *));
void gen_md5_finish PARAMS ((gen_md5_context *, unsigned char *));

#endif /* GEN_MD5_H */
