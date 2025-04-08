/* Copyright (C) 1993-2014 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Written by Per Bothner <bothner@cygnus.com>.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.

   As a special exception, if you link the code in this file with
   files compiled with a GNU compiler to produce an executable,
   that does not cause the resulting executable to be covered by
   the GNU Lesser General Public License.  This exception does not
   however invalidate any other reasons why the executable file
   might be covered by the GNU Lesser General Public License.
   This exception applies to code released by its copyright holders
   in files containing the exception.  */

#ifndef _POSIX_SOURCE
# define _POSIX_SOURCE
#endif
#include "libioP.h"
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#ifdef _LIBC
# include <unistd.h>
# include <shlib-compat.h>
# include <not-cancel.h>
#endif
#include <sys/types.h>
#include <sys/wait.h>

#ifndef __HAIKU__
#include <kernel-features.h>
#else
#define __ASSUME_PIPE2
#endif

#ifndef _IO_fork
#define _IO_fork fork
extern _IO_pid_t _IO_fork (void) __THROW;
#endif

#ifndef _IO_dup2
#define _IO_dup2 dup2
extern int _IO_dup2 (int fd, int fd2) __THROW;
#endif

#ifndef _IO_waitpid
#ifdef _LIBC
#define _IO_waitpid waitpid_not_cancel
#else
#define _IO_waitpid waitpid
#endif
#endif

#ifndef _IO_execl
#define _IO_execl execl
#endif
#ifndef _IO__exit
#define _IO__exit _exit
#endif

#ifndef _IO_close
#ifdef _LIBC
#define _IO_close close_not_cancel
#else
#define _IO_close close
#endif
#endif

struct _IO_proc_file
{
  struct _IO_FILE_plus file;
  /* Following fields must match those in class procbuf (procbuf.h) */
  _IO_pid_t pid;
  struct _IO_proc_file *next;
};
typedef struct _IO_proc_file _IO_proc_file;

static const struct _IO_jump_t _IO_proc_jumps;

static struct _IO_proc_file *proc_file_chain;

#ifdef _IO_MTSAFE_IO
static _IO_lock_t proc_file_chain_lock = _IO_lock_initializer;

static void
unlock (void *not_used)
{
  _IO_lock_unlock (proc_file_chain_lock);
}
#endif

_IO_FILE *
_IO_new_proc_open (fp, command, mode)
     _IO_FILE *fp;
     const char *command;
     const char *mode;
{
  int read_or_write;
  int parent_end, child_end;
  int pipe_fds[2];
  _IO_pid_t child_pid;

  int do_read = 0;
  int do_write = 0;
  int do_cloexec = 0;
  while (*mode != '\0')
    switch (*mode++)
      {
      case 'r':
	do_read = 1;
	break;
      case 'w':
	do_write = 1;
	break;
      case 'e':
	do_cloexec = 1;
	break;
      default:
      errout:
	__set_errno (EINVAL);
	return NULL;
      }

  if ((do_read ^ do_write) == 0)
    goto errout;

  if (_IO_file_is_open (fp))
    return NULL;

#ifdef O_CLOEXEC
# ifndef __ASSUME_PIPE2
  if (__have_pipe2 >= 0)
# endif
    {
      int r = __pipe2 (pipe_fds, O_CLOEXEC);
# ifndef __ASSUME_PIPE2
      if (__have_pipe2 == 0)
	__have_pipe2 = r != -1 || errno != ENOSYS ? 1 : -1;

      if (__have_pipe2 > 0)
# endif
	if (r < 0)
	  return NULL;
    }
#endif
#ifndef __ASSUME_PIPE2
# ifdef O_CLOEXEC
  if (__have_pipe2 < 0)
# endif
    if (__pipe (pipe_fds) < 0)
      return NULL;
#endif

  if (do_read)
    {
      parent_end = pipe_fds[0];
      child_end = pipe_fds[1];
      read_or_write = _IO_NO_WRITES;
    }
  else
    {
      parent_end = pipe_fds[1];
      child_end = pipe_fds[0];
      read_or_write = _IO_NO_READS;
    }

  ((_IO_proc_file *) fp)->pid = child_pid = _IO_fork ();
  if (child_pid == 0)
    {
      int child_std_end = do_read ? 1 : 0;
      struct _IO_proc_file *p;

#ifndef __ASSUME_PIPE2
      /* If we have pipe2 the descriptor is marked for close-on-exec.  */
      _IO_close (parent_end);
#endif
      if (child_end != child_std_end)
	{
	  _IO_dup2 (child_end, child_std_end);
#ifndef __ASSUME_PIPE2
	  _IO_close (child_end);
#endif
	}
#ifdef O_CLOEXEC
      else
	{
	  /* The descriptor is already the one we will use.  But it must
	     not be marked close-on-exec.  Undo the effects.  */
# ifndef __ASSUME_PIPE2
	  if (__have_pipe2 > 0)
# endif
	    __fcntl (child_end, F_SETFD, 0);
	}
#endif
      /* POSIX.2:  "popen() shall ensure that any streams from previous
         popen() calls that remain open in the parent process are closed
	 in the new child process." */
      for (p = proc_file_chain; p; p = p->next)
	{
	  int fd = _IO_fileno ((_IO_FILE *) p);

	  /* If any stream from previous popen() calls has fileno
	     child_std_end, it has been already closed by the dup2 syscall
	     above.  */
	  if (fd != child_std_end)
	    _IO_close (fd);
	}

      _IO_execl ("/bin/sh", "sh", "-c", command, (char *) 0);
      _IO__exit (127);
    }
  _IO_close (child_end);
  if (child_pid < 0)
    {
      _IO_close (parent_end);
      return NULL;
    }

  if (do_cloexec)
    {
#ifndef __ASSUME_PIPE2
# ifdef O_CLOEXEC
      if (__have_pipe2 < 0)
# endif
	__fcntl (parent_end, F_SETFD, FD_CLOEXEC);
#endif
    }
  else
    {
#ifdef O_CLOEXEC
      /* Undo the effects of the pipe2 call which set the
	 close-on-exec flag.  */
# ifndef __ASSUME_PIPE2
      if (__have_pipe2 > 0)
# endif
	__fcntl (parent_end, F_SETFD, 0);
#endif
    }

  _IO_fileno (fp) = parent_end;

  /* Link into proc_file_chain. */
#ifdef _IO_MTSAFE_IO
  _IO_cleanup_region_start_noarg (unlock);
  _IO_lock_lock (proc_file_chain_lock);
#endif
  ((_IO_proc_file *) fp)->next = proc_file_chain;
  proc_file_chain = (_IO_proc_file *) fp;
#ifdef _IO_MTSAFE_IO
  _IO_lock_unlock (proc_file_chain_lock);
  _IO_cleanup_region_end (0);
#endif

  _IO_mask_flags (fp, read_or_write, _IO_NO_READS|_IO_NO_WRITES);
  return fp;
}

