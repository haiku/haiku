/* trap.c -- Not the trap command, but useful functions for manipulating
   those objects.  The trap command is in builtins/trap.def. */

/* Copyright (C) 1987-2002 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne Again SHell.

   Bash is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 2, or (at your option) any later
   version.

   Bash is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.

   You should have received a copy of the GNU General Public License along
   with Bash; see the file COPYING.  If not, write to the Free Software
   Foundation, 59 Temple Place, Suite 330, Boston, MA 02111 USA. */

#include "config.h"

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#include "bashtypes.h"
#include "bashansi.h"

#include <stdio.h>
#include <errno.h>

#include "trap.h"

#include "shell.h"
#include "input.h"	/* for save_token_state, restore_token_state */
#include "signames.h"
#include "builtins.h"
#include "builtins/common.h"
#include "builtins/builtext.h"

#ifndef errno
extern int errno;
#endif

/* Flags which describe the current handling state of a signal. */
#define SIG_INHERITED   0x0	/* Value inherited from parent. */
#define SIG_TRAPPED     0x1	/* Currently trapped. */
#define SIG_HARD_IGNORE 0x2	/* Signal was ignored on shell entry. */
#define SIG_SPECIAL     0x4	/* Treat this signal specially. */
#define SIG_NO_TRAP     0x8	/* Signal cannot be trapped. */
#define SIG_INPROGRESS	0x10	/* Signal handler currently executing. */
#define SIG_CHANGED	0x20	/* Trap value changed in trap handler. */
#define SIG_IGNORED	0x40	/* The signal is currently being ignored. */

#define SPECIAL_TRAP(s)	((s) == EXIT_TRAP || (s) == DEBUG_TRAP || (s) == ERROR_TRAP)

/* An array of such flags, one for each signal, describing what the
   shell will do with a signal.  DEBUG_TRAP == NSIG; some code below
   assumes this. */
static int sigmodes[BASH_NSIG];

static void free_trap_command __P((int));
static void change_signal __P((int, char *));

static void get_original_signal __P((int));

static void _run_trap_internal __P((int, char *));

static void reset_signal __P((int));
static void restore_signal __P((int));
static void reset_or_restore_signal_handlers __P((sh_resetsig_func_t *));

/* Variables used here but defined in other files. */
extern int interrupt_immediately;
extern int last_command_exit_value;
extern int line_number;

extern sh_builtin_func_t *this_shell_builtin;
extern procenv_t wait_intr_buf;

/* The list of things to do originally, before we started trapping. */
SigHandler *original_signals[NSIG];

/* For each signal, a slot for a string, which is a command to be
   executed when that signal is recieved.  The slot can also contain
   DEFAULT_SIG, which means do whatever you were going to do before
   you were so rudely interrupted, or IGNORE_SIG, which says ignore
   this signal. */
char *trap_list[BASH_NSIG];

/* A bitmap of signals received for which we have trap handlers. */
int pending_traps[NSIG];

/* Set to the number of the signal we're running the trap for + 1.
   Used in execute_cmd.c and builtins/common.c to clean up when
   parse_and_execute does not return normally after executing the
   trap command (e.g., when `return' is executed in the trap command). */
int running_trap;

/* The value of line_number when the trap started executing, since
   parse_and_execute resets it to 1 and the trap command might want
   it. */
int trap_line_number;

/* The (trapped) signal received while executing in the `wait' builtin */
int wait_signal_received;

/* A value which can never be the target of a trap handler. */
#define IMPOSSIBLE_TRAP_HANDLER (SigHandler *)initialize_traps

