/* Remote target system call support.
   Copyright 1997, 1998, 2002, 2004 Free Software Foundation, Inc.
   Contributed by Cygnus Solutions.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GAS; see the file COPYING.  If not, write to the Free Software
   Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* This interface isn't intended to be specific to any particular kind
   of remote (hardware, simulator, whatever).  As such, support for it
   (e.g. sim/common/callback.c) should *not* live in the simulator source
   tree, nor should it live in the gdb source tree.  K&R C must be
   supported.  */

#ifdef HAVE_CONFIG_H
#include "cconfig.h"
#endif
#include "ansidecl.h"
#include "libiberty.h"
#ifdef ANSI_PROTOTYPES
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "gdb/callback.h"
#include "targ-vals.h"

#ifndef ENOSYS
#define ENOSYS EINVAL
#endif
#ifndef ENAMETOOLONG
#define ENAMETOOLONG EINVAL
#endif

/* Maximum length of a path name.  */
#ifndef MAX_PATH_LEN
#define MAX_PATH_LEN 1024
#endif

/* When doing file read/writes, do this many bytes at a time.  */
#define FILE_XFR_SIZE 4096

/* FIXME: for now, need to consider target word size.  */
#define TWORD long
#define TADDR unsigned long

/* Utility of cb_syscall to fetch a path name or other string from the target.
   The result is 0 for success or a host errno value.  */

static int
get_string (cb, sc, buf, buflen, addr)
     host_callback *cb;
     CB_SYSCALL *sc;
     char *buf;
     int buflen;
     TADDR addr;
{
  char *p, *pend;

  for (p = buf, pend = buf + buflen; p < pend; ++p, ++addr)
    {
      /* No, it isn't expected that this would cause one transaction with
	 the remote target for each byte.  The target could send the
	 path name along with the syscall request, and cache the file
	 name somewhere (or otherwise tweak this as desired).  */
      unsigned int count = (*sc->read_mem) (cb, sc, addr, p, 1);
				    
      if (count != 1)
	return EINVAL;
      if (*p == 0)
	break;
    }
  if (p == pend)
    return ENAMETOOLONG;
  return 0;
}

/* Utility of cb_syscall to fetch a path name.
   The buffer is malloc'd and the address is stored in BUFP.
   The result is that of get_string.
   If an error occurs, no buffer is left malloc'd.  */

static int
get_path (cb, sc, addr, bufp)
     host_callback *cb;
     CB_SYSCALL *sc;
     TADDR addr;
     char **bufp;
{
  char *buf = xmalloc (MAX_PATH_LEN);
  int result;

  result = get_string (cb, sc, buf, MAX_PATH_LEN, addr);
  if (result == 0)
    *bufp = buf;
  else
    free (buf);
  return result;
}

/* Perform a system call on behalf of the target.  */

