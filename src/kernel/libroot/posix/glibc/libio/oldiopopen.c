/* Copyright (C) 1998, 1999, 2000, 2001, 2002 Free Software Foundation, Inc.
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

#define _IO_USE_OLD_IO_FILE
#ifndef _POSIX_SOURCE
# define _POSIX_SOURCE
#endif
#include "libioP.h"
#if _IO_HAVE_SYS_WAIT
#include <signal.h>
#include <unistd.h>
#ifdef __STDC__
#include <stdlib.h>
#endif
#ifdef _LIBC
# include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/wait.h>

#ifndef _IO_fork
#ifdef _LIBC
#define _IO_fork __vfork
#else
#define _IO_fork vfork /* defined in libiberty, if needed */
#endif
extern _IO_pid_t _IO_fork __P ((void));
#endif

#endif /* _IO_HAVE_SYS_WAIT */

#include <shlib-compat.h>
#if SHLIB_COMPAT (libc, GLIBC_2_0, GLIBC_2_1)

#ifndef _IO_pipe
#ifdef _LIBC
#define _IO_pipe __pipe
#else
#define _IO_pipe pipe
#endif
extern int _IO_pipe __P ((int des[2]));
#endif

#ifndef _IO_dup2
#ifdef _LIBC
#define _IO_dup2 __dup2
#else
#define _IO_dup2 dup2
#endif
extern int _IO_dup2 __P ((int fd, int fd2));
#endif

#ifndef _IO_waitpid
#ifdef _LIBC
#define _IO_waitpid __waitpid
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
#define _IO_close __close
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

static struct _IO_proc_file *old_proc_file_chain;

#ifdef _IO_MTSAFE_IO
static _IO_lock_t proc_file_chain_lock = _IO_lock_initializer;

static void
unlock (void *not_used)
{
  _IO_lock_unlock (proc_file_chain_lock);
}
#endif

_IO_FILE *
_IO_old_proc_open (fp, command, mode)
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
  if (_IO_pipe (pipe_fds) < 0)
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
  ((_IO_proc_file *) fp)->pid = child_pid = _IO_fork ();
  if (child_pid == 0)
    {
      int child_std_end = mode[0] == 'r' ? 1 : 0;
      struct _IO_proc_file *p;

      _IO_close (parent_end);
      if (child_end != child_std_end)
	{
	  _IO_dup2 (child_end, child_std_end);
	  _IO_close (child_end);
	}
      /* POSIX.2:  "popen() shall ensure that any streams from previous
         popen() calls that remain open in the parent process are closed
	 in the new child process." */
      for (p = old_proc_file_chain; p; p = p->next)
	_IO_close (_IO_fileno ((_IO_FILE *) p));

      _IO_execl ("/bin/sh", "sh", "-c", command, (char *) 0);
      _IO__exit (127);
    }
  _IO_close (child_end);
  if (child_pid < 0)
    {
      _IO_close (parent_end);
      return NULL;
    }
  _IO_fileno (fp) = parent_end;

  /* Link into old_proc_file_chain. */
#ifdef _IO_MTSAFE_IO
  _IO_cleanup_region_start_noarg (unlock);
  _IO_lock_lock (proc_file_chain_lock);
#endif
  ((_IO_proc_file *) fp)->next = old_proc_file_chain;
  old_proc_file_chain = (_IO_proc_file *) fp;
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
_IO_old_popen (command, mode)
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
  INTUSE(_IO_init) (fp, 0);
  _IO_JUMPS (&new_f->fpx.file) = &_IO_old_proc_jumps;
  _IO_old_file_init (&new_f->fpx.file);
#if  !_IO_UNIFIED_JUMPTABLES
  new_f->fpx.file.vtable = NULL;
#endif
  if (_IO_old_proc_open (fp, command, mode) != NULL)
    return fp;
  INTUSE(_IO_un_link) (&new_f->fpx.file);
  free (new_f);
  return NULL;
}

int
_IO_old_proc_close (fp)
     _IO_FILE *fp;
{
  /* This is not name-space clean. FIXME! */
#if _IO_HAVE_SYS_WAIT
  int wstatus;
  _IO_proc_file **ptr = &old_proc_file_chain;
  _IO_pid_t wait_pid;
  int status = -1;

  /* Unlink from old_proc_file_chain. */
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
#else /* !_IO_HAVE_SYS_WAIT */
  return -1;
#endif
}

struct _IO_jump_t _IO_old_proc_jumps = {
  JUMP_INIT_DUMMY,
  JUMP_INIT(finish, _IO_old_file_finish),
  JUMP_INIT(overflow, _IO_old_file_overflow),
  JUMP_INIT(underflow, _IO_old_file_underflow),
  JUMP_INIT(uflow, INTUSE(_IO_default_uflow)),
  JUMP_INIT(pbackfail, INTUSE(_IO_default_pbackfail)),
  JUMP_INIT(xsputn, _IO_old_file_xsputn),
  JUMP_INIT(xsgetn, INTUSE(_IO_default_xsgetn)),
  JUMP_INIT(seekoff, _IO_old_file_seekoff),
  JUMP_INIT(seekpos, _IO_default_seekpos),
  JUMP_INIT(setbuf, _IO_old_file_setbuf),
  JUMP_INIT(sync, _IO_old_file_sync),
  JUMP_INIT(doallocate, INTUSE(_IO_file_doallocate)),
  JUMP_INIT(read, INTUSE(_IO_file_read)),
  JUMP_INIT(write, _IO_old_file_write),
  JUMP_INIT(seek, INTUSE(_IO_file_seek)),
  JUMP_INIT(close, _IO_old_proc_close),
  JUMP_INIT(stat, INTUSE(_IO_file_stat)),
  JUMP_INIT(showmanyc, _IO_default_showmanyc),
  JUMP_INIT(imbue, _IO_default_imbue)
};

strong_alias (_IO_old_popen, __old_popen)
compat_symbol (libc, _IO_old_popen, _IO_popen, GLIBC_2_0);
compat_symbol (libc, __old_popen, popen, GLIBC_2_0);
compat_symbol (libc, _IO_old_proc_open, _IO_proc_open, GLIBC_2_0);
compat_symbol (libc, _IO_old_proc_close, _IO_proc_close, GLIBC_2_0);

#endif