void
initialize_traps ()
{
  register int i;

  trap_list[EXIT_TRAP] = trap_list[DEBUG_TRAP] = trap_list[ERROR_TRAP] = (char *)NULL;
  sigmodes[EXIT_TRAP] = sigmodes[DEBUG_TRAP] = sigmodes[ERROR_TRAP] = SIG_INHERITED;
  original_signals[EXIT_TRAP] = IMPOSSIBLE_TRAP_HANDLER;

  for (i = 1; i < NSIG; i++)
    {
      pending_traps[i] = 0;
      trap_list[i] = (char *)DEFAULT_SIG;
      sigmodes[i] = SIG_INHERITED;
      original_signals[i] = IMPOSSIBLE_TRAP_HANDLER;
    }

  /* Show which signals are treated specially by the shell. */
#if defined (SIGCHLD)
  original_signals[SIGCHLD] =
    (SigHandler *) set_signal_handler (SIGCHLD, SIG_DFL);
  set_signal_handler (SIGCHLD, original_signals[SIGCHLD]);
  sigmodes[SIGCHLD] |= (SIG_SPECIAL | SIG_NO_TRAP);
#endif /* SIGCHLD */

  original_signals[SIGINT] =
    (SigHandler *) set_signal_handler (SIGINT, SIG_DFL);
  set_signal_handler (SIGINT, original_signals[SIGINT]);
  sigmodes[SIGINT] |= SIG_SPECIAL;

#if defined (__BEOS__)
  /* BeOS sets SIGINT to SIG_IGN! */
  original_signals[SIGINT] = SIG_DFL;
#endif

  original_signals[SIGQUIT] =
    (SigHandler *) set_signal_handler (SIGQUIT, SIG_DFL);
  set_signal_handler (SIGQUIT, original_signals[SIGQUIT]);
  sigmodes[SIGQUIT] |= SIG_SPECIAL;

  if (interactive)
    {
      original_signals[SIGTERM] =
	(SigHandler *)set_signal_handler (SIGTERM, SIG_DFL);
      set_signal_handler (SIGTERM, original_signals[SIGTERM]);
      sigmodes[SIGTERM] |= SIG_SPECIAL;
    }
}

#ifdef INCLUDE_UNUSED
/* Return a printable representation of the trap handler for SIG. */
static char *
trap_handler_string (sig)
     int sig;
{
  if (trap_list[sig] == (char *)DEFAULT_SIG)
    return "DEFAULT_SIG";
  else if (trap_list[sig] == (char *)IGNORE_SIG)
    return "IGNORE_SIG";
  else if (trap_list[sig] == (char *)IMPOSSIBLE_TRAP_HANDLER)
    return "IMPOSSIBLE_TRAP_HANDLER";
  else if (trap_list[sig])
    return trap_list[sig];
  else
    return "NULL";
}
#endif

/* Return the print name of this signal. */
char *
signal_name (sig)
     int sig;
{
  char *ret;

  /* on cygwin32, signal_names[sig] could be null */
  ret = (sig >= BASH_NSIG || sig < 0) ? "bad signal number" : signal_names[sig];
  if (ret == NULL)
    ret = "unrecognized signal number";
  return ret;
}

/* Turn a string into a signal number, or a number into
   a signal number.  If STRING is "2", "SIGINT", or "INT",
   then (int)2 is returned.  Return NO_SIG if STRING doesn't
   contain a valid signal descriptor. */
int
decode_signal (string)
     char *string;
{
  intmax_t sig;

  if (legal_number (string, &sig))
    return ((sig >= 0 && sig < NSIG) ? (int)sig : NO_SIG);

  /* A leading `SIG' may be omitted. */
  for (sig = 0; sig < BASH_NSIG; sig++)
    {
      if (signal_names[sig] == 0 || signal_names[sig][0] == '\0')
	continue;
      if (strcasecmp (string, signal_names[sig]) == 0 ||
	  (STREQN (signal_names[sig], "SIG", 3) &&
	    strcasecmp (string, &(signal_names[sig])[3]) == 0))
	return ((int)sig);
    }

  return (NO_SIG);
}

/* Non-zero when we catch a trapped signal. */
static int catch_flag;

