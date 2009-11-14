/* sig.c - interface for shell signal handlers and signal initialization. */

/* Copyright (C) 1994-2009 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne Again SHell.

   Bash is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Bash is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Bash.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "config.h"

#include "bashtypes.h"

#if defined (HAVE_UNISTD_H)
#  ifdef _MINIX
#    include <sys/types.h>
#  endif
#  include <unistd.h>
#endif

#include <stdio.h>
#include <signal.h>

#include "bashintl.h"

#include "shell.h"
#if defined (JOB_CONTROL)
#include "jobs.h"
#endif /* JOB_CONTROL */
#include "siglist.h"
#include "sig.h"
#include "trap.h"

#include "builtins/common.h"

#if defined (READLINE)
#  include "bashline.h"
#endif

#if defined (HISTORY)
#  include "bashhist.h"
#endif

extern int last_command_exit_value;
extern int last_command_exit_signal;
extern int return_catch_flag;
extern int loop_level, continuing, breaking;
extern int executing_list;
extern int comsub_ignore_return;
extern int parse_and_execute_level, shell_initialized;
extern int hup_on_exit;

/* Non-zero after SIGINT. */
volatile int interrupt_state = 0;

/* Non-zero after SIGWINCH */
volatile int sigwinch_received = 0;

/* Set to the value of any terminating signal received. */
volatile int terminating_signal = 0;

/* The environment at the top-level R-E loop.  We use this in
   the case of error return. */
procenv_t top_level;

#if defined (JOB_CONTROL) || defined (HAVE_POSIX_SIGNALS)
/* The signal masks that this shell runs with. */
sigset_t top_level_mask;
#endif /* JOB_CONTROL */

/* When non-zero, we throw_to_top_level (). */
int interrupt_immediately = 0;

/* When non-zero, we call the terminating signal handler immediately. */
int terminate_immediately = 0;

#if defined (SIGWINCH)
static SigHandler *old_winch = (SigHandler *)SIG_DFL;
#endif

static void initialize_shell_signals __P((void));

void
initialize_signals (reinit)
     int reinit;
{
  initialize_shell_signals ();
  initialize_job_signals ();
#if !defined (HAVE_SYS_SIGLIST) && !defined (HAVE_UNDER_SYS_SIGLIST) && !defined (HAVE_STRSIGNAL)
  if (reinit == 0)
    initialize_siglist ();
#endif /* !HAVE_SYS_SIGLIST && !HAVE_UNDER_SYS_SIGLIST && !HAVE_STRSIGNAL */
}

/* A structure describing a signal that terminates the shell if not
   caught.  The orig_handler member is present so children can reset
   these signals back to their original handlers. */
struct termsig {
     int signum;
     SigHandler *orig_handler;
     int orig_flags;
};

#define NULL_HANDLER (SigHandler *)SIG_DFL

/* The list of signals that would terminate the shell if not caught.
   We catch them, but just so that we can write the history file,
   and so forth. */
static struct termsig terminating_signals[] = {
#ifdef SIGHUP
{  SIGHUP, NULL_HANDLER, 0 },
#endif

#ifdef SIGINT
{  SIGINT, NULL_HANDLER, 0 },
#endif

#ifdef SIGILL
{  SIGILL, NULL_HANDLER, 0 },
#endif

#ifdef SIGTRAP
{  SIGTRAP, NULL_HANDLER, 0 },
#endif

#ifdef SIGIOT
{  SIGIOT, NULL_HANDLER, 0 },
#endif

#ifdef SIGDANGER
{  SIGDANGER, NULL_HANDLER, 0 },
#endif

#ifdef SIGEMT
{  SIGEMT, NULL_HANDLER, 0 },
#endif

#ifdef SIGFPE
{  SIGFPE, NULL_HANDLER, 0 },
#endif

#ifdef SIGBUS
{  SIGBUS, NULL_HANDLER, 0 },
#endif

#ifdef SIGSEGV
{  SIGSEGV, NULL_HANDLER, 0 },
#endif

#ifdef SIGSYS
{  SIGSYS, NULL_HANDLER, 0 },
#endif

#ifdef SIGPIPE
{  SIGPIPE, NULL_HANDLER, 0 },
#endif

#ifdef SIGALRM
{  SIGALRM, NULL_HANDLER, 0 },
#endif

#ifdef SIGTERM
{  SIGTERM, NULL_HANDLER, 0 },
#endif

#ifdef SIGXCPU
{  SIGXCPU, NULL_HANDLER, 0 },
#endif

#ifdef SIGXFSZ
{  SIGXFSZ, NULL_HANDLER, 0 },
#endif

#ifdef SIGVTALRM
{  SIGVTALRM, NULL_HANDLER, 0 },
#endif

#if 0
#ifdef SIGPROF
{  SIGPROF, NULL_HANDLER, 0 },
#endif
#endif

#ifdef SIGLOST
{  SIGLOST, NULL_HANDLER, 0 },
#endif

#ifdef SIGUSR1
{  SIGUSR1, NULL_HANDLER, 0 },
#endif

#ifdef SIGUSR2
{  SIGUSR2, NULL_HANDLER, 0 },
#endif
};

