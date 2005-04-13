/* Copyright (C) 1996-2000, 2002, 2003 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1996.

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

#ifndef _LOADINFO_H
#define _LOADINFO_H	1

/* Declarations of locale dependent catalog lookup functions.
   Implemented in

     localealias.c    Possibly replace a locale name by another.
     explodename.c    Split a locale name into its various fields.
     l10nflist.c      Generate a list of filenames of possible message catalogs.
     finddomain.c     Find and open the relevant message catalogs.

   The main function _nl_find_domain() in finddomain.c is declared
   in gettextP.h.
 */

#ifndef PARAMS
# if __STDC__ || defined __GNUC__ || defined __SUNPRO_C || defined __cplusplus || __PROTOTYPES
#  define PARAMS(args) args
# else
#  define PARAMS(args) ()
# endif
#endif

//#ifndef internal_function
//# define internal_function
//#endif

/* Tell the compiler when a conditional or integer expression is
   almost always true or almost always false.  */
#ifndef HAVE_BUILTIN_EXPECT
# define __builtin_expect(expr, val) (expr)
#endif

/* Encoding of locale name parts.  */
#define XPG_NORM_CODESET	1
#define XPG_CODESET		2
#define XPG_TERRITORY		4
#define XPG_MODIFIER		8


struct loaded_l10nfile
{
  const char *filename;
  int decided;

  const void *data;

  struct loaded_l10nfile *next;
  struct loaded_l10nfile *successor[1];
};


/* Normalize codeset name.  There is no standard for the codeset
   names.  Normalization allows the user to use any of the common
   names.  The return value is dynamically allocated and has to be
   freed by the caller.  */
extern const char *_nl_normalize_codeset PARAMS ((const char *codeset,
						  size_t name_len));

extern struct loaded_l10nfile *
_nl_make_l10nflist PARAMS ((struct loaded_l10nfile **l10nfile_list,
			    const char *dirlist, size_t dirlist_len, int mask,
			    const char *language, const char *territory,
			    const char *codeset,
			    const char *normalized_codeset,
			    const char *modifier, const char *filename,
			    int do_allocate));


extern const char *_nl_expand_alias PARAMS ((const char *name));

/* normalized_codeset is dynamically allocated and has to be freed by
   the caller.  */
extern int _nl_explode_name PARAMS ((char *name, const char **language,
				     const char **modifier,
				     const char **territory,
				     const char **codeset,
				     const char **normalized_codeset));

#endif	/* loadinfo.h */
