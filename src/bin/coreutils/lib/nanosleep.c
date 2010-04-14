/* Provide a replacement for the POSIX nanosleep function.

   Copyright (C) 1999-2000, 2002, 2004-2010 Free Software Foundation, Inc.

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

#include "intprops.h"
#include "sig-handler.h"
#include "verify.h"

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

int
rpl_nanosleep (const struct timespec *requested_delay,
               struct timespec *remaining_delay)
{
  /* nanosleep mishandles large sleeps due to internal overflow
     problems.  The worst known case of this is cygwin 1.5.x, which
     can't sleep more than 49.7 days (2**32 milliseconds).  Solve this
     by breaking the sleep up into smaller chunks.  Verify that time_t
     is large enough.  */
  verify (TYPE_MAXIMUM (time_t) / 49 / 24 / 60 / 60);
  const time_t limit = 49 * 24 * 60 * 60;
  time_t seconds = requested_delay->tv_sec;
  struct timespec intermediate;
  intermediate.tv_nsec = 0;

  while (limit < seconds)
    {
      int result;
      intermediate.tv_sec = limit;
      result = nanosleep (&intermediate, remaining_delay);
      seconds -= limit;
      if (result)
        {
          if (remaining_delay)
            {
              remaining_delay->tv_sec += seconds;
              remaining_delay->tv_nsec += requested_delay->tv_nsec;
              if (BILLION <= requested_delay->tv_nsec)
                {
                  remaining_delay->tv_sec++;
                  remaining_delay->tv_nsec -= BILLION;
                }
            }
          return result;
        }
    }
  intermediate.tv_sec = seconds;
  intermediate.tv_nsec = requested_delay->tv_nsec;
  return nanosleep (&intermediate, remaining_delay);
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

  if (requested_delay->tv_nsec < 0 || BILLION <= requested_delay->tv_nsec)
    {
      errno = EINVAL;
      return -1;
    }

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