#define TERMSIGS_LENGTH (sizeof (terminating_signals) / sizeof (struct termsig))

#define XSIG(x) (terminating_signals[x].signum)
#define XHANDLER(x) (terminating_signals[x].orig_handler)
#define XSAFLAGS(x) (terminating_signals[x].orig_flags)

static int termsigs_initialized = 0;

/* Initialize signals that will terminate the shell to do some
   unwind protection.  For non-interactive shells, we only call
   this when a trap is defined for EXIT (0). */
void
initialize_terminating_signals ()
{
  register int i;
#if defined (HAVE_POSIX_SIGNALS)
  struct sigaction act, oact;
#endif

  if (termsigs_initialized)
    return;

  /* The following code is to avoid an expensive call to
     set_signal_handler () for each terminating_signals.  Fortunately,
     this is possible in Posix.  Unfortunately, we have to call signal ()
     on non-Posix systems for each signal in terminating_signals. */
#if defined (HAVE_POSIX_SIGNALS)
  act.sa_handler = termsig_sighandler;
  act.sa_flags = 0;
  sigemptyset (&act.sa_mask);
  sigemptyset (&oact.sa_mask);
  for (i = 0; i < TERMSIGS_LENGTH; i++)
    sigaddset (&act.sa_mask, XSIG (i));
  for (i = 0; i < TERMSIGS_LENGTH; i++)
    {
      /* If we've already trapped it, don't do anything. */
      if (signal_is_trapped (XSIG (i)))
	continue;

      sigaction (XSIG (i), &act, &oact);
      XHANDLER(i) = oact.sa_handler;
      XSAFLAGS(i) = oact.sa_flags;
      /* Don't do anything with signals that are ignored at shell entry
	 if the shell is not interactive. */
      if (!interactive_shell && XHANDLER (i) == SIG_IGN)
	{
	  sigaction (XSIG (i), &oact, &act);
	  set_signal_ignored (XSIG (i));
	}
#if defined (SIGPROF) && !defined (_MINIX)
      if (XSIG (i) == SIGPROF && XHANDLER (i) != SIG_DFL && XHANDLER (i) != SIG_IGN)
	sigaction (XSIG (i), &oact, (struct sigaction *)NULL);
#endif /* SIGPROF && !_MINIX */
    }

#else /* !HAVE_POSIX_SIGNALS */

  for (i = 0; i < TERMSIGS_LENGTH; i++)
    {
      /* If we've already trapped it, don't do anything. */
      if (signal_is_trapped (XSIG (i)))
	continue;

      XHANDLER(i) = signal (XSIG (i), termsig_sighandler);
      XSAFLAGS(i) = 0;
      /* Don't do anything with signals that are ignored at shell entry
	 if the shell is not interactive. */
      if (!interactive_shell && XHANDLER (i) == SIG_IGN)
	{
	  signal (XSIG (i), SIG_IGN);
	  set_signal_ignored (XSIG (i));
	}
#ifdef SIGPROF
      if (XSIG (i) == SIGPROF && XHANDLER (i) != SIG_DFL && XHANDLER (i) != SIG_IGN)
	signal (XSIG (i), XHANDLER (i));
#endif
    }

#endif /* !HAVE_POSIX_SIGNALS */

  termsigs_initialized = 1;
}

static void
initialize_shell_signals ()
{
  if (interactive)
    initialize_terminating_signals ();

#if defined (JOB_CONTROL) || defined (HAVE_POSIX_SIGNALS)
  /* All shells use the signal mask they inherit, and pass it along
     to child processes.  Children will never block SIGCHLD, though. */
  sigemptyset (&top_level_mask);
  sigprocmask (SIG_BLOCK, (sigset_t *)NULL, &top_level_mask);
#  if defined (SIGCHLD)
  sigdelset (&top_level_mask, SIGCHLD);
#  endif
#endif /* JOB_CONTROL || HAVE_POSIX_SIGNALS */

  /* And, some signals that are specifically ignored by the shell. */
  set_signal_handler (SIGQUIT, SIG_IGN);

  if (interactive)
    {
      set_signal_handler (SIGINT, sigint_sighandler);
      set_signal_handler (SIGTERM, SIG_IGN);
      set_sigwinch_handler ();
    }
}

