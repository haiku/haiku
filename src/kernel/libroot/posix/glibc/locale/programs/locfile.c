/* Copyright (C) 1996,1997,1998,1999,2000,2001 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@gnu.org>, 1996.

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

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/stat.h>

#include "localedef.h"
#include "locfile.h"

#include "locfile-kw.h"


int
locfile_read (struct localedef_t *result, struct charmap_t *charmap)
{
  const char *filename = result->name;
  const char *repertoire_name = result->repertoire_name;
  int locale_mask = result->needed ^ result->avail;
  struct linereader *ldfile;
  int not_here = ALL_LOCALES;

  /* If no repertoire name was specified use the global one.  */
  if (repertoire_name == NULL)
    repertoire_name = repertoire_global;

  /* Open the locale definition file.  */
  ldfile = lr_open (filename, locfile_hash);
  if (ldfile == NULL)
    {
      if (filename != NULL && filename[0] != '/')
	{
	  char *i18npath = getenv ("I18NPATH");
	  if (i18npath != NULL && *i18npath != '\0')
	    {
	      char path[strlen (filename) + 1 + strlen (i18npath)
		        + sizeof ("/locales/") - 1];
	      char *next;
	      i18npath = strdupa (i18npath);


	      while (ldfile == NULL
		     && (next = strsep (&i18npath, ":")) != NULL)
		{
		  stpcpy (stpcpy (stpcpy (path, next), "/locales/"), filename);

		  ldfile = lr_open (path, locfile_hash);

		  if (ldfile == NULL)
		    {
		      stpcpy (stpcpy (path, next), filename);

		      ldfile = lr_open (path, locfile_hash);
		    }
		}
	    }

	  /* Test in the default directory.  */
	  if (ldfile == NULL)
	    {
	      char path[strlen (filename) + 1 + sizeof (LOCSRCDIR)];

	      stpcpy (stpcpy (stpcpy (path, LOCSRCDIR), "/"), filename);
	      ldfile = lr_open (path, locfile_hash);
	    }
	}

      if (ldfile == NULL)
	return 1;
    }

    /* Parse locale definition file and store result in RESULT.  */
  while (1)
    {
      struct token *now = lr_token (ldfile, charmap, NULL, verbose);
      enum token_t nowtok = now->tok;
      struct token *arg;

      if (nowtok == tok_eof)
	break;

      if (nowtok == tok_eol)
	/* Ignore empty lines.  */
	continue;

      switch (nowtok)
	{
	case tok_escape_char:
	case tok_comment_char:
	  /* We need an argument.  */
	  arg = lr_token (ldfile, charmap, NULL, verbose);

	  if (arg->tok != tok_ident)
	    {
	      SYNTAX_ERROR (_("bad argument"));
	      continue;
	    }

	  if (arg->val.str.lenmb != 1)
	    {
	      lr_error (ldfile, _("\
argument to `%s' must be a single character"),
			nowtok == tok_escape_char
			? "escape_char" : "comment_char");

	      lr_ignore_rest (ldfile, 0);
	      continue;
	    }

	  if (nowtok == tok_escape_char)
	    ldfile->escape_char = *arg->val.str.startmb;
	  else
	    ldfile->comment_char = *arg->val.str.startmb;
	  break;

	case tok_repertoiremap:
	  /* We need an argument.  */
	  arg = lr_token (ldfile, charmap, NULL, verbose);

	  if (arg->tok != tok_ident)
	    {
	      SYNTAX_ERROR (_("bad argument"));
	      continue;
	    }

	  if (repertoire_name == NULL)
	    {
	      repertoire_name = memcpy (xmalloc (arg->val.str.lenmb + 1),
					arg->val.str.startmb,
					arg->val.str.lenmb);
	      ((char *) repertoire_name)[arg->val.str.lenmb] = '\0';
	    }
	  break;

	case tok_lc_ctype:
	  ctype_read (ldfile, result, charmap, repertoire_name,
		      (locale_mask & CTYPE_LOCALE) == 0);
	  result->avail |= locale_mask & CTYPE_LOCALE;
	  not_here ^= CTYPE_LOCALE;
	  continue;

	case tok_lc_collate:
	  collate_read (ldfile, result, charmap, repertoire_name,
			(locale_mask & COLLATE_LOCALE) == 0);
	  result->avail |= locale_mask & COLLATE_LOCALE;
	  not_here ^= COLLATE_LOCALE;
	  continue;

	case tok_lc_monetary:
	  monetary_read (ldfile, result, charmap, repertoire_name,
			 (locale_mask & MONETARY_LOCALE) == 0);
	  result->avail |= locale_mask & MONETARY_LOCALE;
	  not_here ^= MONETARY_LOCALE;
	  continue;

	case tok_lc_numeric:
	  numeric_read (ldfile, result, charmap, repertoire_name,
			(locale_mask & NUMERIC_LOCALE) == 0);
	  result->avail |= locale_mask & NUMERIC_LOCALE;
	  not_here ^= NUMERIC_LOCALE;
	  continue;

	case tok_lc_time:
	  time_read (ldfile, result, charmap, repertoire_name,
		     (locale_mask & TIME_LOCALE) == 0);
	  result->avail |= locale_mask & TIME_LOCALE;
	  not_here ^= TIME_LOCALE;
	  continue;

	case tok_lc_messages:
	  messages_read (ldfile, result, charmap, repertoire_name,
			 (locale_mask & MESSAGES_LOCALE) == 0);
	  result->avail |= locale_mask & MESSAGES_LOCALE;
	  not_here ^= MESSAGES_LOCALE;
	  continue;

	case tok_lc_paper:
	  paper_read (ldfile, result, charmap, repertoire_name,
		      (locale_mask & PAPER_LOCALE) == 0);
	  result->avail |= locale_mask & PAPER_LOCALE;
	  not_here ^= PAPER_LOCALE;
	  continue;

	case tok_lc_name:
	  name_read (ldfile, result, charmap, repertoire_name,
		     (locale_mask & NAME_LOCALE) == 0);
	  result->avail |= locale_mask & NAME_LOCALE;
	  not_here ^= NAME_LOCALE;
	  continue;

	case tok_lc_address:
	  address_read (ldfile, result, charmap, repertoire_name,
			(locale_mask & ADDRESS_LOCALE) == 0);
	  result->avail |= locale_mask & ADDRESS_LOCALE;
	  not_here ^= ADDRESS_LOCALE;
	  continue;

	case tok_lc_telephone:
	  telephone_read (ldfile, result, charmap, repertoire_name,
			  (locale_mask & TELEPHONE_LOCALE) == 0);
	  result->avail |= locale_mask & TELEPHONE_LOCALE;
	  not_here ^= TELEPHONE_LOCALE;
	  continue;

	case tok_lc_measurement:
	  measurement_read (ldfile, result, charmap, repertoire_name,
			    (locale_mask & MEASUREMENT_LOCALE) == 0);
	  result->avail |= locale_mask & MEASUREMENT_LOCALE;
	  not_here ^= MEASUREMENT_LOCALE;
	  continue;

	case tok_lc_identification:
	  identification_read (ldfile, result, charmap, repertoire_name,
			       (locale_mask & IDENTIFICATION_LOCALE) == 0);
	  result->avail |= locale_mask & IDENTIFICATION_LOCALE;
	  not_here ^= IDENTIFICATION_LOCALE;
	  continue;

	default:
	  SYNTAX_ERROR (_("\
syntax error: not inside a locale definition section"));
	  continue;
	}

      /* The rest of the line must be empty.  */
      lr_ignore_rest (ldfile, 1);
    }

  /* We read all of the file.  */
  lr_close (ldfile);

  /* Mark the categories which are not contained in the file.  We assume
     them to be available and the default data will be used.  */
  result->avail |= not_here;

  return 0;
}


