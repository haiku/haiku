/* version.c -- distribution and version numbers. */

/* Copyright (C) 1989 Free Software Foundation, Inc.

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

#include <config.h>

#include <stdio.h>

#include "stdc.h"

#include "version.h"
#include "patchlevel.h"
#include "conftypes.h"

extern char *shell_name;

/* Defines from version.h */
const char *dist_version = DISTVERSION;
int patch_level = PATCHLEVEL;
int build_version = BUILDVERSION;
#ifdef RELSTATUS
const char *release_status = RELSTATUS;
#else
const char *release_status = (char *)0;
#endif
const char *sccs_version = SCCSVERSION;

/* Functions for getting, setting, and displaying the shell version. */

/* Give version information about this shell. */
char *
shell_version_string ()
{
  static char tt[32] = { '\0' };

  if (tt[0] == '\0')
    {
      if (release_status)
	sprintf (tt, "%s.%d(%d)-%s", dist_version, patch_level, build_version, release_status);
      else
	sprintf (tt, "%s.%d(%d)", dist_version, patch_level, build_version);
    }
  return tt;
}

void
show_shell_version (extended)
     int extended;
{
  printf ("GNU bash, version %s (%s)\n", shell_version_string (), MACHTYPE);
  if (extended)
    printf ("Copyright (C) 2002 Free Software Foundation, Inc.\n");
}
