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

#include <config.h>

#if defined (HAVE_UNISTD_H)
#  ifdef _MINIX
#    include <sys/types.h>
#  endif
#  include <unistd.h>
#endif

#include <stdio.h>
#include <chartypes.h>
#include "../bashtypes.h"
#include "posixstat.h"
#include <signal.h>

#include <errno.h>

#if defined (PREFER_STDARG)
#  include <stdarg.h>
#else
#  include <varargs.h>
#endif

#include "../bashansi.h"

#include "../shell.h"
#include "maxpath.h"
#include "../flags.h"
#include "../jobs.h"
#include "../builtins.h"
#include "../input.h"
#include "../execute_cmd.h"
#include "../trap.h"
#include "bashgetopt.h"
#include "common.h"
#include "builtext.h"
#include <tilde/tilde.h>

#if defined (HISTORY)
#  include "../bashhist.h"
#endif

#if !defined (errno)
extern int errno;   
#endif /* !errno */

extern int indirection_level, subshell_environment;
extern int line_number;
extern int last_command_exit_value;
extern int running_trap;
extern int posixly_correct;
extern char *this_command_name, *shell_name;
extern char *bash_getcwd_errstr;

/* Used by some builtins and the mainline code. */
sh_builtin_func_t *last_shell_builtin = (sh_builtin_func_t *)NULL;
sh_builtin_func_t *this_shell_builtin = (sh_builtin_func_t *)NULL;

/* **************************************************************** */
/*								    */
/*	     Error reporting, usage, and option processing	    */
/*								    */
/* **************************************************************** */

/* This is a lot like report_error (), but it is for shell builtins
   instead of shell control structures, and it won't ever exit the
   shell. */
void
#if defined (PREFER_STDARG)
builtin_error (const char *format, ...)
#else
builtin_error (format, va_alist)
     const char *format;
     va_dcl
#endif
{
  va_list args;
  char *name;

  name = get_name_for_error ();
  fprintf (stderr, "%s: ", name);

  if (interactive_shell == 0)
    fprintf (stderr, "line %d: ", executing_line_number ());

  if (this_command_name && *this_command_name)
    fprintf (stderr, "%s: ", this_command_name);

  SH_VA_START (args, format);

  vfprintf (stderr, format, args);
  va_end (args);
  fprintf (stderr, "\n");
}

/* Print a usage summary for the currently-executing builtin command. */
void
builtin_usage ()
{
  if (this_command_name && *this_command_name)
    fprintf (stderr, "%s: usage: ", this_command_name);
  fprintf (stderr, "%s\n", current_builtin->short_doc);
  fflush (stderr);
}

/* Return if LIST is NULL else barf and jump to top_level.  Used by some
   builtins that do not accept arguments. */
void
no_args (list)
     WORD_LIST *list;
{
  if (list)
    {
      builtin_error ("too many arguments");
      jump_to_top_level (DISCARD);
    }
}

/* Check that no options were given to the currently-executing builtin,
   and return 0 if there were options. */
int
no_options (list)
     WORD_LIST *list;
{
  reset_internal_getopt ();
  if (internal_getopt (list, "") != -1)
    {
      builtin_usage ();
      return (1);
    }
  return (0);
}

void
sh_needarg (s)
     char *s;
{
  builtin_error ("%s: option requires an argument", s);
}

void
sh_neednumarg (s)
     char *s;
{
  builtin_error ("%s: numeric argument required", s);
}

void
sh_notfound (s)
     char *s;
{
  builtin_error ("%s: not found", s);
}

/* Function called when one of the builtin commands detects an invalid
   option. */
void
sh_invalidopt (s)
     char *s;
{
  builtin_error ("%s: invalid option", s);
}

void
sh_invalidoptname (s)
     char *s;
{
  builtin_error ("%s: invalid option name", s);
}

void
sh_invalidid (s)
     char *s;
{
  builtin_error ("`%s': not a valid identifier", s);
}

void
sh_invalidnum (s)
     char *s;
{
  builtin_error ("%s: invalid number", s);
}

void
sh_invalidsig (s)
     char *s;
{
  builtin_error ("%s: invalid signal specification", s);
}

void
sh_badpid (s)
     char *s;
{
  builtin_error ("`%s': not a pid or valid job spec", s);
}

void
sh_readonly (s)
     const char *s;
{
  builtin_error ("%s: readonly variable", s);
}

void
sh_erange (s, desc)
     char *s, *desc;
{
  if (s)
    builtin_error ("%s: %s out of range", s, desc ? desc : "argument");
  else
    builtin_error ("%s out of range", desc ? desc : "argument");
}