/* Semantic checking of locale specifications.  */

static void (*const check_funcs[]) (struct localedef_t *,
				    struct charmap_t *) =
{
  [LC_CTYPE] = ctype_finish,
  [LC_COLLATE] = collate_finish,
  [LC_MESSAGES] = messages_finish,
  [LC_MONETARY] = monetary_finish,
  [LC_NUMERIC] = numeric_finish,
  [LC_TIME] = time_finish,
  [LC_PAPER] = paper_finish,
  [LC_NAME] = name_finish,
  [LC_ADDRESS] = address_finish,
  [LC_TELEPHONE] = telephone_finish,
  [LC_MEASUREMENT] = measurement_finish,
  [LC_IDENTIFICATION] = identification_finish
};

void
check_all_categories (struct localedef_t *definitions,
		      struct charmap_t *charmap)
{
  int cnt;

  for (cnt = 0; cnt < sizeof (check_funcs) / sizeof (check_funcs[0]); ++cnt)
    if (check_funcs[cnt] != NULL)
      check_funcs[cnt] (definitions, charmap);
}


/* Writing the locale data files.  All files use the same output_path.  */

static void (*const write_funcs[]) (struct localedef_t *, struct charmap_t *,
				    const char *) =
{
  [LC_CTYPE] = ctype_output,
  [LC_COLLATE] = collate_output,
  [LC_MESSAGES] = messages_output,
  [LC_MONETARY] = monetary_output,
  [LC_NUMERIC] = numeric_output,
  [LC_TIME] = time_output,
  [LC_PAPER] = paper_output,
  [LC_NAME] = name_output,
  [LC_ADDRESS] = address_output,
  [LC_TELEPHONE] = telephone_output,
  [LC_MEASUREMENT] = measurement_output,
  [LC_IDENTIFICATION] = identification_output
};

