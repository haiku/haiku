/* Buffer management for tar.

   Copyright (C) 1988, 1992, 1993, 1994, 1996, 1997, 1999, 2000, 2001,
   2003, 2004 Free Software Foundation, Inc.

   Written by John Gilmore, on 1985-08-25.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any later
   version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
   Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "system.h"

#include <signal.h>

#include <fnmatch.h>
#include <human.h>
#include <quotearg.h>

#include "common.h"
#include "rmt.h"

/* Number of retries before giving up on read.  */
#define	READ_ERROR_MAX 10

/* Globbing pattern to append to volume label if initial match failed.  */
#define VOLUME_LABEL_APPEND " Volume [1-9]*"

/* Variables.  */

static tarlong prev_written;	/* bytes written on previous volumes */
static tarlong bytes_written;	/* bytes written on this volume */

/* FIXME: The following variables should ideally be static to this
   module.  However, this cannot be done yet.  The cleanup continues!  */

union block *record_start;	/* start of record of archive */
union block *record_end;	/* last+1 block of archive record */
union block *current_block;	/* current block of archive */
enum access_mode access_mode;	/* how do we handle the archive */
off_t records_read;		/* number of records read from this archive */
off_t records_written;		/* likewise, for records written */

static off_t record_start_block; /* block ordinal at record_start */

/* Where we write list messages (not errors, not interactions) to.  */
FILE *stdlis;

static void backspace_output (void);
static bool new_volume (enum access_mode);

/* PID of child program, if compress_option or remote archive access.  */
static pid_t child_pid;

/* Error recovery stuff  */
static int read_error_count;

/* Have we hit EOF yet?  */
static int hit_eof;

/* Checkpointing counter */
static int checkpoint;

/* We're reading, but we just read the last block and it's time to update.
   Declared in update.c

   As least EXTERN like this one as possible. (?? --gray)
   FIXME: Either eliminate it or move it to common.h. 
*/
extern bool time_to_start_writing;

static int volno = 1;		/* which volume of a multi-volume tape we're
				   on */
static int global_volno = 1;	/* volume number to print in external
				   messages */

/* The pointer save_name, which is set in function dump_file() of module
   create.c, points to the original long filename instead of the new,
   shorter mangled name that is set in start_header() of module create.c.
   The pointer save_name is only used in multi-volume mode when the file
   being processed is non-sparse; if a file is split between volumes, the
   save_name is used in generating the LF_MULTIVOL record on the second
   volume.  (From Pierce Cantrell, 1991-08-13.)  */

char *save_name;		/* name of the file we are currently writing */
off_t save_totsize;		/* total size of file we are writing, only
				   valid if save_name is nonzero */
off_t save_sizeleft;		/* where we are in the file we are writing,
				   only valid if save_name is nonzero */

bool write_archive_to_stdout;

/* Used by flush_read and flush_write to store the real info about saved
   names.  */
static char *real_s_name;
static off_t real_s_totsize;
static off_t real_s_sizeleft;

/* Functions.  */

void
clear_read_error_count ()
{
  read_error_count = 0;
}

void
print_total_written (void)
{
  tarlong written = prev_written + bytes_written;
  char bytes[sizeof (tarlong) * CHAR_BIT];
  char abbr[LONGEST_HUMAN_READABLE + 1];
  char rate[LONGEST_HUMAN_READABLE + 1];
  double seconds;
  int human_opts = human_autoscale | human_base_1024 | human_SI | human_B;

#if HAVE_CLOCK_GETTIME
  struct timespec now;
  if (clock_gettime (CLOCK_REALTIME, &now) == 0)
    seconds = ((now.tv_sec - start_timespec.tv_sec)
	       + (now.tv_nsec - start_timespec.tv_nsec) / 1e9);
  else
#endif
    seconds = time (0) - start_time;

  sprintf (bytes, TARLONG_FORMAT, written);

  /* Amanda 2.4.1p1 looks for "Total bytes written: [0-9][0-9]*".  */
  fprintf (stderr, _("Total bytes written: %s (%s, %s/s)\n"), bytes,
	   human_readable (written, abbr, human_opts, 1, 1),
	   (0 < seconds && written / seconds < (uintmax_t) -1
	    ? human_readable (written / seconds, rate, human_opts, 1, 1)
	    : "?"));
}

