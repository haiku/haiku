/* System-dependent calls for tar.

   Copyright (C) 2003, 2004, 2005, 2006, 2007 Free Software Foundation, Inc.

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
#include <setenv.h>

#include "common.h"
#include <rmt.h>
#include <signal.h>

#if MSDOS

bool
sys_get_archive_stat (void)
{
  return 0;
}

bool
sys_file_is_archive (struct tar_stat_info *p)
{
  return false;
}

void
sys_save_archive_dev_ino (void)
{
}

void
sys_detect_dev_null_output (void)
{
  static char const dev_null[] = "nul";

  dev_null_output = (strcmp (archive_name_array[0], dev_null) == 0
		     || (! _isrmt (archive)));
}

void
sys_drain_input_pipe (void)
{
}

void
sys_wait_for_child (pid_t child_pid)
{
}

void
sys_spawn_shell (void)
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

size_t
sys_write_archive_buffer (void)
{
  return full_write (archive, record_start->buffer, record_size);
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
sys_get_archive_stat (void)
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
sys_save_archive_dev_ino (void)
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
sys_detect_dev_null_output (void)
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
sys_drain_input_pipe (void)
{
  size_t r;

  if (access_mode == ACCESS_READ
      && ! _isrmt (archive)
      && (S_ISFIFO (archive_stat.st_mode) || S_ISSOCK (archive_stat.st_mode)))
    while ((r = rmtread (archive, record_start->buffer, record_size)) != 0
	   && r != SAFE_READ_ERROR)
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
sys_spawn_shell (void)
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

size_t
sys_write_archive_buffer (void)
{
  return rmtwrite (archive, record_start->buffer, record_size);
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
     a) the file is to be accessed by rmt: compressor doesn't know how;
     b) the file is not a plain file.  */

  if (!_remdev (archive_name_array[0])
      && is_regular_file (archive_name_array[0]))
    {
      if (backup_option)
	maybe_backup_file (archive_name_array[0], 1);

      /* We don't need a grandchild tar.  Open the archive and launch the
	 compressor.  */
      if (strcmp (archive_name_array[0], "-"))
	{
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
	}
      execlp (use_compress_program_option, use_compress_program_option, NULL);
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
      size_t status = 0;
      char *cursor;
      size_t length;

      /* Assemble a record.  */

      for (length = 0, cursor = record_start->buffer;
	   length < record_size;
	   length += status, cursor += status)
	{
	  size_t size = record_size - length;

	  status = safe_read (STDIN_FILENO, cursor, size);
	  if (status == SAFE_READ_ERROR)
	    read_fatal (use_compress_program_option);
	  if (status == 0)
	    break;
	}

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
      size_t status;

      clear_read_error_count ();

    error_loop:
      status = rmtread (archive, record_start->buffer, record_size);
      if (status == SAFE_READ_ERROR)
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



static void
dec_to_env (char *envar, uintmax_t num)
{
  char buf[UINTMAX_STRSIZE_BOUND];
  char *numstr;

  numstr = STRINGIFY_BIGINT (num, buf);
  if (setenv (envar, numstr, 1) != 0)
    xalloc_die ();
}

static void
time_to_env (char *envar, struct timespec t)
{
  char buf[TIMESPEC_STRSIZE_BOUND];
  if (setenv (envar, code_timespec (t, buf), 1) != 0)
    xalloc_die ();
}

static void
oct_to_env (char *envar, unsigned long num)
{
  char buf[1+1+(sizeof(unsigned long)*CHAR_BIT+2)/3];

  snprintf (buf, sizeof buf, "0%lo", num);
  if (setenv (envar, buf, 1) != 0)
    xalloc_die ();
}

static void
str_to_env (char *envar, char const *str)
{
  if (str)
    {
      if (setenv (envar, str, 1) != 0)
	xalloc_die ();
    }
  else
    unsetenv (envar);
}

static void
chr_to_env (char *envar, char c)
{
  char buf[2];
  buf[0] = c;
  buf[1] = 0;
  if (setenv (envar, buf, 1) != 0)
    xalloc_die ();
}

static void
stat_to_env (char *name, char type, struct tar_stat_info *st)
{
  str_to_env ("TAR_VERSION", PACKAGE_VERSION);
  chr_to_env ("TAR_FILETYPE", type);
  oct_to_env ("TAR_MODE", st->stat.st_mode);
  str_to_env ("TAR_FILENAME", name);
  str_to_env ("TAR_REALNAME", st->file_name);
  str_to_env ("TAR_UNAME", st->uname);
  str_to_env ("TAR_GNAME", st->gname);
  time_to_env ("TAR_ATIME", st->atime);
  time_to_env ("TAR_MTIME", st->mtime);
  time_to_env ("TAR_CTIME", st->ctime);
  dec_to_env ("TAR_SIZE", st->stat.st_size);
  dec_to_env ("TAR_UID", st->stat.st_uid);
  dec_to_env ("TAR_GID", st->stat.st_gid);

  switch (type)
    {
    case 'b':
    case 'c':
      dec_to_env ("TAR_MINOR", minor (st->stat.st_rdev));
      dec_to_env ("TAR_MAJOR", major (st->stat.st_rdev));
      unsetenv ("TAR_LINKNAME");
      break;

    case 'l':
    case 'h':
      unsetenv ("TAR_MINOR");
      unsetenv ("TAR_MAJOR");
      str_to_env ("TAR_LINKNAME", st->link_name);
      break;

    default:
      unsetenv ("TAR_MINOR");
      unsetenv ("TAR_MAJOR");
      unsetenv ("TAR_LINKNAME");
      break;
    }
}

static pid_t global_pid;
static RETSIGTYPE (*pipe_handler) (int sig);

int
sys_exec_command (char *file_name, int typechar, struct tar_stat_info *st)
{
  int p[2];
  char *argv[4];

  xpipe (p);
  pipe_handler = signal (SIGPIPE, SIG_IGN);
  global_pid = xfork ();

  if (global_pid != 0)
    {
      xclose (p[PREAD]);
      return p[PWRITE];
    }

  /* Child */
  xdup2 (p[PREAD], STDIN_FILENO);
  xclose (p[PWRITE]);

  stat_to_env (file_name, typechar, st);

  argv[0] = "/bin/sh";
  argv[1] = "-c";
  argv[2] = to_command_option;
  argv[3] = NULL;

  execv ("/bin/sh", argv);

  exec_fatal (file_name);
}

void
sys_wait_command (void)
{
  int status;

  if (global_pid < 0)
    return;

  signal (SIGPIPE, pipe_handler);
  while (waitpid (global_pid, &status, 0) == -1)
    if (errno != EINTR)
      {
        global_pid = -1;
        waitpid_error (to_command_option);
        return;
      }

  if (WIFEXITED (status))
    {
      if (!ignore_command_error_option && WEXITSTATUS (status))
	ERROR ((0, 0, _("%lu: Child returned status %d"),
		(unsigned long) global_pid, WEXITSTATUS (status)));
    }
  else if (WIFSIGNALED (status))
    {
      WARN ((0, 0, _("%lu: Child terminated on signal %d"),
	     (unsigned long) global_pid, WTERMSIG (status)));
    }
  else
    ERROR ((0, 0, _("%lu: Child terminated on unknown reason"),
	    (unsigned long) global_pid));

  global_pid = -1;
}

int
sys_exec_info_script (const char **archive_name, int volume_number)
{
  pid_t pid;
  char *argv[4];
  char uintbuf[UINTMAX_STRSIZE_BOUND];
  int p[2];

  xpipe (p);
  pipe_handler = signal (SIGPIPE, SIG_IGN);

  pid = xfork ();

  if (pid != 0)
    {
      /* Master */

      int rc;
      int status;
      char *buf;
      size_t size = 0;
      FILE *fp;

      xclose (p[PWRITE]);
      fp = fdopen (p[PREAD], "r");
      rc = getline (&buf, &size, fp);
      fclose (fp);

      if (rc > 0 && buf[rc-1] == '\n')
	buf[--rc] = 0;

      while (waitpid (pid, &status, 0) == -1)
	if (errno != EINTR)
	  {
	    waitpid_error (info_script_option);
	    return -1;
	  }

      if (WIFEXITED (status))
	{
	  if (WEXITSTATUS (status) == 0 && rc > 0)
	    *archive_name = buf;
	  else
	    free (buf);
	  return WEXITSTATUS (status);
	}

      free (buf);
      return -1;
    }

  /* Child */
  setenv ("TAR_VERSION", PACKAGE_VERSION, 1);
  setenv ("TAR_ARCHIVE", *archive_name, 1);
  setenv ("TAR_VOLUME", STRINGIFY_BIGINT (volume_number, uintbuf), 1);
  setenv ("TAR_SUBCOMMAND", subcommand_string (subcommand_option), 1);
  setenv ("TAR_FORMAT",
	  archive_format_string (current_format == DEFAULT_FORMAT ?
				 archive_format : current_format), 1);
  setenv ("TAR_FD", STRINGIFY_BIGINT (p[PWRITE], uintbuf), 1);

  xclose (p[PREAD]);

  argv[0] = "/bin/sh";
  argv[1] = "-c";
  argv[2] = (char*) info_script_option;
  argv[3] = NULL;

  execv (argv[0], argv);

  exec_fatal (info_script_option);
}


#endif /* not MSDOS */
