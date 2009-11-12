/* evalstring.c - evaluate a string as one or more shell commands. 

/* Copyright (C) 1996-2009 Free Software Foundation, Inc.

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

#include <y.tab.h>

#if defined (HISTORY)
#  include "../bashhist.h"
#endif

#include "common.h"

#if !defined (errno)
extern int errno;
#endif

#define IS_BUILTIN(s)	(builtin_address_internal(s, 0) != (struct builtin *)NULL)

extern int indirection_level, subshell_environment;
extern int line_number;
extern int current_token, shell_eof_token;
extern int last_command_exit_value;
extern int running_trap;
extern int loop_level;
extern int executing_list;
extern int comsub_ignore_return;
extern int posixly_correct;

int parse_and_execute_level = 0;

static int cat_file __P((REDIRECT *));

#define PE_TAG "parse_and_execute top"
#define PS_TAG "parse_string top"

#if defined (HISTORY)
static void
set_history_remembering ()
{
  remember_on_history = enable_history_list;
}
#endif

/* How to force parse_and_execute () to clean up after itself. */
void
parse_and_execute_cleanup ()
{
  if (running_trap)
    {
      run_trap_cleanup (running_trap - 1);
      unfreeze_jobs_list ();
    }

  if (have_unwind_protects ())
     run_unwind_frame (PE_TAG);
  else
    parse_and_execute_level = 0;			/* XXX */
}

static void
parse_prologue (string, flags, tag)
     char *string;
     int flags;
     char *tag;
{
  char *orig_string;
  int x;

  orig_string = string;
  /* Unwind protect this invocation of parse_and_execute (). */
  begin_unwind_frame (tag);
  unwind_protect_int (parse_and_execute_level);
  unwind_protect_jmp_buf (top_level);
  unwind_protect_int (indirection_level);
  unwind_protect_int (line_number);
  unwind_protect_int (loop_level);
  unwind_protect_int (executing_list);
  unwind_protect_int (comsub_ignore_return);
  if (flags & (SEVAL_NONINT|SEVAL_INTERACT))
    unwind_protect_int (interactive);

#if defined (HISTORY)
  if (parse_and_execute_level == 0)
    add_unwind_protect (set_history_remembering, (char *)NULL);
  else
    unwind_protect_int (remember_on_history);	/* can be used in scripts */
#  if defined (BANG_HISTORY)
  if (interactive_shell)
    unwind_protect_int (history_expansion_inhibited);
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

  if (flags & (SEVAL_NONINT|SEVAL_INTERACT))
    interactive = (flags & SEVAL_NONINT) ? 0 : 1;

#if defined (HISTORY)
  if (flags & SEVAL_NOHIST)
    bash_history_disable ();
#endif /* HISTORY */
}

/* Parse and execute the commands in STRING.  Returns whatever
   execute_command () returns.  This frees STRING.  FLAGS is a
   flags word; look in common.h for the possible values.  Actions
   are:
   	(flags & SEVAL_NONINT) -> interactive = 0;
   	(flags & SEVAL_INTERACT) -> interactive = 1;
   	(flags & SEVAL_NOHIST) -> call bash_history_disable ()
   	(flags & SEVAL_NOFREE) -> don't free STRING when finished
   	(flags & SEVAL_RESETLINE) -> reset line_number to 1
*/

