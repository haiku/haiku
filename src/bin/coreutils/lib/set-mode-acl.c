/* set-mode-acl.c - set access control list equivalent to a mode

   Copyright (C) 2002-2003, 2005-2009 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Written by Paul Eggert and Andreas Gruenbacher, and Bruno Haible.  */

#include <config.h>

#include "acl.h"

#include "acl-internal.h"

#include "gettext.h"
#define _(msgid) gettext (msgid)


/* If DESC is a valid file descriptor use fchmod to change the
   file's mode to MODE on systems that have fchown. On systems
   that don't have fchown and if DESC is invalid, use chown on
   NAME instead.
   Return 0 if successful.  Return -1 and set errno upon failure.  */

int
chmod_or_fchmod (const char *name, int desc, mode_t mode)
{
  if (HAVE_FCHMOD && desc != -1)
    return fchmod (desc, mode);
  else
    return chmod (name, mode);
}

/* Set the access control lists of a file. If DESC is a valid file
   descriptor, use file descriptor operations where available, else use
   filename based operations on NAME.  If access control lists are not
   available, fchmod the target file to MODE.  Also sets the
   non-permission bits of the destination file (S_ISUID, S_ISGID, S_ISVTX)
   to those from MODE if any are set.
   Return 0 if successful.  Return -1 and set errno upon failure.  */

