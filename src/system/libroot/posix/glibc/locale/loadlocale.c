/* Functions to read locale data files.
   Copyright (C) 1996,1997,1998,1999,2000,2001 Free Software Foundation, Inc.
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

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef _POSIX_MAPPED_FILES
# include <sys/mman.h>
#endif
#include <sys/stat.h>

#include "localeinfo.h"


static const size_t _nl_category_num_items[] =
{
#define DEFINE_CATEGORY(category, category_name, items, a) \
  [category] = _NL_ITEM_INDEX (_NL_NUM_##category),
#include "categories.def"
#undef	DEFINE_CATEGORY
};


#define NO_PAREN(arg, rest...) arg, ##rest

#define DEFINE_CATEGORY(category, category_name, items, a) \
static const enum value_type _nl_value_type_##category[] = { NO_PAREN items };
#define DEFINE_ELEMENT(element, element_name, optstd, type, rest...) \
  [_NL_ITEM_INDEX (element)] = type,
#include "categories.def"
#undef DEFINE_CATEGORY

static const enum value_type *_nl_value_types[] =
{
#define DEFINE_CATEGORY(category, category_name, items, a) \
  [category] = _nl_value_type_##category,
#include "categories.def"
#undef DEFINE_CATEGORY
};


void
_nl_load_locale (struct loaded_l10nfile *file, int category)
{
  int fd;
  struct
    {
      unsigned int magic;
      unsigned int nstrings;
      unsigned int strindex[0];
    } *filedata;
  struct stat64 st;
  struct locale_data *newdata;
  int save_err;
  int mmaped = 1;
  size_t cnt;

  file->decided = 1;
  file->data = NULL;

  fd = __open (file->filename, O_RDONLY);
  if (__builtin_expect (fd, 0) < 0)
    /* Cannot open the file.  */
    return;

  if (__builtin_expect (__fxstat64 (_STAT_VER, fd, &st), 0) < 0)
    goto puntfd;
  if (__builtin_expect (S_ISDIR (st.st_mode), 0))
    {
      /* LOCALE/LC_foo is a directory; open LOCALE/LC_foo/SYS_LC_foo
           instead.  */
      char *newp;
      size_t filenamelen;

      __close (fd);

      filenamelen = strlen (file->filename);
      newp = (char *) alloca (filenamelen
			      + 5 + _nl_category_name_sizes[category] + 1);
      __mempcpy (__mempcpy (__mempcpy (newp, file->filename, filenamelen),
			    "/SYS_", 5),
		 _nl_category_names[category],
		 _nl_category_name_sizes[category] + 1);

      fd = __open (newp, O_RDONLY);
      if (__builtin_expect (fd, 0) < 0)
	return;

      if (__builtin_expect (__fxstat64 (_STAT_VER, fd, &st), 0) < 0)
	goto puntfd;
    }

  /* Map in the file's data.  */
  save_err = errno;
#ifdef _POSIX_MAPPED_FILES
# ifndef MAP_COPY
  /* Linux seems to lack read-only copy-on-write.  */
#  define MAP_COPY MAP_PRIVATE
# endif
# ifndef MAP_FILE
  /* Some systems do not have this flag; it is superfluous.  */
#  define MAP_FILE 0
# endif
# ifndef MAP_INHERIT
  /* Some systems might lack this; they lose.  */
#  define MAP_INHERIT 0
# endif
  filedata = (void *) __mmap ((caddr_t) 0, st.st_size, PROT_READ,
			      MAP_FILE|MAP_COPY|MAP_INHERIT, fd, 0);
  if (__builtin_expect ((void *) filedata != MAP_FAILED, 1))
    {
      if (__builtin_expect (st.st_size < sizeof (*filedata), 0))
	/* This cannot be a locale data file since it's too small.  */
	goto puntfd;
    }
  else
    {
      if (__builtin_expect (errno, ENOSYS) == ENOSYS)
	{
#endif	/* _POSIX_MAPPED_FILES */
	  /* No mmap; allocate a buffer and read from the file.  */
	  mmaped = 0;
	  filedata = malloc (st.st_size);
	  if (filedata != NULL)
	    {
	      off_t to_read = st.st_size;
	      ssize_t nread;
	      char *p = (char *) filedata;
	      while (to_read > 0)
		{
		  nread = __read (fd, p, to_read);
		  if (__builtin_expect (nread, 1) <= 0)
		    {
		      free (filedata);
		      if (nread == 0)
			__set_errno (EINVAL); /* Bizarreness going on.  */
		      goto puntfd;
		    }
		  p += nread;
		  to_read -= nread;
		}
	    }
	  else
	    goto puntfd;
	  __set_errno (save_err);
#ifdef _POSIX_MAPPED_FILES
	}
      else
	goto puntfd;
    }
#endif	/* _POSIX_MAPPED_FILES */

  if (__builtin_expect (filedata->magic != LIMAGIC (category), 0))
    /* Bad data file in either byte order.  */
    {
    puntmap:
#ifdef _POSIX_MAPPED_FILES
      __munmap ((caddr_t) filedata, st.st_size);
#endif
    puntfd:
      __close (fd);
      return;
    }

  if (__builtin_expect (filedata->nstrings < _nl_category_num_items[category],
			0)
      || (__builtin_expect (sizeof *filedata
			    + filedata->nstrings * sizeof (unsigned int)
			    >= (size_t) st.st_size, 0)))
    {
      /* Insufficient data.  */
      __set_errno (EINVAL);
      goto puntmap;
    }

  newdata = malloc (sizeof *newdata
		    + filedata->nstrings * sizeof (union locale_data_value));
  if (newdata == NULL)
    goto puntmap;

  newdata->name = NULL;	/* This will be filled if necessary in findlocale.c. */
  newdata->filedata = (void *) filedata;
  newdata->filesize = st.st_size;
  newdata->mmaped = mmaped;
  newdata->usage_count = 0;
  newdata->use_translit = 0;
  newdata->options = NULL;
  newdata->nstrings = filedata->nstrings;
  for (cnt = 0; cnt < newdata->nstrings; ++cnt)
    {
      off_t idx = filedata->strindex[cnt];
      if (__builtin_expect (idx > newdata->filesize, 0))
	{
	  free (newdata);
	  __set_errno (EINVAL);
	  goto puntmap;
	}
      if (__builtin_expect (_nl_value_types[category][cnt] == word, 0))
	{
	  assert (idx % __alignof__ (u_int32_t) == 0);
	  newdata->values[cnt].word =
	    *((u_int32_t *) (newdata->filedata + idx));
	}
      else
	newdata->values[cnt].string = newdata->filedata + idx;
    }

  __close (fd);
  file->data = newdata;
}

void
_nl_unload_locale (struct locale_data *locale)
{
#ifdef _POSIX_MAPPED_FILES
  if (__builtin_expect (locale->mmaped, 1))
    __munmap ((caddr_t) locale->filedata, locale->filesize);
  else
#endif
    free ((void *) locale->filedata);

  free ((char *) locale->options);
  free ((char *) locale->name);
  free (locale);
}