int
parse_and_execute (string, from_file, flags)
     char *string;
     const char *from_file;
     int flags;
{
  int code, lreset;
  volatile int should_jump_to_top_level, last_result;
  COMMAND *volatile command;

  parse_prologue (string, flags, PE_TAG);

  parse_and_execute_level++;

  lreset = flags & SEVAL_RESETLINE;

  /* Reset the line number if the caller wants us to.  If we don't reset the
     line number, we have to subtract one, because we will add one just
     before executing the next command (resetting the line number sets it to
     0; the first line number is 1). */
  push_stream (lreset);
  if (lreset == 0)
    line_number--;
    
  indirection_level++;

  code = should_jump_to_top_level = 0;
  last_result = EXECUTION_SUCCESS;

  with_input_from_string (string, from_file);
  while (*(bash_input.location.string))
    {
      command = (COMMAND *)NULL;

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
	    case ERREXIT:
	    case EXITPROG:
	      if (command)
		run_unwind_frame ("pe_dispose");
	      /* Remember to call longjmp (top_level) after the old
		 value for it is restored. */
	      should_jump_to_top_level = 1;
	      goto out;

	    case DISCARD:
	      if (command)
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
	  if ((flags & SEVAL_PARSEONLY) || (interactive_shell == 0 && read_but_dont_execute))
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

	      if ((subshell_environment & SUBSHELL_COMSUB) && comsub_ignore_return)
		command->flags |= CMD_IGNORE_RETURN;

#if defined (ONESHOT)
	      /*
	       * IF
	       *   we were invoked as `bash -c' (startup_state == 2) AND
	       *   parse_and_execute has not been called recursively AND
	       *   we're not running a trap AND
	       *   we have parsed the full command (string == '\0') AND
	       *   we're not going to run the exit trap AND
	       *   we have a simple command without redirections AND
	       *   the command is not being timed AND
	       *   the command's return status is not being inverted
	       * THEN
	       *   tell the execution code that we don't need to fork
	       */
	      if (startup_state == 2 && parse_and_execute_level == 1 &&
		  running_trap == 0 &&
		  *bash_input.location.string == '\0' &&
		  command->type == cm_simple &&
		  signal_is_trapped (EXIT_TRAP) == 0 &&
		  command->redirects == 0 && command->value.Simple->redirects == 0 &&
		  ((command->flags & CMD_TIME_PIPELINE) == 0) &&
		  ((command->flags & CMD_INVERT_RETURN) == 0))
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

  run_unwind_frame (PE_TAG);

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

/* Parse a command contained in STRING according to FLAGS and return the
   number of characters consumed from the string.  If non-NULL, set *ENDP
   to the position in the string where the parse ended.  Used to validate
   command substitutions during parsing to obey Posix rules about finding
   the end of the command and balancing parens. */
int
parse_string (string, from_file, flags, endp)
     char *string;
     const char *from_file;
     int flags;
     char **endp;
{
  int code, nc;
  volatile int should_jump_to_top_level;
  COMMAND *volatile command, *oglobal;
  char *ostring;

  parse_prologue (string, flags, PS_TAG);

  /* Reset the line number if the caller wants us to.  If we don't reset the
     line number, we have to subtract one, because we will add one just
     before executing the next command (resetting the line number sets it to
     0; the first line number is 1). */
  push_stream (0);
    
  code = should_jump_to_top_level = 0;
  oglobal = global_command;
  ostring = string;

  with_input_from_string (string, from_file);
  while (*(bash_input.location.string))
    {
      command = (COMMAND *)NULL;

#if 0
      if (interrupt_state)
	break;
#endif

      /* Provide a location for functions which `longjmp (top_level)' to
	 jump to. */
      code = setjmp (top_level);

      if (code)
	{
#if defined (DEBUG)
itrace("parse_string: longjmp executed: code = %d", code);
#endif
	  should_jump_to_top_level = 0;
	  switch (code)
	    {
	    case FORCE_EOF:
	    case ERREXIT:
	    case EXITPROG:
	    case DISCARD:		/* XXX */
	      if (command)
		dispose_command (command);
	      /* Remember to call longjmp (top_level) after the old
		 value for it is restored. */
	      should_jump_to_top_level = 1;
	      goto out;

	    default:
	      command_error ("parse_string", CMDERR_BADJUMP, code, 0);
	      break;
	    }
	}
	  
      if (parse_command () == 0)
	{
	  dispose_command (global_command);
	  global_command = (COMMAND *)NULL;
	}
      else
	{
	  if ((flags & SEVAL_NOLONGJMP) == 0)
	    {
	      should_jump_to_top_level = 1;
	      code = DISCARD;
	    }
	  else
	    reset_parser ();	/* XXX - sets token_to_read */
	  break;
	}

      if (current_token == yacc_EOF || current_token == shell_eof_token)
	  break;
    }

 out:

  global_command = oglobal;
  nc = bash_input.location.string - ostring;
  if (endp)
    *endp = bash_input.location.string;

  run_unwind_frame (PS_TAG);

  if (should_jump_to_top_level)
    jump_to_top_level (code);

  return (nc);
}

/* Handle a $( < file ) command substitution.  This expands the filename,
   returning errors as appropriate, then just cats the file to the standard
   output. */
static int
cat_file (r)
     REDIRECT *r;
{
  char *fn;
  int fd, rval;

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
