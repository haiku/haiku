/* Copyright (C) 1996 Free Software Foundation, Inc.

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

#include <config.h>

#if defined (HAVE_UNISTD_H)
#  ifdef _MINIX
#    include <sys/types.h>
#  endif
#  include <unistd.h>
#endif

#include <stdio.h>
#include <signal.h>

#include <errno.h>

#include "filecntl.h"
#include "../bashansi.h"

#include "../shell.h"
#include "../jobs.h"
#include "../builtins.h"
#include "../flags.h"
#include "../input.h"
#include "../execute_cmd.h"
#include "../redir.h"
#include "../trap.h"

#if defined (HISTORY)
#  include "../bashhist.h"
#endif

#include "common.h"

#if !defined (errno)
extern int errno;
#endif

#define IS_BUILTIN(s)	(builtin_address_internal(s, 0) != (struct builtin *)NULL)

extern int indirection_level, startup_state, subshell_environment;
extern int line_number;
extern int last_command_exit_value;
extern int running_trap;
extern int posixly_correct;

int parse_and_execute_level = 0;

static int cat_file __P((REDIRECT *));

/* How to force parse_and_execute () to clean up after itself. */
void
parse_and_execute_cleanup ()
{
  if (running_trap)
    {
      run_trap_cleanup (running_trap - 1);
      unfreeze_jobs_list ();
    }
  run_unwind_frame ("parse_and_execute_top");
}

/* Parse and execute the commands in STRING.  Returns whatever
   execute_command () returns.  This frees STRING.  FLAGS is a
   flags word; look in common.h for the possible values.  Actions
   are:
   	(flags & SEVAL_NONINT) -> interactive = 0;
   	(flags & SEVAL_INTERACT) -> interactive = 1;
   	(flags & SEVAL_NOHIST) -> call bash_history_disable ()
   	(flags & SEVAL_NOFREE) -> don't free STRING when finished
*/