/* Compute and return the block ordinal at current_block.  */
off_t
current_block_ordinal (void)
{
  return record_start_block + (current_block - record_start);
}

/* If the EOF flag is set, reset it, as well as current_block, etc.  */
void
reset_eof (void)
{
  if (hit_eof)
    {
      hit_eof = 0;
      current_block = record_start;
      record_end = record_start + blocking_factor;
      access_mode = ACCESS_WRITE;
    }
}

/* Return the location of the next available input or output block.
   Return zero for EOF.  Once we have returned zero, we just keep returning
   it, to avoid accidentally going on to the next file on the tape.  */
union block *
find_next_block (void)
{
  if (current_block == record_end)
    {
      if (hit_eof)
	return 0;
      flush_archive ();
      if (current_block == record_end)
	{
	  hit_eof = 1;
	  return 0;
	}
    }
  return current_block;
}

/* Indicate that we have used all blocks up thru BLOCK. */
void
set_next_block_after (union block *block)
{
  while (block >= current_block)
    current_block++;

  /* Do *not* flush the archive here.  If we do, the same argument to
     set_next_block_after could mean the next block (if the input record
     is exactly one block long), which is not what is intended.  */

  if (current_block > record_end)
    abort ();
}

/* Return the number of bytes comprising the space between POINTER
   through the end of the current buffer of blocks.  This space is
   available for filling with data, or taking data from.  POINTER is
   usually (but not always) the result of previous find_next_block call.  */
size_t
available_space_after (union block *pointer)
{
  return record_end->buffer - pointer->buffer;
}

/* Close file having descriptor FD, and abort if close unsuccessful.  */
void
xclose (int fd)
{
  if (close (fd) != 0)
    close_error (_("(pipe)"));
}

/* Check the LABEL block against the volume label, seen as a globbing
   pattern.  Return true if the pattern matches.  In case of failure,
   retry matching a volume sequence number before giving up in
   multi-volume mode.  */
static bool
check_label_pattern (union block *label)
{
  char *string;
  bool result;

  if (! memchr (label->header.name, '\0', sizeof label->header.name))
    return false;

  if (fnmatch (volume_label_option, label->header.name, 0) == 0)
    return true;

  if (!multi_volume_option)
    return false;

  string = xmalloc (strlen (volume_label_option)
		    + sizeof VOLUME_LABEL_APPEND + 1);
  strcpy (string, volume_label_option);
  strcat (string, VOLUME_LABEL_APPEND);
  result = fnmatch (string, label->header.name, 0) == 0;
  free (string);
  return result;
}

/* Open an archive file.  The argument specifies whether we are
   reading or writing, or both.  */
