/* fchdir replacement.
   Copyright (C) 2006, 2007 Free Software Foundation, Inc.

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
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#include <config.h>

/* Specification.  */
#include <unistd.h>

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "canonicalize.h"
#include "dirfd.h"

/* This replacement assumes that a directory is not renamed while opened
   through a file descriptor.  */

/* Array of file descriptors opened.  If it points to a directory, it stores
   info about this directory; otherwise it stores an errno value of ENOTDIR.  */
typedef struct
{
  char *name;       /* Absolute name of the directory, or NULL.  */
  int saved_errno;  /* If name == NULL: The error code describing the failure
		       reason.  */
} dir_info_t;
static dir_info_t *dirs;
static size_t dirs_allocated;

/* Try to ensure dirs has enough room for a slot at index fd.  */
static void
ensure_dirs_slot (size_t fd)
{
  if (fd >= dirs_allocated)
    {
      size_t new_allocated;
      dir_info_t *new_dirs;
      size_t i;

      new_allocated = 2 * dirs_allocated + 1;
      if (new_allocated <= fd)
	new_allocated = fd + 1;
      new_dirs =
	(dirs != NULL
	 ? (dir_info_t *) realloc (dirs, new_allocated * sizeof (dir_info_t))
	 : (dir_info_t *) malloc (new_allocated * sizeof (dir_info_t)));
      if (new_dirs != NULL)
	{
	  for (i = dirs_allocated; i < new_allocated; i++)
	    {
	      new_dirs[i].name = NULL;
	      new_dirs[i].saved_errno = ENOTDIR;
	    }
	  dirs = new_dirs;
	  dirs_allocated = new_allocated;
	}
    }
}

/* Override open() and close(), to keep track of the open file descriptors.  */

int
close (int fd)
#undef close
{
  int retval = close (fd);

  if (retval >= 0 && fd >= 0 && fd < dirs_allocated)
    {
      if (dirs[fd].name != NULL)
	free (dirs[fd].name);
      dirs[fd].name = NULL;
      dirs[fd].saved_errno = ENOTDIR;
    }
  return retval;
}

int
open (const char *filename, int flags, ...)
#undef open
{
  mode_t mode;
  int fd;
  struct stat statbuf;

  mode = 0;
  if (flags & O_CREAT)
    {
      va_list arg;
      va_start (arg, flags);

      /* If mode_t is narrower than int, use the promoted type (int),
	 not mode_t.  Use sizeof to guess whether mode_t is narrower;
	 we don't know of any practical counterexamples.  */
      mode = (sizeof (mode_t) < sizeof (int)
	      ? va_arg (arg, int)
	      : va_arg (arg, mode_t));

      va_end (arg);
    }
  fd = open (filename, flags, mode);
  if (fd >= 0)
    {
      ensure_dirs_slot (fd);
      if (fd < dirs_allocated
	  && fstat (fd, &statbuf) >= 0 && S_ISDIR (statbuf.st_mode))
	{
	  dirs[fd].name = canonicalize_file_name (filename);
	  if (dirs[fd].name == NULL)
	    dirs[fd].saved_errno = errno;
	}
    }
  return fd;
}

/* Override opendir() and closedir(), to keep track of the open file
   descriptors.  Needed because there is a function dirfd().  */

int
closedir (DIR *dp)
#undef closedir
{
  int fd = dirfd (dp);
  int retval = closedir (dp);

  if (retval >= 0 && fd >= 0 && fd < dirs_allocated)
    {
      if (dirs[fd].name != NULL)
	free (dirs[fd].name);
      dirs[fd].name = NULL;
      dirs[fd].saved_errno = ENOTDIR;
    }
  return retval;
}

DIR *
opendir (const char *filename)
#undef opendir
{
  DIR *dp;

  dp = opendir (filename);
  if (dp != NULL)
    {
      int fd = dirfd (dp);
      if (fd >= 0)
	{
	  ensure_dirs_slot (fd);
	  if (fd < dirs_allocated)
	    {
	      dirs[fd].name = canonicalize_file_name (filename);
	      if (dirs[fd].name == NULL)
	        dirs[fd].saved_errno = errno;
	    }
	}
    }
  return dp;
}

/* Override dup() and dup2(), to keep track of open file descriptors.  */

int
dup (int oldfd)
#undef dup
{
  int newfd = dup (oldfd);

  if (oldfd >= 0 && newfd >= 0)
    {
      ensure_dirs_slot (newfd);
      if (newfd < dirs_allocated)
	{
	  if (oldfd < dirs_allocated)
	    {
	      if (dirs[oldfd].name != NULL)
		{
		  dirs[newfd].name = strdup (dirs[oldfd].name);
		  if (dirs[newfd].name == NULL)
		    dirs[newfd].saved_errno = ENOMEM;
		}
	      else
		{
		  dirs[newfd].name = NULL;
		  dirs[newfd].saved_errno = dirs[oldfd].saved_errno;
		}
	    }
	  else
	    {
	      dirs[newfd].name = NULL;
	      dirs[newfd].saved_errno = ENOMEM;
	    }
	}
    }
  return newfd;
}

int
dup2 (int oldfd, int newfd)
#undef dup2
{
  int retval = dup2 (oldfd, newfd);

  if (retval >= 0 && oldfd >= 0 && newfd >= 0 && newfd != oldfd)
    {
      ensure_dirs_slot (newfd);
      if (newfd < dirs_allocated)
	{
	  if (oldfd < dirs_allocated)
	    {
	      if (dirs[oldfd].name != NULL)
		{
		  dirs[newfd].name = strdup (dirs[oldfd].name);
		  if (dirs[newfd].name == NULL)
		    dirs[newfd].saved_errno = ENOMEM;
		}
	      else
		{
		  dirs[newfd].name = NULL;
		  dirs[newfd].saved_errno = dirs[oldfd].saved_errno;
		}
	    }
	  else
	    {
	      dirs[newfd].name = NULL;
	      dirs[newfd].saved_errno = ENOMEM;
	    }
	}
    }
  return retval;
}

/* Implement fchdir() in terms of chdir().  */

int
fchdir (int fd)
{
  if (fd >= 0)
    {
      if (fd < dirs_allocated)
	{
	  if (dirs[fd].name != NULL)
	    return chdir (dirs[fd].name);
	  else
	    {
	      errno = dirs[fd].saved_errno;
	      return -1;
	    }
	}
      else
	{
	  errno = ENOMEM;
	  return -1;
	}
    }
  else
    {
      errno = EBADF;
      return -1;
    }
}
