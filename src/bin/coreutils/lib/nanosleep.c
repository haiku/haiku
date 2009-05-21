/* Provide a replacement for the POSIX nanosleep function.

   Copyright (C) 1999, 2000, 2002, 2004, 2005, 2006, 2007, 2008 Free
   Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* written by Jim Meyering */

#include <config.h>

#include <time.h>

#include "sig-handler.h"
#include "timespec.h"

#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/select.h>
#include <signal.h>

#include <sys/time.h>
#include <errno.h>

#include <unistd.h>

#undef nanosleep

enum { BILLION = 1000 * 1000 * 1000 };

#if HAVE_BUG_BIG_NANOSLEEP

static void
getnow (struct timespec *t)
{
# if defined CLOCK_MONOTONIC && HAVE_CLOCK_GETTIME
  if (clock_gettime (CLOCK_MONOTONIC, t) == 0)
    return;
# endif
  gettime (t);
}

int
rpl_nanosleep (const struct timespec *requested_delay,
	       struct timespec *remaining_delay)
{
  /* nanosleep mishandles large sleeps due to internal overflow
     problems, so check that the proper amount of time has actually
     elapsed.  */

  struct timespec delay = *requested_delay;
  struct timespec t0;
  getnow (&t0);

  for (;;)
    {
      int r = nanosleep (&delay, remaining_delay);
      if (r == 0)
	{
	  time_t secs_sofar;
	  struct timespec now;
	  getnow (&now);

	  secs_sofar = now.tv_sec - t0.tv_sec;
	  if (requested_delay->tv_sec < secs_sofar)
	    return 0;
	  delay.tv_sec = requested_delay->tv_sec - secs_sofar;
	  delay.tv_nsec = requested_delay->tv_nsec - (now.tv_nsec - t0.tv_nsec);
	  if (delay.tv_nsec < 0)
	    {
	      if (delay.tv_sec == 0)
		return 0;
	      delay.tv_nsec += BILLION;
	      delay.tv_sec--;
	    }
	  else if (BILLION <= delay.tv_nsec)
	    {
	      delay.tv_nsec -= BILLION;
	      delay.tv_sec++;
	    }
	}
    }
}

#else

/* Some systems (MSDOS) don't have SIGCONT.
   Using SIGTERM here turns the signal-handling code below
   into a no-op on such systems. */
# ifndef SIGCONT
#  define SIGCONT SIGTERM
# endif

static sig_atomic_t volatile suspended;

/* Handle SIGCONT. */

static void
sighandler (int sig)
{
  suspended = 1;
}

/* Suspend execution for at least *TS_DELAY seconds.  */

static void
my_usleep (const struct timespec *ts_delay)
{
  struct timeval tv_delay;
  tv_delay.tv_sec = ts_delay->tv_sec;
  tv_delay.tv_usec = (ts_delay->tv_nsec + 999) / 1000;
  if (tv_delay.tv_usec == 1000000)
    {
      time_t t1 = tv_delay.tv_sec + 1;
      if (t1 < tv_delay.tv_sec)
	tv_delay.tv_usec = 1000000 - 1; /* close enough */
      else
	{
	  tv_delay.tv_sec = t1;
	  tv_delay.tv_usec = 0;
	}
    }
  select (0, NULL, NULL, NULL, &tv_delay);
}

/* Suspend execution for at least *REQUESTED_DELAY seconds.  The
   *REMAINING_DELAY part isn't implemented yet.  */

int
rpl_nanosleep (const struct timespec *requested_delay,
	       struct timespec *remaining_delay)
{
  static bool initialized;

  /* set up sig handler */
  if (! initialized)
    {
      struct sigaction oldact;

      sigaction (SIGCONT, NULL, &oldact);
      if (get_handler (&oldact) != SIG_IGN)
	{
	  struct sigaction newact;

	  newact.sa_handler = sighandler;
	  sigemptyset (&newact.sa_mask);
	  newact.sa_flags = 0;
	  sigaction (SIGCONT, &newact, NULL);
	}
      initialized = true;
    }

  suspended = 0;

  my_usleep (requested_delay);

  if (suspended)
    {
      /* Calculate time remaining.  */
      /* FIXME: the code in sleep doesn't use this, so there's no
	 rush to implement it.  */

      errno = EINTR;
    }

  /* FIXME: Restore sig handler?  */

  return suspended;
}
#endif
