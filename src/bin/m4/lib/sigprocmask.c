/* POSIX compatible signal blocking.
   Copyright (C) 2006 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2006.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#include <config.h>

/* Specification.  */
#include "sigprocmask.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

/* We assume that a platform without POSIX signal blocking functions also
   does not have the POSIX sigaction() function, only the signal() function.
   This is true for Woe32 platforms.  */

/* A signal handler.  */
typedef void (*handler_t) (int signal);

int
sigismember (const sigset_t *set, int sig)
{
  if (sig >= 0 && sig < NSIG)
    return (*set >> sig) & 1;
  else
    return 0;
}

int
sigemptyset (sigset_t *set)
{
  *set = 0;
  return 0;
}

int
sigaddset (sigset_t *set, int sig)
{
  if (sig >= 0 && sig < NSIG)
    {
      *set |= 1U << sig;
      return 0;
    }
  else
    {
      errno = EINVAL;
      return -1;
    }
}

int
sigdelset (sigset_t *set, int sig)
{
  if (sig >= 0 && sig < NSIG)
    {
      *set &= ~(1U << sig);
      return 0;
    }
  else
    {
      errno = EINVAL;
      return -1;
    }
}

int
sigfillset (sigset_t *set)
{
  *set = (2U << (NSIG - 1)) - 1;
  return 0;
}

/* Set of currently blocked signals.  */
static sigset_t blocked_set /* = 0 */;

/* Set of currently blocked and pending signals.  */
static volatile sig_atomic_t pending_array[NSIG] /* = { 0 } */;

/* Signal handler that is installed for blocked signals.  */
static void
blocked_handler (int sig)
{
  if (sig >= 0 && sig < NSIG)
    pending_array[sig] = 1;
}

int
sigpending (sigset_t *set)
{
  sigset_t pending = 0;
  int sig;

  for (sig = 0; sig < NSIG; sig++)
    if (pending_array[sig])
      pending |= 1U << sig;
  return pending;
}

/* The previous signal handlers.
   Only the array elements corresponding to blocked signals are relevant.  */
static handler_t old_handlers[NSIG];

int
sigprocmask (int operation, const sigset_t *set, sigset_t *old_set)
{
  if (old_set != NULL)
    *old_set = blocked_set;

  if (set != NULL)
    {
      sigset_t new_blocked_set;
      sigset_t to_unblock;
      sigset_t to_block;

      switch (operation)
	{
	case SIG_BLOCK:
	  new_blocked_set = blocked_set | *set;
	  break;
	case SIG_SETMASK:
	  new_blocked_set = *set;
	  break;
	case SIG_UNBLOCK:
	  new_blocked_set = blocked_set & ~*set;
	  break;
	default:
	  errno = EINVAL;
	  return -1;
	}
      to_unblock = blocked_set & ~new_blocked_set;
      to_block = new_blocked_set & ~blocked_set;

      if (to_block != 0)
	{
	  int sig;

	  for (sig = 0; sig < NSIG; sig++)
	    if ((to_block >> sig) & 1)
	      {
		pending_array[sig] = 0;
		if ((old_handlers[sig] = signal (sig, blocked_handler)) != SIG_ERR)
		  blocked_set |= 1U << sig;
	      }
	}

      if (to_unblock != 0)
	{
	  sig_atomic_t received[NSIG];
	  int sig;

	  for (sig = 0; sig < NSIG; sig++)
	    if ((to_unblock >> sig) & 1)
	      {
		if (signal (sig, old_handlers[sig]) != blocked_handler)
		  /* The application changed a signal handler while the signal
		     was blocked.  We don't support this.  */
		  abort ();
		received[sig] = pending_array[sig];
		blocked_set &= ~(1U << sig);
		pending_array[sig] = 0;
	      }
	    else
	      received[sig] = 0;

	  for (sig = 0; sig < NSIG; sig++)
	    if (received[sig])
	      {
		#if HAVE_RAISE
		raise (sig);
		#else
		kill (getpid (), sig);
		#endif
	      }
	}
    }
  return 0;
}
