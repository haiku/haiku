/* System-dependent calls for tar.

   Copyright (C) 2003 Free Software Foundation, Inc.

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

#include "common.h"
#include "rmt.h"
#include <signal.h>

void
sys_stat_nanoseconds(struct tar_stat_info *stat)
{
#if defined(HAVE_STRUCT_STAT_ST_SPARE1)
  stat->atime_nsec = stat->stat.st_spare1 * 1000;
  stat->mtime_nsec = stat->stat.st_spare2 * 1000;
  stat->ctime_nsec = stat->stat.st_spare3 * 1000;
#elif defined(HAVE_STRUCT_STAT_ST_ATIM_TV_NSEC)
  stat->atime_nsec = stat->stat.st_atim.tv_nsec;
  stat->mtime_nsec = stat->stat.st_mtim.tv_nsec;
  stat->ctime_nsec = stat->stat.st_ctim.tv_nsec;
#elif defined(HAVE_STRUCT_STAT_ST_ATIMESPEC_TV_NSEC)
  stat->atime_nsec = stat->stat.st_atimespec.tv_nsec;
  stat->mtime_nsec = stat->stat.st_mtimespec.tv_nsec;
  stat->ctime_nsec = stat->stat.st_ctimespec.tv_nsec;
#elif defined(HAVE_STRUCT_STAT_ST_ATIMENSEC)
  stat->atime_nsec = stat->stat.st_atimensec;
  stat->mtime_nsec = stat->stat.st_mtimensec;
  stat->ctime_nsec = stat->stat.st_ctimensec;
#else
  stat->atime_nsec  = stat->mtime_nsec = stat->ctime_nsec = 0;
#endif
}

int
sys_utimes(char *file_name, struct timeval tvp[3])
{
#ifdef HAVE_UTIMES
  return utimes (file_name, tvp);
#else
  struct utimbuf utimbuf;
  utimbuf.actime = tvp[0].tv_sec;
  utimbuf.modtime = tvp[1].tv_sec;
  return utime (file_name, &utimbuf);
#endif
}

#if MSDOS

bool
sys_get_archive_stat ()
{
  return 0;
}

bool
sys_file_is_archive (struct tar_stat_info *p)
{
  return false;
}

void
sys_save_archive_dev_ino ()
{
}

void
sys_detect_dev_null_output ()
{
  static char const dev_null[] = "nul";

  dev_null_output = (strcmp (archive_name_array[0], dev_null) == 0
		     || (! _isrmt (archive)));
}

void
sys_drain_input_pipe ()
{
}

void
sys_wait_for_child (pid_t child_pid)
{
}

void
sys_spawn_shell ()
{
  spawnl (P_WAIT, getenv ("COMSPEC"), "-", 0);
}

/* stat() in djgpp's C library gives a constant number of 42 as the
   uid and gid of a file.  So, comparing an FTP'ed archive just after
   unpack would fail on MSDOS.  */

bool
sys_compare_uid (struct stat *a, struct stat *b)
{
  return true;
}

bool
sys_compare_gid (struct stat *a, struct stat *b)
{
  return true;
}

void
sys_compare_links (struct stat *link_data, struct stat *stat_data)
{
  return true;
}

int
sys_truncate (int fd)
{
  return write (fd, "", 0);
}

void
sys_reset_uid_gid ()
{
}

ssize_t
sys_write_archive_buffer (void)
{
  ssize_t status;
  ssize_t written = 0;

  while (0 <= (status = full_write (archive, record_start->buffer + written,
				    record_size - written)))
    {
      written += status;
      if (written == record_size)
        break;
    }

  return written ? written : status;
}

/* Set ARCHIVE for writing, then compressing an archive.  */
void
sys_child_open_for_compress (void)
{
  FATAL_ERROR ((0, 0, _("Cannot use compressed or remote archives")));
}

/* Set ARCHIVE for uncompressing, then reading an archive.  */
void
sys_child_open_for_uncompress (void)
{
  FATAL_ERROR ((0, 0, _("Cannot use compressed or remote archives")));
}

#else

extern union block *record_start; /* FIXME */

static struct stat archive_stat; /* stat block for archive file */

bool
sys_get_archive_stat ()
{
  return fstat (archive, &archive_stat) == 0;
}
    
bool
sys_file_is_archive (struct tar_stat_info *p)
{
  return (ar_dev && p->stat.st_dev == ar_dev && p->stat.st_ino == ar_ino);
}

