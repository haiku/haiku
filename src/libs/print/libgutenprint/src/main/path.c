/*
 * "$Id: path.c,v 1.20 2008/06/01 14:41:18 rlk Exp $"
 *
 *   Gutenprint path functions - split and search paths.
 *
 *   Copyright 2002 Roger Leigh (rleigh@debian.org)
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <gutenprint/gutenprint.h>
#include "gutenprint-internal.h"
#include <gutenprint/gutenprint-intl-internal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

static int stpi_path_check(const struct dirent *module);
static int stpi_scandir (const char *dir,
			 struct dirent ***namelist,
			 int (*sel) (const struct dirent *),
			 int (*cmp) (const void *, const void *));

/* WARNING: This is not thread safe! -- rlk 20030721 */
static const char *path_check_path;   /* Path for stpi_scandir() callback */
static const char *path_check_suffix; /* Suffix for stpi_scandir() callback */


static int
dirent_sort(const void *a,
	    const void *b)
{
  return strcoll ((*(const struct dirent * const *) a)->d_name,
		  (*(const struct dirent * const *) b)->d_name);
}


/*
 * Make a list of all modules in the search path.
 */
stp_list_t *
stp_path_search(stp_list_t *dirlist, /* List of directories to search */
		const char *suffix)  /* Required filename suffix */
{
  stp_list_t *findlist;              /* List of files to return */
  stp_list_item_t *diritem;          /* Current directory */
  struct dirent** module_dir = NULL; /* Current directory contents */
  char *module_name;                 /* File name to check */
  int n;                             /* Number of directory entries */

  if (!dirlist)
    return NULL;

  path_check_suffix = suffix;

  findlist = stp_list_create();
  if (!findlist)
    return NULL;
  stp_list_set_freefunc(findlist, stp_list_node_free_data);

  diritem = stp_list_get_start(dirlist);
  while (diritem)
    {
      path_check_path = (const char *) stp_list_item_get_data(diritem);
      stp_deprintf(STP_DBG_PATH, "stp-path: directory: %s\n",
		   (const char *) stp_list_item_get_data(diritem));
      n = stpi_scandir ((const char *) stp_list_item_get_data(diritem),
			&module_dir, stpi_path_check, dirent_sort);
      if (n >= 0)
	{
	  int idx;
	  for (idx = 0; idx < n; ++idx)
	    {
	      module_name = stpi_path_merge((const char *) stp_list_item_get_data(diritem),
					   module_dir[idx]->d_name);
	      stp_list_item_create(findlist, NULL, module_name);
	      free (module_dir[idx]); /* Must use plain free() */
	    }
	  free (module_dir); /* Must use plain free() */
	}
      diritem = stp_list_item_next(diritem);
    }
  return findlist;
}


/*
 * stpi_scandir() callback.  Check the filename is sane, has the
 * correct mode bits and suffix.
 */
static int
stpi_path_check(const struct dirent *module) /* File to check */
{
  int namelen;                              /* Filename length */
  int status = 0;                           /* Error status */
  int savederr;                             /* Saved errno */
  char *filename;                           /* Filename */
  struct stat modstat;                      /* stat() output */

  savederr = errno; /* since we are a callback, preserve
		       stpi_scandir() state */

  filename = stpi_path_merge(path_check_path, module->d_name);

  namelen = strlen(filename);
  /* make sure we can take off suffix (e.g. .la)
     and still have a sane filename */
  if (namelen >= strlen(path_check_suffix) + 1) 
    {
      if (!stat (filename, &modstat))
	{
	  /* check file exists, and is a regular file */
	  if (S_ISREG(modstat.st_mode))
	    status = 1;
	  if (strncmp(filename + (namelen - strlen(path_check_suffix)),
		      path_check_suffix,
		      strlen(path_check_suffix)))
	    {
	      status = 0;
	    }
	}
    }

  if (status)
    stp_deprintf(STP_DBG_PATH, "stp-path: file: `%s'\n", filename);

  stp_free(filename);
  filename = NULL;

  errno = savederr;
  return status;
}

stp_list_t *
stpi_data_path(void)
{
  stp_list_t *dir_list;                  /* List of directories to scan */
  if (!(dir_list = stp_list_create()))
    return NULL;
  stp_list_set_freefunc(dir_list, stp_list_node_free_data);
  if (getenv("STP_DATA_PATH"))
    stp_path_split(dir_list, getenv("STP_DATA_PATH"));
  else
    stp_path_split(dir_list, PKGXMLDATADIR);
  return dir_list;
}

stp_list_t *
stpi_list_files_on_data_path(const char *name)
{
  stp_list_t *dir_list = stpi_data_path(); /* List of directories to scan */
  stp_list_t *file_list = stp_path_search(dir_list, name);
  stp_list_destroy(dir_list);
  return file_list;
}