void
write_all_categories (struct localedef_t *definitions,
		      struct charmap_t *charmap,
		      const char *output_path)
{
  int cnt;

  for (cnt = 0; cnt < sizeof (write_funcs) / sizeof (write_funcs[0]); ++cnt)
    if (write_funcs[cnt] != NULL)
      write_funcs[cnt] (definitions, charmap, output_path);
}

/* Return a NULL terminated list of the directories next to output_path
   that have the same owner, group, permissions and device as output_path.  */
static const char **
siblings_uncached (const char *output_path)
{
  size_t len;
  char *base, *p;
  struct stat output_stat;
  DIR *dirp;
  int nelems;
  const char **elems;

  /* Remove trailing slashes and trailing pathname component.  */
  len = strlen (output_path);
  base = (char *) alloca (len);
  memcpy (base, output_path, len);
  p = base + len;
  while (p > base && p[-1] == '/')
    p--;
  if (p == base)
    return NULL;
  do
    p--;
  while (p > base && p[-1] != '/');
  if (p == base)
    return NULL;
  *--p = '\0';
  len = p - base;

  /* Get the properties of output_path.  */
  if (lstat (output_path, &output_stat) < 0 || !S_ISDIR (output_stat.st_mode))
    return NULL;

  /* Iterate through the directories in base directory.  */
  dirp = opendir (base);
  if (dirp == NULL)
    return NULL;
  nelems = 0;
  elems = NULL;
  for (;;)
    {
      struct dirent *other_dentry;
      const char *other_name;
      char *other_path;
      struct stat other_stat;

      other_dentry = readdir (dirp);
      if (other_dentry == NULL)
	break;

      other_name = other_dentry->d_name;
      if (strcmp (other_name, ".") == 0 || strcmp (other_name, "..") == 0)
	continue;

      other_path = (char *) xmalloc (len + 1 + strlen (other_name) + 2);
      memcpy (other_path, base, len);
      other_path[len] = '/';
      strcpy (other_path + len + 1, other_name);

      if (lstat (other_path, &other_stat) >= 0
	  && S_ISDIR (other_stat.st_mode)
	  && other_stat.st_uid == output_stat.st_uid
	  && other_stat.st_gid == output_stat.st_gid
	  && other_stat.st_mode == output_stat.st_mode
	  && other_stat.st_dev == output_stat.st_dev)
	{
	  /* Found a subdirectory.  Add a trailing slash and store it.  */
	  p = other_path + len + 1 + strlen (other_name);
	  *p++ = '/';
	  *p = '\0';
	  elems = (const char **) xrealloc ((char *) elems,
					    (nelems + 2) * sizeof (char **));
	  elems[nelems++] = other_path;
	}
      else
	free (other_path);
    }
  closedir (dirp);

  if (elems != NULL)
    elems[nelems] = NULL;
  return elems;
}

/* Return a NULL terminated list of the directories next to output_path
   that have the same owner, group, permissions and device as output_path.
   Cache the result for future calls.  */