void
run_pending_traps ()
{
  register int sig;
  int old_exit_value, *token_state;

  if (catch_flag == 0)		/* simple optimization */
    return;

  catch_flag = 0;

  /* Preserve $? when running trap. */
  old_exit_value = last_command_exit_value;

  for (sig = 1; sig < NSIG; sig++)
    {
      /* XXX this could be made into a counter by using
	 while (pending_traps[sig]--) instead of the if statement. */
      if (pending_traps[sig])
	{
#if defined (HAVE_POSIX_SIGNALS)
	  sigset_t set, oset;

	  sigemptyset (&set);
	  sigemptyset (&oset);

	  sigaddset (&set, sig);
	  sigprocmask (SIG_BLOCK, &set, &oset);
#else
#  if defined (HAVE_BSD_SIGNALS)
	  int oldmask = sigblock (sigmask (sig));
#  endif
#endif /* HAVE_POSIX_SIGNALS */

	  if (sig == SIGINT)
	    {
	      run_interrupt_trap ();
	      CLRINTERRUPT;
	    }
	  else if (trap_list[sig] == (char *)DEFAULT_SIG ||
		   trap_list[sig] == (char *)IGNORE_SIG ||
		   trap_list[sig] == (char *)IMPOSSIBLE_TRAP_HANDLER)
	    {
	      /* This is possible due to a race condition.  Say a bash
		 process has SIGTERM trapped.  A subshell is spawned
		 using { list; } & and the parent does something and kills
		 the subshell with SIGTERM.  It's possible for the subshell
		 to set pending_traps[SIGTERM] to 1 before the code in
		 execute_cmd.c eventually calls restore_original_signals
		 to reset the SIGTERM signal handler in the subshell.  The
		 next time run_pending_traps is called, pending_traps[SIGTERM]
		 will be 1, but the trap handler in trap_list[SIGTERM] will
		 be invalid (probably DEFAULT_SIG, but it could be IGNORE_SIG).
		 Unless we catch this, the subshell will dump core when
		 trap_list[SIGTERM] == DEFAULT_SIG, because DEFAULT_SIG is
		 usually 0x0. */
	      internal_warning ("run_pending_traps: bad value in trap_list[%d]: %p",
				sig, trap_list[sig]);
	      if (trap_list[sig] == (char *)DEFAULT_SIG)
		{
		  internal_warning ("run_pending_traps: signal handler is SIG_DFL, resending %d (%s) to myself", sig, signal_name (sig));
		  kill (getpid (), sig);
		}
	    }
	  else
	    {
	      token_state = save_token_state ();
	      parse_and_execute (savestring (trap_list[sig]), "trap", SEVAL_NONINT|SEVAL_NOHIST);
	      restore_token_state (token_state);
	      free (token_state);
	    }

	  pending_traps[sig] = 0;

#if defined (HAVE_POSIX_SIGNALS)
	  sigprocmask (SIG_SETMASK, &oset, (sigset_t *)NULL);
#else
#  if defined (HAVE_BSD_SIGNALS)
	  sigsetmask (oldmask);
#  endif
#endif /* POSIX_VERSION */
	}
    }

  last_command_exit_value = old_exit_value;
}

sighandler
trap_handler (sig)
     int sig;
{
  int oerrno;

  if ((sig >= NSIG) ||
      (trap_list[sig] == (char *)DEFAULT_SIG) ||
      (trap_list[sig] == (char *)IGNORE_SIG))
    programming_error ("trap_handler: bad signal %d", sig);
  else
    {
      oerrno = errno;
#if defined (MUST_REINSTALL_SIGHANDLERS)
      set_signal_handler (sig, trap_handler);
#endif /* MUST_REINSTALL_SIGHANDLERS */

      catch_flag = 1;
      pending_traps[sig]++;

      if (interrupt_immediately && this_shell_builtin && (this_shell_builtin == wait_builtin))
	{
	  wait_signal_received = sig;
	  longjmp (wait_intr_buf, 1);
	}

      if (interrupt_immediately)
	run_pending_traps ();

      errno = oerrno;
    }

  SIGRETURN (0);
}

#if defined (JOB_CONTROL) && defined (SIGCHLD)

#ifdef INCLUDE_UNUSED
/* Make COMMAND_STRING be executed when SIGCHLD is caught. */
void
set_sigchld_trap (command_string)
     char *command_string;
{
  set_signal (SIGCHLD, command_string);
}
#endif