/*
 * Join a path and filename together.
 */
char *
stpi_path_merge(const char *path, /* Path */
	       const char *file) /* Filename */
{
  char *filename;                /* Filename to return */
  int namelen = strlen(path) + strlen(file) + 2;
  filename = (char *) stp_malloc(namelen * sizeof(char));
  strcpy (filename, path);
  strcat (filename, "/");
  strcat (filename, file);
  filename[namelen - 1] = '\0';
  return filename;
}


/*
 * Split a PATH-type string (colon-delimited) into separate
 * directories.
 */
void
stp_path_split(stp_list_t *list, /* List to add directories to */
	       const char *path) /* Path to split */
{
  const char *start = path;      /* Start of path name */
  const char *end = NULL;        /* End of path name */
  char *dir = NULL;              /* Path name */
  int len;                       /* Length of path name */

  while (start)
    {
      end = (const char *) strchr(start, ':');
      if (!end)
	len = strlen(start) + 1;
      else
	len = (end - start);

      if (len && !(len == 1 && !end))
	{
	  dir = (char *) stp_malloc(len + 1);
	  strncpy(dir, start, len);
	  dir[len] = '\0';
	  stp_list_item_create(list, NULL, dir);
	}
      if (!end)
	{
	  start = NULL;
	  break;
	}
      start = end + 1;
    }
}

/* Adapted from GNU libc <dirent.h>
   These macros extract size information from a `struct dirent *'.
   They may evaluate their argument multiple times, so it must not
   have side effects.  Each of these may involve a relatively costly
   call to `strlen' on some systems, so these values should be cached.

   _D_EXACT_NAMLEN (DP) returns the length of DP->d_name, not including
   its terminating null character.

   _D_ALLOC_NAMLEN (DP) returns a size at least (_D_EXACT_NAMLEN (DP) + 1);
   that is, the allocation size needed to hold the DP->d_name string.
   Use this macro when you don't need the exact length, just an upper bound.
   This macro is less likely to require calling `strlen' than _D_EXACT_NAMLEN.
   */

#ifdef _DIRENT_HAVE_D_NAMLEN
# ifndef _D_EXACT_NAMLEN
#  define _D_EXACT_NAMLEN(d) ((d)->d_namlen)
# endif
# ifndef _D_ALLOC_NAMLEN
#  define _D_ALLOC_NAMLEN(d) (_D_EXACT_NAMLEN (d) + 1)
# endif
#else
# ifndef _D_EXACT_NAMLEN
#  define _D_EXACT_NAMLEN(d) (strlen ((d)->d_name))
# endif
# ifndef _D_ALLOC_NAMLEN
#  ifdef _DIRENT_HAVE_D_RECLEN
#   define _D_ALLOC_NAMLEN(d) (((char *) (d) + (d)->d_reclen) - &(d)->d_name[0])
#  else
#   define _D_ALLOC_NAMLEN(d) (sizeof (d)->d_name > 1 ? sizeof (d)->d_name : \
                               _D_EXACT_NAMLEN (d) + 1)
#  endif
# endif
#endif

/*
 * BSD scandir() replacement, from glibc
 */
static int
stpi_scandir (const char *dir,
	      struct dirent ***namelist,
	      int (*sel) (const struct dirent *),
	      int (*cmp) (const void *, const void *))
{
  DIR *dp = opendir (dir);
  struct dirent **v = NULL;
  size_t vsize = 0, i;
  struct dirent *d;
  int save;

  if (dp == NULL)
    return -1;

  save = errno;
  errno = 0;

  i = 0;
  while ((d = readdir (dp)) != NULL)
    if (sel == NULL || (*sel) (d))
      {
	struct dirent *vnew;
	size_t dsize;

	/* Ignore errors from sel or readdir */
        errno = 0;

	if (i == vsize)
	  {
	    struct dirent **new;
	    if (vsize == 0)
	      vsize = 10;
	    else
	      vsize *= 2;
	    new = (struct dirent **) realloc (v, vsize * sizeof (*v));
	    if (new == NULL)
	      break;
	    v = new;
	  }

	dsize = &d->d_name[_D_ALLOC_NAMLEN (d)] - (char *) d;
	vnew = (struct dirent *) malloc (dsize);
	if (vnew == NULL)
	  break;

	v[i++] = (struct dirent *) memcpy (vnew, d, dsize);
      }

  if (errno != 0)
    {
      save = errno;

      while (i > 0)
	free (v[--i]);
      free (v);

      i = -1;
    }
  else
    {
      /* Sort the list if we have a comparison function to sort with.  */
      if (cmp != NULL)
	qsort (v, i, sizeof (*v), cmp);

      *namelist = v;
    }

  (void) closedir (dp);
  errno = save;

  return i;
}
