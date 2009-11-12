This file is wait.def, from which is created wait.c.
It implements the builtin "wait" in Bash.

Copyright (C) 1987-2009 Free Software Foundation, Inc.

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

$BUILTIN wait
$FUNCTION wait_builtin
$DEPENDS_ON JOB_CONTROL
$PRODUCES wait.c
$SHORT_DOC wait [id]
Wait for job completion and return exit status.

Waits for the process identified by ID, which may be a process ID or a
job specification, and reports its termination status.  If ID is not
given, waits for all currently active child processes, and the return
status is zero.  If ID is a a job specification, waits for all processes
in the job's pipeline.

Exit Status:
Returns the status of ID; fails if ID is invalid or an invalid option is
given.
$END

$BUILTIN wait
$FUNCTION wait_builtin
$DEPENDS_ON !JOB_CONTROL
$SHORT_DOC wait [pid]
Wait for process completion and return exit status.

Waits for the specified process and reports its termination status.  If
PID is not given, all currently active child processes are waited for,
and the return code is zero.  PID must be a process ID.

Exit Status:
Returns the status of ID; fails if ID is invalid or an invalid option is
given.
$END

#include <config.h>

#include "../bashtypes.h"
#include <signal.h>

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#include <chartypes.h>

#include "../bashansi.h"

#include "../shell.h"
#include "../jobs.h"
#include "common.h"
#include "bashgetopt.h"

extern int wait_signal_received;

procenv_t wait_intr_buf;

/* Wait for the pid in LIST to stop or die.  If no arguments are given, then
   wait for all of the active background processes of the shell and return
   0.  If a list of pids or job specs are given, return the exit status of
   the last one waited for. */

#define WAIT_RETURN(s) \
  do \
    { \
      interrupt_immediately = old_interrupt_immediately;\
      return (s);\
    } \
  while (0)

int
wait_builtin (list)
     WORD_LIST *list;
{
  int status, code;
  volatile int old_interrupt_immediately;

  USE_VAR(list);

  if (no_options (list))
    return (EX_USAGE);
  list = loptend;

  old_interrupt_immediately = interrupt_immediately;
  interrupt_immediately++;

  /* POSIX.2 says:  When the shell is waiting (by means of the wait utility)
     for asynchronous commands to complete, the reception of a signal for
     which a trap has been set shall cause the wait utility to return
     immediately with an exit status greater than 128, after which the trap
     associated with the signal shall be taken.

     We handle SIGINT here; it's the only one that needs to be treated
     specially (I think), since it's handled specially in {no,}jobs.c. */
  code = setjmp (wait_intr_buf);
  if (code)
    {
      status = 128 + wait_signal_received;
      WAIT_RETURN (status);
    }

  /* We support jobs or pids.
     wait <pid-or-job> [pid-or-job ...] */

  /* But wait without any arguments means to wait for all of the shell's
     currently active background processes. */
  if (list == 0)
    {
      wait_for_background_pids ();
      WAIT_RETURN (EXECUTION_SUCCESS);
    }

  status = EXECUTION_SUCCESS;
  while (list)
    {
      pid_t pid;
      char *w;
      intmax_t pid_value;

      w = list->word->word;
      if (DIGIT (*w))
	{
	  if (legal_number (w, &pid_value) && pid_value == (pid_t)pid_value)
	    {
	      pid = (pid_t)pid_value;
	      status = wait_for_single_pid (pid);
	    }
	  else
	    {
	      sh_badpid (w);
	      WAIT_RETURN (EXECUTION_FAILURE);
	    }
	}
#if defined (JOB_CONTROL)
      else if (*w && *w == '%')
	/* Must be a job spec.  Check it out. */
	{
	  int job;
	  sigset_t set, oset;

	  BLOCK_CHILD (set, oset);
	  job = get_job_spec (list);

	  if (INVALID_JOB (job))
	    {
	      if (job != DUP_JOB)
		sh_badjob (list->word->word);
	      UNBLOCK_CHILD (oset);
	      status = 127;	/* As per Posix.2, section 4.70.2 */
	      list = list->next;
	      continue;
	    }

	  /* Job spec used.  Wait for the last pid in the pipeline. */
	  UNBLOCK_CHILD (oset);
	  status = wait_for_job (job);
	}
#endif /* JOB_CONTROL */
      else
	{
	  sh_badpid (w);
	  status = EXECUTION_FAILURE;
	}
      list = list->next;
    }

  WAIT_RETURN (status);
}
