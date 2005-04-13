/* Copyright (C) 2002, 2003 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 2002.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <inttypes.h>
#include <libintl.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/stat.h>

#include "../../crypt/md5.h"
#include "../localeinfo.h"
#include "../locarchive.h"
#include "localedef.h"

/* Define the hash function.  We define the function as static inline.
   We must change the name so as not to conflict with simple-hash.h.  */
#define compute_hashval static inline archive_hashval
#define hashval_t uint32_t
#include "hashval.h"
#undef compute_hashval

extern const char *output_prefix;

#define ARCHIVE_NAME LOCALEDIR "/locale-archive"

static const char *locnames[] =
  {
#define DEFINE_CATEGORY(category, category_name, items, a) \
  [category] = category_name,
#include "categories.def"
#undef  DEFINE_CATEGORY
  };


/* Size of the initial archive header.  */
#define INITIAL_NUM_NAMES	450
#define INITIAL_SIZE_STRINGS	3500
#define INITIAL_NUM_LOCREC	350
#define INITIAL_NUM_SUMS	2000


static void
create_archive (const char *archivefname, struct locarhandle *ah)
{
  int fd;
  char fname[strlen (archivefname) + sizeof (".XXXXXX")];
  struct locarhead head;
  void *p;
  size_t total;

  strcpy (stpcpy (fname, archivefname), ".XXXXXX");

  /* Create a temporary file in the correct directory.  */
  fd = mkstemp (fname);
  if (fd == -1)
    error (EXIT_FAILURE, errno, _("cannot create temporary file"));

  /* Create the initial content of the archive.  */
  head.magic = AR_MAGIC;
  head.namehash_offset = sizeof (struct locarhead);
  head.namehash_used = 0;
  head.namehash_size = next_prime (INITIAL_NUM_NAMES);

  head.string_offset = (head.namehash_offset
			+ head.namehash_size * sizeof (struct namehashent));
  head.string_used = 0;
  head.string_size = INITIAL_SIZE_STRINGS;

  head.locrectab_offset = head.string_offset + head.string_size;
  head.locrectab_used = 0;
  head.locrectab_size = INITIAL_NUM_LOCREC;

  head.sumhash_offset = (head.locrectab_offset
			 + head.locrectab_size * sizeof (struct locrecent));
  head.sumhash_used = 0;
  head.sumhash_size = next_prime (INITIAL_NUM_SUMS);

  total = head.sumhash_offset + head.sumhash_size * sizeof (struct sumhashent);

  /* Write out the header and create room for the other data structures.  */
  if (TEMP_FAILURE_RETRY (write (fd, &head, sizeof (head))) != sizeof (head))
    {
      int errval = errno;
      unlink (fname);
      error (EXIT_FAILURE, errval, _("cannot initialize archive file"));
    }

  if (ftruncate64 (fd, total) != 0)
    {
      int errval = errno;
      unlink (fname);
      error (EXIT_FAILURE, errval, _("cannot resize archive file"));
    }

  /* Map the header and all the administration data structures.  */
  p = mmap64 (NULL, total, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (p == MAP_FAILED)
    {
      int errval = errno;
      unlink (fname);
      error (EXIT_FAILURE, errval, _("cannot map archive header"));
    }

  /* Now try to rename it.  We don't use the rename function since
     this would overwrite a file which has been created in
     parallel.  */
  if (link (fname, archivefname) == -1)
    {
      int errval = errno;

      /* We cannot use the just created file.  */
      close (fd);
      unlink (fname);

      if (errval == EEXIST)
	{
	  /* There is already an archive.  Must have been a localedef run
	     which happened in parallel.  Simply open this file then.  */
	  open_archive (ah, false);
	  return;
	}

      error (EXIT_FAILURE, errval, _("failed to create new locale archive"));
    }

  /* Remove the temporary name.  */
  unlink (fname);

  /* Make the file globally readable.  */
  if (fchmod (fd, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH) == -1)
    {
      int errval = errno;
      unlink (archivefname);
      error (EXIT_FAILURE, errval,
	     _("cannot change mode of new locale archive"));
    }

  ah->fd = fd;
  ah->addr = p;
  ah->len = total;
}


/* This structure and qsort comparator function are used below to sort an
   old archive's locrec table in order of data position in the file.  */
struct oldlocrecent
{
  unsigned int cnt;
  struct locrecent *locrec;
};

static int
oldlocrecentcmp (const void *a, const void *b)
{
  struct locrecent *la = ((const struct oldlocrecent *) a)->locrec;
  struct locrecent *lb = ((const struct oldlocrecent *) b)->locrec;
  uint32_t start_a = -1, end_a = 0;
  uint32_t start_b = -1, end_b = 0;
  int cnt;

  for (cnt = 0; cnt < __LC_LAST; ++cnt)
    if (cnt != LC_ALL)
      {
	if (la->record[cnt].offset < start_a)
	  start_a = la->record[cnt].offset;
	if (la->record[cnt].offset + la->record[cnt].len > end_a)
	  end_a = la->record[cnt].offset + la->record[cnt].len;
      }
  assert (start_a != (uint32_t)-1);
  assert (end_a != 0);

  for (cnt = 0; cnt < __LC_LAST; ++cnt)
    if (cnt != LC_ALL)
      {
	if (lb->record[cnt].offset < start_b)
	  start_b = lb->record[cnt].offset;
	if (lb->record[cnt].offset + lb->record[cnt].len > end_b)
	  end_b = lb->record[cnt].offset + lb->record[cnt].len;
      }
  assert (start_b != (uint32_t)-1);
  assert (end_b != 0);

  if (start_a != start_b)
    return (int)start_a - (int)start_b;
  return (int)end_a - (int)end_b;
}


/* forward decl for below */
static uint32_t add_locale (struct locarhandle *ah, const char *name,
			    locale_data_t data, bool replace);

static void
enlarge_archive (struct locarhandle *ah, const struct locarhead *head)
{
  struct stat64 st;
  int fd;
  struct locarhead newhead;
  size_t total;
  void *p;
  unsigned int cnt, loccnt;
  struct namehashent *oldnamehashtab;
  struct locrecent *oldlocrectab;
  struct locarhandle new_ah;
  size_t prefix_len = output_prefix ? strlen (output_prefix) : 0;
  char archivefname[prefix_len + sizeof (ARCHIVE_NAME)];
  char fname[prefix_len + sizeof (ARCHIVE_NAME) + sizeof (".XXXXXX") - 1];
  struct oldlocrecent *oldlocrecarray;

  if (output_prefix)
    memcpy (archivefname, output_prefix, prefix_len);
  strcpy (archivefname + prefix_len, ARCHIVE_NAME);
  strcpy (stpcpy (fname, archivefname), ".XXXXXX");

  /* Not all of the old file has to be mapped.  Change this now this
     we will have to access the whole content.  */
  if (fstat64 (ah->fd, &st) != 0
      || (ah->addr = mmap64 (NULL, st.st_size, PROT_READ | PROT_WRITE,
			     MAP_SHARED, ah->fd, 0)) == MAP_FAILED)
    error (EXIT_FAILURE, errno, _("cannot map locale archive file"));
  ah->len = st.st_size;

  /* Create a temporary file in the correct directory.  */
  fd = mkstemp (fname);
  if (fd == -1)
    error (EXIT_FAILURE, errno, _("cannot create temporary file"));

  /* Copy the existing head information.  */
  newhead = *head;

  /* Create the new archive header.  The sizes of the various tables
     should be double from what is currently used.  */
  newhead.namehash_size = MAX (next_prime (2 * newhead.namehash_used),
			       newhead.namehash_size);
  if (! be_quiet)
    printf ("name: size: %u, used: %d, new: size: %u\n",
	    head->namehash_size, head->namehash_used, newhead.namehash_size);

  newhead.string_offset = (newhead.namehash_offset
			   + (newhead.namehash_size
			      * sizeof (struct namehashent)));
  /* Keep the string table size aligned to 4 bytes, so that
     all the struct { uint32_t } types following are happy.  */
  newhead.string_size = MAX ((2 * newhead.string_used + 3) & -4,
			     newhead.string_size);

  newhead.locrectab_offset = newhead.string_offset + newhead.string_size;
  newhead.locrectab_size = MAX (2 * newhead.locrectab_used,
				newhead.locrectab_size);

  newhead.sumhash_offset = (newhead.locrectab_offset
			    + (newhead.locrectab_size
			       * sizeof (struct locrecent)));
  newhead.sumhash_size = MAX (next_prime (2 * newhead.sumhash_used),
			      newhead.sumhash_size);

  total = (newhead.sumhash_offset
	   + newhead.sumhash_size * sizeof (struct sumhashent));

  /* The new file is empty now.  */
  newhead.namehash_used = 0;
  newhead.string_used = 0;
  newhead.locrectab_used = 0;
  newhead.sumhash_used = 0;

  /* Write out the header and create room for the other data structures.  */
  if (TEMP_FAILURE_RETRY (write (fd, &newhead, sizeof (newhead)))
      != sizeof (newhead))
    {
      int errval = errno;
      unlink (fname);
      error (EXIT_FAILURE, errval, _("cannot initialize archive file"));
    }

  if (ftruncate64 (fd, total) != 0)
    {
      int errval = errno;
      unlink (fname);
      error (EXIT_FAILURE, errval, _("cannot resize archive file"));
    }

  /* Map the header and all the administration data structures.  */
  p = mmap64 (NULL, total, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (p == MAP_FAILED)
    {
      int errval = errno;
      unlink (fname);
      error (EXIT_FAILURE, errval, _("cannot map archive header"));
    }

  /* Lock the new file.  */
  if (lockf64 (fd, F_LOCK, total) != 0)
    {
      int errval = errno;
      unlink (fname);
      error (EXIT_FAILURE, errval, _("cannot lock new archive"));
    }

  new_ah.len = total;
  new_ah.addr = p;
  new_ah.fd = fd;

  /* Walk through the hash name hash table to find out what data is
     still referenced and transfer it into the new file.  */
  oldnamehashtab = (struct namehashent *) ((char *) ah->addr
					   + head->namehash_offset);
  oldlocrectab = (struct locrecent *) ((char *) ah->addr
				       + head->locrectab_offset);

  /* Sort the old locrec table in order of data position.  */
  oldlocrecarray = (struct oldlocrecent *)
		   alloca (head->namehash_size
			   * sizeof (struct oldlocrecent));
  for (cnt = 0, loccnt = 0; cnt < head->namehash_size; ++cnt)
    if (oldnamehashtab[cnt].locrec_offset != 0)
      {
	oldlocrecarray[loccnt].cnt = cnt;
	oldlocrecarray[loccnt++].locrec
	  = (struct locrecent *) ((char *) ah->addr
				  + oldnamehashtab[cnt].locrec_offset);
      }
  qsort (oldlocrecarray, loccnt, sizeof (struct oldlocrecent),
	 oldlocrecentcmp);

  for (cnt = 0; cnt < loccnt; ++cnt)
    {
      /* Insert this entry in the new hash table.  */
      locale_data_t old_data;
      unsigned int idx;
      struct locrecent *oldlocrec = oldlocrecarray[cnt].locrec;

      for (idx = 0; idx < __LC_LAST; ++idx)
	if (idx != LC_ALL)
	  {
	    old_data[idx].size = oldlocrec->record[idx].len;
	    old_data[idx].addr
	      = ((char *) ah->addr + oldlocrec->record[idx].offset);

	    __md5_buffer (old_data[idx].addr, old_data[idx].size,
			  old_data[idx].sum);
	  }

      if (add_locale (&new_ah,
	  ((char *) ah->addr
	   + oldnamehashtab[oldlocrecarray[cnt].cnt].name_offset),
	  old_data, 0) == 0)
	error (EXIT_FAILURE, 0, _("cannot extend locale archive file"));
    }

  /* Make the file globally readable.  */
  if (fchmod (fd, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH) == -1)
    {
      int errval = errno;
      unlink (fname);
      error (EXIT_FAILURE, errval,
	     _("cannot change mode of resized locale archive"));
    }

  /* Rename the new file.  */
  if (rename (fname, archivefname) != 0)
    {
      int errval = errno;
      unlink (fname);
      error (EXIT_FAILURE, errval, _("cannot rename new archive"));
    }

  /* Close the old file.  */
  close_archive (ah);

  /* Add the information for the new one.  */
  *ah = new_ah;
}


void
open_archive (struct locarhandle *ah, bool readonly)
{
  struct stat64 st;
  struct stat64 st2;
  int fd;
  struct locarhead head;
  int retry = 0;
  size_t prefix_len = output_prefix ? strlen (output_prefix) : 0;
  char archivefname[prefix_len + sizeof (ARCHIVE_NAME)];

  if (output_prefix)
    memcpy (archivefname, output_prefix, prefix_len);
  strcpy (archivefname + prefix_len, ARCHIVE_NAME);

  while (1)
    {
      /* Open the archive.  We must have exclusive write access.  */
      fd = open64 (archivefname, readonly ? O_RDONLY : O_RDWR);
      if (fd == -1)
	{
	  /* Maybe the file does not yet exist.  */
	  if (errno == ENOENT)
	    {
	      if (readonly)
		{
		  static const struct locarhead nullhead =
		    {
		      .namehash_used = 0,
		      .namehash_offset = 0,
		      .namehash_size = 0
		    };

		  ah->addr = (void *) &nullhead;
		  ah->fd = -1;
		}
	      else
		create_archive (archivefname, ah);

	      return;
	    }
	  else
	    error (EXIT_FAILURE, errno, _("cannot open locale archive \"%s\""),
		   archivefname);
	}

      if (fstat64 (fd, &st) < 0)
	error (EXIT_FAILURE, errno, _("cannot stat locale archive \"%s\""),
	       archivefname);

      if (!readonly && lockf64 (fd, F_LOCK, sizeof (struct locarhead)) == -1)
	{
	  close (fd);

	  if (retry++ < max_locarchive_open_retry)
	    {
	      struct timespec req;

	      /* Wait for a bit.  */
	      req.tv_sec = 0;
	      req.tv_nsec = 1000000 * (random () % 500 + 1);
	      (void) nanosleep (&req, NULL);

	      continue;
	    }

	  error (EXIT_FAILURE, errno, _("cannot lock locale archive \"%s\""),
		 archivefname);
	}

      /* One more check.  Maybe another process replaced the archive file
	 with a new, larger one since we opened the file.  */
      if (stat64 (archivefname, &st2) == -1
	  || st.st_dev != st2.st_dev
	  || st.st_ino != st2.st_ino)
	{
	  (void) lockf64 (fd, F_ULOCK, sizeof (struct locarhead));
	  close (fd);
	  continue;
	}

      /* Leave the loop.  */
      break;
    }

  /* Read the header.  */
  if (TEMP_FAILURE_RETRY (read (fd, &head, sizeof (head))) != sizeof (head))
    {
      (void) lockf64 (fd, F_ULOCK, sizeof (struct locarhead));
      error (EXIT_FAILURE, errno, _("cannot read archive header"));
    }

  ah->fd = fd;
  ah->len = (head.sumhash_offset
	     + head.sumhash_size * sizeof (struct sumhashent));

  /* Now we know how large the administrative information part is.
     Map all of it.  */
  ah->addr = mmap64 (NULL, ah->len, PROT_READ | (readonly ? 0 : PROT_WRITE),
		     MAP_SHARED, fd, 0);
  if (ah->addr == MAP_FAILED)
    {
      (void) lockf64 (fd, F_ULOCK, sizeof (struct locarhead));
      error (EXIT_FAILURE, errno, _("cannot map archive header"));
    }
}


void
close_archive (struct locarhandle *ah)
{
  if (ah->fd != -1)
    {
      munmap (ah->addr, ah->len);
      close (ah->fd);
    }
}

#include "../../intl/explodename.c"
#include "../../intl/l10nflist.c"

static struct namehashent *
insert_name (struct locarhandle *ah,
	     const char *name, size_t name_len, bool replace)
{
  const struct locarhead *const head = ah->addr;
  struct namehashent *namehashtab
    = (struct namehashent *) ((char *) ah->addr + head->namehash_offset);
  unsigned int insert_idx, idx, incr;

  /* Hash value of the locale name.  */
  uint32_t hval = archive_hashval (name, name_len);

  insert_idx = -1;
  idx = hval % head->namehash_size;
  incr = 1 + hval % (head->namehash_size - 2);

  /* If the name_offset field is zero this means this is a
     deleted entry and therefore no entry can be found.  */
  while (namehashtab[idx].name_offset != 0)
    {
      if (namehashtab[idx].hashval == hval
	  && strcmp (name,
		     (char *) ah->addr + namehashtab[idx].name_offset) == 0)
	{
	  /* Found the entry.  */
	  if (namehashtab[idx].locrec_offset != 0 && ! replace)
	    {
	      if (! be_quiet)
		error (0, 0, _("locale '%s' already exists"), name);
	      return NULL;
	    }

	  break;
	}

      if (namehashtab[idx].hashval == hval && ! be_quiet)
	{
	  error (0, 0, "hash collision (%u) %s, %s",
		 hval, name, (char *) ah->addr + namehashtab[idx].name_offset);
	}

      /* Remember the first place we can insert the new entry.  */
      if (namehashtab[idx].locrec_offset == 0 && insert_idx == -1)
	insert_idx = idx;

      idx += incr;
      if (idx >= head->namehash_size)
	idx -= head->namehash_size;
    }

  /* Add as early as possible.  */
  if (insert_idx != -1)
    idx = insert_idx;

  namehashtab[idx].hashval = hval; /* no-op if replacing an old entry.  */
  return &namehashtab[idx];
}

static void
add_alias (struct locarhandle *ah, const char *alias, bool replace,
	   const char *oldname, uint32_t *locrec_offset_p)
{
  uint32_t locrec_offset = *locrec_offset_p;
  struct locarhead *head = ah->addr;
  const size_t name_len = strlen (alias);
  struct namehashent *namehashent = insert_name (ah, alias, strlen (alias),
						 replace);
  if (namehashent == NULL && ! replace)
    return;

  if (namehashent->name_offset == 0)
    {
      /* We are adding a new hash entry for this alias.
	 Determine whether we have to resize the file.  */
      if (head->string_used + name_len + 1 > head->string_size
	  || 100 * head->namehash_used > 75 * head->namehash_size)
	{
	  /* The current archive is not large enough.  */
	  enlarge_archive (ah, head);

	  /* The locrecent might have moved, so we have to look up
	     the old name afresh.  */
	  namehashent = insert_name (ah, oldname, strlen (oldname), true);
	  assert (namehashent->name_offset != 0);
	  assert (namehashent->locrec_offset != 0);
	  *locrec_offset_p = namehashent->locrec_offset;

	  /* Tail call to try the whole thing again.  */
	  add_alias (ah, alias, replace, oldname, locrec_offset_p);
	  return;
	}

      /* Add the name string.  */
      memcpy (ah->addr + head->string_offset + head->string_used,
	      alias, name_len + 1);
      namehashent->name_offset = head->string_offset + head->string_used;
      head->string_used += name_len + 1;

      ++head->namehash_used;
    }

  if (namehashent->locrec_offset != 0)
    {
      /* Replacing an existing entry.
	 Mark that we are no longer using the old locrecent.  */
      struct locrecent *locrecent
	= (struct locrecent *) ((char *) ah->addr
				+ namehashent->locrec_offset);
      --locrecent->refs;
    }

  /* Point this entry at the locrecent installed for the main name.  */
  namehashent->locrec_offset = locrec_offset;
}

static int			/* qsort comparator used below */
cmpcategorysize (const void *a, const void *b)
{
  if (*(const void **) a == NULL)
    return 1;
  if (*(const void **) b == NULL)
    return -1;
  return ((*(const struct locale_category_data **) a)->size
	  - (*(const struct locale_category_data **) b)->size);
}

/* Check the content of the archive for duplicates.  Add the content
   of the files if necessary.  Returns the locrec_offset.  */
static uint32_t
add_locale (struct locarhandle *ah,
	    const char *name, locale_data_t data, bool replace)
{
  /* First look for the name.  If it already exists and we are not
     supposed to replace it don't do anything.  If it does not exist
     we have to allocate a new locale record.  */
  size_t name_len = strlen (name);
  uint32_t file_offsets[__LC_LAST];
  unsigned int num_new_offsets = 0;
  struct sumhashent *sumhashtab;
  uint32_t hval;
  unsigned int cnt, idx;
  struct locarhead *head;
  struct namehashent *namehashent;
  unsigned int incr;
  struct locrecent *locrecent;
  off64_t lastoffset;
  char *ptr;
  struct locale_category_data *size_order[__LC_LAST];
  const size_t pagesz = getpagesize ();
  int small_mask;

  head = ah->addr;
  sumhashtab = (struct sumhashent *) ((char *) ah->addr
				      + head->sumhash_offset);

  memset (file_offsets, 0, sizeof (file_offsets));

  size_order[LC_ALL] = NULL;
  for (cnt = 0; cnt < __LC_LAST; ++cnt)
    if (cnt != LC_ALL)
      size_order[cnt] = &data[cnt];

  /* Sort the array in ascending order of data size.  */
  qsort (size_order, __LC_LAST, sizeof size_order[0], cmpcategorysize);

  small_mask = 0;
  data[LC_ALL].size = 0;
  for (cnt = 0; cnt < __LC_LAST; ++cnt)
    if (size_order[cnt] != NULL)
      {
	const size_t rounded_size = (size_order[cnt]->size + 15) & -16;
	if (data[LC_ALL].size + rounded_size > 2 * pagesz)
	  {
	    /* This category makes the small-categories block
	       stop being small, so this is the end of the road.  */
	    do
	      size_order[cnt++] = NULL;
	    while (cnt < __LC_LAST);
	    break;
	  }
	data[LC_ALL].size += rounded_size;
	small_mask |= 1 << (size_order[cnt] - data);
      }

  /* Copy the data for all the small categories into the LC_ALL
     pseudo-category.  */

  data[LC_ALL].addr = alloca (data[LC_ALL].size);
  memset (data[LC_ALL].addr, 0, data[LC_ALL].size);

  ptr = data[LC_ALL].addr;
  for (cnt = 0; cnt < __LC_LAST; ++cnt)
    if (small_mask & (1 << cnt))
      {
	memcpy (ptr, data[cnt].addr, data[cnt].size);
	ptr += (data[cnt].size + 15) & -16;
      }
  __md5_buffer (data[LC_ALL].addr, data[LC_ALL].size, data[LC_ALL].sum);

  /* For each locale category data set determine whether the same data
     is already somewhere in the archive.  */
  for (cnt = 0; cnt < __LC_LAST; ++cnt)
    if (small_mask == 0 ? cnt != LC_ALL : !(small_mask & (1 << cnt)))
      {
	++num_new_offsets;

	/* Compute the hash value of the checksum to determine a
	   starting point for the search in the MD5 hash value
	   table.  */
	hval = archive_hashval (data[cnt].sum, 16);

	idx = hval % head->sumhash_size;
	incr = 1 + hval % (head->sumhash_size - 2);

	while (sumhashtab[idx].file_offset != 0)
	  {
	    if (memcmp (data[cnt].sum, sumhashtab[idx].sum, 16) == 0)
	      {
		/* Found it.  */
		file_offsets[cnt] = sumhashtab[idx].file_offset;
		--num_new_offsets;
		break;
	      }

	    idx += incr;
	    if (idx >= head->sumhash_size)
	      idx -= head->sumhash_size;
	  }
      }

  /* Find a slot for the locale name in the hash table.  */
  namehashent = insert_name (ah, name, name_len, replace);
  if (namehashent == NULL)	/* Already exists and !REPLACE.  */
    return 0;

  /* Determine whether we have to resize the file.  */
  if (100 * (head->sumhash_used + num_new_offsets) > 75 * head->sumhash_size
      || (namehashent->locrec_offset == 0
	  && (head->locrectab_used == head->locrectab_size
	      || head->string_used + name_len + 1 > head->string_size
	      || 100 * head->namehash_used > 75 * head->namehash_size)))
    {
      /* The current archive is not large enough.  */
      enlarge_archive (ah, head);
      return add_locale (ah, name, data, replace);
    }

  /* Add the locale data which is not yet in the archive.  */
  for (cnt = 0, lastoffset = 0; cnt < __LC_LAST; ++cnt)
    if ((small_mask == 0 ? cnt != LC_ALL : !(small_mask & (1 << cnt)))
	&& file_offsets[cnt] == 0)
      {
	/* The data for this section is not yet available in the
	   archive.  Append it.  */
	off64_t lastpos;
	uint32_t md5hval;

	lastpos = lseek64 (ah->fd, 0, SEEK_END);
	if (lastpos == (off64_t) -1)
	  error (EXIT_FAILURE, errno, _("cannot add to locale archive"));

	/* If block of small categories would cross page boundary,
	   align it unless it immediately follows a large category.  */
	if (cnt == LC_ALL && lastoffset != lastpos
	    && ((((lastpos & (pagesz - 1)) + data[cnt].size + pagesz - 1)
		 & -pagesz)
		> ((data[cnt].size + pagesz - 1) & -pagesz)))
	  {
	    size_t sz = pagesz - (lastpos & (pagesz - 1));
	    char *zeros = alloca (sz);

	    memset (zeros, 0, sz);
	    if (TEMP_FAILURE_RETRY (write (ah->fd, zeros, sz) != sz))
	      error (EXIT_FAILURE, errno,
		     _("cannot add to locale archive"));

	    lastpos += sz;
	  }

	/* Align all data to a 16 byte boundary.  */
	if ((lastpos & 15) != 0)
	  {
	    static const char zeros[15] = { 0, };

	    if (TEMP_FAILURE_RETRY (write (ah->fd, zeros, 16 - (lastpos & 15)))
		!= 16 - (lastpos & 15))
	      error (EXIT_FAILURE, errno, _("cannot add to locale archive"));

	    lastpos += 16 - (lastpos & 15);
	  }

	/* Remember the position.  */
	file_offsets[cnt] = lastpos;
	lastoffset = lastpos + data[cnt].size;

	/* Write the data.  */
	if (TEMP_FAILURE_RETRY (write (ah->fd, data[cnt].addr, data[cnt].size))
	    != data[cnt].size)
	  error (EXIT_FAILURE, errno, _("cannot add to locale archive"));

	/* Add the hash value to the hash table.  */
	md5hval = archive_hashval (data[cnt].sum, 16);

	idx = md5hval % head->sumhash_size;
	incr = 1 + md5hval % (head->sumhash_size - 2);

	while (sumhashtab[idx].file_offset != 0)
	  {
	    idx += incr;
	    if (idx >= head->sumhash_size)
	      idx -= head->sumhash_size;
	  }

	memcpy (sumhashtab[idx].sum, data[cnt].sum, 16);
	sumhashtab[idx].file_offset = file_offsets[cnt];

	++head->sumhash_used;
      }

  lastoffset = file_offsets[LC_ALL];
  for (cnt = 0; cnt < __LC_LAST; ++cnt)
    if (small_mask & (1 << cnt))
      {
	file_offsets[cnt] = lastoffset;
	lastoffset += (data[cnt].size + 15) & -16;
      }

  if (namehashent->name_offset == 0)
    {
      /* Add the name string.  */
      memcpy ((char *) ah->addr + head->string_offset + head->string_used,
	      name, name_len + 1);
      namehashent->name_offset = head->string_offset + head->string_used;
      head->string_used += name_len + 1;
      ++head->namehash_used;
    }

  if (namehashent->locrec_offset == 0)
    {
      /* Allocate a name location record.  */
      namehashent->locrec_offset = (head->locrectab_offset
				    + (head->locrectab_used++
				       * sizeof (struct locrecent)));
      locrecent = (struct locrecent *) ((char *) ah->addr
					+ namehashent->locrec_offset);
      locrecent->refs = 1;
    }
  else
    {
      /* If there are other aliases pointing to this locrecent,
	 we still need a new one.  If not, reuse the old one.  */

      locrecent = (struct locrecent *) ((char *) ah->addr
					+ namehashent->locrec_offset);
      if (locrecent->refs > 1)
	{
	  --locrecent->refs;
	  namehashent->locrec_offset = (head->locrectab_offset
					+ (head->locrectab_used++
					   * sizeof (struct locrecent)));
	  locrecent = (struct locrecent *) ((char *) ah->addr
					    + namehashent->locrec_offset);
	  locrecent->refs = 1;
	}
    }

  /* Fill in the table with the locations of the locale data.  */
  for (cnt = 0; cnt < __LC_LAST; ++cnt)
    {
      locrecent->record[cnt].offset = file_offsets[cnt];
      locrecent->record[cnt].len = data[cnt].size;
    }

  return namehashent->locrec_offset;
}


/* Check the content of the archive for duplicates.  Add the content
   of the files if necessary.  Add all the names, possibly overwriting
   old files.  */
int
add_locale_to_archive (ah, name, data, replace)
     struct locarhandle *ah;
     const char *name;
     locale_data_t data;
     bool replace;
{
  char *normalized_name = NULL;
  uint32_t locrec_offset;

  /* First analyze the name to decide how to archive it.  */
  const char *language;
  const char *modifier;
  const char *territory;
  const char *codeset;
  const char *normalized_codeset;
  int mask = _nl_explode_name (strdupa (name),
			       &language, &modifier, &territory,
			       &codeset, &normalized_codeset);

  if (mask & XPG_NORM_CODESET)
    /* This name contains a codeset in unnormalized form.
       We will store it in the archive with a normalized name.  */
    asprintf (&normalized_name, "%s%s%s.%s%s%s",
	      language, territory == NULL ? "" : "_", territory ?: "",
	      (mask & XPG_NORM_CODESET) ? normalized_codeset : codeset,
	      modifier == NULL ? "" : "@", modifier ?: "");

  /* This call does the main work.  */
  locrec_offset = add_locale (ah, normalized_name ?: name, data, replace);
  if (locrec_offset == 0)
    {
      free (normalized_name);
      if (mask & XPG_NORM_CODESET)
	free ((char *) normalized_codeset);
      return -1;
    }

  if ((mask & XPG_CODESET) == 0)
    {
      /* This name lacks a codeset, so determine the locale's codeset and
	 add an alias for its name with normalized codeset appended.  */

      const struct
      {
	unsigned int magic;
	unsigned int nstrings;
	unsigned int strindex[0];
      } *filedata = data[LC_CTYPE].addr;
      codeset = (char *) filedata
	+ filedata->strindex[_NL_ITEM_INDEX (_NL_CTYPE_CODESET_NAME)];
      char *normalized_codeset_name = NULL;

      normalized_codeset = _nl_normalize_codeset (codeset, strlen (codeset));
      mask |= XPG_NORM_CODESET;

      asprintf (&normalized_codeset_name, "%s%s%s.%s%s%s",
		language, territory == NULL ? "" : "_", territory ?: "",
		normalized_codeset,
		modifier == NULL ? "" : "@", modifier ?: "");

      add_alias (ah, normalized_codeset_name, replace,
		 normalized_name ?: name, &locrec_offset);
      free (normalized_codeset_name);
    }

  /* Now read the locale.alias files looking for lines whose
     right hand side matches our name after normalization.  */
  if (alias_file != NULL)
    {
      FILE *fp;
      fp = fopen (alias_file, "rm");
      if (fp == NULL)
	error (1, errno, _("locale alias file `%s' not found"),
	       alias_file);

      /* No threads present.  */
      __fsetlocking (fp, FSETLOCKING_BYCALLER);

      while (! feof_unlocked (fp))
	{
	  /* It is a reasonable approach to use a fix buffer here
	     because
	     a) we are only interested in the first two fields
	     b) these fields must be usable as file names and so must
	     not be that long  */
	  char buf[BUFSIZ];
	  char *alias;
	  char *value;
	  char *cp;

	  if (fgets_unlocked (buf, BUFSIZ, fp) == NULL)
	    /* EOF reached.  */
	    break;

	  cp = buf;
	  /* Ignore leading white space.  */
	  while (isspace (cp[0]) && cp[0] != '\n')
	    ++cp;

	  /* A leading '#' signals a comment line.  */
	  if (cp[0] != '\0' && cp[0] != '#' && cp[0] != '\n')
	    {
	      alias = cp++;
	      while (cp[0] != '\0' && !isspace (cp[0]))
		++cp;
	      /* Terminate alias name.  */
	      if (cp[0] != '\0')
		*cp++ = '\0';

	      /* Now look for the beginning of the value.  */
	      while (isspace (cp[0]))
		++cp;

	      if (cp[0] != '\0')
		{
		  value = cp++;
		  while (cp[0] != '\0' && !isspace (cp[0]))
		    ++cp;
		  /* Terminate value.  */
		  if (cp[0] == '\n')
		    {
		      /* This has to be done to make the following
			 test for the end of line possible.  We are
			 looking for the terminating '\n' which do not
			 overwrite here.  */
		      *cp++ = '\0';
		      *cp = '\n';
		    }
		  else if (cp[0] != '\0')
		    *cp++ = '\0';

		  /* Does this alias refer to our locale?  We will
		     normalize the right hand side and compare the
		     elements of the normalized form.  */
		  {
		    const char *rhs_language;
		    const char *rhs_modifier;
		    const char *rhs_territory;
		    const char *rhs_codeset;
		    const char *rhs_normalized_codeset;
		    int rhs_mask = _nl_explode_name (value,
						     &rhs_language,
						     &rhs_modifier,
						     &rhs_territory,
						     &rhs_codeset,
						     &rhs_normalized_codeset);
		    if (!strcmp (language, rhs_language)
			&& ((rhs_mask & XPG_CODESET)
			    /* He has a codeset, it must match normalized.  */
			    ? !strcmp ((mask & XPG_NORM_CODESET)
				       ? normalized_codeset : codeset,
				       (rhs_mask & XPG_NORM_CODESET)
				       ? rhs_normalized_codeset : rhs_codeset)
			    /* He has no codeset, we must also have none.  */
			    : (mask & XPG_CODESET) == 0)
			/* Codeset (or lack thereof) matches.  */
			&& !strcmp (territory ?: "", rhs_territory ?: "")
			&& !strcmp (modifier ?: "", rhs_modifier ?: ""))
		      /* We have a winner.  */
		      add_alias (ah, alias, replace,
				 normalized_name ?: name, &locrec_offset);
		    if (rhs_mask & XPG_NORM_CODESET)
		      free ((char *) rhs_normalized_codeset);
		  }
		}
	    }

	  /* Possibly not the whole line fits into the buffer.
	     Ignore the rest of the line.  */
	  while (strchr (cp, '\n') == NULL)
	    {
	      cp = buf;
	      if (fgets_unlocked (buf, BUFSIZ, fp) == NULL)
		/* Make sure the inner loop will be left.  The outer
		   loop will exit at the `feof' test.  */
		*cp = '\n';
	    }
	}

      fclose (fp);
    }

  free (normalized_name);

  if (mask & XPG_NORM_CODESET)
    free ((char *) normalized_codeset);

  return 0;
}


int
add_locales_to_archive (nlist, list, replace)
     size_t nlist;
     char *list[];
     bool replace;
{
  struct locarhandle ah;
  int result = 0;

  /* Open the archive.  This call never returns if we cannot
     successfully open the archive.  */
  open_archive (&ah, false);

  while (nlist-- > 0)
    {
      const char *fname = *list++;
      size_t fnamelen = strlen (fname);
      struct stat64 st;
      DIR *dirp;
      struct dirent64 *d;
      int seen;
      locale_data_t data;
      int cnt;

      if (! be_quiet)
	printf (_("Adding %s\n"), fname);

      /* First see whether this really is a directory and whether it
	 contains all the require locale category files.  */
      if (stat64 (fname, &st) < 0)
	{
	  error (0, 0, _("stat of \"%s\" failed: %s: ignored"), fname,
		 strerror (errno));
	  continue;
	}
      if (!S_ISDIR (st.st_mode))
	{
	  error (0, 0, _("\"%s\" is no directory; ignored"), fname);
	  continue;
	}

      dirp = opendir (fname);
      if (dirp == NULL)
	{
	  error (0, 0, _("cannot open directory \"%s\": %s: ignored"),
		 fname, strerror (errno));
	  continue;
	}

      seen = 0;
      while ((d = readdir64 (dirp)) != NULL)
	{
	  for (cnt = 0; cnt < __LC_LAST; ++cnt)
	    if (cnt != LC_ALL)
	      if (strcmp (d->d_name, locnames[cnt]) == 0)
		{
		  unsigned char d_type;

		  /* We have an object of the required name.  If it's
		     a directory we have to look at a file with the
		     prefix "SYS_".  Otherwise we have found what we
		     are looking for.  */
#ifdef _DIRENT_HAVE_D_TYPE
		  d_type = d->d_type;

		  if (d_type != DT_REG)
#endif
		    {
		      char fullname[fnamelen + 2 * strlen (d->d_name) + 7];

#ifdef _DIRENT_HAVE_D_TYPE
		      if (d_type == DT_UNKNOWN)
#endif
			{
			  strcpy (stpcpy (stpcpy (fullname, fname), "/"),
				  d->d_name);

			  if (stat64 (fullname, &st) == -1)
			    /* We cannot stat the file, ignore it.  */
			    break;

			  d_type = IFTODT (st.st_mode);
			}

		      if (d_type == DT_DIR)
			{
			  /* We have to do more tests.  The file is a
			     directory and it therefore must contain a
			     regular file with the same name except a
			     "SYS_" prefix.  */
			  char *t = stpcpy (stpcpy (fullname, fname), "/");
			  strcpy (stpcpy (stpcpy (t, d->d_name), "/SYS_"),
				  d->d_name);

			  if (stat64 (fullname, &st) == -1)
			    /* There is no SYS_* file or we cannot
			       access it.  */
			    break;

			  d_type = IFTODT (st.st_mode);
			}
		    }

		  /* If we found a regular file (eventually after
		     following a symlink) we are successful.  */
		  if (d_type == DT_REG)
		    ++seen;
		  break;
		}
	}

      closedir (dirp);

      if (seen != __LC_LAST - 1)
	{
	  /* We don't have all locale category files.  Ignore the name.  */
	  error (0, 0, _("incomplete set of locale files in \"%s\""),
		 fname);
	  continue;
	}

      /* Add the files to the archive.  To do this we first compute
	 sizes and the MD5 sums of all the files.  */
      for (cnt = 0; cnt < __LC_LAST; ++cnt)
	if (cnt != LC_ALL)
	  {
	    char fullname[fnamelen + 2 * strlen (locnames[cnt]) + 7];
	    int fd;

	    strcpy (stpcpy (stpcpy (fullname, fname), "/"), locnames[cnt]);
	    fd = open64 (fullname, O_RDONLY);
	    if (fd == -1 || fstat64 (fd, &st) == -1)
	      {
		/* Cannot read the file.  */
		if (fd != -1)
		  close (fd);
		break;
	      }

	    if (S_ISDIR (st.st_mode))
	      {
		char *t;
		close (fd);
		t = stpcpy (stpcpy (fullname, fname), "/");
		strcpy (stpcpy (stpcpy (t, locnames[cnt]), "/SYS_"),
			locnames[cnt]);

		fd = open64 (fullname, O_RDONLY);
		if (fd == -1 || fstat64 (fd, &st) == -1
		    || !S_ISREG (st.st_mode))
		  {
		    if (fd != -1)
		      close (fd);
		    break;
		  }
	      }

	    /* Map the file.  */
	    data[cnt].addr = mmap64 (NULL, st.st_size, PROT_READ, MAP_SHARED,
				     fd, 0);
	    if (data[cnt].addr == MAP_FAILED)
	      {
		/* Cannot map it.  */
		close (fd);
		break;
	      }

	    data[cnt].size = st.st_size;
	    __md5_buffer (data[cnt].addr, st.st_size, data[cnt].sum);

	    /* We don't need the file descriptor anymore.  */
	    close (fd);
	  }

      if (cnt != __LC_LAST)
	{
	  while (cnt-- > 0)
	    if (cnt != LC_ALL)
	      munmap (data[cnt].addr, data[cnt].size);

	  error (0, 0, _("cannot read all files in \"%s\": ignored"), fname);

	  continue;
	}

      result |= add_locale_to_archive (&ah, basename (fname), data, replace);

      for (cnt = 0; cnt < __LC_LAST; ++cnt)
	if (cnt != LC_ALL)
	  munmap (data[cnt].addr, data[cnt].size);
    }

  /* We are done.  */
  close_archive (&ah);

  return result;
}


int
delete_locales_from_archive (nlist, list)
     size_t nlist;
     char *list[];
{
  struct locarhandle ah;
  struct locarhead *head;
  struct namehashent *namehashtab;

  /* Open the archive.  This call never returns if we cannot
     successfully open the archive.  */
  open_archive (&ah, false);

  head = ah.addr;
  namehashtab = (struct namehashent *) ((char *) ah.addr
					+ head->namehash_offset);

  while (nlist-- > 0)
    {
      const char *locname = *list++;
      uint32_t hval;
      unsigned int idx;
      unsigned int incr;

      /* Search for this locale in the archive.  */
      hval = archive_hashval (locname, strlen (locname));

      idx = hval % head->namehash_size;
      incr = 1 + hval % (head->namehash_size - 2);

      /* If the name_offset field is zero this means this is no
	 deleted entry and therefore no entry can be found.  */
      while (namehashtab[idx].name_offset != 0)
	{
	  if (namehashtab[idx].hashval == hval
	      && (strcmp (locname,
			  (char *) ah.addr + namehashtab[idx].name_offset)
		  == 0))
	    {
	      /* Found the entry.  Now mark it as removed by zero-ing
		 the reference to the locale record.  */
	      namehashtab[idx].locrec_offset = 0;
	      break;
	    }

	  idx += incr;
	  if (idx >= head->namehash_size)
	    idx -= head->namehash_size;
	}

      if (namehashtab[idx].name_offset == 0 && ! be_quiet)
	error (0, 0, _("locale \"%s\" not in archive"), locname);
    }

  close_archive (&ah);

  return 0;
}


struct nameent
{
  char *name;
  uint32_t locrec_offset;
};


struct dataent
{
  const unsigned char *sum;
  uint32_t file_offset;
  uint32_t nlink;
};


static int
nameentcmp (const void *a, const void *b)
{
  return strcmp (((const struct nameent *) a)->name,
		 ((const struct nameent *) b)->name);
}


static int
dataentcmp (const void *a, const void *b)
{
  if (((const struct dataent *) a)->file_offset
      < ((const struct dataent *) b)->file_offset)
    return -1;

  if (((const struct dataent *) a)->file_offset
      > ((const struct dataent *) b)->file_offset)
    return 1;

  return 0;
}


void
show_archive_content (int verbose)
{
  struct locarhandle ah;
  struct locarhead *head;
  struct namehashent *namehashtab;
  struct nameent *names;
  size_t cnt, used;

  /* Open the archive.  This call never returns if we cannot
     successfully open the archive.  */
  open_archive (&ah, true);

  head = ah.addr;

  names = (struct nameent *) xmalloc (head->namehash_used
				      * sizeof (struct nameent));

  namehashtab = (struct namehashent *) ((char *) ah.addr
					+ head->namehash_offset);
  for (cnt = used = 0; cnt < head->namehash_size; ++cnt)
    if (namehashtab[cnt].locrec_offset != 0)
      {
	assert (used < head->namehash_used);
	names[used].name = ah.addr + namehashtab[cnt].name_offset;
	names[used++].locrec_offset = namehashtab[cnt].locrec_offset;
      }

  /* Sort the names.  */
  qsort (names, used, sizeof (struct nameent), nameentcmp);

  if (verbose)
    {
      struct dataent *files;
      struct sumhashent *sumhashtab;
      int sumused;

      files = (struct dataent *) xmalloc (head->sumhash_used
					  * sizeof (struct sumhashent));

      sumhashtab = (struct sumhashent *) ((char *) ah.addr
					  + head->sumhash_offset);
      for (cnt = sumused = 0; cnt < head->sumhash_size; ++cnt)
	if (sumhashtab[cnt].file_offset != 0)
	  {
	    assert (sumused < head->sumhash_used);
	    files[sumused].sum = (const unsigned char *) sumhashtab[cnt].sum;
	    files[sumused].file_offset = sumhashtab[cnt].file_offset;
	    files[sumused++].nlink = 0;
	  }

      /* Sort by file locations.  */
      qsort (files, sumused, sizeof (struct dataent), dataentcmp);

      /* Compute nlink fields.  */
      for (cnt = 0; cnt < used; ++cnt)
	{
	  struct locrecent *locrec;
	  int idx;

	  locrec = (struct locrecent *) ((char *) ah.addr
					 + names[cnt].locrec_offset);
	  for (idx = 0; idx < __LC_LAST; ++idx)
	    if (locrec->record[LC_ALL].offset != 0
		? (idx == LC_ALL
		   || (locrec->record[idx].offset
		       < locrec->record[LC_ALL].offset)
		   || (locrec->record[idx].offset + locrec->record[idx].len
		       > (locrec->record[LC_ALL].offset
			  + locrec->record[LC_ALL].len)))
		: idx != LC_ALL)
	      {
		struct dataent *data, dataent;

		dataent.file_offset = locrec->record[idx].offset;
		data = (struct dataent *) bsearch (&dataent, files, sumused,
						   sizeof (struct dataent),
						   dataentcmp);
		assert (data != NULL);
		++data->nlink;
	      }
	}

      /* Print it.  */
      for (cnt = 0; cnt < used; ++cnt)
	{
	  struct locrecent *locrec;
	  int idx, i;

	  locrec = (struct locrecent *) ((char *) ah.addr
					 + names[cnt].locrec_offset);
	  for (idx = 0; idx < __LC_LAST; ++idx)
	    if (idx != LC_ALL)
	      {
		struct dataent *data, dataent;

		dataent.file_offset = locrec->record[idx].offset;
		if (locrec->record[LC_ALL].offset != 0
		    && dataent.file_offset >= locrec->record[LC_ALL].offset
		    && (dataent.file_offset + locrec->record[idx].len
			<= (locrec->record[LC_ALL].offset
			    + locrec->record[LC_ALL].len)))
		  dataent.file_offset = locrec->record[LC_ALL].offset;

		data = (struct dataent *) bsearch (&dataent, files, sumused,
						   sizeof (struct dataent),
						   dataentcmp);
		printf ("%6d %7x %3d%c ",
			locrec->record[idx].len, locrec->record[idx].offset,
			data->nlink,
			dataent.file_offset == locrec->record[LC_ALL].offset
			? '+' : ' ');
		for (i = 0; i < 16; i += 4)
		    printf ("%02x%02x%02x%02x",
			    data->sum[i], data->sum[i + 1],
			    data->sum[i + 2], data->sum[i + 3]);
		printf (" %s/%s\n", names[cnt].name,
			idx == LC_MESSAGES ? "LC_MESSAGES/SYS_LC_MESSAGES"
			: locnames[idx]);
	      }
	}
    }
  else
    for (cnt = 0; cnt < used; ++cnt)
      puts (names[cnt].name);

  close_archive (&ah);

  exit (EXIT_SUCCESS);
}
