/* Diff files from a tar archive.

   Copyright (C) 1988, 1992, 1993, 1994, 1996, 1997, 1999, 2000, 2001,
   2003, 2004, 2005, 2006, 2007 Free Software Foundation, Inc.

   Written by John Gilmore, on 1987-04-30.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 3, or (at your option) any later
   version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
   Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#include <system.h>
#include <system-ioctl.h>

#if HAVE_LINUX_FD_H
# include <linux/fd.h>
#endif

#include "common.h"
#include <quotearg.h>
#include <rmt.h>
#include <stdarg.h>

/* Nonzero if we are verifying at the moment.  */
bool now_verifying;

/* File descriptor for the file we are diffing.  */
static int diff_handle;

/* Area for reading file contents into.  */
static char *diff_buffer;

/* Initialize for a diff operation.  */
void
diff_init (void)
{
  void *ptr;
  diff_buffer = page_aligned_alloc (&ptr, record_size);
  if (listed_incremental_option)
    read_directory_file ();
}

/* Sigh about something that differs by writing a MESSAGE to stdlis,
   given MESSAGE is nonzero.  Also set the exit status if not already.  */
void
report_difference (struct tar_stat_info *st, const char *fmt, ...)
{
  if (fmt)
    {
      va_list ap;

      fprintf (stdlis, "%s: ", quotearg_colon (st->file_name));
      va_start (ap, fmt);
      vfprintf (stdlis, fmt, ap);
      va_end (ap);
      fprintf (stdlis, "\n");
    }

  if (exit_status == TAREXIT_SUCCESS)
    exit_status = TAREXIT_DIFFERS;
}

/* Take a buffer returned by read_and_process and do nothing with it.  */
static int
process_noop (size_t size __attribute__ ((unused)),
	      char *data __attribute__ ((unused)))
{
  return 1;
}

static int
process_rawdata (size_t bytes, char *buffer)
{
  size_t status = safe_read (diff_handle, diff_buffer, bytes);

  if (status != bytes)
    {
      if (status == SAFE_READ_ERROR)
	{
	  read_error (current_stat_info.file_name);
	  report_difference (&current_stat_info, NULL);
	}
      else
	{
	  report_difference (&current_stat_info,
			     ngettext ("Could only read %lu of %lu byte",
				       "Could only read %lu of %lu bytes",
				       bytes),
			     (unsigned long) status, (unsigned long) bytes);
	}
      return 0;
    }

  if (memcmp (buffer, diff_buffer, bytes))
    {
      report_difference (&current_stat_info, _("Contents differ"));
      return 0;
    }

  return 1;
}

/* Some other routine wants SIZE bytes in the archive.  For each chunk
   of the archive, call PROCESSOR with the size of the chunk, and the
   address of the chunk it can work with.  The PROCESSOR should return
   nonzero for success.  Once it returns error, continue skipping
   without calling PROCESSOR anymore.  */

static void
read_and_process (struct tar_stat_info *st, int (*processor) (size_t, char *))
{
  union block *data_block;
  size_t data_size;
  off_t size = st->stat.st_size;

  mv_begin (st);
  while (size)
    {
      data_block = find_next_block ();
      if (! data_block)
	{
	  ERROR ((0, 0, _("Unexpected EOF in archive")));
	  return;
	}

      data_size = available_space_after (data_block);
      if (data_size > size)
	data_size = size;
      if (!(*processor) (data_size, data_block->buffer))
	processor = process_noop;
      set_next_block_after ((union block *)
			    (data_block->buffer + data_size - 1));
      size -= data_size;
      mv_size_left (size);
    }
  mv_end ();
}

/* Call either stat or lstat over STAT_DATA, depending on
   --dereference (-h), for a file which should exist.  Diagnose any
   problem.  Return nonzero for success, zero otherwise.  */