/* Save archive file inode and device numbers */
void
sys_save_archive_dev_ino ()
{
  if (!_isrmt (archive) && S_ISREG (archive_stat.st_mode))
    {
      ar_dev = archive_stat.st_dev;
      ar_ino = archive_stat.st_ino;
    }
  else
    ar_dev = 0;
}

/* Detect if outputting to "/dev/null".  */
void
sys_detect_dev_null_output ()
{
  static char const dev_null[] = "/dev/null";
  struct stat dev_null_stat;

  dev_null_output = (strcmp (archive_name_array[0], dev_null) == 0
		     || (! _isrmt (archive)
			 && S_ISCHR (archive_stat.st_mode)
			 && stat (dev_null, &dev_null_stat) == 0
			 && archive_stat.st_dev == dev_null_stat.st_dev
			 && archive_stat.st_ino == dev_null_stat.st_ino));
}

/* Manage to fully drain a pipe we might be reading, so to not break it on
   the producer after the EOF block.  FIXME: one of these days, GNU tar
   might become clever enough to just stop working, once there is no more
   work to do, we might have to revise this area in such time.  */

void
sys_drain_input_pipe ()
{
  if (access_mode == ACCESS_READ
      && ! _isrmt (archive)
      && (S_ISFIFO (archive_stat.st_mode) || S_ISSOCK (archive_stat.st_mode)))
    while (rmtread (archive, record_start->buffer, record_size) > 0)
      continue;
}

void
sys_wait_for_child (pid_t child_pid)
{
  if (child_pid)
    {
      int wait_status;

      while (waitpid (child_pid, &wait_status, 0) == -1)
	if (errno != EINTR)
	  {
	    waitpid_error (use_compress_program_option);
	    break;
	  }

      if (WIFSIGNALED (wait_status))
	ERROR ((0, 0, _("Child died with signal %d"),
		WTERMSIG (wait_status)));
      else if (WEXITSTATUS (wait_status) != 0)
	ERROR ((0, 0, _("Child returned status %d"),
		WEXITSTATUS (wait_status)));
    }
}

void
sys_spawn_shell ()
{
  pid_t child;
  const char *shell = getenv ("SHELL");
  if (! shell)
    shell = "/bin/sh";
  child = xfork ();
  if (child == 0)
    {
      execlp (shell, "-sh", "-i", (char *) 0);
      exec_fatal (shell);
    }
  else
    {
      int wait_status;
      while (waitpid (child, &wait_status, 0) == -1)
	if (errno != EINTR)
	  {
	    waitpid_error (shell);
	    break;
	  }
    }
}

bool
sys_compare_uid (struct stat *a, struct stat *b)
{
  return a->st_uid == b->st_uid;
}

bool
sys_compare_gid (struct stat *a, struct stat *b)
{
  return a->st_gid == b->st_gid;
}

bool
sys_compare_links (struct stat *link_data, struct stat *stat_data)
{
  return stat_data->st_dev == link_data->st_dev
         && stat_data->st_ino == link_data->st_ino;
}

int
sys_truncate (int fd)
{
  off_t pos = lseek (fd, (off_t) 0, SEEK_CUR);
  return pos < 0 ? -1 : ftruncate (fd, pos);
}

void
sys_reset_uid_gid ()
{
  setuid (getuid ());
  setgid (getgid ());
}

/* Return nonzero if NAME is the name of a regular file, or if the file
   does not exist (so it would be created as a regular file).  */
static int
is_regular_file (const char *name)
{
  struct stat stbuf;

  if (stat (name, &stbuf) == 0)
    return S_ISREG (stbuf.st_mode);
  else
    return errno == ENOENT;
}

ssize_t
sys_write_archive_buffer (void)
{
  ssize_t status;
  ssize_t written = 0;

  while (0 <= (status = rmtwrite (archive, record_start->buffer + written,
				  record_size - written)))
    {
      written += status;
      if (written == record_size
	  || _isrmt (archive)
	  || ! (S_ISFIFO (archive_stat.st_mode)
		|| S_ISSOCK (archive_stat.st_mode)))
	break;
    }

  return written ? written : status;
}

#define	PREAD 0			/* read file descriptor from pipe() */
#define	PWRITE 1		/* write file descriptor from pipe() */

/* Duplicate file descriptor FROM into becoming INTO.
   INTO is closed first and has to be the next available slot.  */