static const char **
siblings (const char *output_path)
{
  static const char *last_output_path;
  static const char **last_result;

  if (output_path != last_output_path)
    {
      if (last_result != NULL)
	{
	  const char **p;

	  for (p = last_result; *p != NULL; p++)
	    free ((char *) *p);
	  free (last_result);
	}

      last_output_path = output_path;
      last_result = siblings_uncached (output_path);
    }
  return last_result;
}

/* Read as many bytes from a file descriptor as possible.  */
static ssize_t
full_read (int fd, void *bufarea, size_t nbyte)
{
  char *buf = (char *) bufarea;

  while (nbyte > 0)
    {
      ssize_t retval = read (fd, buf, nbyte);

      if (retval == 0)
	break;
      else if (retval > 0)
	{
	  buf += retval;
	  nbyte -= retval;
	}
      else if (errno != EINTR)
	return retval;
    }
  return buf - (char *) bufarea;
}

/* Compare the contents of two regular files of the same size.  Return 0
   if they are equal, 1 if they are different, or -1 if an error occurs.  */
static int
compare_files (const char *filename1, const char *filename2, size_t size,
	       size_t blocksize)
{
  int fd1, fd2;
  int ret = -1;

  fd1 = open (filename1, O_RDONLY);
  if (fd1 >= 0)
    {
      fd2 = open (filename2, O_RDONLY);
      if (fd2 >= 0)
	{
	  char *buf1 = (char *) xmalloc (2 * blocksize);
	  char *buf2 = buf1 + blocksize;

	  ret = 0;
	  while (size > 0)
	    {
	      size_t bytes = (size < blocksize ? size : blocksize);

	      if (full_read (fd1, buf1, bytes) < (ssize_t) bytes)
		{
		  ret = -1;
		  break;
		}
	      if (full_read (fd2, buf2, bytes) < (ssize_t) bytes)
		{
		  ret = -1;
		  break;
		}
	      if (memcmp (buf1, buf2, bytes) != 0)
		{
		  ret = 1;
		  break;
		}
	      size -= bytes;
	    }

	  free (buf1);
	  close (fd2);
	}
      close (fd1);
    }
  return ret;
}

