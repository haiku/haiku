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
#  include <unistd.h>
#endif

#include "../bashtypes.h"
#include "posixstat.h"
#include "filecntl.h"

#include <stdio.h>
#include <signal.h>
#include <errno.h>

#include "../bashansi.h"

#include "../shell.h"
#include "../jobs.h"
#include "../builtins.h"
#include "../flags.h"
#include "../input.h"
#include "../execute_cmd.h"

#if defined (HISTORY)
#  include "../bashhist.h"
#endif

#include "common.h"

#if !defined (errno)
extern int errno;
#endif

/* Flags for _evalfile() */
#define FEVAL_ENOENTOK		0x001
#define FEVAL_BUILTIN		0x002
#define FEVAL_UNWINDPROT	0x004
#define FEVAL_NONINT		0x008
#define FEVAL_LONGJMP		0x010
#define FEVAL_HISTORY		0x020
#define FEVAL_CHECKBINARY	0x040
#define FEVAL_REGFILE		0x080

extern int posixly_correct;
extern int indirection_level, startup_state, subshell_environment;
extern int return_catch_flag, return_catch_value;
extern int last_command_exit_value;

/* How many `levels' of sourced files we have. */
int sourcelevel = 0;

static int
_evalfile (filename, flags)
     const char *filename;
     int flags;
{
  volatile int old_interactive;
  procenv_t old_return_catch;
  int return_val, fd, result, pflags;
  char *string;
  struct stat finfo;
  size_t file_size;
  sh_vmsg_func_t *errfunc;

  USE_VAR(pflags);

  fd = open (filename, O_RDONLY);

  if (fd < 0 || (fstat (fd, &finfo) == -1))
    {
file_error_and_exit:
      if (((flags & FEVAL_ENOENTOK) == 0) || errno != ENOENT)
	file_error (filename);

      if (flags & FEVAL_LONGJMP)
	{
	  last_command_exit_value = 1;
	  jump_to_top_level (EXITPROG);
	}

      return ((flags & FEVAL_BUILTIN) ? EXECUTION_FAILURE
      				      : ((errno == ENOENT) ? 0 : -1));
    }

  errfunc = ((flags & FEVAL_BUILTIN) ? builtin_error : internal_error);

  if (S_ISDIR (finfo.st_mode))
    {
      (*errfunc) ("%s: is a directory", filename);
      return ((flags & FEVAL_BUILTIN) ? EXECUTION_FAILURE : -1);
    }
  else if ((flags & FEVAL_REGFILE) && S_ISREG (finfo.st_mode) == 0)
    {
      (*errfunc) ("%s: not a regular file", filename);
      return ((flags & FEVAL_BUILTIN) ? EXECUTION_FAILURE : -1);
    }

  file_size = (size_t)finfo.st_size;
  /* Check for overflow with large files. */
  if (file_size != finfo.st_size || file_size + 1 < file_size)
    {
      (*errfunc) ("%s: file is too large", filename);
      return ((flags & FEVAL_BUILTIN) ? EXECUTION_FAILURE : -1);
    }      

#if defined (__CYGWIN__) && defined (O_TEXT)
  setmode (fd, O_TEXT);
#endif

  string = (char *)xmalloc (1 + file_size);
  result = read (fd, string, file_size);
  string[result] = '\0';

  return_val = errno;
  close (fd);
  errno = return_val;

  if (result < 0)		/* XXX was != file_size, not < 0 */
    {
      free (string);
      goto file_error_and_exit;
    }

  if (result == 0)
    {
      free (string);
      return ((flags & FEVAL_BUILTIN) ? EXECUTION_SUCCESS : 1);
    }
      
  if ((flags & FEVAL_CHECKBINARY) && 
      check_binary_file (string, (result > 80) ? 80 : result))
    {
      free (string);
      (*errfunc) ("%s: cannot execute binary file", filename);
      return ((flags & FEVAL_BUILTIN) ? EX_BINARY_FILE : -1);
    }

  if (flags & FEVAL_UNWINDPROT)
    {
      begin_unwind_frame ("_evalfile");

      unwind_protect_int (return_catch_flag);
      unwind_protect_jmp_buf (return_catch);
      if (flags & FEVAL_NONINT)
	unwind_protect_int (interactive);
      unwind_protect_int (sourcelevel);
    }
  else
    {
      COPY_PROCENV (return_catch, old_return_catch);
      if (flags & FEVAL_NONINT)
	old_interactive = interactive;
    }

  if (flags & FEVAL_NONINT)
    interactive = 0;

  return_catch_flag++;
  sourcelevel++;

  /* set the flags to be passed to parse_and_execute */
  pflags = (flags & FEVAL_HISTORY) ? 0 : SEVAL_NOHIST;

  if (flags & FEVAL_BUILTIN)
    result = EXECUTION_SUCCESS;

  return_val = setjmp (return_catch);

  /* If `return' was seen outside of a function, but in the script, then
     force parse_and_execute () to clean up. */
  if (return_val)
    {
      parse_and_execute_cleanup ();
      result = return_catch_value;
    }
  else
    result = parse_and_execute (string, filename, pflags);

  if (flags & FEVAL_UNWINDPROT)
    run_unwind_frame ("_evalfile");
  else
    {
      if (flags & FEVAL_NONINT)
	interactive = old_interactive;
      return_catch_flag--;
      sourcelevel--;
      COPY_PROCENV (old_return_catch, return_catch);
    }

  return ((flags & FEVAL_BUILTIN) ? result : 1);
}

int
maybe_execute_file (fname, force_noninteractive)
     const char *fname;
     int force_noninteractive;
{
  char *filename;
  int result, flags;

  filename = bash_tilde_expand (fname, 0);
  flags = FEVAL_ENOENTOK;
  if (force_noninteractive)
    flags |= FEVAL_NONINT;
  result = _evalfile (filename, flags);
  free (filename);
  return result;
}

#if defined (HISTORY)
int
fc_execute_file (filename)
     const char *filename;
{
  int flags;

  /* We want these commands to show up in the history list if
     remember_on_history is set. */
  flags = FEVAL_ENOENTOK|FEVAL_HISTORY|FEVAL_REGFILE;
  return (_evalfile (filename, flags));
}
#endif /* HISTORY */

int
source_file (filename)
     const char *filename;
{
  int flags;

  flags = FEVAL_BUILTIN|FEVAL_UNWINDPROT|FEVAL_NONINT;
  /* POSIX shells exit if non-interactive and file error. */
  if (posixly_correct && !interactive_shell)
    flags |= FEVAL_LONGJMP;
  return (_evalfile (filename, flags));
}