static void
xdup2 (int from, int into)
{
  if (from != into)
    {
      int status = close (into);

      if (status != 0 && errno != EBADF)
	{
	  int e = errno;
	  FATAL_ERROR ((0, e, _("Cannot close")));
	}
      status = dup (from);
      if (status != into)
	{
	  if (status < 0)
	    {
	      int e = errno;
	      FATAL_ERROR ((0, e, _("Cannot dup")));
	    }
	  abort ();
	}
      xclose (from);
    }
}

/* Set ARCHIVE for writing, then compressing an archive.  */
pid_t
sys_child_open_for_compress (void)
{
  int parent_pipe[2];
  int child_pipe[2];
  pid_t grandchild_pid;
  pid_t child_pid;
  int wait_status;

  xpipe (parent_pipe);
  child_pid = xfork ();

  if (child_pid > 0)
    {
      /* The parent tar is still here!  Just clean up.  */

      archive = parent_pipe[PWRITE];
      xclose (parent_pipe[PREAD]);
      return child_pid;
    }

  /* The new born child tar is here!  */

  program_name = _("tar (child)");

  xdup2 (parent_pipe[PREAD], STDIN_FILENO);
  xclose (parent_pipe[PWRITE]);

  /* Check if we need a grandchild tar.  This happens only if either:
     a) we are writing stdout: to force reblocking;
     b) the file is to be accessed by rmt: compressor doesn't know how;
     c) the file is not a plain file.  */

  if (strcmp (archive_name_array[0], "-") != 0
      && !_remdev (archive_name_array[0])
      && is_regular_file (archive_name_array[0]))
    {
      if (backup_option)
	maybe_backup_file (archive_name_array[0], 1);

      /* We don't need a grandchild tar.  Open the archive and launch the
	 compressor.  */

      archive = creat (archive_name_array[0], MODE_RW);
      if (archive < 0)
	{
	  int saved_errno = errno;

	  if (backup_option)
	    undo_last_backup ();
	  errno = saved_errno;
	  open_fatal (archive_name_array[0]);
	}
      xdup2 (archive, STDOUT_FILENO);
      execlp (use_compress_program_option, use_compress_program_option,
	      (char *) 0);
      exec_fatal (use_compress_program_option);
    }

  /* We do need a grandchild tar.  */

  xpipe (child_pipe);
  grandchild_pid = xfork ();

  if (grandchild_pid == 0)
    {
      /* The newborn grandchild tar is here!  Launch the compressor.  */

      program_name = _("tar (grandchild)");

      xdup2 (child_pipe[PWRITE], STDOUT_FILENO);
      xclose (child_pipe[PREAD]);
      execlp (use_compress_program_option, use_compress_program_option,
	      (char *) 0);
      exec_fatal (use_compress_program_option);
    }

  /* The child tar is still here!  */

  /* Prepare for reblocking the data from the compressor into the archive.  */

  xdup2 (child_pipe[PREAD], STDIN_FILENO);
  xclose (child_pipe[PWRITE]);

  if (strcmp (archive_name_array[0], "-") == 0)
    archive = STDOUT_FILENO;
  else
    {
      archive = rmtcreat (archive_name_array[0], MODE_RW, rsh_command_option);
      if (archive < 0)
	open_fatal (archive_name_array[0]);
    }

  /* Let's read out of the stdin pipe and write an archive.  */

  while (1)
    {
      ssize_t status = 0;
      char *cursor;
      size_t length;

      /* Assemble a record.  */

      for (length = 0, cursor = record_start->buffer;
	   length < record_size;
	   length += status, cursor += status)
	{
	  size_t size = record_size - length;

	  status = safe_read (STDIN_FILENO, cursor, size);
	  if (status <= 0)
	    break;
	}

      if (status < 0)
	read_fatal (use_compress_program_option);

      /* Copy the record.  */

      if (status == 0)
	{
	  /* We hit the end of the file.  Write last record at
	     full length, as the only role of the grandchild is
	     doing proper reblocking.  */

	  if (length > 0)
	    {
	      memset (record_start->buffer + length, 0, record_size - length);
	      status = sys_write_archive_buffer ();
	      if (status != record_size)
		archive_write_error (status);
	    }

	  /* There is nothing else to read, break out.  */
	  break;
	}

      status = sys_write_archive_buffer ();
      if (status != record_size)
	archive_write_error (status);
    }

  /* Propagate any failure of the grandchild back to the parent.  */

  while (waitpid (grandchild_pid, &wait_status, 0) == -1)
    if (errno != EINTR)
      {
	waitpid_error (use_compress_program_option);
	break;
      }

  if (WIFSIGNALED (wait_status))
    {
      kill (child_pid, WTERMSIG (wait_status));
      exit_status = TAREXIT_FAILURE;
    }
  else if (WEXITSTATUS (wait_status) != 0)
    exit_status = WEXITSTATUS (wait_status);

  exit (exit_status);
}