static int
get_stat_data (char const *file_name, struct stat *stat_data)
{
  int status = deref_stat (dereference_option, file_name, stat_data);

  if (status != 0)
    {
      if (errno == ENOENT)
	stat_warn (file_name);
      else
	stat_error (file_name);
      report_difference (&current_stat_info, NULL);
      return 0;
    }

  return 1;
}


static void
diff_dir (void)
{
  struct stat stat_data;

  if (!get_stat_data (current_stat_info.file_name, &stat_data))
    return;

  if (!S_ISDIR (stat_data.st_mode))
    report_difference (&current_stat_info, _("File type differs"));
  else if ((current_stat_info.stat.st_mode & MODE_ALL) !=
	   (stat_data.st_mode & MODE_ALL))
    report_difference (&current_stat_info, _("Mode differs"));
}

static void
diff_file (void)
{
  char const *file_name = current_stat_info.file_name;
  struct stat stat_data;

  if (!get_stat_data (file_name, &stat_data))
    skip_member ();
  else if (!S_ISREG (stat_data.st_mode))
    {
      report_difference (&current_stat_info, _("File type differs"));
      skip_member ();
    }
  else
    {
      if ((current_stat_info.stat.st_mode & MODE_ALL) !=
	  (stat_data.st_mode & MODE_ALL))
	report_difference (&current_stat_info, _("Mode differs"));

      if (!sys_compare_uid (&stat_data, &current_stat_info.stat))
	report_difference (&current_stat_info, _("Uid differs"));
      if (!sys_compare_gid (&stat_data, &current_stat_info.stat))
	report_difference (&current_stat_info, _("Gid differs"));

      if (tar_timespec_cmp (get_stat_mtime (&stat_data),
                            current_stat_info.mtime))
	report_difference (&current_stat_info, _("Mod time differs"));
      if (current_header->header.typeflag != GNUTYPE_SPARSE
	  && stat_data.st_size != current_stat_info.stat.st_size)
	{
	  report_difference (&current_stat_info, _("Size differs"));
	  skip_member ();
	}
      else
	{
	  int atime_flag =
	    (atime_preserve_option == system_atime_preserve
	     ? O_NOATIME
	     : 0);

	  diff_handle = open (file_name, O_RDONLY | O_BINARY | atime_flag);

	  if (diff_handle < 0)
	    {
	      open_error (file_name);
	      skip_member ();
	      report_difference (&current_stat_info, NULL);
	    }
	  else
	    {
	      int status;

	      if (current_stat_info.is_sparse)
		sparse_diff_file (diff_handle, &current_stat_info);
	      else
		read_and_process (&current_stat_info, process_rawdata);

	      if (atime_preserve_option == replace_atime_preserve)
		{
		  struct timespec ts[2];
		  ts[0] = get_stat_atime (&stat_data);
		  ts[1] = get_stat_mtime (&stat_data);
		  if (set_file_atime (diff_handle, file_name, ts) != 0)
		    utime_error (file_name);
		}

	      status = close (diff_handle);
	      if (status != 0)
		close_error (file_name);
	    }
	}
    }
}

static void
diff_link (void)
{
  struct stat file_data;
  struct stat link_data;

  if (get_stat_data (current_stat_info.file_name, &file_data)
      && get_stat_data (current_stat_info.link_name, &link_data)
      && !sys_compare_links (&file_data, &link_data))
    report_difference (&current_stat_info,
		       _("Not linked to %s"),
		       quote (current_stat_info.link_name));
}

#ifdef HAVE_READLINK
static void
diff_symlink (void)
{
  size_t len = strlen (current_stat_info.link_name);
  char *linkbuf = alloca (len + 1);

  int status = readlink (current_stat_info.file_name, linkbuf, len + 1);

  if (status < 0)
    {
      if (errno == ENOENT)
	readlink_warn (current_stat_info.file_name);
      else
	readlink_error (current_stat_info.file_name);
      report_difference (&current_stat_info, NULL);
    }
  else if (status != len
	   || strncmp (current_stat_info.link_name, linkbuf, len) != 0)
    report_difference (&current_stat_info, _("Symlink differs"));
}
#endif