/* Make COMMAND_STRING be executed when SIGCHLD is caught iff the current
   SIGCHLD trap handler is DEFAULT_SIG. */
void
maybe_set_sigchld_trap (command_string)
     char *command_string;
{
  if ((sigmodes[SIGCHLD] & SIG_TRAPPED) == 0)
    set_signal (SIGCHLD, command_string);
}
#endif /* JOB_CONTROL && SIGCHLD */

void
set_debug_trap (command)
     char *command;
{
  set_signal (DEBUG_TRAP, command);
}

void
set_error_trap (command)
     char *command;
{
  set_signal (ERROR_TRAP, command);
}

#ifdef INCLUDE_UNUSED
void
set_sigint_trap (command)
     char *command;
{
  set_signal (SIGINT, command);
}
#endif

/* Reset the SIGINT handler so that subshells that are doing `shellsy'
   things, like waiting for command substitution or executing commands
   in explicit subshells ( ( cmd ) ), can catch interrupts properly. */
SigHandler *
set_sigint_handler ()
{
  if (sigmodes[SIGINT] & SIG_HARD_IGNORE)
    return ((SigHandler *)SIG_IGN);

  else if (sigmodes[SIGINT] & SIG_IGNORED)
    return ((SigHandler *)set_signal_handler (SIGINT, SIG_IGN)); /* XXX */

  else if (sigmodes[SIGINT] & SIG_TRAPPED)
    return ((SigHandler *)set_signal_handler (SIGINT, trap_handler));

  /* The signal is not trapped, so set the handler to the shell's special
     interrupt handler. */
  else if (interactive)	/* XXX - was interactive_shell */
    return (set_signal_handler (SIGINT, sigint_sighandler));
  else
    return (set_signal_handler (SIGINT, termination_unwind_protect));
}

/* Return the correct handler for signal SIG according to the values in
   sigmodes[SIG]. */
SigHandler *
trap_to_sighandler (sig)
     int sig;
{
  if (sigmodes[sig] & (SIG_IGNORED|SIG_HARD_IGNORE))
    return (SIG_IGN);
  else if (sigmodes[sig] & SIG_TRAPPED)
    return (trap_handler);
  else
    return (SIG_DFL);
}

/* Set SIG to call STRING as a command. */
void
set_signal (sig, string)
     int sig;
     char *string;
{
  if (SPECIAL_TRAP (sig))
    {
      change_signal (sig, savestring (string));
      if (sig == EXIT_TRAP && interactive == 0)
	initialize_terminating_signals ();
      return;
    }

  /* A signal ignored on entry to the shell cannot be trapped or reset, but
     no error is reported when attempting to do so.  -- Posix.2 */
  if (sigmodes[sig] & SIG_HARD_IGNORE)
    return;

  /* Make sure we have original_signals[sig] if the signal has not yet
     been trapped. */
  if ((sigmodes[sig] & SIG_TRAPPED) == 0)
    {
      /* If we aren't sure of the original value, check it. */
      if (original_signals[sig] == IMPOSSIBLE_TRAP_HANDLER)
	{
	  original_signals[sig] = (SigHandler *)set_signal_handler (sig, SIG_DFL);
	  set_signal_handler (sig, original_signals[sig]);
	}

      /* Signals ignored on entry to the shell cannot be trapped or reset. */
      if (original_signals[sig] == SIG_IGN)
	{
	  sigmodes[sig] |= SIG_HARD_IGNORE;
	  return;
	}
    }

  /* Only change the system signal handler if SIG_NO_TRAP is not set.
     The trap command string is changed in either case.  The shell signal
     handlers for SIGINT and SIGCHLD run the user specified traps in an
     environment in which it is safe to do so. */
  if ((sigmodes[sig] & SIG_NO_TRAP) == 0)
    {
      set_signal_handler (sig, SIG_IGN);
      change_signal (sig, savestring (string));
      set_signal_handler (sig, trap_handler);
    }
  else
    change_signal (sig, savestring (string));
}