#if defined (JOB_CONTROL)
void
sh_badjob (s)
     char *s;
{
  builtin_error ("%s: no such job", s);
}

void
sh_nojobs (s)
     char *s;
{
  if (s)
    builtin_error ("%s: no job control");
  else
    builtin_error ("no job control");
}
#endif

#if defined (RESTRICTED_SHELL)
void
sh_restricted (s)
     char *s;
{
  if (s)
    builtin_error ("%s: restricted", s);
  else
    builtin_error ("restricted");
}
#endif

/* **************************************************************** */
/*								    */
/*	     Shell positional parameter manipulation		    */
/*								    */
/* **************************************************************** */

/* Convert a WORD_LIST into a C-style argv.  Return the number of elements
   in the list in *IP, if IP is non-null.  A convenience function for
   loadable builtins; also used by `test'. */
char **
make_builtin_argv (list, ip)
     WORD_LIST *list;
     int *ip;
{
  char **argv;

  argv = strvec_from_word_list (list, 0, 1, ip);
  argv[0] = this_command_name;
  return argv;
}

/* Remember LIST in $0 ... $9, and REST_OF_ARGS.  If DESTRUCTIVE is
   non-zero, then discard whatever the existing arguments are, else
   only discard the ones that are to be replaced. */
void
remember_args (list, destructive)
     WORD_LIST *list;
     int destructive;
{
  register int i;

  for (i = 1; i < 10; i++)
    {
      if ((destructive || list) && dollar_vars[i])
	{
	  free (dollar_vars[i]);
	  dollar_vars[i] = (char *)NULL;
	}

      if (list)
	{
	  dollar_vars[i] = savestring (list->word->word);
	  list = list->next;
	}
    }

  /* If arguments remain, assign them to REST_OF_ARGS.
     Note that copy_word_list (NULL) returns NULL, and
     that dispose_words (NULL) does nothing. */
  if (destructive || list)
    {
      dispose_words (rest_of_args);
      rest_of_args = copy_word_list (list);
    }

  if (destructive)
    set_dollar_vars_changed ();
}

static int changed_dollar_vars;

/* Have the dollar variables been reset to new values since we last
   checked? */
int
dollar_vars_changed ()
{
  return (changed_dollar_vars);
}

void
set_dollar_vars_unchanged ()
{
  changed_dollar_vars = 0;
}

void
set_dollar_vars_changed ()
{
  if (variable_context)
    changed_dollar_vars |= ARGS_FUNC;
  else if (this_shell_builtin == set_builtin)
    changed_dollar_vars |= ARGS_SETBLTIN;
  else
    changed_dollar_vars |= ARGS_INVOC;
}

/* **************************************************************** */
/*								    */
/*		Validating numeric input and arguments		    */
/*								    */
/* **************************************************************** */

/* Read a numeric arg for this_command_name, the name of the shell builtin
   that wants it.  LIST is the word list that the arg is to come from.
   Accept only the numeric argument; report an error if other arguments
   follow.  If FATAL is true, call throw_to_top_level, which exits the
   shell; if not, call jump_to_top_level (DISCARD), which aborts the
   current command. */
intmax_t
get_numeric_arg (list, fatal)
     WORD_LIST *list;
     int fatal;
{
  intmax_t count = 1;

  if (list && list->word && ISOPTION (list->word->word, '-'))
    list = list->next;

  if (list)
    {
      register char *arg;

      arg = list->word->word;
      if (arg == 0 || (legal_number (arg, &count) == 0))
	{
	  sh_neednumarg (list->word->word);
	  if (fatal)
	    throw_to_top_level ();
	  else
	    jump_to_top_level (DISCARD);
	}
      no_args (list->next);
    }

  return (count);
}

/* Get an eight-bit status value from LIST */
int
get_exitstat (list)
     WORD_LIST *list;
{
  int status;
  intmax_t sval;
  char *arg;

  if (list && list->word && ISOPTION (list->word->word, '-'))
    list = list->next;

  if (list == 0)
    return (last_command_exit_value);      

  arg = list->word->word;
  if (arg == 0 || legal_number (arg, &sval) == 0)
    {
      sh_neednumarg (list->word->word ? list->word->word : "`'");
      return 255;
    }
  no_args (list->next);

  status = sval & 255;
  return status;
}

/* Return the octal number parsed from STRING, or -1 to indicate
   that the string contained a bad number. */
int
read_octal (string)
     char *string;
{
  int result, digits;

  result = digits = 0;
  while (*string && ISOCTAL (*string))
    {
      digits++;
      result = (result * 8) + (*string++ - '0');
      if (result > 0777)
	return -1;
    }

  if (digits == 0 || *string)
    result = -1;

  return (result);
}