int
qset_acl (char const *name, int desc, mode_t mode)
{
#if USE_ACL
# if HAVE_ACL_GET_FILE
  /* POSIX 1003.1e draft 17 (abandoned) specific version.  */
  /* Linux, FreeBSD, MacOS X, IRIX, Tru64 */
#  if MODE_INSIDE_ACL
  /* Linux, FreeBSD, IRIX, Tru64 */

  /* We must also have acl_from_text and acl_delete_def_file.
     (acl_delete_def_file could be emulated with acl_init followed
      by acl_set_file, but acl_set_file with an empty acl is
      unspecified.)  */

#   ifndef HAVE_ACL_FROM_TEXT
#    error Must have acl_from_text (see POSIX 1003.1e draft 17).
#   endif
#   ifndef HAVE_ACL_DELETE_DEF_FILE
#    error Must have acl_delete_def_file (see POSIX 1003.1e draft 17).
#   endif

  acl_t acl;
  int ret;

  if (HAVE_ACL_FROM_MODE) /* Linux */
    {
      acl = acl_from_mode (mode);
      if (!acl)
	return -1;
    }
  else /* FreeBSD, IRIX, Tru64 */
    {
      /* If we were to create the ACL using the functions acl_init(),
	 acl_create_entry(), acl_set_tag_type(), acl_set_qualifier(),
	 acl_get_permset(), acl_clear_perm[s](), acl_add_perm(), we
	 would need to create a qualifier.  I don't know how to do this.
	 So create it using acl_from_text().  */

#   if HAVE_ACL_FREE_TEXT /* Tru64 */
      char acl_text[] = "u::---,g::---,o::---,";
#   else /* FreeBSD, IRIX */
      char acl_text[] = "u::---,g::---,o::---";
#   endif

      if (mode & S_IRUSR) acl_text[ 3] = 'r';
      if (mode & S_IWUSR) acl_text[ 4] = 'w';
      if (mode & S_IXUSR) acl_text[ 5] = 'x';
      if (mode & S_IRGRP) acl_text[10] = 'r';
      if (mode & S_IWGRP) acl_text[11] = 'w';
      if (mode & S_IXGRP) acl_text[12] = 'x';
      if (mode & S_IROTH) acl_text[17] = 'r';
      if (mode & S_IWOTH) acl_text[18] = 'w';
      if (mode & S_IXOTH) acl_text[19] = 'x';

      acl = acl_from_text (acl_text);
      if (!acl)
	return -1;
    }
  if (HAVE_ACL_SET_FD && desc != -1)
    ret = acl_set_fd (desc, acl);
  else
    ret = acl_set_file (name, ACL_TYPE_ACCESS, acl);
  if (ret != 0)
    {
      int saved_errno = errno;
      acl_free (acl);

      if (ACL_NOT_WELL_SUPPORTED (errno))
	return chmod_or_fchmod (name, desc, mode);
      else
	{
	  errno = saved_errno;
	  return -1;
	}
    }
  else
    acl_free (acl);

  if (S_ISDIR (mode) && acl_delete_def_file (name))
    return -1;

  if (mode & (S_ISUID | S_ISGID | S_ISVTX))
    {
      /* We did not call chmod so far, so the special bits have not yet
	 been set.  */
      return chmod_or_fchmod (name, desc, mode);
    }
  return 0;

#  else /* !MODE_INSIDE_ACL */
  /* MacOS X */

#   if !HAVE_ACL_TYPE_EXTENDED
#    error Must have ACL_TYPE_EXTENDED
#   endif

  /* On MacOS X,  acl_get_file (name, ACL_TYPE_ACCESS)
     and          acl_get_file (name, ACL_TYPE_DEFAULT)
     always return NULL / EINVAL.  You have to use
		  acl_get_file (name, ACL_TYPE_EXTENDED)
     or           acl_get_fd (open (name, ...))
     to retrieve an ACL.
     On the other hand,
		  acl_set_file (name, ACL_TYPE_ACCESS, acl)
     and          acl_set_file (name, ACL_TYPE_DEFAULT, acl)
     have the same effect as
		  acl_set_file (name, ACL_TYPE_EXTENDED, acl):
     Each of these calls sets the file's ACL.  */

  acl_t acl;
  int ret;

  /* Remove the ACL if the file has ACLs.  */
  if (HAVE_ACL_GET_FD && desc != -1)
    acl = acl_get_fd (desc);
  else
    acl = acl_get_file (name, ACL_TYPE_EXTENDED);
  if (acl)
    {
      acl_free (acl);

      acl = acl_init (0);
      if (acl)
	{
	  if (HAVE_ACL_SET_FD && desc != -1)
	    ret = acl_set_fd (desc, acl);
	  else
	    ret = acl_set_file (name, ACL_TYPE_EXTENDED, acl);
	  if (ret != 0)
	    {
	      int saved_errno = errno;

	      acl_free (acl);

	      if (ACL_NOT_WELL_SUPPORTED (saved_errno))
		return chmod_or_fchmod (name, desc, mode);
	      else
		{
		  errno = saved_errno;
		  return -1;
		}
	    }
	  acl_free (acl);
	}
    }

  /* Since !MODE_INSIDE_ACL, we have to call chmod explicitly.  */
  return chmod_or_fchmod (name, desc, mode);
#  endif

# elif HAVE_ACL && defined GETACLCNT /* Solaris, Cygwin, not HP-UX */

#  if defined ACL_NO_TRIVIAL
  /* Solaris 10 (newer version), which has additional API declared in
     <sys/acl.h> (acl_t) and implemented in libsec (acl_set, acl_trivial,
     acl_fromtext, ...).  */

  acl_t *aclp;
  char acl_text[] = "user::---,group::---,mask:---,other:---";
  int ret;
  int saved_errno;

  if (mode & S_IRUSR) acl_text[ 6] = 'r';
  if (mode & S_IWUSR) acl_text[ 7] = 'w';
  if (mode & S_IXUSR) acl_text[ 8] = 'x';
  if (mode & S_IRGRP) acl_text[17] = acl_text[26] = 'r';
  if (mode & S_IWGRP) acl_text[18] = acl_text[27] = 'w';
  if (mode & S_IXGRP) acl_text[19] = acl_text[28] = 'x';
  if (mode & S_IROTH) acl_text[36] = 'r';
  if (mode & S_IWOTH) acl_text[37] = 'w';
  if (mode & S_IXOTH) acl_text[38] = 'x';

  if (acl_fromtext (acl_text, &aclp) != 0)
    {
      errno = ENOMEM;
      return -1;
    }

  ret = (desc < 0 ? acl_set (name, aclp) : facl_set (desc, aclp));
  saved_errno = errno;
  acl_free (aclp);
  if (ret < 0)
    {
      if (saved_errno == ENOSYS)
	return chmod_or_fchmod (name, desc, mode);
      errno = saved_errno;
      return -1;
    }

  if (mode & (S_ISUID | S_ISGID | S_ISVTX))
    {
      /* We did not call chmod so far, so the special bits have not yet
	 been set.  */
      return chmod_or_fchmod (name, desc, mode);
    }
  return 0;

#  else /* Solaris, Cygwin, general case */

#   ifdef ACE_GETACL
  /* Solaris also has a different variant of ACLs, used in ZFS and NFSv4
     file systems (whereas the other ones are used in UFS file systems).  */

  /* The flags in the ace_t structure changed in a binary incompatible way
     when ACL_NO_TRIVIAL etc. were introduced in <sys/acl.h> version 1.15.
     How to distinguish the two conventions at runtime?
     We fetch the existing ACL.  In the old convention, usually three ACEs have
     a_flags = ACE_OWNER / ACE_GROUP / ACE_OTHER, in the range 0x0100..0x0400.
     In the new convention, these values are not used.  */
  int convention;

  {
    int count;
    ace_t *entries;

    for (;;)
      {
	if (desc != -1)
	  count = facl (desc, ACE_GETACLCNT, 0, NULL);
	else
	  count = acl (name, ACE_GETACLCNT, 0, NULL);
	if (count <= 0)
	  {
	    convention = -1;
	    break;
	  }
	entries = (ace_t *) malloc (count * sizeof (ace_t));
	if (entries == NULL)
	  {
	    errno = ENOMEM;
	    return -1;
	  }
	if ((desc != -1
	     ? facl (desc, ACE_GETACL, count, entries)
	     : acl (name, ACE_GETACL, count, entries))
	    == count)
	  {
	    int i;

	    convention = 0;
	    for (i = 0; i < count; i++)
	      if (entries[i].a_flags & (ACE_OWNER | ACE_GROUP | ACE_OTHER))
		{
		  convention = 1;
		  break;
		}
	    free (entries);
	    break;
	  }
	/* Huh? The number of ACL entries changed since the last call.
	   Repeat.  */
	free (entries);
      }
  }

  if (convention >= 0)
    {
      ace_t entries[3];
      int ret;

      if (convention)
	{
	  /* Running on Solaris 10.  */
	  entries[0].a_type = ALLOW;
	  entries[0].a_flags = ACE_OWNER;
	  entries[0].a_who = 0; /* irrelevant */
	  entries[0].a_access_mask = (mode >> 6) & 7;
	  entries[1].a_type = ALLOW;
	  entries[1].a_flags = ACE_GROUP;
	  entries[1].a_who = 0; /* irrelevant */
	  entries[1].a_access_mask = (mode >> 3) & 7;
	  entries[2].a_type = ALLOW;
	  entries[2].a_flags = ACE_OTHER;
	  entries[2].a_who = 0;
	  entries[2].a_access_mask = mode & 7;
	}
      else
	{
	  /* Running on Solaris 10 (newer version) or Solaris 11.  */
	  entries[0].a_type = ACE_ACCESS_ALLOWED_ACE_TYPE;
	  entries[0].a_flags = NEW_ACE_OWNER;
	  entries[0].a_who = 0; /* irrelevant */
	  entries[0].a_access_mask =
	    (mode & 0400 ? NEW_ACE_READ_DATA : 0)
	    | (mode & 0200 ? NEW_ACE_WRITE_DATA : 0)
	    | (mode & 0100 ? NEW_ACE_EXECUTE : 0);
	  entries[1].a_type = ACE_ACCESS_ALLOWED_ACE_TYPE;
	  entries[1].a_flags = NEW_ACE_GROUP | NEW_ACE_IDENTIFIER_GROUP;
	  entries[1].a_who = 0; /* irrelevant */
	  entries[1].a_access_mask =
	    (mode & 0040 ? NEW_ACE_READ_DATA : 0)
	    | (mode & 0020 ? NEW_ACE_WRITE_DATA : 0)
	    | (mode & 0010 ? NEW_ACE_EXECUTE : 0);
	  entries[2].a_type = ACE_ACCESS_ALLOWED_ACE_TYPE;
	  entries[2].a_flags = ACE_EVERYONE;
	  entries[2].a_who = 0;
	  entries[2].a_access_mask =
	    (mode & 0004 ? NEW_ACE_READ_DATA : 0)
	    | (mode & 0002 ? NEW_ACE_WRITE_DATA : 0)
	    | (mode & 0001 ? NEW_ACE_EXECUTE : 0);
	}
      if (desc != -1)
	ret = facl (desc, ACE_SETACL,
		    sizeof (entries) / sizeof (ace_t), entries);
      else
	ret = acl (name, ACE_SETACL,
		   sizeof (entries) / sizeof (ace_t), entries);
      if (ret < 0 && errno != EINVAL && errno != ENOTSUP)
	{
	  if (errno == ENOSYS)
	    return chmod_or_fchmod (name, desc, mode);
	  return -1;
	}
    }
#   endif

  {
    aclent_t entries[3];
    int ret;

    entries[0].a_type = USER_OBJ;
    entries[0].a_id = 0; /* irrelevant */
    entries[0].a_perm = (mode >> 6) & 7;
    entries[1].a_type = GROUP_OBJ;
    entries[1].a_id = 0; /* irrelevant */
    entries[1].a_perm = (mode >> 3) & 7;
    entries[2].a_type = OTHER_OBJ;
    entries[2].a_id = 0;
    entries[2].a_perm = mode & 7;

    if (desc != -1)
      ret = facl (desc, SETACL, sizeof (entries) / sizeof (aclent_t), entries);
    else
      ret = acl (name, SETACL, sizeof (entries) / sizeof (aclent_t), entries);
    if (ret < 0)
      {
	if (errno == ENOSYS)
	  return chmod_or_fchmod (name, desc, mode);
	return -1;
      }
  }

  if (!MODE_INSIDE_ACL || (mode & (S_ISUID | S_ISGID | S_ISVTX)))
    {
      /* We did not call chmod so far, so the special bits have not yet
	 been set.  */
      return chmod_or_fchmod (name, desc, mode);
    }
  return 0;

#  endif

# elif HAVE_GETACL /* HP-UX */

  struct stat statbuf;
  struct acl_entry entries[3];
  int ret;

  if (desc != -1)
    ret = fstat (desc, &statbuf);
  else
    ret = stat (name, &statbuf);
  if (ret < 0)
    return -1;

  entries[0].uid = statbuf.st_uid;
  entries[0].gid = ACL_NSGROUP;
  entries[0].mode = (mode >> 6) & 7;
  entries[1].uid = ACL_NSUSER;
  entries[1].gid = statbuf.st_gid;
  entries[1].mode = (mode >> 3) & 7;
  entries[2].uid = ACL_NSUSER;
  entries[2].gid = ACL_NSGROUP;
  entries[2].mode = mode & 7;

  if (desc != -1)
    ret = fsetacl (desc, sizeof (entries) / sizeof (struct acl_entry), entries);
  else
    ret = setacl (name, sizeof (entries) / sizeof (struct acl_entry), entries);
  if (ret < 0)
    {
      if (errno == ENOSYS || errno == EOPNOTSUPP)
	return chmod_or_fchmod (name, desc, mode);
      return -1;
    }

  if (mode & (S_ISUID | S_ISGID | S_ISVTX))
    {
      /* We did not call chmod so far, so the special bits have not yet
	 been set.  */
      return chmod_or_fchmod (name, desc, mode);
    }
  return 0;

# elif HAVE_ACLX_GET && 0 /* AIX */

  /* TODO: use aclx_fput or aclx_put, respectively */

# elif HAVE_STATACL /* older AIX */

  union { struct acl a; char room[128]; } u;
  int ret;

  u.a.acl_len = (char *) &u.a.acl_ext[0] - (char *) &u.a; /* no entries */
  u.a.acl_mode = mode & ~(S_IXACL | 0777);
  u.a.u_access = (mode >> 6) & 7;
  u.a.g_access = (mode >> 3) & 7;
  u.a.o_access = mode & 7;

  if (desc != -1)
    ret = fchacl (desc, &u.a, u.a.acl_len);
  else
    ret = chacl (name, &u.a, u.a.acl_len);

  if (ret < 0 && errno == ENOSYS)
    return chmod_or_fchmod (name, desc, mode);

  return ret;

# else /* Unknown flavor of ACLs */
  return chmod_or_fchmod (name, desc, mode);
# endif
#else /* !USE_ACL */
  return chmod_or_fchmod (name, desc, mode);
#endif
}

/* As with qset_acl, but also output a diagnostic on failure.  */

int
set_acl (char const *name, int desc, mode_t mode)
{
  int r = qset_acl (name, desc, mode);
  if (r != 0)
    error (0, errno, _("setting permissions for %s"), quote (name));
  return r;
}