/* Write a locale file, with contents given by N_ELEM and VEC.  */
void
write_locale_data (const char *output_path, const char *category,
		   size_t n_elem, struct iovec *vec)
{
  size_t cnt, step, maxiov;
  int fd;
  char *fname;
  const char **other_paths;

  fname = xmalloc (strlen (output_path) + 2 * strlen (category) + 7);

  /* Normally we write to the directory pointed to by the OUTPUT_PATH.
     But for LC_MESSAGES we have to take care for the translation
     data.  This means we need to have a directory LC_MESSAGES in
     which we place the file under the name SYS_LC_MESSAGES.  */
  sprintf (fname, "%s%s", output_path, category);
  fd = -2;
  if (strcmp (category, "LC_MESSAGES") == 0)
    {
      struct stat st;

      if (stat (fname, &st) < 0)
	{
	  if (mkdir (fname, 0777) >= 0)
	    {
	      fd = -1;
	      errno = EISDIR;
	    }
	}
      else if (!S_ISREG (st.st_mode))
	{
	  fd = -1;
	  errno = EISDIR;
	}
    }

  /* Create the locale file with nlinks == 1; this avoids crashing processes
     which currently use the locale and damaging files belonging to other
     locales as well.  */
  if (fd == -2)
    {
      unlink (fname);
      fd = creat (fname, 0666);
    }

  if (fd == -1)
    {
      int save_err = errno;

      if (errno == EISDIR)
	{
	  sprintf (fname, "%1$s%2$s/SYS_%2$s", output_path, category);
	  unlink (fname);
	  fd = creat (fname, 0666);
	  if (fd == -1)
	    save_err = errno;
	}

      if (fd == -1)
	{
	  if (!be_quiet)
	    error (0, save_err, _("\
cannot open output file `%s' for category `%s'"),
		   fname, category);
	  free (fname);
	  return;
	}
    }

#ifdef UIO_MAXIOV
  maxiov = UIO_MAXIOV;
#else
  maxiov = sysconf (_SC_UIO_MAXIOV);
#endif

  /* Write the data using writev.  But we must take care for the
     limitation of the implementation.  */
  for (cnt = 0; cnt < n_elem; cnt += step)
    {
      step = n_elem - cnt;
      if (maxiov > 0)
	step = MIN (maxiov, step);

      if (writev (fd, &vec[cnt], step) < 0)
	{
	  if (!be_quiet)
	    error (0, errno, _("failure while writing data for category `%s'"),
		   category);
	  break;
	}
    }

  close (fd);

  /* Compare the file with the locale data files for the same category in
     other locales, and see if we can reuse it, to save disk space.  */
  other_paths = siblings (output_path);
  if (other_paths != NULL)
    {
      struct stat fname_stat;

      if (lstat (fname, &fname_stat) >= 0
	  && S_ISREG (fname_stat.st_mode))
	{
	  const char *fname_tail = fname + strlen (output_path);
	  const char **other_p;
	  int seen_count;
	  ino_t *seen_inodes;

	  seen_count = 0;
	  for (other_p = other_paths; *other_p; other_p++)
	    seen_count++;
	  seen_inodes = (ino_t *) xmalloc (seen_count * sizeof (ino_t));
	  seen_count = 0;

	  for (other_p = other_paths; *other_p; other_p++)
	    {
	      const char *other_path = *other_p;
	      size_t other_path_len = strlen (other_path);
	      char *other_fname;
	      struct stat other_fname_stat;

	      other_fname =
		(char *) xmalloc (other_path_len + strlen (fname_tail) + 1);
	      memcpy (other_fname, other_path, other_path_len);
	      strcpy (other_fname + other_path_len, fname_tail);

	      if (lstat (other_fname, &other_fname_stat) >= 0
		  && S_ISREG (other_fname_stat.st_mode)
		  /* Consider only files on the same device.
		     Otherwise hard linking won't work anyway.  */
		  && other_fname_stat.st_dev == fname_stat.st_dev
		  /* Consider only files with the same permissions.
		     Otherwise there are security risks.  */
		  && other_fname_stat.st_uid == fname_stat.st_uid
		  && other_fname_stat.st_gid == fname_stat.st_gid
		  && other_fname_stat.st_mode == fname_stat.st_mode
		  /* Don't compare fname with itself.  */
		  && other_fname_stat.st_ino != fname_stat.st_ino
		  /* Files must have the same size, otherwise they
		     cannot be the same.  */
		  && other_fname_stat.st_size == fname_stat.st_size)
		{
		  /* Skip this file if we have already read it (under a
		     different name).  */
		  int i;

		  for (i = seen_count - 1; i >= 0; i--)
		    if (seen_inodes[i] == other_fname_stat.st_ino)
		      break;
		  if (i < 0)
		    {
		      /* Now compare fname and other_fname for real.  */
		      blksize_t blocksize;

#ifdef _STATBUF_ST_BLKSIZE
		      blocksize = MAX (fname_stat.st_blksize,
				       other_fname_stat.st_blksize);
		      if (blocksize > 8 * 1024)
			blocksize = 8 * 1024;
#else
		      blocksize = 8 * 1024;
#endif

		      if (compare_files (fname, other_fname,
					 fname_stat.st_size, blocksize) == 0)
			{
			  /* Found! other_fname is identical to fname.  */
			  /* Link other_fname to fname.  But use a temporary
			     file, in case hard links don't work on the
			     particular filesystem.  */
			  char * tmp_fname =
			    (char *) xmalloc (strlen (fname) + 4 + 1);

			  strcpy (tmp_fname, fname);
			  strcat (tmp_fname, ".tmp");

			  if (link (other_fname, tmp_fname) >= 0)
			    {
			      unlink (fname);
			      if (rename (tmp_fname, fname) < 0)
				{
				  if (!be_quiet)
				    error (0, errno, _("\
cannot create output file `%s' for category `%s'"),
					   fname, category);
				}
			      free (tmp_fname);
			      free (other_fname);
			      break;
			    }
			  free (tmp_fname);
			}

		      /* Don't compare with this file a second time.  */
		      seen_inodes[seen_count++] = other_fname_stat.st_ino;
		    }
		}
	      free (other_fname);
	    }
	  free (seen_inodes);
	}
    }

  free (fname);
}