/* **************************************************************** */
/*								    */
/*	     Manipulating the current working directory		    */
/*								    */
/* **************************************************************** */

/* Return a consed string which is the current working directory.
   FOR_WHOM is the name of the caller for error printing.  */
char *the_current_working_directory = (char *)NULL;

char *
get_working_directory (for_whom)
     char *for_whom;
{
  char *directory;

  if (no_symbolic_links)
    {
      if (the_current_working_directory)
	free (the_current_working_directory);

      the_current_working_directory = (char *)NULL;
    }

  if (the_current_working_directory == 0)
    {
      the_current_working_directory = (char *)xmalloc (PATH_MAX);
      the_current_working_directory[0] = '\0';
      directory = getcwd (the_current_working_directory, PATH_MAX);
      if (directory == 0)
	{
	  fprintf (stderr, "%s: could not get current directory: %s: %s\n",
		   (for_whom && *for_whom) ? for_whom : get_name_for_error (),
		   bash_getcwd_errstr, strerror (errno));

	  free (the_current_working_directory);
	  the_current_working_directory = (char *)NULL;
	  return (char *)NULL;
	}
    }

  return (savestring (the_current_working_directory));
}

/* Make NAME our internal idea of the current working directory. */
void
set_working_directory (name)
     char *name;
{
  FREE (the_current_working_directory);
  the_current_working_directory = savestring (name);
}

/* **************************************************************** */
/*								    */
/*	     	Job control support functions			    */
/*								    */
/* **************************************************************** */

#if defined (JOB_CONTROL)
int
get_job_by_name (name, flags)
     const char *name;
     int flags;
{
  register int i, wl, cl, match, job;
  register PROCESS *p;

  job = NO_JOB;
  wl = strlen (name);
  for (i = job_slots - 1; i >= 0; i--)
    {
      if (jobs[i] == 0 || ((flags & JM_STOPPED) && JOBSTATE(i) != JSTOPPED))
        continue;

      p = jobs[i]->pipe;
      do
        {
	  if (flags & JM_EXACT)
	    {
	      cl = strlen (p->command);
	      match = STREQN (p->command, name, cl);
	    }
	  else if (flags & JM_SUBSTRING)
	    match = strindex (p->command, name) != (char *)0;
	  else
	    match = STREQN (p->command, name, wl);

	  if (match == 0)
	    {
	      p = p->next;
	      continue;
	    }
	  else if (flags & JM_FIRSTMATCH)
	    return i;		/* return first match */
	  else if (job != NO_JOB)
	    {
	      if (this_shell_builtin)
	        builtin_error ("%s: ambiguous job spec", name);
	      else
	        report_error ("%s: ambiguous job spec", name);
	      return (DUP_JOB);
	    }
	  else
	    job = i;
        }
      while (p != jobs[i]->pipe);
    }

  return (job);
}

/* Return the job spec found in LIST. */
int
get_job_spec (list)
     WORD_LIST *list;
{
  register char *word;
  int job, jflags;

  if (list == 0)
    return (current_job);

  word = list->word->word;

  if (*word == '\0')
    return (current_job);

  if (*word == '%')
    word++;

  if (DIGIT (*word) && all_digits (word))
    {
      job = atoi (word);
#if 0
      return (job >= job_slots ? NO_JOB : job - 1);
#else
      return (job > job_slots ? NO_JOB : job - 1);
#endif
    }

  jflags = 0;
  switch (*word)
    {
    case 0:
    case '%':
    case '+':
      return (current_job);

    case '-':
      return (previous_job);

    case '?':			/* Substring search requested. */
      jflags |= JM_SUBSTRING;
      word++;
      /* FALLTHROUGH */

    default:
      return get_job_by_name (word, jflags);
    }
}
#endif /* JOB_CONTROL */

