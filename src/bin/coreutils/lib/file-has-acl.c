/* Test whether a file has a nontrivial access control list.

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

#include "acl.h"

#include "acl-internal.h"

/* Return 1 if NAME has a nontrivial access control list, 0 if NAME
   only has no or a base access control list, and -1 (setting errno)
   on error.  SB must be set to the stat buffer of FILE.  */

int
file_has_acl (char const *name, struct stat const *sb)
{
  if (! S_ISLNK (sb->st_mode))
    {
#if USE_ACL && HAVE_ACL_TRIVIAL

      /* Solaris 10, which also has NFSv4 and ZFS style ACLs.  */
      return acl_trivial (name);

#elif USE_ACL && HAVE_ACL && defined GETACLCNT

      /* Solaris 2.5 through Solaris 9, and contemporaneous versions of
	 HP-UX and Unixware.  */
      int n = acl (name, GETACLCNT, 0, NULL);
      return n < 0 ? (errno == ENOSYS ? 0 : -1) : (MIN_ACL_ENTRIES < n);

#elif USE_ACL && HAVE_ACL_GET_FILE && HAVE_ACL_FREE

      /* POSIX 1003.1e (draft 17 -- abandoned) specific version.  */
      int ret;

      if (HAVE_ACL_EXTENDED_FILE)
	ret = acl_extended_file (name);
      else
	{
	  acl_t acl = acl_get_file (name, ACL_TYPE_ACCESS);
	  if (acl)
	    {
	      ret = (3 < acl_entries (acl));
	      acl_free (acl);
	      if (ret == 0 && S_ISDIR (sb->st_mode))
		{
		  acl = acl_get_file (name, ACL_TYPE_DEFAULT);
		  if (acl)
		    {
		      ret = (0 < acl_entries (acl));
		      acl_free (acl);
		    }
		  else
		    ret = -1;
		}
	    }
	  else
	    ret = -1;
	}
      if (ret < 0)
	return ACL_NOT_WELL_SUPPORTED (errno) ? 0 : -1;
      return ret;
#endif
    }

  /* FIXME: Add support for AIX, Irix, and Tru64.  Please see Samba's
     source/lib/sysacls.c file for fix-related ideas.  */

  return 0;
}