void
reset_terminating_signals ()
{
  register int i;
#if defined (HAVE_POSIX_SIGNALS)
  struct sigaction act;
#endif

  if (termsigs_initialized == 0)
    return;

#if defined (HAVE_POSIX_SIGNALS)
  act.sa_flags = 0;
  sigemptyset (&act.sa_mask);
  for (i = 0; i < TERMSIGS_LENGTH; i++)
    {
      /* Skip a signal if it's trapped or handled specially, because the
	 trap code will restore the correct value. */
      if (signal_is_trapped (XSIG (i)) || signal_is_special (XSIG (i)))
	continue;

      act.sa_handler = XHANDLER (i);
      act.sa_flags = XSAFLAGS (i);
      sigaction (XSIG (i), &act, (struct sigaction *) NULL);
    }
#else /* !HAVE_POSIX_SIGNALS */
  for (i = 0; i < TERMSIGS_LENGTH; i++)
    {
      if (signal_is_trapped (XSIG (i)) || signal_is_special (XSIG (i)))
	continue;

      signal (XSIG (i), XHANDLER (i));
    }
#endif /* !HAVE_POSIX_SIGNALS */
}
#undef XSIG
#undef XHANDLER

/* Run some of the cleanups that should be performed when we run
   jump_to_top_level from a builtin command context.  XXX - might want to
   also call reset_parser here. */
void
top_level_cleanup ()
{
  /* Clean up string parser environment. */
  while (parse_and_execute_level)
    parse_and_execute_cleanup ();

#if defined (PROCESS_SUBSTITUTION)
  unlink_fifo_list ();
#endif /* PROCESS_SUBSTITUTION */

  run_unwind_protects ();
  loop_level = continuing = breaking = 0;
  executing_list = comsub_ignore_return = return_catch_flag = 0;
}

/* What to do when we've been interrupted, and it is safe to handle it. */
void
throw_to_top_level ()
{
  int print_newline = 0;

  if (interrupt_state)
    {
      print_newline = 1;
      DELINTERRUPT;
    }

  if (interrupt_state)
    return;

  last_command_exit_signal = (last_command_exit_value > 128) ?
				(last_command_exit_value - 128) : 0;
  last_command_exit_value |= 128;

  /* Run any traps set on SIGINT. */
  run_interrupt_trap ();

  /* Clean up string parser environment. */
  while (parse_and_execute_level)
    parse_and_execute_cleanup ();

#if defined (JOB_CONTROL)
  give_terminal_to (shell_pgrp, 0);
#endif /* JOB_CONTROL */

#if defined (JOB_CONTROL) || defined (HAVE_POSIX_SIGNALS)
  /* This should not be necessary on systems using sigsetjmp/siglongjmp. */
  sigprocmask (SIG_SETMASK, &top_level_mask, (sigset_t *)NULL);
#endif

  reset_parser ();

#if defined (READLINE)
  if (interactive)
    bashline_reset ();
#endif /* READLINE */

#if defined (PROCESS_SUBSTITUTION)
  unlink_fifo_list ();
#endif /* PROCESS_SUBSTITUTION */

  run_unwind_protects ();
  loop_level = continuing = breaking = 0;
  executing_list = comsub_ignore_return = return_catch_flag = 0;

  if (interactive && print_newline)
    {
      fflush (stdout);
      fprintf (stderr, "\n");
      fflush (stderr);
    }

  /* An interrupted `wait' command in a script does not exit the script. */
  if (interactive || (interactive_shell && !shell_initialized) ||
      (print_newline && signal_is_trapped (SIGINT)))
    jump_to_top_level (DISCARD);
  else
    jump_to_top_level (EXITPROG);
}

/* This is just here to isolate the longjmp calls. */
void
jump_to_top_level (value)
     int value;
{
  longjmp (top_level, value);
}

sighandler
termsig_sighandler (sig)
     int sig;
{
  /* If we get called twice with the same signal before handling it,
     terminate right away. */
  if (
#ifdef SIGHUP
    sig != SIGHUP &&
#endif
#ifdef SIGINT
    sig != SIGINT &&
#endif
#ifdef SIGDANGER
    sig != SIGDANGER &&
#endif
#ifdef SIGPIPE
    sig != SIGPIPE &&
#endif
#ifdef SIGALRM
    sig != SIGALRM &&
#endif
#ifdef SIGTERM
    sig != SIGTERM &&
#endif
#ifdef SIGXCPU
    sig != SIGXCPU &&
#endif
#ifdef SIGXFSZ
    sig != SIGXFSZ &&
#endif
#ifdef SIGVTALRM
    sig != SIGVTALRM &&
#endif
#ifdef SIGLOST
    sig != SIGLOST &&
#endif
#ifdef SIGUSR1
    sig != SIGUSR1 &&
#endif
#ifdef SIGUSR2
   sig != SIGUSR2 &&
#endif
   sig == terminating_signal)
    terminate_immediately = 1;

  terminating_signal = sig;

  /* XXX - should this also trigger when interrupt_immediately is set? */
  if (terminate_immediately)
    {
      terminate_immediately = 0;
      termsig_handler (sig);
    }

  SIGRETURN (0);
}