void
open_archive (enum access_mode wanted_access)
{
  int backed_up_flag = 0;

  if (index_file_name)
    {
      stdlis = fopen (index_file_name, "w");
      if (! stdlis)
	open_error (index_file_name);
    }
  else
    stdlis = to_stdout_option ? stderr : stdout;

  if (record_size == 0)
    FATAL_ERROR ((0, 0, _("Invalid value for record_size")));

  if (archive_names == 0)
    FATAL_ERROR ((0, 0, _("No archive name given")));

  tar_stat_destroy (&current_stat_info);
  save_name = 0;
  real_s_name = 0;

  if (multi_volume_option)
    {
      record_start = valloc (record_size + (2 * BLOCKSIZE));
      if (record_start)
	record_start += 2;
    }
  else
    record_start = valloc (record_size);
  if (!record_start)
    FATAL_ERROR ((0, 0, _("Cannot allocate memory for blocking factor %d"),
		  blocking_factor));

  current_block = record_start;
  record_end = record_start + blocking_factor;
  /* When updating the archive, we start with reading.  */
  access_mode = wanted_access == ACCESS_UPDATE ? ACCESS_READ : wanted_access;

  if (use_compress_program_option)
    {
      switch (wanted_access)
	{
	case ACCESS_READ:
	  read_full_records_option = false;
	  child_pid = sys_child_open_for_uncompress ();
	  break;

	case ACCESS_WRITE:
	  child_pid = sys_child_open_for_compress ();
	  break;

	case ACCESS_UPDATE:
	  abort (); /* Should not happen */
	  break;
	}

      if (wanted_access == ACCESS_WRITE
	  && strcmp (archive_name_array[0], "-") == 0)
	stdlis = stderr;
    }
  else if (strcmp (archive_name_array[0], "-") == 0)
    {
      read_full_records_option = true; /* could be a pipe, be safe */
      if (verify_option)
	FATAL_ERROR ((0, 0, _("Cannot verify stdin/stdout archive")));

      switch (wanted_access)
	{
	case ACCESS_READ:
	  archive = STDIN_FILENO;
	  break;

	case ACCESS_WRITE:
	  archive = STDOUT_FILENO;
	  stdlis = stderr;
	  break;

	case ACCESS_UPDATE:
	  archive = STDIN_FILENO;
	  stdlis = stderr;
	  write_archive_to_stdout = true;
	  break;
	}
    }
  else if (verify_option)
    archive = rmtopen (archive_name_array[0], O_RDWR | O_CREAT | O_BINARY,
		       MODE_RW, rsh_command_option);
  else
    switch (wanted_access)
      {
      case ACCESS_READ:
	archive = rmtopen (archive_name_array[0], O_RDONLY | O_BINARY,
			   MODE_RW, rsh_command_option);
	break;

      case ACCESS_WRITE:
	if (backup_option)
	  {
	    maybe_backup_file (archive_name_array[0], 1);
	    backed_up_flag = 1;
	  }
	archive = rmtcreat (archive_name_array[0], MODE_RW,
			    rsh_command_option);
	break;

      case ACCESS_UPDATE:
	archive = rmtopen (archive_name_array[0], O_RDWR | O_CREAT | O_BINARY,
			   MODE_RW, rsh_command_option);
	break;
      }

  if (archive < 0
      || (! _isrmt (archive) && !sys_get_archive_stat ()))
    {
      int saved_errno = errno;

      if (backed_up_flag)
	undo_last_backup ();
      errno = saved_errno;
      open_fatal (archive_name_array[0]);
    }

  sys_detect_dev_null_output ();
  sys_save_archive_dev_ino ();
  SET_BINARY_MODE (archive);

  switch (wanted_access)
    {
    case ACCESS_UPDATE:
      records_written = 0;
    case ACCESS_READ:
      records_read = 0;
      record_end = record_start; /* set up for 1st record = # 0 */
      find_next_block ();	/* read it in, check for EOF */

      if (volume_label_option)
	{
	  union block *label = find_next_block ();

	  if (!label)
	    FATAL_ERROR ((0, 0, _("Archive not labeled to match %s"),
			  quote (volume_label_option)));
	  if (!check_label_pattern (label))
	    FATAL_ERROR ((0, 0, _("Volume %s does not match %s"),
			  quote_n (0, label->header.name),
			  quote_n (1, volume_label_option)));
	}
      break;

    case ACCESS_WRITE:
      records_written = 0;
      if (volume_label_option)
	{
	  memset (record_start, 0, BLOCKSIZE);
	  if (multi_volume_option)
	    sprintf (record_start->header.name, "%s Volume 1",
		     volume_label_option);
	  else
	    strcpy (record_start->header.name, volume_label_option);

	  assign_string (&current_stat_info.file_name,
			 record_start->header.name);
	  current_stat_info.had_trailing_slash =
	    strip_trailing_slashes (current_stat_info.file_name);

	  record_start->header.typeflag = GNUTYPE_VOLHDR;
	  TIME_TO_CHARS (start_time, record_start->header.mtime);
	  finish_header (&current_stat_info, record_start, -1);
	}
      break;
    }
}

