/* Copyright (C) 1991, 1994-2000, 2001, 2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

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

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libintl.h>
#include <bits/libc-lock.h>


#ifndef	HAVE_GNU_LD
#define	_sys_siglist	sys_siglist
#endif

/* Defined in siglist.c.  */
extern const char *const _sys_siglist[];
extern const char *const _sys_siglist_internal[] attribute_hidden;
static __libc_key_t key;

/* If nonzero the key allocation failed and we should better use a
   static buffer than fail.  */
#define BUFFERSIZ	100
static char local_buf[BUFFERSIZ];
static char *static_buf;

/* Destructor for the thread-specific data.  */
static void init (void);
static void free_key_mem (void *mem);
static char *getbuffer (void);


/* Return a string describing the meaning of the signal number SIGNUM.  */
char *
strsignal (int signum)
{
  __libc_once_define (static, once);
  const char *desc;

  /* If we have not yet initialized the buffer do it now.  */
  __libc_once (once, init);

  if (
#ifdef SIGRTMIN
      (signum >= SIGRTMIN && signum <= SIGRTMAX) ||
#endif
      signum < 0 || signum >= NSIG
      || (desc = INTUSE(_sys_siglist)[signum]) == NULL)
    {
      char *buffer = getbuffer ();
      int len;
#ifdef SIGRTMIN
      if (signum >= SIGRTMIN && signum <= SIGRTMAX)
	len = __snprintf (buffer, BUFFERSIZ - 1, _("Real-time signal %d"),
			  signum - SIGRTMIN);
      else
#endif
	len = __snprintf (buffer, BUFFERSIZ - 1, _("Unknown signal %d"),
			  signum);
      if (len >= BUFFERSIZ)
	buffer = NULL;
      else
	buffer[len] = '\0';

      return buffer;
    }

  return (char *) _(desc);
}


/* Initialize buffer.  */
static void
init (void)
{
  if (__libc_key_create (&key, free_key_mem))
    /* Creating the key failed.  This means something really went
       wrong.  In any case use a static buffer which is better than
       nothing.  */
    static_buf = local_buf;
}


/* Free the thread specific data, this is done if a thread terminates.  */
static void
free_key_mem (void *mem)
{
  free (mem);
  __libc_setspecific (key, NULL);
}


/* Return the buffer to be used.  */
static char *
getbuffer (void)
{
  char *result;

  if (static_buf != NULL)
    result = static_buf;
  else
    {
      /* We don't use the static buffer and so we have a key.  Use it
	 to get the thread-specific buffer.  */
      result = __libc_getspecific (key);
      if (result == NULL)
	{
	  /* No buffer allocated so far.  */
	  result = malloc (BUFFERSIZ);
	  if (result == NULL)
	    /* No more memory available.  We use the static buffer.  */
	    result = local_buf;
	  else
	    __libc_setspecific (key, result);
	}
    }

  return result;
}