static void
free_trap_command (sig)
     int sig;
{
  if ((sigmodes[sig] & SIG_TRAPPED) && trap_list[sig] &&
      (trap_list[sig] != (char *)IGNORE_SIG) &&
      (trap_list[sig] != (char *)DEFAULT_SIG) &&
      (trap_list[sig] != (char *)IMPOSSIBLE_TRAP_HANDLER))
    free (trap_list[sig]);
}

/* If SIG has a string assigned to it, get rid of it.  Then give it
   VALUE. */
static void
change_signal (sig, value)
     int sig;
     char *value;
{
  if ((sigmodes[sig] & SIG_INPROGRESS) == 0)
    free_trap_command (sig);
  trap_list[sig] = value;

  sigmodes[sig] |= SIG_TRAPPED;
  if (value == (char *)IGNORE_SIG)
    sigmodes[sig] |= SIG_IGNORED;
  else
    sigmodes[sig] &= ~SIG_IGNORED;
  if (sigmodes[sig] & SIG_INPROGRESS)
    sigmodes[sig] |= SIG_CHANGED;
}

#define GET_ORIGINAL_SIGNAL(sig) \
  if (sig && sig < NSIG && original_signals[sig] == IMPOSSIBLE_TRAP_HANDLER) \
    get_original_signal (sig)

static void
get_original_signal (sig)
     int sig;
{
  /* If we aren't sure the of the original value, then get it. */
  if (original_signals[sig] == (SigHandler *)IMPOSSIBLE_TRAP_HANDLER)
    {
      original_signals[sig] =
	(SigHandler *) set_signal_handler (sig, SIG_DFL);
      set_signal_handler (sig, original_signals[sig]);

      /* Signals ignored on entry to the shell cannot be trapped. */
      if (original_signals[sig] == SIG_IGN)
	sigmodes[sig] |= SIG_HARD_IGNORE;
    }
}

/* Restore the default action for SIG; i.e., the action the shell
   would have taken before you used the trap command.  This is called
   from trap_builtin (), which takes care to restore the handlers for
   the signals the shell treats specially. */
void
restore_default_signal (sig)
     int sig;
{
  if (SPECIAL_TRAP (sig))
    {
      if ((sig != DEBUG_TRAP && sig != ERROR_TRAP) || (sigmodes[sig] & SIG_INPROGRESS) == 0)
	free_trap_command (sig);
      trap_list[sig] = (char *)NULL;
      sigmodes[sig] &= ~SIG_TRAPPED;
      if (sigmodes[sig] & SIG_INPROGRESS)
	sigmodes[sig] |= SIG_CHANGED;
      return;
    }

  GET_ORIGINAL_SIGNAL (sig);

  /* A signal ignored on entry to the shell cannot be trapped or reset, but
     no error is reported when attempting to do so.  Thanks Posix.2. */
  if (sigmodes[sig] & SIG_HARD_IGNORE)
    return;

  /* If we aren't trapping this signal, don't bother doing anything else. */
  if ((sigmodes[sig] & SIG_TRAPPED) == 0)
    return;

  /* Only change the signal handler for SIG if it allows it. */
  if ((sigmodes[sig] & SIG_NO_TRAP) == 0)
    set_signal_handler (sig, original_signals[sig]);

  /* Change the trap command in either case. */
  change_signal (sig, (char *)DEFAULT_SIG);

  /* Mark the signal as no longer trapped. */
  sigmodes[sig] &= ~SIG_TRAPPED;
}

/* Make this signal be ignored. */
void
ignore_signal (sig)
     int sig;
{
  if (SPECIAL_TRAP (sig) && ((sigmodes[sig] & SIG_IGNORED) == 0))
    {
      change_signal (sig, (char *)IGNORE_SIG);
      return;
    }

  GET_ORIGINAL_SIGNAL (sig);

  /* A signal ignored on entry to the shell cannot be trapped or reset.
     No error is reported when the user attempts to do so. */
  if (sigmodes[sig] & SIG_HARD_IGNORE)
    return;

  /* If already trapped and ignored, no change necessary. */
  if (sigmodes[sig] & SIG_IGNORED)
    return;

  /* Only change the signal handler for SIG if it allows it. */
  if ((sigmodes[sig] & SIG_NO_TRAP) == 0)
    set_signal_handler (sig, SIG_IGN);

  /* Change the trap command in either case. */
  change_signal (sig, (char *)IGNORE_SIG);
}

