/* Copyright (C) 1993, 1997-2002, 2003 Free Software Foundation, Inc.
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
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.

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

#include <sys/types.h>
#include <sys/wait.h>
#include "libioP.h"
#if _IO_HAVE_SYS_WAIT
#include <signal.h>
#include <unistd.h>
#ifdef __STDC__
#include <stdlib.h>
#endif
#ifdef _LIBC
# include <unistd.h>
# include <shlib-compat.h>
#endif

#endif /* _IO_HAVE_SYS_WAIT */

#ifndef _IO_close
#	define _IO_close close
#endif

struct _IO_proc_file
{
  struct _IO_FILE_plus file;
  /* Following fields must match those in class procbuf (procbuf.h) */
  _IO_pid_t pid;
  struct _IO_proc_file *next;
};
typedef struct _IO_proc_file _IO_proc_file;

static struct _IO_jump_t _IO_proc_jumps;
static struct _IO_jump_t _IO_wproc_jumps;

static struct _IO_proc_file *proc_file_chain;

#ifdef _IO_MTSAFE_IO
static _IO_lock_t proc_file_chain_lock = _IO_lock_initializer;

#if 0
static void
unlock (void *not_used)
{
  _IO_lock_unlock (proc_file_chain_lock);
}
#endif
#endif

_IO_FILE *
_IO_new_proc_open (fp, command, mode)
     _IO_FILE *fp;
     const char *command;
     const char *mode;
{
#if _IO_HAVE_SYS_WAIT
  volatile int read_or_write;
  volatile int parent_end, child_end;
  int pipe_fds[2];
  _IO_pid_t child_pid;
  if (_IO_file_is_open (fp))
    return NULL;
  if (pipe(pipe_fds) < 0)
    return NULL;
  if (mode[0] == 'r' && mode[1] == '\0')
    {
      parent_end = pipe_fds[0];
      child_end = pipe_fds[1];
      read_or_write = _IO_NO_WRITES;
    }
  else if (mode[0] == 'w' && mode[1] == '\0')
    {
      parent_end = pipe_fds[1];
      child_end = pipe_fds[0];
      read_or_write = _IO_NO_READS;
    }
  else
    {
      _IO_close (pipe_fds[0]);
      _IO_close (pipe_fds[1]);
      __set_errno (EINVAL);
      return NULL;
    }
  ((_IO_proc_file *) fp)->pid = child_pid = fork();
  if (child_pid == 0)
    {
      int child_std_end = mode[0] == 'r' ? 1 : 0;
      struct _IO_proc_file *p;

      _IO_close (parent_end);
      if (child_end != child_std_end)
	{
	  dup2(child_end, child_std_end);
	  _IO_close (child_end);
	}
      /* POSIX.2:  "popen() shall ensure that any streams from previous
         popen() calls that remain open in the parent process are closed
	 in the new child process." */
      for (p = proc_file_chain; p; p = p->next)
	_IO_close (_IO_fileno ((_IO_FILE *) p));

      execl("/bin/sh", "sh", "-c", command, (char *)0);
      _exit(127);
    }
  _IO_close (child_end);
  if (child_pid < 0)
    {
      _IO_close (parent_end);
      return NULL;
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
#else /* !_IO_HAVE_SYS_WAIT */
  return NULL;
#endif
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
    struct _IO_wide_data wd;
  } *new_f;
  _IO_FILE *fp;

  new_f = (struct locked_FILE *) malloc (sizeof (struct locked_FILE));
  if (new_f == NULL)
    return NULL;
#ifdef _IO_MTSAFE_IO
  new_f->fpx.file.file._lock = &new_f->lock;
#endif
  fp = &new_f->fpx.file.file;
  _IO_no_init (fp, 0, 0, &new_f->wd, &_IO_wproc_jumps);
  _IO_JUMPS (&new_f->fpx.file) = &_IO_proc_jumps;
  _IO_new_file_init (&new_f->fpx.file);
#if  !_IO_UNIFIED_JUMPTABLES
  new_f->fpx.file.vtable = NULL;
#endif
  if (_IO_new_proc_open (fp, command, mode) != NULL)
    return (_IO_FILE *) &new_f->fpx.file;
  INTUSE(_IO_un_link) (&new_f->fpx.file);
  free (new_f);
  return NULL;
}

