/* Emergency actions in case of a fatal signal.
   Copyright (C) 2003-2004, 2006 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2003.

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
#include "fatal-signal.h"

#include <stdbool.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include "sigprocmask.h"
#include "xalloc.h"

#define SIZEOF(a) (sizeof(a) / sizeof(a[0]))


/* ========================================================================= */


/* The list of fatal signals.
   These are those signals whose default action is to terminate the process
   without a core dump, except
     SIGKILL - because it cannot be caught,
     SIGALRM SIGUSR1 SIGUSR2 SIGPOLL SIGIO SIGLOST - because applications
       often use them for their own purpose,
     SIGPROF SIGVTALRM - because they are used for profiling,
     SIGSTKFLT - because it is more similar to SIGFPE, SIGSEGV, SIGBUS,
     SIGSYS - because it is more similar to SIGABRT, SIGSEGV,
     SIGPWR - because it of too special use,
     SIGRTMIN...SIGRTMAX - because they are reserved for application use.
   plus
     SIGXCPU, SIGXFSZ - because they are quite similar to SIGTERM.  */

static int fatal_signals[] =
  {
    /* ISO C 99 signals.  */
#ifdef SIGINT
    SIGINT,
#endif
#ifdef SIGTERM
    SIGTERM,
#endif
    /* POSIX:2001 signals.  */
#ifdef SIGHUP
    SIGHUP,
#endif
#ifdef SIGPIPE
    SIGPIPE,
#endif
    /* BSD signals.  */
#ifdef SIGXCPU
    SIGXCPU,
#endif
#ifdef SIGXFSZ
    SIGXFSZ,
#endif
    /* Woe32 signals.  */
#ifdef SIGBREAK
    SIGBREAK,
#endif
    0
  };

#define num_fatal_signals (SIZEOF (fatal_signals) - 1)

/* Eliminate signals whose signal handler is SIG_IGN.  */

static void
init_fatal_signals (void)
{
  static bool fatal_signals_initialized = false;
  if (!fatal_signals_initialized)
    {
#if HAVE_SIGACTION
      size_t i;

      for (i = 0; i < num_fatal_signals; i++)
	{
	  struct sigaction action;

	  if (sigaction (fatal_signals[i], NULL, &action) >= 0
	      && action.sa_handler == SIG_IGN)
	    fatal_signals[i] = -1;
	}
#endif

      fatal_signals_initialized = true;
    }
}


/* ========================================================================= */


typedef void (*action_t) (void);

/* Type of an entry in the actions array.
   The 'action' field is accessed from within the fatal_signal_handler(),
   therefore we mark it as 'volatile'.  */
typedef struct
{
  volatile action_t action;
}
actions_entry_t;

/* The registered cleanup actions.  */
static actions_entry_t static_actions[32];
static actions_entry_t * volatile actions = static_actions;
static sig_atomic_t volatile actions_count = 0;
static size_t actions_allocated = SIZEOF (static_actions);


/* Uninstall the handlers.  */
static inline void
uninstall_handlers ()
{
  size_t i;

  for (i = 0; i < num_fatal_signals; i++)
    if (fatal_signals[i] >= 0)
      signal (fatal_signals[i], SIG_DFL);
}


/* The signal handler.  It gets called asynchronously.  */
static void
fatal_signal_handler (int sig)
{
  for (;;)
    {
      /* Get the last registered cleanup action, in a reentrant way.  */
      action_t action;
      size_t n = actions_count;
      if (n == 0)
	break;
      n--;
      actions_count = n;
      action = actions[n].action;
      /* Execute the action.  */
      action ();
    }

  /* Now execute the signal's default action.
     If signal() blocks the signal being delivered for the duration of the
     signal handler's execution, the re-raised signal is delivered when this
     handler returns; otherwise it is delivered already during raise().  */
  uninstall_handlers ();
#if HAVE_RAISE
  raise (sig);
#else
  kill (getpid (), sig);
#endif
}


/* Install the handlers.  */
static inline void
install_handlers ()
{
  size_t i;

  for (i = 0; i < num_fatal_signals; i++)
    if (fatal_signals[i] >= 0)
      signal (fatal_signals[i], &fatal_signal_handler);
}


/* Register a cleanup function to be executed when a catchable fatal signal
   occurs.  */
void
at_fatal_signal (action_t action)
{
  static bool cleanup_initialized = false;
  if (!cleanup_initialized)
    {
      init_fatal_signals ();
      install_handlers ();
      cleanup_initialized = true;
    }

  if (actions_count == actions_allocated)
    {
      /* Extend the actions array.  Note that we cannot use xrealloc(),
	 because then the cleanup() function could access an already
	 deallocated array.  */
      actions_entry_t *old_actions = actions;
      size_t old_actions_allocated = actions_allocated;
      size_t new_actions_allocated = 2 * actions_allocated;
      actions_entry_t *new_actions =
	XNMALLOC (new_actions_allocated, actions_entry_t);
      size_t k;

      /* Don't use memcpy() here, because memcpy takes non-volatile arguments
	 and is therefore not guaranteed to complete all memory stores before
	 the next statement.  */
      for (k = 0; k < old_actions_allocated; k++)
	new_actions[k] = old_actions[k];
      actions = new_actions;
      actions_allocated = new_actions_allocated;
      /* Now we can free the old actions array.  */
      if (old_actions != static_actions)
	free (old_actions);
    }
  /* The two uses of 'volatile' in the types above (and ISO C 99 section
     5.1.2.3.(5)) ensure that we increment the actions_count only after
     the new action has been written to the memory location
     actions[actions_count].  */
  actions[actions_count].action = action;
  actions_count++;
}


/* ========================================================================= */


static sigset_t fatal_signal_set;

static void
init_fatal_signal_set ()
{
  static bool fatal_signal_set_initialized = false;
  if (!fatal_signal_set_initialized)
    {
      size_t i;

      init_fatal_signals ();

      sigemptyset (&fatal_signal_set);
      for (i = 0; i < num_fatal_signals; i++)
	if (fatal_signals[i] >= 0)
	  sigaddset (&fatal_signal_set, fatal_signals[i]);

      fatal_signal_set_initialized = true;
    }
}

/* Temporarily delay the catchable fatal signals.  */
void
block_fatal_signals ()
{
  init_fatal_signal_set ();
  sigprocmask (SIG_BLOCK, &fatal_signal_set, NULL);
}

/* Stop delaying the catchable fatal signals.  */
void
unblock_fatal_signals ()
{
  init_fatal_signal_set ();
  sigprocmask (SIG_UNBLOCK, &fatal_signal_set, NULL);
}
