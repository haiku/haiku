/* shell.c -- tilde utility functions that are normally provided by
	      bash when readline is linked as part of the shell. */

/* Copyright (C) 1998 Free Software Foundation, Inc.

   This file is part of the GNU Tilde Library.

   The GNU Tilde Library is free software; you can redistribute it
   and/or modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2, or
   (at your option) any later version.

   The GNU Tilde Library is distributed in the hope that it will be
   useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   The GNU General Public License is often shipped with GNU software, and
   is generally kept in a file called COPYING or LICENSE.  If you do not
   have a copy of the license, write to the Free Software Foundation,
   59 Temple Place, Suite 330, Boston, MA 02111 USA. */

#if defined (HAVE_CONFIG_H)
#  include <config.h>
#endif

#if defined (HAVE_UNISTD_H)
#  ifdef _MINIX
#    include <sys/types.h>
#  endif
#  include <unistd.h>
#endif /* HAVE_UNISTD_H */

#if defined (HAVE_STDLIB_H)
#  include <stdlib.h>
#else
#  include "ansi_stdlib.h"
#endif /* HAVE_STDLIB_H */

#if defined (HAVE_STRING_H)
#  include <string.h>
#else
#  include <strings.h>
#endif /* !HAVE_STRING_H */

#include <pwd.h>

#if !defined (HAVE_GETPW_DECLS)
extern struct passwd *getpwuid ();
#endif /* !HAVE_GETPW_DECLS */

char *
get_env_value (varname)
     char *varname;
{
  return ((char *)getenv (varname));
}

char *
get_home_dir ()
{
  char *home_dir;
  struct passwd *entry;

  home_dir = (char *)NULL;
  entry = getpwuid (getuid ());
  if (entry)
    home_dir = entry->pw_dir;
  return (home_dir);
}