int
_IO_new_proc_close (fp)
     _IO_FILE *fp;
{
  /* This is not name-space clean. FIXME! */
#if _IO_HAVE_SYS_WAIT
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
      wait_pid = waitpid(((_IO_proc_file *) fp)->pid, &wstatus, 0);
    }
  while (wait_pid == -1 && errno == EINTR);
  if (wait_pid == -1)
    return -1;
  return wstatus;
#else /* !_IO_HAVE_SYS_WAIT */
  return -1;
#endif
}

static struct _IO_jump_t _IO_proc_jumps = {
  JUMP_INIT_DUMMY,
  JUMP_INIT(finish, _IO_new_file_finish),
  JUMP_INIT(overflow, _IO_new_file_overflow),
  JUMP_INIT(underflow, _IO_new_file_underflow),
  JUMP_INIT(uflow, INTUSE(_IO_default_uflow)),
  JUMP_INIT(pbackfail, INTUSE(_IO_default_pbackfail)),
  JUMP_INIT(xsputn, _IO_new_file_xsputn),
  JUMP_INIT(xsgetn, INTUSE(_IO_default_xsgetn)),
  JUMP_INIT(seekoff, _IO_new_file_seekoff),
  JUMP_INIT(seekpos, _IO_default_seekpos),
  JUMP_INIT(setbuf, _IO_new_file_setbuf),
  JUMP_INIT(sync, _IO_new_file_sync),
  JUMP_INIT(doallocate, INTUSE(_IO_file_doallocate)),
  JUMP_INIT(read, INTUSE(_IO_file_read)),
  JUMP_INIT(write, _IO_new_file_write),
  JUMP_INIT(seek, INTUSE(_IO_file_seek)),
  JUMP_INIT(close, _IO_new_proc_close),
  JUMP_INIT(stat, INTUSE(_IO_file_stat)),
  JUMP_INIT(showmanyc, _IO_default_showmanyc),
  JUMP_INIT(imbue, _IO_default_imbue)
};

static struct _IO_jump_t _IO_wproc_jumps = {
  JUMP_INIT_DUMMY,
  JUMP_INIT(finish, _IO_new_file_finish),
  JUMP_INIT(overflow, _IO_new_file_overflow),
  JUMP_INIT(underflow, _IO_new_file_underflow),
  JUMP_INIT(uflow, INTUSE(_IO_default_uflow)),
  JUMP_INIT(pbackfail, INTUSE(_IO_default_pbackfail)),
  JUMP_INIT(xsputn, _IO_new_file_xsputn),
  JUMP_INIT(xsgetn, INTUSE(_IO_default_xsgetn)),
  JUMP_INIT(seekoff, _IO_new_file_seekoff),
  JUMP_INIT(seekpos, _IO_default_seekpos),
  JUMP_INIT(setbuf, _IO_new_file_setbuf),
  JUMP_INIT(sync, _IO_new_file_sync),
  JUMP_INIT(doallocate, INTUSE(_IO_file_doallocate)),
  JUMP_INIT(read, INTUSE(_IO_file_read)),
  JUMP_INIT(write, _IO_new_file_write),
  JUMP_INIT(seek, INTUSE(_IO_file_seek)),
  JUMP_INIT(close, _IO_new_proc_close),
  JUMP_INIT(stat, INTUSE(_IO_file_stat)),
  JUMP_INIT(showmanyc, _IO_default_showmanyc),
  JUMP_INIT(imbue, _IO_default_imbue)
};

strong_alias (_IO_new_popen, __new_popen)
versioned_symbol (libc, _IO_new_popen, _IO_popen, GLIBC_2_1);
versioned_symbol (libc, __new_popen, popen, GLIBC_2_1);
versioned_symbol (libc, _IO_new_proc_open, _IO_proc_open, GLIBC_2_1);
versioned_symbol (libc, _IO_new_proc_close, _IO_proc_close, GLIBC_2_1);