/* Handle the calling of "trap 0".  The only sticky situation is when
   the command to be executed includes an "exit".  This is why we have
   to provide our own place for top_level to jump to. */
int
run_exit_trap ()
{
  char *trap_command;
  int code, old_exit_value;

  old_exit_value = last_command_exit_value;

  /* Run the trap only if signal 0 is trapped and not ignored, and we are not
     currently running in the trap handler (call to exit in the list of
     commands given to trap 0). */
  if ((sigmodes[EXIT_TRAP] & SIG_TRAPPED) &&
      (sigmodes[EXIT_TRAP] & (SIG_IGNORED|SIG_INPROGRESS)) == 0)
    {
      trap_command = savestring (trap_list[EXIT_TRAP]);
      sigmodes[EXIT_TRAP] &= ~SIG_TRAPPED;
      sigmodes[EXIT_TRAP] |= SIG_INPROGRESS;

      code = setjmp (top_level);

      if (code == 0)
	{
	  reset_parser ();
	  parse_and_execute (trap_command, "exit trap", SEVAL_NONINT|SEVAL_NOHIST);
	}
      else if (code == EXITPROG)
	return (last_command_exit_value);
      else
	return (old_exit_value);
    }

  return (old_exit_value);
}

void
run_trap_cleanup (sig)
     int sig;
{
  sigmodes[sig] &= ~(SIG_INPROGRESS|SIG_CHANGED);
}

/* Run a trap command for SIG.  SIG is one of the signals the shell treats
   specially. */
static void
_run_trap_internal (sig, tag)
     int sig;
     char *tag;
{
  char *trap_command, *old_trap;
  int old_exit_value, *token_state;

  /* Run the trap only if SIG is trapped and not ignored, and we are not
     currently executing in the trap handler. */
  if ((sigmodes[sig] & SIG_TRAPPED) && ((sigmodes[sig] & SIG_IGNORED) == 0) &&
      (trap_list[sig] != (char *)IMPOSSIBLE_TRAP_HANDLER) &&
      ((sigmodes[sig] & SIG_INPROGRESS) == 0))
    {
      old_trap = trap_list[sig];
      sigmodes[sig] |= SIG_INPROGRESS;
      sigmodes[sig] &= ~SIG_CHANGED;		/* just to be sure */
      trap_command =  savestring (old_trap);

      running_trap = sig + 1;
      old_exit_value = last_command_exit_value;
      /* Need to copy the value of line_number because parse_and_execute
	 resets it to 1, and the trap command might want it. */
      trap_line_number = line_number;

      token_state = save_token_state ();
      parse_and_execute (trap_command, tag, SEVAL_NONINT|SEVAL_NOHIST);
      restore_token_state (token_state);
      free (token_state);

      last_command_exit_value = old_exit_value;
      running_trap = 0;

      sigmodes[sig] &= ~SIG_INPROGRESS;

      if (sigmodes[sig] & SIG_CHANGED)
	{
	  free (old_trap);
	  sigmodes[sig] &= ~SIG_CHANGED;
	}
    }
}

void
run_debug_trap ()
{
  if ((sigmodes[DEBUG_TRAP] & SIG_TRAPPED) && ((sigmodes[DEBUG_TRAP] & SIG_INPROGRESS) == 0))
    _run_trap_internal (DEBUG_TRAP, "debug trap");
}

void
run_error_trap ()
{
  if ((sigmodes[ERROR_TRAP] & SIG_TRAPPED) && (sigmodes[ERROR_TRAP] & SIG_INPROGRESS) == 0)
    _run_trap_internal (ERROR_TRAP, "error trap");
}

/* Run a trap set on SIGINT.  This is called from throw_to_top_level (), and
   declared here to localize the trap functions. */
void
run_interrupt_trap ()
{
  _run_trap_internal (SIGINT, "interrupt trap");
}