CB_RC
cb_syscall (cb, sc)
     host_callback *cb;
     CB_SYSCALL *sc;
{
  TWORD result = 0, errcode = 0;

  if (sc->magic != CB_SYSCALL_MAGIC)
    abort ();

  switch (cb_target_to_host_syscall (cb, sc->func))
    {
#if 0 /* FIXME: wip */
    case CB_SYS_argvlen :
      {
	/* Compute how much space is required to store the argv,envp
	   strings so that the program can allocate the space and then
	   call SYS_argv to fetch the values.  */
	int addr_size = cb->addr_size;
	int argc,envc,arglen,envlen;
	const char **argv = cb->init_argv;
	const char **envp = cb->init_envp;

	argc = arglen = 0;
	if (argv)
	  {
	    for ( ; argv[argc]; ++argc)
	      arglen += strlen (argv[argc]) + 1;
	  }
	envc = envlen = 0;
	if (envp)
	  {
	    for ( ; envp[envc]; ++envc)
	      envlen += strlen (envp[envc]) + 1;
	  }
	result = arglen + envlen;
	break;
      }

    case CB_SYS_argv :
      {
	/* Pointer to target's buffer.  */
	TADDR tbuf = sc->arg1;
	/* Buffer size.  */
	int bufsize = sc->arg2;
	/* Q is the target address of where all the strings go.  */
	TADDR q;
	int word_size = cb->word_size;
	int i,argc,envc,len;
	const char **argv = cb->init_argv;
	const char **envp = cb->init_envp;

	argc = 0;
	if (argv)
	  {
	    for ( ; argv[argc]; ++argc)
	      {
		int len = strlen (argv[argc]);
		int written = (*sc->write_mem) (cb, sc, tbuf, argv[argc], len + 1);
		if (written != len)
		  {
		    result = -1;
		    errcode = EINVAL;
		    goto FinishSyscall;
		  }
		tbuf = len + 1;
	      }
	  }
	if ((*sc->write_mem) (cb, sc, tbuf, "", 1) != 1)
	  {
	    result = -1;
	    errcode = EINVAL;
	    goto FinishSyscall;
	  }
	tbuf++;
	envc = 0;
	if (envp)
	  {
	    for ( ; envp[envc]; ++envc)
	      {
		int len = strlen (envp[envc]);
		int written = (*sc->write_mem) (cb, sc, tbuf, envp[envc], len + 1);
		if (written != len)
		  {
		    result = -1;
		    errcode = EINVAL;
		    goto FinishSyscall;
		  }
		tbuf = len + 1;
	      }
	  }
	if ((*sc->write_mem) (cb, sc, tbuf, "", 1) != 1)
	  {
	    result = -1;
	    errcode = EINVAL;
	    goto FinishSyscall;
	  }
	result = argc;
	sc->result2 = envc;
	break;
      }
#endif /* wip */

    case CB_SYS_exit :
      /* Caller must catch and handle.  */
      break;

    case CB_SYS_open :
      {
	char *path;

	errcode = get_path (cb, sc, sc->arg1, &path);
	if (errcode != 0)
	  {
	    result = -1;
	    goto FinishSyscall;
	  }
	result = (*cb->open) (cb, path, sc->arg2 /*, sc->arg3*/);
	free (path);
	if (result < 0)
	  goto ErrorFinish;
      }
      break;

    case CB_SYS_close :
      result = (*cb->close) (cb, sc->arg1);
      if (result < 0)
	goto ErrorFinish;
      break;

    case CB_SYS_read :
      {
	/* ??? Perfect handling of error conditions may require only one
	   call to cb->read.  One can't assume all the data is
	   contiguously stored in host memory so that would require
	   malloc'ing/free'ing the space.  Maybe later.  */
	char buf[FILE_XFR_SIZE];
	int fd = sc->arg1;
	TADDR addr = sc->arg2;
	size_t count = sc->arg3;
	size_t bytes_read = 0;
	int bytes_written;

	while (count > 0)
	  {
	    if (fd == 0)
	      result = (int) (*cb->read_stdin) (cb, buf,
						(count < FILE_XFR_SIZE
						 ? count : FILE_XFR_SIZE));
	    else
	      result = (int) (*cb->read) (cb, fd, buf,
					  (count < FILE_XFR_SIZE
					   ? count : FILE_XFR_SIZE));
	    if (result == -1)
	      goto ErrorFinish;
	    if (result == 0)	/* EOF */
	      break;
	    bytes_written = (*sc->write_mem) (cb, sc, addr, buf, result);
	    if (bytes_written != result)
	      {
		result = -1;
		errcode = EINVAL;
		goto FinishSyscall;
	      }
	    bytes_read += result;
	    count -= result;
	    addr += result;
	    /* If this is a short read, don't go back for more */
	    if (result != FILE_XFR_SIZE)
	      break;
	  }
	result = bytes_read;
      }
      break;

    case CB_SYS_write :
      {
	/* ??? Perfect handling of error conditions may require only one
	   call to cb->write.  One can't assume all the data is
	   contiguously stored in host memory so that would require
	   malloc'ing/free'ing the space.  Maybe later.  */
	char buf[FILE_XFR_SIZE];
	int fd = sc->arg1;
	TADDR addr = sc->arg2;
	size_t count = sc->arg3;
	int bytes_read;
	size_t bytes_written = 0;

	while (count > 0)
	  {
	    int bytes_to_read = count < FILE_XFR_SIZE ? count : FILE_XFR_SIZE;
	    bytes_read = (*sc->read_mem) (cb, sc, addr, buf, bytes_to_read);
	    if (bytes_read != bytes_to_read)
	      {
		result = -1;
		errcode = EINVAL;
		goto FinishSyscall;
	      }
	    if (fd == 1)
	      {
		result = (int) (*cb->write_stdout) (cb, buf, bytes_read);
		(*cb->flush_stdout) (cb);
	      }
	    else if (fd == 2)
	      {
		result = (int) (*cb->write_stderr) (cb, buf, bytes_read);
		(*cb->flush_stderr) (cb);
	      }
	    else
	      result = (int) (*cb->write) (cb, fd, buf, bytes_read);
	    if (result == -1)
	      goto ErrorFinish;
	    bytes_written += result;
	    count -= result;
	    addr += result;
	  }
	result = bytes_written;
      }
      break;

    case CB_SYS_lseek :
      {
	int fd = sc->arg1;
	unsigned long offset = sc->arg2;
	int whence = sc->arg3;

	result = (*cb->lseek) (cb, fd, offset, whence);
	if (result < 0)
	  goto ErrorFinish;
      }
      break;

    case CB_SYS_unlink :
      {
	char *path;

	errcode = get_path (cb, sc, sc->arg1, &path);
	if (errcode != 0)
	  {
	    result = -1;
	    goto FinishSyscall;
	  }
	result = (*cb->unlink) (cb, path);
	free (path);
	if (result < 0)
	  goto ErrorFinish;
      }
      break;

    case CB_SYS_stat :
      {
	char *path,*buf;
	int buflen;
	struct stat statbuf;
	TADDR addr = sc->arg2;

	errcode = get_path (cb, sc, sc->arg1, &path);
	if (errcode != 0)
	  {
	    result = -1;
	    goto FinishSyscall;
	  }
	result = (*cb->stat) (cb, path, &statbuf);
	free (path);
	if (result < 0)
	  goto ErrorFinish;
	buflen = cb_host_to_target_stat (cb, NULL, NULL);
	buf = xmalloc (buflen);
	if (cb_host_to_target_stat (cb, &statbuf, buf) != buflen)
	  {
	    /* The translation failed.  This is due to an internal
	       host program error, not the target's fault.  */
	    free (buf);
	    errcode = ENOSYS;
	    result = -1;
	    goto FinishSyscall;
	  }
	if ((*sc->write_mem) (cb, sc, addr, buf, buflen) != buflen)
	  {
	    free (buf);
	    errcode = EINVAL;
	    result = -1;
	    goto FinishSyscall;
	  }
	free (buf);
      }
      break;

    case CB_SYS_fstat :
      {
	char *buf;
	int buflen;
	struct stat statbuf;
	TADDR addr = sc->arg2;

	result = (*cb->fstat) (cb, sc->arg1, &statbuf);
	if (result < 0)
	  goto ErrorFinish;
	buflen = cb_host_to_target_stat (cb, NULL, NULL);
	buf = xmalloc (buflen);
	if (cb_host_to_target_stat (cb, &statbuf, buf) != buflen)
	  {
	    /* The translation failed.  This is due to an internal
	       host program error, not the target's fault.  */
	    free (buf);
	    errcode = ENOSYS;
	    result = -1;
	    goto FinishSyscall;
	  }
	if ((*sc->write_mem) (cb, sc, addr, buf, buflen) != buflen)
	  {
	    free (buf);
	    errcode = EINVAL;
	    result = -1;
	    goto FinishSyscall;
	  }
	free (buf);
      }
      break;

    case CB_SYS_time :
      {
	/* FIXME: May wish to change CB_SYS_time to something else.
	   We might also want gettimeofday or times, but if system calls
	   can be built on others, we can keep the number we have to support
	   here down.  */
	time_t t = (*cb->time) (cb, (time_t *) 0);
	result = t;
	/* It is up to target code to process the argument to time().  */
      }
      break;

    case CB_SYS_chdir :
    case CB_SYS_chmod :
    case CB_SYS_utime :
      /* fall through for now */

    default :
      result = -1;
      errcode = ENOSYS;
      break;
    }

 FinishSyscall:
  sc->result = result;
  if (errcode == 0)
    sc->errcode = 0;
  else
    sc->errcode = cb_host_to_target_errno (cb, errcode);
  return CB_RC_OK;

 ErrorFinish:
  sc->result = result;
  sc->errcode = (*cb->get_errno) (cb);
  return CB_RC_OK;
}