void
termsig_handler (sig)
     int sig;
{
  static int handling_termsig = 0;

  /* Simple semaphore to keep this function from being executed multiple
     times.  Since we no longer are running as a signal handler, we don't
     block multiple occurrences of the terminating signals while running. */
  if (handling_termsig)
    return;
  handling_termsig = 1;
  terminating_signal = 0;	/* keep macro from re-testing true. */

  /* I don't believe this condition ever tests true. */
  if (sig == SIGINT && signal_is_trapped (SIGINT))
    run_interrupt_trap ();

#if defined (HISTORY)
  if (interactive_shell && sig != SIGABRT)
    maybe_save_shell_history ();
#endif /* HISTORY */

#if defined (JOB_CONTROL)
  if (interactive && sig == SIGHUP && hup_on_exit)
    hangup_all_jobs ();
  end_job_control ();
#endif /* JOB_CONTROL */

#if defined (PROCESS_SUBSTITUTION)
  unlink_fifo_list ();
#endif /* PROCESS_SUBSTITUTION */

  /* Reset execution context */
  loop_level = continuing = breaking = 0;
  executing_list = comsub_ignore_return = return_catch_flag = 0;

  run_exit_trap ();
  set_signal_handler (sig, SIG_DFL);
  kill (getpid (), sig);
}

/* What we really do when SIGINT occurs. */
sighandler
sigint_sighandler (sig)
     int sig;
{
#if defined (MUST_REINSTALL_SIGHANDLERS)
  signal (sig, sigint_sighandler);
#endif

  /* interrupt_state needs to be set for the stack of interrupts to work
     right.  Should it be set unconditionally? */
  if (interrupt_state == 0)
    ADDINTERRUPT;

  if (interrupt_immediately)
    {
      interrupt_immediately = 0;
#if defined (__BEOS__) && !defined (__HAIKU__)
      /* XXXdbg -- throw_to_top_level() calls longjmp() which leaves our
	 sigmask in a screwed up state so we reset it here. */
      /* FIXME - fnf: Is this a generic problem with bash, or something
	 Be specific? */
      {
	sigset_t oset;
	sigemptyset(&oset);
	sigprocmask(SIG_BLOCK, NULL, &oset);
	sigdelset(&oset, SIGINT);
	sigprocmask(SIG_SETMASK, &oset, NULL);
      }
#endif
      throw_to_top_level ();
    }

  SIGRETURN (0);
}

#if defined (SIGWINCH)
sighandler
sigwinch_sighandler (sig)
     int sig;
{
#if defined (MUST_REINSTALL_SIGHANDLERS)
  set_signal_handler (SIGWINCH, sigwinch_sighandler);
#endif /* MUST_REINSTALL_SIGHANDLERS */
  sigwinch_received = 1;
  SIGRETURN (0);
}
#endif /* SIGWINCH */

void
set_sigwinch_handler ()
{
#if defined (SIGWINCH)
 old_winch = set_signal_handler (SIGWINCH, sigwinch_sighandler);
#endif
}

void
unset_sigwinch_handler ()
{
#if defined (SIGWINCH)
  set_signal_handler (SIGWINCH, old_winch);
#endif
}

/* Signal functions used by the rest of the code. */
#if !defined (HAVE_POSIX_SIGNALS)

#if defined (JOB_CONTROL)
/* Perform OPERATION on NEWSET, perhaps leaving information in OLDSET. */
sigprocmask (operation, newset, oldset)
     int operation, *newset, *oldset;
{
  int old, new;

  if (newset)
    new = *newset;
  else
    new = 0;

  switch (operation)
    {
    case SIG_BLOCK:
      old = sigblock (new);
      break;

    case SIG_SETMASK:
      sigsetmask (new);
      break;

    default:
      internal_error (_("sigprocmask: %d: invalid operation"), operation);
    }

  if (oldset)
    *oldset = old;
}
#endif /* JOB_CONTROL */

#else

#if !defined (SA_INTERRUPT)
#  define SA_INTERRUPT 0
#endif

#if !defined (SA_RESTART)
#  define SA_RESTART 0
#endif

SigHandler *
set_signal_handler (sig, handler)
     int sig;
     SigHandler *handler;
{
  struct sigaction act, oact;

  act.sa_handler = handler;
  act.sa_flags = 0;
#if 0
  if (sig == SIGALRM)
    act.sa_flags |= SA_INTERRUPT;	/* XXX */
  else
    act.sa_flags |= SA_RESTART;		/* XXX */
#endif
  sigemptyset (&act.sa_mask);
  sigemptyset (&oact.sa_mask);
  sigaction (sig, &act, &oact);
  return (oact.sa_handler);
}
#endif /* HAVE_POSIX_SIGNALS */