/* Perform a write to flush the buffer.  */
void
flush_write (void)
{
  int copy_back;
  ssize_t status;

  if (checkpoint_option && !(++checkpoint % 10))
    WARN ((0, 0, _("Write checkpoint %d"), checkpoint));

  if (tape_length_option && tape_length_option <= bytes_written)
    {
      errno = ENOSPC;
      status = 0;
    }
  else if (dev_null_output)
    status = record_size;
  else
    status = sys_write_archive_buffer ();
  if (status != record_size && !multi_volume_option)
    archive_write_error (status);

  if (status > 0)
    {
      records_written++;
      bytes_written += status;
    }

  if (status == record_size)
    {
      if (multi_volume_option)
	{
	  if (save_name)
	    {
	      assign_string (&real_s_name, safer_name_suffix (save_name, false));
	      real_s_totsize = save_totsize;
	      real_s_sizeleft = save_sizeleft;
	    }
	  else
	    {
	      assign_string (&real_s_name, 0);
	      real_s_totsize = 0;
	      real_s_sizeleft = 0;
	    }
	}
      return;
    }

  /* We're multivol.  Panic if we didn't get the right kind of response.  */

  /* ENXIO is for the UNIX PC.  */
  if (status < 0 && errno != ENOSPC && errno != EIO && errno != ENXIO)
    archive_write_error (status);

  /* If error indicates a short write, we just move to the next tape.  */

  if (!new_volume (ACCESS_WRITE))
    return;

  if (totals_option)
    prev_written += bytes_written;
  bytes_written = 0;

  if (volume_label_option && real_s_name)
    {
      copy_back = 2;
      record_start -= 2;
    }
  else if (volume_label_option || real_s_name)
    {
      copy_back = 1;
      record_start--;
    }
  else
    copy_back = 0;

  if (volume_label_option)
    {
      memset (record_start, 0, BLOCKSIZE);
      sprintf (record_start->header.name, "%s Volume %d",
	       volume_label_option, volno);
      TIME_TO_CHARS (start_time, record_start->header.mtime);
      record_start->header.typeflag = GNUTYPE_VOLHDR;
      finish_header (&current_stat_info, record_start, -1);
    }

  if (real_s_name)
    {
      int tmp;

      if (volume_label_option)
	record_start++;

      memset (record_start, 0, BLOCKSIZE);

      /* FIXME: Michael P Urban writes: [a long name file] is being written
	 when a new volume rolls around [...]  Looks like the wrong value is
	 being preserved in real_s_name, though.  */

      strcpy (record_start->header.name, real_s_name);
      record_start->header.typeflag = GNUTYPE_MULTIVOL;
      OFF_TO_CHARS (real_s_sizeleft, record_start->header.size);
      OFF_TO_CHARS (real_s_totsize - real_s_sizeleft,
		    record_start->oldgnu_header.offset);
      tmp = verbose_option;
      verbose_option = 0;
      finish_header (&current_stat_info, record_start, -1);
      verbose_option = tmp;

      if (volume_label_option)
	record_start--;
    }

  status = sys_write_archive_buffer ();
  if (status != record_size)
    archive_write_error (status);

  bytes_written += status;

  if (copy_back)
    {
      record_start += copy_back;
      memcpy (current_block,
	      record_start + blocking_factor - copy_back,
	      copy_back * BLOCKSIZE);
      current_block += copy_back;

      if (real_s_sizeleft >= copy_back * BLOCKSIZE)
	real_s_sizeleft -= copy_back * BLOCKSIZE;
      else if ((real_s_sizeleft + BLOCKSIZE - 1) / BLOCKSIZE <= copy_back)
	assign_string (&real_s_name, 0);
      else
	{
	  assign_string (&real_s_name, safer_name_suffix (save_name, false));
	  real_s_sizeleft = save_sizeleft;
	  real_s_totsize = save_totsize;
	}
      copy_back = 0;
    }
}