#ifdef INCLUDE_UNUSED
/* Free all the allocated strings in the list of traps and reset the trap
   values to the default. */
void
free_trap_strings ()
{
  register int i;

  for (i = 0; i < BASH_NSIG; i++)
    {
      free_trap_command (i);
      trap_list[i] = (char *)DEFAULT_SIG;
      sigmodes[i] &= ~SIG_TRAPPED;
    }
  trap_list[DEBUG_TRAP] = trap_list[EXIT_TRAP] = trap_list[ERROR_TRAP] = (char *)NULL;
}
#endif

/* Reset the handler for SIG to the original value. */
static void
reset_signal (sig)
     int sig;
{
  set_signal_handler (sig, original_signals[sig]);
  sigmodes[sig] &= ~SIG_TRAPPED;
}

/* Set the handler signal SIG to the original and free any trap
   command associated with it. */
static void
restore_signal (sig)
     int sig;
{
  set_signal_handler (sig, original_signals[sig]);
  change_signal (sig, (char *)DEFAULT_SIG);
  sigmodes[sig] &= ~SIG_TRAPPED;
}

static void
reset_or_restore_signal_handlers (reset)
     sh_resetsig_func_t *reset;
{
  register int i;

  /* Take care of the exit trap first */
  if (sigmodes[EXIT_TRAP] & SIG_TRAPPED)
    {
      free_trap_command (EXIT_TRAP);
      trap_list[EXIT_TRAP] = (char *)NULL;
      sigmodes[EXIT_TRAP] &= ~SIG_TRAPPED;
    }
  for (i = 1; i < NSIG; i++)
    {
      if (sigmodes[i] & SIG_TRAPPED)
	{
	  if (trap_list[i] == (char *)IGNORE_SIG)
	    set_signal_handler (i, SIG_IGN);
	  else
	    (*reset) (i);
	}
      else if (sigmodes[i] & SIG_SPECIAL)
	(*reset) (i);
    }

  /* Command substitution and other child processes don't inherit the
     debug or error traps. */
  sigmodes[DEBUG_TRAP] &= ~SIG_TRAPPED;  
  sigmodes[ERROR_TRAP] &= ~SIG_TRAPPED;
}

/* Reset trapped signals to their original values, but don't free the
   trap strings.  Called by the command substitution code. */
void
reset_signal_handlers ()
{
  reset_or_restore_signal_handlers (reset_signal);
}

/* Reset all trapped signals to their original values.  Signals set to be
   ignored with trap '' SIGNAL should be ignored, so we make sure that they
   are.  Called by child processes after they are forked. */
void
restore_original_signals ()
{
  reset_or_restore_signal_handlers (restore_signal);
}

/* If a trap handler exists for signal SIG, then call it; otherwise just
   return failure. */
int
maybe_call_trap_handler (sig)
     int sig;
{
  /* Call the trap handler for SIG if the signal is trapped and not ignored. */
  if ((sigmodes[sig] & SIG_TRAPPED) && ((sigmodes[sig] & SIG_IGNORED) == 0))
    {
      switch (sig)
	{
	case SIGINT:
	  run_interrupt_trap ();
	  break;
	case EXIT_TRAP:
	  run_exit_trap ();
	  break;
	case DEBUG_TRAP:
	  run_debug_trap ();
	  break;
	case ERROR_TRAP:
	  run_error_trap ();
	  break;
	default:
	  trap_handler (sig);
	  break;
	}
      return (1);
    }
  else
    return (0);
}

int
signal_is_trapped (sig)
     int sig;
{
  return (sigmodes[sig] & SIG_TRAPPED);
}

int
signal_is_special (sig)
     int sig;
{
  return (sigmodes[sig] & SIG_SPECIAL);
}

int
signal_is_ignored (sig)
     int sig;
{
  return (sigmodes[sig] & SIG_IGNORED);
}

void
set_signal_ignored (sig)
     int sig;
{
  sigmodes[sig] |= SIG_HARD_IGNORE;
  original_signals[sig] = SIG_IGN;
}