int
parse_and_execute (string, from_file, flags)
     char *string;
     const char *from_file;
     int flags;
{
  int code, x;
  volatile int should_jump_to_top_level, last_result;
  char *orig_string;
  COMMAND *volatile command;

  orig_string = string;
  /* Unwind protect this invocation of parse_and_execute (). */
  begin_unwind_frame ("parse_and_execute_top");
  unwind_protect_int (parse_and_execute_level);
  unwind_protect_jmp_buf (top_level);
  unwind_protect_int (indirection_level);
  unwind_protect_int (line_number);
  if (flags & (SEVAL_NONINT|SEVAL_INTERACT))
    unwind_protect_int (interactive);

#if defined (HISTORY)
  unwind_protect_int (remember_on_history);	/* can be used in scripts */
#  if defined (BANG_HISTORY)
  if (interactive_shell)
    {
      unwind_protect_int (history_expansion_inhibited);
    }
#  endif /* BANG_HISTORY */
#endif /* HISTORY */

  if (interactive_shell)
    {
      x = get_current_prompt_level ();
      add_unwind_protect (set_current_prompt_level, x);
    }
  
  add_unwind_protect (pop_stream, (char *)NULL);
  if (orig_string && ((flags & SEVAL_NOFREE) == 0))
    add_unwind_protect (xfree, orig_string);
  end_unwind_frame ();

  parse_and_execute_level++;
  push_stream (1);	/* reset the line number */
  indirection_level++;
  if (flags & (SEVAL_NONINT|SEVAL_INTERACT))
    interactive = (flags & SEVAL_NONINT) ? 0 : 1;

#if defined (HISTORY)
  if (flags & SEVAL_NOHIST)
    bash_history_disable ();
#endif /* HISTORY */

  code = should_jump_to_top_level = 0;
  last_result = EXECUTION_SUCCESS;
  command = (COMMAND *)NULL;

  with_input_from_string (string, from_file);
  while (*(bash_input.location.string))
    {
      if (interrupt_state)
	{
	  last_result = EXECUTION_FAILURE;
	  break;
	}

      /* Provide a location for functions which `longjmp (top_level)' to
	 jump to.  This prevents errors in substitution from restarting
	 the reader loop directly, for example. */
      code = setjmp (top_level);

      if (code)
	{
	  should_jump_to_top_level = 0;
	  switch (code)
	    {
	    case FORCE_EOF:
	    case EXITPROG:
	      run_unwind_frame ("pe_dispose");
	      /* Remember to call longjmp (top_level) after the old
		 value for it is restored. */
	      should_jump_to_top_level = 1;
	      goto out;

	    case DISCARD:
	      run_unwind_frame ("pe_dispose");
	      last_result = last_command_exit_value = EXECUTION_FAILURE; /* XXX */
	      if (subshell_environment)
		{
		  should_jump_to_top_level = 1;
		  goto out;
		}
	      else
		{
#if 0
		  dispose_command (command);	/* pe_dispose does this */
#endif
		  continue;
		}

	    default:
	      command_error ("parse_and_execute", CMDERR_BADJUMP, code, 0);
	      break;
	    }
	}
	  
      if (parse_command () == 0)
	{
	  if (interactive_shell == 0 && read_but_dont_execute)
	    {
	      last_result = EXECUTION_SUCCESS;
	      dispose_command (global_command);
	      global_command = (COMMAND *)NULL;
	    }
	  else if (command = global_command)
	    {
	      struct fd_bitmap *bitmap;

	      bitmap = new_fd_bitmap (FD_BITMAP_SIZE);
	      begin_unwind_frame ("pe_dispose");
	      add_unwind_protect (dispose_fd_bitmap, bitmap);
	      add_unwind_protect (dispose_command, command);	/* XXX */

	      global_command = (COMMAND *)NULL;

#if defined (ONESHOT)
	      /*
	       * IF
	       *   we were invoked as `bash -c' (startup_state == 2) AND
	       *   parse_and_execute has not been called recursively AND
	       *   we have parsed the full command (string == '\0') AND
	       *   we have a simple command without redirections AND
	       *   the command is not being timed
	       * THEN
	       *   tell the execution code that we don't need to fork
	       */
	      if (startup_state == 2 && parse_and_execute_level == 1 &&
		  *bash_input.location.string == '\0' &&
		  command->type == cm_simple &&
		  !command->redirects && !command->value.Simple->redirects &&
		  ((command->flags & CMD_TIME_PIPELINE) == 0))
		{
		  command->flags |= CMD_NO_FORK;
		  command->value.Simple->flags |= CMD_NO_FORK;
		}
#endif /* ONESHOT */

	      /* See if this is a candidate for $( <file ). */
	      if (startup_state == 2 &&
		  (subshell_environment & SUBSHELL_COMSUB) &&
		  *bash_input.location.string == '\0' &&
		  command->type == cm_simple && !command->redirects &&
		  (command->flags & CMD_TIME_PIPELINE) == 0 &&
		  command->value.Simple->words == 0 &&
		  command->value.Simple->redirects &&
		  command->value.Simple->redirects->next == 0 &&
		  command->value.Simple->redirects->instruction == r_input_direction)
		{
		  int r;
		  r = cat_file (command->value.Simple->redirects);
		  last_result = (r < 0) ? EXECUTION_FAILURE : EXECUTION_SUCCESS;
		}
	      else
		last_result = execute_command_internal
				(command, 0, NO_PIPE, NO_PIPE, bitmap);

	      dispose_command (command);
	      dispose_fd_bitmap (bitmap);
	      discard_unwind_frame ("pe_dispose");
	    }
	}
      else
	{
	  last_result = EXECUTION_FAILURE;

	  /* Since we are shell compatible, syntax errors in a script
	     abort the execution of the script.  Right? */
	  break;
	}
    }

 out:

  run_unwind_frame ("parse_and_execute_top");

  if (interrupt_state && parse_and_execute_level == 0)
    {
      /* An interrupt during non-interactive execution in an
	 interactive shell (e.g. via $PROMPT_COMMAND) should
	 not cause the shell to exit. */
      interactive = interactive_shell;
      throw_to_top_level ();
    }

  if (should_jump_to_top_level)
    jump_to_top_level (code);

  return (last_result);
}

/* Handle a $( < file ) command substitution.  This expands the filename,
   returning errors as appropriate, then just cats the file to the standard
   output. */
static int
cat_file (r)
     REDIRECT *r;
{
  char lbuf[128], *fn;
  int fd, rval;
  ssize_t nr;

  if (r->instruction != r_input_direction)
    return -1;

  /* Get the filename. */
  if (posixly_correct && !interactive_shell)
    disallow_filename_globbing++;
  fn = redirection_expand (r->redirectee.filename);
  if (posixly_correct && !interactive_shell)
    disallow_filename_globbing--;

  if (fn == 0)
    {
      redirection_error (r, AMBIGUOUS_REDIRECT);
      return -1;
    }

  fd = open(fn, O_RDONLY);
  if (fd < 0)
    {
      file_error (fn);
      free (fn);
      return -1;
    }

  rval = zcatfd (fd, 1, fn);

  free (fn);
  close (fd);

  return (rval);
}