/* Handle write errors on the archive.  Write errors are always fatal.
   Hitting the end of a volume does not cause a write error unless the
   write was the first record of the volume.  */
void
archive_write_error (ssize_t status)
{
  /* It might be useful to know how much was written before the error
     occurred.  */
  if (totals_option)
    {
      int e = errno;
      print_total_written ();
      errno = e;
    }

  write_fatal_details (*archive_name_cursor, status, record_size);
}

/* Handle read errors on the archive.  If the read should be retried,
   return to the caller.  */
void
archive_read_error (void)
{
  read_error (*archive_name_cursor);

  if (record_start_block == 0)
    FATAL_ERROR ((0, 0, _("At beginning of tape, quitting now")));

  /* Read error in mid archive.  We retry up to READ_ERROR_MAX times and
     then give up on reading the archive.  */

  if (read_error_count++ > READ_ERROR_MAX)
    FATAL_ERROR ((0, 0, _("Too many errors, quitting")));
  return;
}

static void
short_read (ssize_t status)
{
  size_t left;			/* bytes left */
  char *more;			/* pointer to next byte to read */

  more = record_start->buffer + status;
  left = record_size - status;

  while (left % BLOCKSIZE != 0
	 || (left && status && read_full_records_option))
    {
      if (status)
	while ((status = rmtread (archive, more, left)) < 0)
	  archive_read_error ();

      if (status == 0)
	{
	  char buf[UINTMAX_STRSIZE_BOUND];
	  
	  WARN((0, 0, _("Read %s bytes from %s"),
		STRINGIFY_BIGINT (record_size - left, buf),
		*archive_name_cursor));
	  break;
	}
  
      if (! read_full_records_option)
	{
	  unsigned long rest = record_size - left;
	  
	  FATAL_ERROR ((0, 0,
			ngettext ("Unaligned block (%lu byte) in archive",
				  "Unaligned block (%lu bytes) in archive",
				  rest),
			rest));
	}

      /* User warned us about this.  Fix up.  */

      left -= status;
      more += status;
    }

  /* FIXME: for size=0, multi-volume support.  On the first record, warn
     about the problem.  */

  if (!read_full_records_option && verbose_option > 1
      && record_start_block == 0 && status > 0)
    {
      unsigned long rsize = (record_size - left) / BLOCKSIZE;
      WARN ((0, 0,
	     ngettext ("Record size = %lu block",
		       "Record size = %lu blocks",
		       rsize),
	     rsize));
    }

  record_end = record_start + (record_size - left) / BLOCKSIZE;
  records_read++;
}