static void
diff_special (void)
{
  struct stat stat_data;

  /* FIXME: deal with umask.  */

  if (!get_stat_data (current_stat_info.file_name, &stat_data))
    return;

  if (current_header->header.typeflag == CHRTYPE
      ? !S_ISCHR (stat_data.st_mode)
      : current_header->header.typeflag == BLKTYPE
      ? !S_ISBLK (stat_data.st_mode)
      : /* current_header->header.typeflag == FIFOTYPE */
      !S_ISFIFO (stat_data.st_mode))
    {
      report_difference (&current_stat_info, _("File type differs"));
      return;
    }

  if ((current_header->header.typeflag == CHRTYPE
       || current_header->header.typeflag == BLKTYPE)
      && current_stat_info.stat.st_rdev != stat_data.st_rdev)
    {
      report_difference (&current_stat_info, _("Device number differs"));
      return;
    }

  if ((current_stat_info.stat.st_mode & MODE_ALL) !=
      (stat_data.st_mode & MODE_ALL))
    report_difference (&current_stat_info, _("Mode differs"));
}

static int
dumpdir_cmp (const char *a, const char *b)
{
  size_t len;
  
  while (*a)
    switch (*a)
      {
      case 'Y':
      case 'N':
	if (!strchr ("YN", *b))
	  return 1;
	if (strcmp(a + 1, b + 1))
	  return 1;
	len = strlen (a) + 1;
	a += len;
	b += len;
	break;
	
      case 'D':
	if (strcmp(a, b))
	  return 1;
	len = strlen (a) + 1;
	a += len;
	b += len;
	break;
	
      case 'R':
      case 'T':
      case 'X':
	return *b;
      }
  return *b;
}

static void
diff_dumpdir (void)
{
  char *dumpdir_buffer;
  dev_t dev = 0;
  struct stat stat_data;

  if (deref_stat (true, current_stat_info.file_name, &stat_data))
    {
      if (errno == ENOENT)
	stat_warn (current_stat_info.file_name);
      else
	stat_error (current_stat_info.file_name);
    }
  else
    dev = stat_data.st_dev;

  dumpdir_buffer = get_directory_contents (current_stat_info.file_name, dev);

  if (dumpdir_buffer)
    {
      if (dumpdir_cmp (current_stat_info.dumpdir, dumpdir_buffer))
	report_difference (&current_stat_info, _("Contents differ"));
    }
  else
    read_and_process (&current_stat_info, process_noop);
}

static void
diff_multivol (void)
{
  struct stat stat_data;
  int fd, status;
  off_t offset;

  if (current_stat_info.had_trailing_slash)
    {
      diff_dir ();
      return;
    }

  if (!get_stat_data (current_stat_info.file_name, &stat_data))
    return;

  if (!S_ISREG (stat_data.st_mode))
    {
      report_difference (&current_stat_info, _("File type differs"));
      skip_member ();
      return;
    }

  offset = OFF_FROM_HEADER (current_header->oldgnu_header.offset);
  if (stat_data.st_size != current_stat_info.stat.st_size + offset)
    {
      report_difference (&current_stat_info, _("Size differs"));
      skip_member ();
      return;
    }

  fd = open (current_stat_info.file_name, O_RDONLY | O_BINARY);

  if (fd < 0)
    {
      open_error (current_stat_info.file_name);
      report_difference (&current_stat_info, NULL);
      skip_member ();
      return;
    }

  if (lseek (fd, offset, SEEK_SET) < 0)
    {
      seek_error_details (current_stat_info.file_name, offset);
      report_difference (&current_stat_info, NULL);
      return;
    }

  read_and_process (&current_stat_info, process_rawdata);

  status = close (fd);
  if (status != 0)
    close_error (current_stat_info.file_name);
}