int
display_signal_list (list, forcecols)
     WORD_LIST *list;
     int forcecols;
{
  register int i, column;
  char *name;
  int result;
  int signum;
  intmax_t lsignum;

  result = EXECUTION_SUCCESS;
  if (!list)
    {
      for (i = 1, column = 0; i < NSIG; i++)
	{
	  name = signal_name (i);
	  if (STREQN (name, "SIGJUNK", 7) || STREQN (name, "Unknown", 7))
	    continue;

	  if (posixly_correct && !forcecols)
	    printf ("%s%s", name, (i == NSIG - 1) ? "" : " ");
	  else
	    {
	      printf ("%2d) %s", i, name);

	      if (++column < 4)
		printf ("\t");
	      else
		{
		  printf ("\n");
		  column = 0;
		}
	    }
	}

      if ((posixly_correct && !forcecols) || column != 0)
	printf ("\n");
      return result;
    }

  /* List individual signal names or numbers. */
  while (list)
    {
      if (legal_number (list->word->word, &lsignum))
	{
	  /* This is specified by Posix.2 so that exit statuses can be
	     mapped into signal numbers. */
	  if (lsignum > 128)
	    lsignum -= 128;
	  if (lsignum < 0 || lsignum >= NSIG)
	    {
	      sh_invalidsig (list->word->word);
	      result = EXECUTION_FAILURE;
	      list = list->next;
	      continue;
	    }

	  signum = lsignum;
	  name = signal_name (signum);
	  if (STREQN (name, "SIGJUNK", 7) || STREQN (name, "Unknown", 7))
	    {
	      list = list->next;
	      continue;
	    }
#if defined (JOB_CONTROL)
	  /* POSIX.2 says that `kill -l signum' prints the signal name without
	     the `SIG' prefix. */
	  printf ("%s\n", (this_shell_builtin == kill_builtin) ? name + 3 : name);
#else
	  printf ("%s\n", name);
#endif
	}
      else
	{
	  signum = decode_signal (list->word->word);
	  if (signum == NO_SIG)
	    {
	      sh_invalidsig (list->word->word);
	      result = EXECUTION_FAILURE;
	      list = list->next;
	      continue;
	    }
	  printf ("%d\n", signum);
	}
      list = list->next;
    }
  return (result);
}

/* **************************************************************** */
/*								    */
/*	    Finding builtin commands and their functions	    */
/*								    */
/* **************************************************************** */

/* Perform a binary search and return the address of the builtin function
   whose name is NAME.  If the function couldn't be found, or the builtin
   is disabled or has no function associated with it, return NULL.
   Return the address of the builtin.
   DISABLED_OKAY means find it even if the builtin is disabled. */
struct builtin *
builtin_address_internal (name, disabled_okay)
     char *name;
     int disabled_okay;
{
  int hi, lo, mid, j;

  hi = num_shell_builtins - 1;
  lo = 0;

  while (lo <= hi)
    {
      mid = (lo + hi) / 2;

      j = shell_builtins[mid].name[0] - name[0];

      if (j == 0)
	j = strcmp (shell_builtins[mid].name, name);

      if (j == 0)
	{
	  /* It must have a function pointer.  It must be enabled, or we
	     must have explicitly allowed disabled functions to be found,
	     and it must not have been deleted. */
	  if (shell_builtins[mid].function &&
	      ((shell_builtins[mid].flags & BUILTIN_DELETED) == 0) &&
	      ((shell_builtins[mid].flags & BUILTIN_ENABLED) || disabled_okay))
	    return (&shell_builtins[mid]);
	  else
	    return ((struct builtin *)NULL);
	}
      if (j > 0)
	hi = mid - 1;
      else
	lo = mid + 1;
    }
  return ((struct builtin *)NULL);
}

/* Return the pointer to the function implementing builtin command NAME. */
sh_builtin_func_t *
find_shell_builtin (name)
     char *name;
{
  current_builtin = builtin_address_internal (name, 0);
  return (current_builtin ? current_builtin->function : (sh_builtin_func_t *)NULL);
}

/* Return the address of builtin with NAME, whether it is enabled or not. */
sh_builtin_func_t *
builtin_address (name)
     char *name;
{
  current_builtin = builtin_address_internal (name, 1);
  return (current_builtin ? current_builtin->function : (sh_builtin_func_t *)NULL);
}

/* Return the function implementing the builtin NAME, but only if it is a
   POSIX.2 special builtin. */
sh_builtin_func_t *
find_special_builtin (name)
     char *name;
{
  current_builtin = builtin_address_internal (name, 0);
  return ((current_builtin && (current_builtin->flags & SPECIAL_BUILTIN)) ?
  			current_builtin->function :
  			(sh_builtin_func_t *)NULL);
}
  
static int
shell_builtin_compare (sbp1, sbp2)
     struct builtin *sbp1, *sbp2;
{
  int result;

  if ((result = sbp1->name[0] - sbp2->name[0]) == 0)
    result = strcmp (sbp1->name, sbp2->name);

  return (result);
}

/* Sort the table of shell builtins so that the binary search will work
   in find_shell_builtin. */
void
initialize_shell_builtins ()
{
  qsort (shell_builtins, num_shell_builtins, sizeof (struct builtin),
    (QSFUNC *)shell_builtin_compare);
}