/* Perform a read to flush the buffer.  */
void
flush_read (void)
{
  ssize_t status;		/* result from system call */

  if (checkpoint_option && !(++checkpoint % 10))
    WARN ((0, 0, _("Read checkpoint %d"), checkpoint));

  /* Clear the count of errors.  This only applies to a single call to
     flush_read.  */

  read_error_count = 0;		/* clear error count */

  if (write_archive_to_stdout && record_start_block != 0)
    {
      archive = STDOUT_FILENO;
      status = sys_write_archive_buffer ();
      archive = STDIN_FILENO;
      if (status != record_size)
	archive_write_error (status);
    }
  if (multi_volume_option)
    {
      if (save_name)
	{
	  assign_string (&real_s_name, safer_name_suffix (save_name, false));
	  real_s_sizeleft = save_sizeleft;
	  real_s_totsize = save_totsize;
	}
      else
	{
	  assign_string (&real_s_name, 0);
	  real_s_totsize = 0;
	  real_s_sizeleft = 0;
	}
    }

 error_loop:
  status = rmtread (archive, record_start->buffer, record_size);
  if (status == record_size)
    {
      records_read++;
      return;
    }

  /* The condition below used to include
	      || (status > 0 && !read_full_records_option)
     This is incorrect since even if new_volume() succeeds, the
     subsequent call to rmtread will overwrite the chunk of data
     already read in the buffer, so the processing will fail */
        
  if ((status == 0
       || (status < 0 && errno == ENOSPC))
      && multi_volume_option)
    {
      union block *cursor;

    try_volume:
      switch (subcommand_option)
	{
	case APPEND_SUBCOMMAND:
	case CAT_SUBCOMMAND:
	case UPDATE_SUBCOMMAND:
	  if (!new_volume (ACCESS_UPDATE))
	    return;
	  break;

	default:
	  if (!new_volume (ACCESS_READ))
	    return;
	  break;
	}

      while ((status =
	      rmtread (archive, record_start->buffer, record_size)) < 0)
	archive_read_error ();
      
      if (status != record_size)
	short_read (status); 

      cursor = record_start;

      if (cursor->header.typeflag == GNUTYPE_VOLHDR)
	{
	  if (volume_label_option)
	    {
	      if (!check_label_pattern (cursor))
		{
		  WARN ((0, 0, _("Volume %s does not match %s"),
			 quote_n (0, cursor->header.name),
			 quote_n (1, volume_label_option)));
		  volno--;
		  global_volno--;
		  goto try_volume;
		}
	    }
	  if (verbose_option)
	    fprintf (stdlis, _("Reading %s\n"), quote (cursor->header.name));
	  cursor++;
	}
      else if (volume_label_option)
	WARN ((0, 0, _("WARNING: No volume header")));

      if (real_s_name)
	{
	  uintmax_t s1, s2;
	  if (cursor->header.typeflag != GNUTYPE_MULTIVOL
	      || strcmp (cursor->header.name, real_s_name))
	    {
	      WARN ((0, 0, _("%s is not continued on this volume"),
		     quote (real_s_name)));
	      volno--;
	      global_volno--;
	      goto try_volume;
	    }
	  s1 = UINTMAX_FROM_HEADER (cursor->header.size);
	  s2 = UINTMAX_FROM_HEADER (cursor->oldgnu_header.offset);
	  if (real_s_totsize != s1 + s2 || s1 + s2 < s2)
	    {
	      char totsizebuf[UINTMAX_STRSIZE_BOUND];
	      char s1buf[UINTMAX_STRSIZE_BOUND];
	      char s2buf[UINTMAX_STRSIZE_BOUND];

	      WARN ((0, 0, _("%s is the wrong size (%s != %s + %s)"),
		     quote (cursor->header.name),
		     STRINGIFY_BIGINT (save_totsize, totsizebuf),
		     STRINGIFY_BIGINT (s1, s1buf),
		     STRINGIFY_BIGINT (s2, s2buf)));
	      volno--;
	      global_volno--;
	      goto try_volume;
	    }
	  if (real_s_totsize - real_s_sizeleft
	      != OFF_FROM_HEADER (cursor->oldgnu_header.offset))
	    {
	      WARN ((0, 0, _("This volume is out of sequence")));
	      volno--;
	      global_volno--;
	      goto try_volume;
	    }
	  cursor++;
	}
      current_block = cursor;
      records_read++;
      return;
    }
  else if (status < 0)
    {
      archive_read_error ();
      goto error_loop;		/* try again */
    }

  short_read (status);
}