/* Diff a file against the archive.  */
void
diff_archive (void)
{

  set_next_block_after (current_header);
  decode_header (current_header, &current_stat_info, &current_format, 1);

  /* Print the block from current_header and current_stat_info.  */

  if (verbose_option)
    {
      if (now_verifying)
	fprintf (stdlis, _("Verify "));
      print_header (&current_stat_info, -1);
    }

  switch (current_header->header.typeflag)
    {
    default:
      ERROR ((0, 0, _("%s: Unknown file type `%c', diffed as normal file"),
	      quotearg_colon (current_stat_info.file_name),
	      current_header->header.typeflag));
      /* Fall through.  */

    case AREGTYPE:
    case REGTYPE:
    case GNUTYPE_SPARSE:
    case CONTTYPE:

      /* Appears to be a file.  See if it's really a directory.  */

      if (current_stat_info.had_trailing_slash)
	diff_dir ();
      else
	diff_file ();
      break;

    case LNKTYPE:
      diff_link ();
      break;

#ifdef HAVE_READLINK
    case SYMTYPE:
      diff_symlink ();
      break;
#endif

    case CHRTYPE:
    case BLKTYPE:
    case FIFOTYPE:
      diff_special ();
      break;

    case GNUTYPE_DUMPDIR:
    case DIRTYPE:
      if (is_dumpdir (&current_stat_info))
	diff_dumpdir ();
      diff_dir ();
      break;

    case GNUTYPE_VOLHDR:
      break;

    case GNUTYPE_MULTIVOL:
      diff_multivol ();
    }
}

void
verify_volume (void)
{
  if (removed_prefixes_p ())
    {
      WARN((0, 0,
	    _("Archive contains file names with leading prefixes removed.")));
      WARN((0, 0,
	    _("Verification may fail to locate original files.")));
    }

  if (!diff_buffer)
    diff_init ();

  /* Verifying an archive is meant to check if the physical media got it
     correctly, so try to defeat clever in-memory buffering pertaining to
     this particular media.  On Linux, for example, the floppy drive would
     not even be accessed for the whole verification.

     The code was using fsync only when the ioctl is unavailable, but
     Marty Leisner says that the ioctl does not work when not preceded by
     fsync.  So, until we know better, or maybe to please Marty, let's do it
     the unbelievable way :-).  */

#if HAVE_FSYNC
  fsync (archive);
#endif
#ifdef FDFLUSH
  ioctl (archive, FDFLUSH);
#endif

#ifdef MTIOCTOP
  {
    struct mtop operation;
    int status;

    operation.mt_op = MTBSF;
    operation.mt_count = 1;
    if (status = rmtioctl (archive, MTIOCTOP, (char *) &operation), status < 0)
      {
	if (errno != EIO
	    || (status = rmtioctl (archive, MTIOCTOP, (char *) &operation),
		status < 0))
	  {
#endif
	    if (rmtlseek (archive, (off_t) 0, SEEK_SET) != 0)
	      {
		/* Lseek failed.  Try a different method.  */
		seek_warn (archive_name_array[0]);
		return;
	      }
#ifdef MTIOCTOP
	  }
      }
  }
#endif

  access_mode = ACCESS_READ;
  now_verifying = 1;

  flush_read ();
  while (1)
    {
      enum read_header status = read_header (false);

      if (status == HEADER_FAILURE)
	{
	  int counter = 0;

	  do
	    {
	      counter++;
	      set_next_block_after (current_header);
	      status = read_header (false);
	    }
	  while (status == HEADER_FAILURE);

	  ERROR ((0, 0,
		  ngettext ("VERIFY FAILURE: %d invalid header detected",
			    "VERIFY FAILURE: %d invalid headers detected",
			    counter), counter));
	}
      if (status == HEADER_ZERO_BLOCK || status == HEADER_END_OF_FILE)
	break;

      diff_archive ();
      tar_stat_destroy (&current_stat_info);
    }

  access_mode = ACCESS_WRITE;
  now_verifying = 0;
}