/* Set ARCHIVE for uncompressing, then reading an archive.  */
pid_t
sys_child_open_for_uncompress (void)
{
  int parent_pipe[2];
  int child_pipe[2];
  pid_t grandchild_pid;
  pid_t child_pid;
  int wait_status;

  xpipe (parent_pipe);
  child_pid = xfork ();

  if (child_pid > 0)
    {
      /* The parent tar is still here!  Just clean up.  */

      read_full_records_option = true;
      archive = parent_pipe[PREAD];
      xclose (parent_pipe[PWRITE]);
      return child_pid;
    }

  /* The newborn child tar is here!  */

  program_name = _("tar (child)");

  xdup2 (parent_pipe[PWRITE], STDOUT_FILENO);
  xclose (parent_pipe[PREAD]);

  /* Check if we need a grandchild tar.  This happens only if either:
     a) we're reading stdin: to force unblocking;
     b) the file is to be accessed by rmt: compressor doesn't know how;
     c) the file is not a plain file.  */

  if (strcmp (archive_name_array[0], "-") != 0
      && !_remdev (archive_name_array[0])
      && is_regular_file (archive_name_array[0]))
    {
      /* We don't need a grandchild tar.  Open the archive and lauch the
	 uncompressor.  */

      archive = open (archive_name_array[0], O_RDONLY | O_BINARY, MODE_RW);
      if (archive < 0)
	open_fatal (archive_name_array[0]);
      xdup2 (archive, STDIN_FILENO);
      execlp (use_compress_program_option, use_compress_program_option,
	      "-d", (char *) 0);
      exec_fatal (use_compress_program_option);
    }

  /* We do need a grandchild tar.  */

  xpipe (child_pipe);
  grandchild_pid = xfork ();

  if (grandchild_pid == 0)
    {
      /* The newborn grandchild tar is here!  Launch the uncompressor.  */

      program_name = _("tar (grandchild)");

      xdup2 (child_pipe[PREAD], STDIN_FILENO);
      xclose (child_pipe[PWRITE]);
      execlp (use_compress_program_option, use_compress_program_option,
	      "-d", (char *) 0);
      exec_fatal (use_compress_program_option);
    }

  /* The child tar is still here!  */

  /* Prepare for unblocking the data from the archive into the
     uncompressor.  */

  xdup2 (child_pipe[PWRITE], STDOUT_FILENO);
  xclose (child_pipe[PREAD]);

  if (strcmp (archive_name_array[0], "-") == 0)
    archive = STDIN_FILENO;
  else
    archive = rmtopen (archive_name_array[0], O_RDONLY | O_BINARY,
		       MODE_RW, rsh_command_option);
  if (archive < 0)
    open_fatal (archive_name_array[0]);

  /* Let's read the archive and pipe it into stdout.  */

  while (1)
    {
      char *cursor;
      size_t maximum;
      size_t count;
      ssize_t status;

      clear_read_error_count ();

    error_loop:
      status = rmtread (archive, record_start->buffer, record_size);
      if (status < 0)
	{
	  archive_read_error ();
	  goto error_loop;
	}
      if (status == 0)
	break;
      cursor = record_start->buffer;
      maximum = status;
      while (maximum)
	{
	  count = maximum < BLOCKSIZE ? maximum : BLOCKSIZE;
	  if (full_write (STDOUT_FILENO, cursor, count) != count)
	    write_error (use_compress_program_option);
	  cursor += count;
	  maximum -= count;
	}
    }

  xclose (STDOUT_FILENO);

  /* Propagate any failure of the grandchild back to the parent.  */

  while (waitpid (grandchild_pid, &wait_status, 0) == -1)
    if (errno != EINTR)
      {
	waitpid_error (use_compress_program_option);
	break;
      }

  if (WIFSIGNALED (wait_status))
    {
      kill (child_pid, WTERMSIG (wait_status));
      exit_status = TAREXIT_FAILURE;
    }
  else if (WEXITSTATUS (wait_status) != 0)
    exit_status = WEXITSTATUS (wait_status);

  exit (exit_status);
}

#endif /* not MSDOS */