/*  Flush the current buffer to/from the archive.  */
void
flush_archive (void)
{
  record_start_block += record_end - record_start;
  current_block = record_start;
  record_end = record_start + blocking_factor;

  if (access_mode == ACCESS_READ && time_to_start_writing)
    {
      access_mode = ACCESS_WRITE;
      time_to_start_writing = false;
      backspace_output ();
    }

  switch (access_mode)
    {
    case ACCESS_READ:
      flush_read ();
      break;

    case ACCESS_WRITE:
      flush_write ();
      break;

    case ACCESS_UPDATE:
      abort ();
    }
}

/* Backspace the archive descriptor by one record worth.  If it's a
   tape, MTIOCTOP will work.  If it's something else, try to seek on
   it.  If we can't seek, we lose!  */
static void
backspace_output (void)
{
#ifdef MTIOCTOP
  {
    struct mtop operation;

    operation.mt_op = MTBSR;
    operation.mt_count = 1;
    if (rmtioctl (archive, MTIOCTOP, (char *) &operation) >= 0)
      return;
    if (errno == EIO && rmtioctl (archive, MTIOCTOP, (char *) &operation) >= 0)
      return;
  }
#endif

  {
    off_t position = rmtlseek (archive, (off_t) 0, SEEK_CUR);

    /* Seek back to the beginning of this record and start writing there.  */

    position -= record_size;
    if (position < 0)
      position = 0;
    if (rmtlseek (archive, position, SEEK_SET) != position)
      {
	/* Lseek failed.  Try a different method.  */

	WARN ((0, 0,
	       _("Cannot backspace archive file; it may be unreadable without -i")));

	/* Replace the first part of the record with NULs.  */

	if (record_start->buffer != output_start)
	  memset (record_start->buffer, 0,
		  output_start - record_start->buffer);
      }
  }
}

/* Close the archive file.  */
void
close_archive (void)
{
  if (time_to_start_writing || access_mode == ACCESS_WRITE)
    flush_archive ();

  sys_drain_input_pipe ();
  
  if (verify_option)
    verify_volume ();

  if (rmtclose (archive) != 0)
    close_warn (*archive_name_cursor);

  sys_wait_for_child (child_pid);
  
  tar_stat_destroy (&current_stat_info);
  if (save_name)
    free (save_name);
  if (real_s_name)
    free (real_s_name);
  free (multi_volume_option ? record_start - 2 : record_start);
}

/* Called to initialize the global volume number.  */
void
init_volume_number (void)
{
  FILE *file = fopen (volno_file_option, "r");

  if (file)
    {
      if (fscanf (file, "%d", &global_volno) != 1
	  || global_volno < 0)
	FATAL_ERROR ((0, 0, _("%s: contains invalid volume number"),
		      quotearg_colon (volno_file_option)));
      if (ferror (file))
	read_error (volno_file_option);
      if (fclose (file) != 0)
	close_error (volno_file_option);
    }
  else if (errno != ENOENT)
    open_error (volno_file_option);
}

/* Called to write out the closing global volume number.  */
void
closeout_volume_number (void)
{
  FILE *file = fopen (volno_file_option, "w");

  if (file)
    {
      fprintf (file, "%d\n", global_volno);
      if (ferror (file))
	write_error (volno_file_option);
      if (fclose (file) != 0)
	close_error (volno_file_option);
    }
  else
    open_error (volno_file_option);
}

