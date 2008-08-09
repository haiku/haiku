/* General MD5 header file.
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

int gen_md5_context_size (void);
void gen_md5_init (gen_md5_context *);
void gen_md5_update (const unsigned char *, int, gen_md5_context *);
void gen_md5_finish (gen_md5_context *, unsigned char *);

#endif /* GEN_MD5_H */