_IO_FILE *
_IO_new_popen (command, mode)
     const char *command;
     const char *mode;
{
  struct locked_FILE
  {
    struct _IO_proc_file fpx;
#ifdef _IO_MTSAFE_IO
    _IO_lock_t lock;
#endif
  } *new_f;
  _IO_FILE *fp;

  new_f = (struct locked_FILE *) malloc (sizeof (struct locked_FILE));
  if (new_f == NULL)
    return NULL;
#ifdef _IO_MTSAFE_IO
  new_f->fpx.file.file._lock = &new_f->lock;
#endif
  fp = &new_f->fpx.file.file;
  _IO_init (fp, 0);
  _IO_JUMPS (&new_f->fpx.file) = &_IO_proc_jumps;
  _IO_new_file_init (&new_f->fpx.file);
#if  !_IO_UNIFIED_JUMPTABLES
  new_f->fpx.file.vtable = NULL;
#endif
  if (_IO_new_proc_open (fp, command, mode) != NULL)
    return (_IO_FILE *) &new_f->fpx.file;
  _IO_un_link (&new_f->fpx.file);
  free (new_f);
  return NULL;
}

int
_IO_new_proc_close (fp)
     _IO_FILE *fp;
{
  /* This is not name-space clean. FIXME! */
  int wstatus;
  _IO_proc_file **ptr = &proc_file_chain;
  _IO_pid_t wait_pid;
  int status = -1;

  /* Unlink from proc_file_chain. */
#ifdef _IO_MTSAFE_IO
  _IO_cleanup_region_start_noarg (unlock);
  _IO_lock_lock (proc_file_chain_lock);
#endif
  for ( ; *ptr != NULL; ptr = &(*ptr)->next)
    {
      if (*ptr == (_IO_proc_file *) fp)
	{
	  *ptr = (*ptr)->next;
	  status = 0;
	  break;
	}
    }
#ifdef _IO_MTSAFE_IO
  _IO_lock_unlock (proc_file_chain_lock);
  _IO_cleanup_region_end (0);
#endif

  if (status < 0 || _IO_close (_IO_fileno(fp)) < 0)
    return -1;
  /* POSIX.2 Rationale:  "Some historical implementations either block
     or ignore the signals SIGINT, SIGQUIT, and SIGHUP while waiting
     for the child process to terminate.  Since this behavior is not
     described in POSIX.2, such implementations are not conforming." */
  do
    {
      wait_pid = _IO_waitpid (((_IO_proc_file *) fp)->pid, &wstatus, 0);
    }
  while (wait_pid == -1 && errno == EINTR);
  if (wait_pid == -1)
    return -1;
  return wstatus;
}

static const struct _IO_jump_t _IO_proc_jumps = {
  JUMP_INIT_DUMMY,
  JUMP_INIT(finish, _IO_new_file_finish),
  JUMP_INIT(overflow, _IO_new_file_overflow),
  JUMP_INIT(underflow, _IO_new_file_underflow),
  JUMP_INIT(uflow, _IO_default_uflow),
  JUMP_INIT(pbackfail, _IO_default_pbackfail),
  JUMP_INIT(xsputn, _IO_new_file_xsputn),
  JUMP_INIT(xsgetn, _IO_default_xsgetn),
  JUMP_INIT(seekoff, _IO_new_file_seekoff),
  JUMP_INIT(seekpos, _IO_default_seekpos),
  JUMP_INIT(setbuf, _IO_new_file_setbuf),
  JUMP_INIT(sync, _IO_new_file_sync),
  JUMP_INIT(doallocate, _IO_file_doallocate),
  JUMP_INIT(read, _IO_file_read),
  JUMP_INIT(write, _IO_new_file_write),
  JUMP_INIT(seek, _IO_file_seek),
  JUMP_INIT(close, _IO_new_proc_close),
  JUMP_INIT(stat, _IO_file_stat),
  JUMP_INIT(showmanyc, _IO_default_showmanyc),
  JUMP_INIT(imbue, _IO_default_imbue)
};

strong_alias (_IO_new_popen, __new_popen)
versioned_symbol (libc, _IO_new_popen, _IO_popen, GLIBC_2_1);
versioned_symbol (libc, __new_popen, popen, GLIBC_2_1);
versioned_symbol (libc, _IO_new_proc_open, _IO_proc_open, GLIBC_2_1);
versioned_symbol (libc, _IO_new_proc_close, _IO_proc_close, GLIBC_2_1);