/* We've hit the end of the old volume.  Close it and open the next one.
   Return nonzero on success.
*/
static bool
new_volume (enum access_mode access)
{
  static FILE *read_file;
  static int looped;

  if (!read_file && !info_script_option)
    /* FIXME: if fopen is used, it will never be closed.  */
    read_file = archive == STDIN_FILENO ? fopen (TTY_NAME, "r") : stdin;

  if (now_verifying)
    return false;
  if (verify_option)
    verify_volume ();

  if (rmtclose (archive) != 0)
    close_warn (*archive_name_cursor);

  global_volno++;
  if (global_volno < 0)
    FATAL_ERROR ((0, 0, _("Volume number overflow")));
  volno++;
  archive_name_cursor++;
  if (archive_name_cursor == archive_name_array + archive_names)
    {
      archive_name_cursor = archive_name_array;
      looped = 1;
    }

 tryagain:
  if (looped)
    {
      /* We have to prompt from now on.  */

      if (info_script_option)
	{
	  if (volno_file_option)
	    closeout_volume_number ();
	  if (system (info_script_option) != 0)
	    FATAL_ERROR ((0, 0, _("`%s' command failed"), info_script_option));
	}
      else
	while (1)
	  {
	    char input_buffer[80];

	    fputc ('\007', stderr);
	    fprintf (stderr,
		     _("Prepare volume #%d for %s and hit return: "),
		     global_volno, quote (*archive_name_cursor));
	    fflush (stderr);

	    if (fgets (input_buffer, sizeof input_buffer, read_file) == 0)
	      {
		WARN ((0, 0, _("EOF where user reply was expected")));

		if (subcommand_option != EXTRACT_SUBCOMMAND
		    && subcommand_option != LIST_SUBCOMMAND
		    && subcommand_option != DIFF_SUBCOMMAND)
		  WARN ((0, 0, _("WARNING: Archive is incomplete")));

		fatal_exit ();
	      }
	    if (input_buffer[0] == '\n'
		|| input_buffer[0] == 'y'
		|| input_buffer[0] == 'Y')
	      break;

	    switch (input_buffer[0])
	      {
	      case '?':
		{
		  /* FIXME: Might it be useful to disable the '!' command? */
		  fprintf (stderr, _("\
 n [name]   Give a new file name for the next (and subsequent) volume(s)\n\
 q          Abort tar\n\
 !          Spawn a subshell\n\
 ?          Print this list\n"));
		}
		break;

	      case 'q':
		/* Quit.  */

		WARN ((0, 0, _("No new volume; exiting.\n")));

		if (subcommand_option != EXTRACT_SUBCOMMAND
		    && subcommand_option != LIST_SUBCOMMAND
		    && subcommand_option != DIFF_SUBCOMMAND)
		  WARN ((0, 0, _("WARNING: Archive is incomplete")));

		fatal_exit ();

	      case 'n':
		/* Get new file name.  */

		{
		  char *name = &input_buffer[1];
		  char *cursor;

		  for (name = input_buffer + 1;
		       *name == ' ' || *name == '\t';
		       name++)
		    ;

		  for (cursor = name; *cursor && *cursor != '\n'; cursor++)
		    ;
		  *cursor = '\0';

		  /* FIXME: the following allocation is never reclaimed.  */
		  *archive_name_cursor = xstrdup (name);
		}
		break;

	      case '!':
		sys_spawn_shell ();
		break;
	      }
	  }
    }

  if (strcmp (archive_name_cursor[0], "-") == 0)
    {
      read_full_records_option = true;
      archive = STDIN_FILENO;
    }
  else if (verify_option)
    archive = rmtopen (*archive_name_cursor, O_RDWR | O_CREAT, MODE_RW,
		       rsh_command_option);
  else
    switch (access)
      {
      case ACCESS_READ:
	archive = rmtopen (*archive_name_cursor, O_RDONLY, MODE_RW,
			   rsh_command_option);
	break;

      case ACCESS_WRITE:
	if (backup_option)
	  maybe_backup_file (*archive_name_cursor, 1);
	archive = rmtcreat (*archive_name_cursor, MODE_RW,
			    rsh_command_option);
	break;

      case ACCESS_UPDATE:
	archive = rmtopen (*archive_name_cursor, O_RDWR | O_CREAT, MODE_RW,
			   rsh_command_option);
	break;
      }

  if (archive < 0)
    {
      open_warn (*archive_name_cursor);
      if (!verify_option && access == ACCESS_WRITE && backup_option)
	undo_last_backup ();
      goto tryagain;
    }

  SET_BINARY_MODE (archive);

  return true;
}

