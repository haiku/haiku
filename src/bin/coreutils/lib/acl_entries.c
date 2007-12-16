/* Return the number of entries in an ACL.

   Copyright (C) 2002, 2003, 2005, 2006, 2007 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

   Written by Paul Eggert and Andreas Gruenbacher.  */

#include <config.h>

#include "acl-internal.h"

/* Return the number of entries in ACL.  */

int
acl_entries (acl_t acl)
{
  char *t;
  int entries = 0;
  char *text = acl_to_text (acl, NULL);
  if (! text)
    return -1;
  for (t = text; *t; t++)
    entries += (*t == '\n');
  acl_free (text);
  return entries;
}
