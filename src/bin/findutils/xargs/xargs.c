/* xargs -- build and execute command lines from standard input
   Copyright (C) 1990, 91, 92, 93, 94 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* Written by Mike Rendell <michael@cs.mun.ca>
   and David MacKenzie <djm@gnu.ai.mit.edu>.  */

#include <config.h>

#if __STDC__
#define P_(s) s
#else
#define P_(s) ()
#endif

#define _GNU_SOURCE
#include <ctype.h>

#if !defined (isascii) || defined (STDC_HEADERS)
#ifdef isascii
#undef isascii
#endif
#define isascii(c) 1
#endif

#ifdef isblank
#define ISBLANK(c) (isascii (c) && isblank (c))
#else
#define ISBLANK(c) ((c) == ' ' || (c) == '\t')
#endif

#define ISSPACE(c) (ISBLANK (c) || (c) == '\n' || (c) == '\r' \
		    || (c) == '\f' || (c) == '\v')

#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <getopt.h>

#if defined(HAVE_STRING_H) || defined(STDC_HEADERS)
#include <string.h>
#if !defined(STDC_HEADERS)
#include <memory.h>
#endif
#else
#include <strings.h>
#define memcpy(dest, source, count) (bcopy((source), (dest), (count)))
#endif

char *strstr ();
char *strdup ();

#ifndef _POSIX_SOURCE
#include <sys/param.h>
#endif

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <signal.h>

#if !defined(SIGCHLD) && defined(SIGCLD)
#define SIGCHLD SIGCLD
#endif

/* COMPAT:  SYSV version defaults size (and has a max value of) to 470.
   We try to make it as large as possible. */
#if !defined(ARG_MAX) && defined(_SC_ARG_MAX)
#define ARG_MAX sysconf (_SC_ARG_MAX)
#endif
#ifndef ARG_MAX
#define ARG_MAX NCARGS
#endif

#include "wait.h"

/* States for read_line. */
#define NORM 0
#define SPACE 1
#define QUOTE 2
#define BACKSLASH 3

#ifdef STDC_HEADERS
#include <stdlib.h>
#else
char *malloc ();
void exit ();
void free ();
long strtol ();

extern int errno;
#endif

/* Return nonzero if S is the EOF string.  */
#define EOF_STR(s) (eof_str && *eof_str == *s && !strcmp (eof_str, s))

extern char **environ;

/* Not char because of type promotion; NeXT gcc can't handle it.  */
typedef int boolean;
#define		true    1
#define		false	0

#if __STDC__
#define VOID void
#else
#define VOID char
#endif

VOID *xmalloc P_ ((size_t n));
VOID *xrealloc P_ ((VOID * p, size_t n));
void error P_ ((int status, int errnum, char *message,...));

extern char *version_string;

/* The name this program was run with.  */
char *program_name;

/* Buffer for reading arguments from stdin.  */
static char *linebuf;

/* Line number in stdin since the last command was executed.  */
static int lineno = 0;

/* If nonzero, then instead of putting the args from stdin at
   the end of the command argument list, they are each stuck into the
   initial args, replacing each occurrence of the `replace_pat' in the
   initial args.  */
static char *replace_pat = NULL;

/* The length of `replace_pat'.  */
static size_t rplen = 0;

/* If nonzero, when this string is read on stdin it is treated as
   end of file.
   I don't like this - it should default to NULL.  */
static char *eof_str = "_";

/* If nonzero, the maximum number of nonblank lines from stdin to use
   per command line.  */
static long lines_per_exec = 0;

/* The maximum number of arguments to use per command line.  */
static long args_per_exec = 1024;

/* If true, exit if lines_per_exec or args_per_exec is exceeded.  */
static boolean exit_if_size_exceeded = false;

/* The maximum number of characters that can be used per command line.  */
static long arg_max;

/* Storage for elements of `cmd_argv'.  */
static char *argbuf;

/* The list of args being built.  */
static char **cmd_argv = NULL;

/* Number of elements allocated for `cmd_argv'.  */
static int cmd_argv_alloc = 0;

/* Number of valid elements in `cmd_argv'.  */
static int cmd_argc = 0;

/* Number of chars being used in `cmd_argv'.  */
static int cmd_argv_chars = 0;

/* Number of initial arguments given on the command line.  */
static int initial_argc = 0;

/* Number of chars in the initial args.  */
static int initial_argv_chars = 0;

/* true when building up initial arguments in `cmd_argv'.  */
static boolean initial_args = true;

/* If nonzero, the maximum number of child processes that can be running
   at once.  */
static int proc_max = 1;

/* Total number of child processes that have been executed.  */
static int procs_executed = 0;

/* The number of elements in `pids'.  */
static int procs_executing = 0;

/* List of child processes currently executing.  */
static pid_t *pids = NULL;

/* The number of allocated elements in `pids'. */
static int pids_alloc = 0;

/* Exit status; nonzero if any child process exited with a
   status of 1-125.  */
static int child_error = 0;

/* If true, print each command on stderr before executing it.  */
static boolean print_command = false;

/* If true, query the user before executing each command, and only
   execute the command if the user responds affirmatively.  */
static boolean query_before_executing = false;

static struct option const longopts[] =
{
  {"null", no_argument, NULL, '0'},
  {"eof", optional_argument, NULL, 'e'},
  {"replace", optional_argument, NULL, 'i'},
  {"max-lines", optional_argument, NULL, 'l'},
  {"max-args", required_argument, NULL, 'n'},
  {"interactive", no_argument, NULL, 'p'},
  {"no-run-if-empty", no_argument, NULL, 'r'},
  {"max-chars", required_argument, NULL, 's'},
  {"verbose", no_argument, NULL, 't'},
  {"exit", no_argument, NULL, 'x'},
  {"max-procs", required_argument, NULL, 'P'},
  {"version", no_argument, NULL, 'v'},
  {"help", no_argument, NULL, 'h'},
  {NULL, no_argument, NULL, 0}
};

static int read_line P_ ((void));
static int read_string P_ ((void));
static void do_insert P_ ((char *arg, size_t arglen, size_t lblen));
static void push_arg P_ ((char *arg, size_t len));
static boolean print_args P_ ((boolean ask));
static void do_exec P_ ((void));
static void add_proc P_ ((pid_t pid));
static void wait_for_proc P_ ((boolean all));
static long parse_num P_ ((char *str, int option, long min, long max));
static long env_size P_ ((char **envp));
static void usage P_ ((FILE * stream, int status));

void
main (argc, argv)
     int argc;
     char **argv;
{
  int optc;
  int always_run_command = 1;
  long orig_arg_max;
  char *default_cmd = "/bin/echo";
  int (*read_args) P_ ((void)) = read_line;

  program_name = argv[0];

  orig_arg_max = ARG_MAX - 2048; /* POSIX.2 requires subtracting 2048.  */
  arg_max = orig_arg_max;

  /* Sanity check for systems with huge ARG_MAX defines (e.g., Suns which
     have it at 1 meg).  Things will work fine with a large ARG_MAX but it
     will probably hurt the system more than it needs to; an array of this
     size is allocated.  */
  if (arg_max > 20 * 1024)
    arg_max = 20 * 1024;

  /* Take the size of the environment into account.  */
  arg_max -= env_size (environ);
  if (arg_max <= 0)
    error (1, 0, "environment is too large for exec");

  while ((optc = getopt_long (argc, argv, "+0e::i::l::n:prs:txP:",
			      longopts, (int *) 0)) != -1)
    {
      switch (optc)
	{
	case '0':
	  read_args = read_string;
	  break;

	case 'e':
	  if (optarg)
	    eof_str = optarg;
	  else
	    eof_str = 0;
	  break;

	case 'h':
	  usage (stdout, 0);

	case 'i':
	  if (optarg)
	    replace_pat = optarg;
	  else
	    replace_pat = "{}";
	  /* -i excludes -n -l.  */
	  args_per_exec = 0;
	  lines_per_exec = 0;
	  break;

	case 'l':
	  if (optarg)
	    lines_per_exec = parse_num (optarg, 'l', 1L, -1L);
	  else
	    lines_per_exec = 1;
	  /* -l excludes -i -n.  */
	  args_per_exec = 0;
	  replace_pat = NULL;
	  break;

	case 'n':
	  args_per_exec = parse_num (optarg, 'n', 1L, -1L);
	  /* -n excludes -i -l.  */
	  lines_per_exec = 0;
	  replace_pat = NULL;
	  break;

	case 's':
	  arg_max = parse_num (optarg, 's', 1L, orig_arg_max);
	  break;

	case 't':
	  print_command = true;
	  break;

	case 'x':
	  exit_if_size_exceeded = true;
	  break;

	case 'p':
	  query_before_executing = true;
	  print_command = true;
	  break;

	case 'r':
	  always_run_command = 0;
	  break;

	case 'P':
	  proc_max = parse_num (optarg, 'P', 0L, -1L);
	  break;

	case 'v':
	  printf ("GNU xargs version %s\n", version_string);
	  exit (0);

	default:
	  usage (stderr, 1);
	}
    }

  if (replace_pat || lines_per_exec)
    exit_if_size_exceeded = true;

  if (optind == argc)
    {
      optind = 0;
      argc = 1;
      argv = &default_cmd;
    }

  linebuf = (char *) xmalloc (arg_max + 1);
  argbuf = (char *) xmalloc (arg_max + 1);

  /* Make sure to listen for the kids.  */
  signal (SIGCHLD, SIG_DFL);

  if (!replace_pat)
    {
      for (; optind < argc; optind++)
	push_arg (argv[optind], strlen (argv[optind]) + 1);
      initial_args = false;
      initial_argc = cmd_argc;
      initial_argv_chars = cmd_argv_chars;

      while ((*read_args) () != -1)
	if (lines_per_exec && lineno >= lines_per_exec)
	  {
	    do_exec ();
	    lineno = 0;
	  }

      /* SYSV xargs seems to do at least one exec, even if the
         input is empty.  */
      if (cmd_argc != initial_argc
	  || (always_run_command && procs_executed == 0))
	do_exec ();
    }
  else
    {
      int i;
      size_t len;
      size_t *arglen = (size_t *) xmalloc (sizeof (size_t) * argc);

      for (i = optind; i < argc; i++)
	arglen[i] = strlen(argv[i]);
      rplen = strlen (replace_pat);
      while ((len = (*read_args) ()) != -1)
	{
	  /* Don't do insert on the command name.  */
	  push_arg (argv[optind], arglen[optind] + 1);
	  len--;
	  for (i = optind + 1; i < argc; i++)
	    do_insert (argv[i], arglen[i], len);
	  do_exec ();
	}
    }

  wait_for_proc (true);
  exit (child_error);
}

/* Read a line of arguments from stdin and add them to the list of
   arguments to pass to the command.  Ignore blank lines and initial blanks.
   Single and double quotes and backslashes quote metacharacters and blanks
   as they do in the shell.
   Return -1 if eof (either physical or logical) is reached,
   otherwise the length of the last string read (including the null).  */

static int
read_line ()
{
  static boolean eof = false;
  /* Start out in mode SPACE to always strip leading spaces (even with -i).  */
  int state = SPACE;		/* The type of character we last read.  */
  int prevc;			/* The previous value of c.  */
  int quotc = 0;		/* The last quote character read.  */
  int c = EOF;
  boolean first = true;		/* true if reading first arg on line.  */
  int len;
  char *p = linebuf;
  /* Including the NUL, the args must not grow past this point.  */
  char *endbuf = linebuf + arg_max - initial_argv_chars - 1;

  if (eof)
    return -1;
  while (1)
    {
      prevc = c;
      c = getc (stdin);
      if (c == EOF)
	{
	  /* COMPAT: SYSV seems to ignore stuff on a line that
	     ends without a \n; we don't.  */
	  eof = true;
	  if (p == linebuf)
	    return -1;
	  *p++ = '\0';
	  len = p - linebuf;
	  /* FIXME we don't check for unterminated quotes here.  */
	  if (first && EOF_STR (linebuf))
	    return -1;
	  if (!replace_pat)
	    push_arg (linebuf, len);
	  return len;
	}
      switch (state)
	{
	case SPACE:
	  if (ISSPACE (c))
	    continue;
	  state = NORM;
	  /* aaahhhh....  */

	case NORM:
	  if (c == '\n')
	    {
	      if (!ISBLANK (prevc))
		lineno++;	/* For -l.  */
	      if (p == linebuf)
		{
		  /* Blank line.  */
		  state = SPACE;
		  continue;
		}
	      *p++ = '\0';
	      len = p - linebuf;
	      if (EOF_STR (linebuf))
		{
		  eof = true;
		  return first ? -1 : len;
		}
	      if (!replace_pat)
		push_arg (linebuf, len);
	      return len;
	    }
	  if (!replace_pat && ISSPACE (c))
	    {
	      *p++ = '\0';
	      len = p - linebuf;
	      if (EOF_STR (linebuf))
		{
		  eof = true;
		  return first ? -1 : len;
		}
	      push_arg (linebuf, len);
	      p = linebuf;
	      state = SPACE;
	      first = false;
	      continue;
	    }
	  switch (c)
	    {
	    case '\\':
	      state = BACKSLASH;
	      continue;

	    case '\'':
	    case '"':
	      state = QUOTE;
	      quotc = c;
	      continue;
	    }
	  break;

	case QUOTE:
	  if (c == '\n')
	    error (1, 0, "unmatched %s quote",
		   quotc == '"' ? "double" : "single");
	  if (c == quotc)
	    {
	      state = NORM;
	      continue;
	    }
	  break;

	case BACKSLASH:
	  state = NORM;
	  break;
	}
      if (p >= endbuf)
	error (1, 0, "argument line too long");
      *p++ = c;
    }
}

/* Read a null-terminated string from stdin and add it to the list of
   arguments to pass to the command.
   Return -1 if eof (either physical or logical) is reached,
   otherwise the length of the string read (including the null).  */

static int
read_string ()
{
  static boolean eof = false;
  int len;
  char *p = linebuf;
  /* Including the NUL, the args must not grow past this point.  */
  char *endbuf = linebuf + arg_max - initial_argv_chars - 1;

  if (eof)
    return -1;
  while (1)
    {
      int c = getc (stdin);
      if (c == EOF)
	{
	  eof = true;
	  if (p == linebuf)
	    return -1;
	  *p++ = '\0';
	  len = p - linebuf;
	  if (!replace_pat)
	    push_arg (linebuf, len);
	  return len;
	}
      if (c == '\0')
	{
	  lineno++;		/* For -l.  */
	  *p++ = '\0';
	  len = p - linebuf;
	  if (!replace_pat)
	    push_arg (linebuf, len);
	  return len;
	}
      if (p >= endbuf)
	error (1, 0, "argument line too long");
      *p++ = c;
    }
}

/* Replace all instances of `replace_pat' in ARG with `linebuf',
   and add the resulting string to the list of arguments for the command
   to execute.
   ARGLEN is the length of ARG, not including the null.
   LBLEN is the length of `linebuf', not including the null.

   COMPAT: insertions on the SYSV version are limited to 255 chars per line,
   and a max of 5 occurences of replace_pat in the initial-arguments.
   Those restrictions do not exist here.  */

static void
do_insert (arg, arglen, lblen)
     char *arg;
     size_t arglen;
     size_t lblen;
{
  /* Temporary copy of each arg with the replace pattern replaced by the
     real arg.  */
  static char *insertbuf;
  char *p;
  int bytes_left = arg_max - 1;	/* Bytes left on the command line.  */

  if (!insertbuf)
    insertbuf = (char *) xmalloc (arg_max + 1);
  p = insertbuf;

  do
    {
      size_t len;		/* Length in ARG before `replace_pat'.  */
      char *s = strstr (arg, replace_pat);
      if (s)
	len = s - arg;
      else
	len = arglen;
      bytes_left -= len;
      if (bytes_left <= 0)
	break;

      strncpy (p, arg, len);
      p += len;
      arg += len;

      if (s)
	{
	  bytes_left -= lblen;
	  if (bytes_left <= 0)
	    break;
	  strcpy (p, linebuf);
	  arg += rplen;
	  p += lblen;
	}
    }
  while (*arg);
  if (*arg)
    error (1, 0, "command too long");
  *p++ = '\0';
  push_arg (insertbuf, p - insertbuf);
}

/* Add ARG to the end of the list of arguments `cmd_argv' to pass
   to the command.
   LEN is the length of ARG, including the terminating null.
   If this brings the list up to its maximum size, execute the command.  */

static void
push_arg (arg, len)
     char *arg;
     size_t len;
{
  if (arg)
    {
      if (cmd_argv_chars + len > arg_max)
	{
	  if (initial_args || cmd_argc == initial_argc)
	    error (1, 0, "can not fit single argument within argument list size limit");
	  if (replace_pat
	      || (exit_if_size_exceeded &&
		  (lines_per_exec || args_per_exec)))
	    error (1, 0, "argument list too long");
	  do_exec ();
	}
      if (!initial_args && args_per_exec &&
	  cmd_argc - initial_argc == args_per_exec)
	do_exec ();
    }

  if (cmd_argc >= cmd_argv_alloc)
    {
      if (!cmd_argv)
	{
	  cmd_argv_alloc = 64;
	  cmd_argv = (char **) xmalloc (sizeof (char *) * cmd_argv_alloc);
	}
      else
	{
	  cmd_argv_alloc *= 2;
	  cmd_argv = (char **) xrealloc (cmd_argv,
					 sizeof (char *) * cmd_argv_alloc);
	}
    }

  if (!arg)
    cmd_argv[cmd_argc++] = NULL;
  else
    {
      cmd_argv[cmd_argc++] = argbuf + cmd_argv_chars;
      strcpy (argbuf + cmd_argv_chars, arg);
      cmd_argv_chars += len;
    }
}

/* Print the arguments of the command to execute.
   If ASK is nonzero, prompt the user for a response, and
   if the user responds affirmatively, return true;
   otherwise, return false.  */

static boolean
print_args (ask)
     boolean ask;
{
  int i;

  for (i = 0; i < cmd_argc - 1; i++)
    fprintf (stderr, "%s ", cmd_argv[i]);
  if (ask)
    {
      static FILE *tty_stream;
      int c, savec;

      if (!tty_stream)
	{
	  tty_stream = fopen ("/dev/tty", "r");
	  if (!tty_stream)
	    error (1, errno, "/dev/tty");
	}
      fputs ("?...", stderr);
      fflush (stderr);
      c = savec = getc (tty_stream);
      while (c != EOF && c != '\n')
	c = getc (tty_stream);
      if (savec == 'y' || savec == 'Y')
	return true;
    }
  else
    putc ('\n', stderr);

  return false;
}

/* Execute the command that has been built in `cmd_argv'.  This may involve
   waiting for processes that were previously executed.  */

static void
do_exec ()
{
  pid_t child;

  push_arg ((char *) NULL, 0);	/* Null terminate the arg list.  */
  if (!query_before_executing || print_args (true))
    {
      if (proc_max && procs_executing >= proc_max)
	wait_for_proc (false);
      if (!query_before_executing && print_command)
	print_args (false);
      /* If we run out of processes, wait for a child to return and
         try again.  */
      while ((child = fork ()) < 0 && errno == EAGAIN && procs_executing)
	wait_for_proc (false);
      switch (child)
	{
	case -1:
	  error (1, errno, "cannot fork");

	case 0:		/* Child.  */
	  execvp (cmd_argv[0], cmd_argv);
	  error (0, errno, "%s", cmd_argv[0]);
	  _exit (errno == ENOENT ? 127 : 126);
	}
      add_proc (child);
    }

  cmd_argc = initial_argc;
  cmd_argv_chars = initial_argv_chars;
}

/* Add the process with id PID to the list of processes that have
   been executed.  */

static void
add_proc (pid)
     pid_t pid;
{
  int i;

  /* Find an empty slot.  */
  for (i = 0; i < pids_alloc && pids[i]; i++)
    ;
  if (i == pids_alloc)
    {
      if (pids_alloc == 0)
	{
	  pids_alloc = proc_max ? proc_max : 64;
	  pids = (pid_t *) xmalloc (sizeof (pid_t) * pids_alloc);
	}
      else
	{
	  pids_alloc *= 2;
	  pids = (pid_t *) xrealloc (pids,
				     sizeof (pid_t) * pids_alloc);
	}
      memset (&pids[i], '\0', sizeof (pid_t) * (pids_alloc - i));
    }
  pids[i] = pid;
  procs_executing++;
  procs_executed++;
}

/* If ALL is true, wait for all child processes to finish;
   otherwise, wait for one child process to finish.
   Remove the processes that finish from the list of executing processes.  */

static void
wait_for_proc (all)
     boolean all;
{
  while (procs_executing)
    {
      int i, status;

      do
	{
	  pid_t pid;

	  pid = wait (&status);
	  if (pid < 0)
	    error (1, errno, "error waiting for child process");

	  /* Find the entry in `pids' for the child process
	     that exited.  */
	  for (i = 0; i < pids_alloc && pid != pids[i]; i++)
	    ;
	}
      while (i == pids_alloc);	/* A child died that we didn't start? */

      /* Remove the child from the list.  */
      pids[i] = 0;
      procs_executing--;

      if (WEXITSTATUS (status) == 126 || WEXITSTATUS (status) == 127)
	exit (WEXITSTATUS (status));	/* Can't find or run the command.  */
      if (WEXITSTATUS (status) == 255)
	error (124, 0, "%s: exited with status 255; aborting", cmd_argv[0]);
      if (WIFSTOPPED (status))
	error (125, 0, "%s: stopped by signal %d", cmd_argv[0], WSTOPSIG (status));
      if (WIFSIGNALED (status))
	error (125, 0, "%s: terminated by signal %d", cmd_argv[0], WTERMSIG (status));
      if (WEXITSTATUS (status) != 0)
	child_error = 123;

      if (!all)
	break;
    }
}

/* Return the value of the number represented in STR.
   OPTION is the command line option to which STR is the argument.
   If the value does not fall within the boundaries MIN and MAX,
   Print an error message mentioning OPTION and exit.  */

static long
parse_num (str, option, min, max)
     char *str;
     int option;
     long min;
     long max;
{
  char *eptr;
  long val;

  val = strtol (str, &eptr, 10);
  if (eptr == str || *eptr)
    {
      fprintf (stderr, "%s: invalid number for -%c option\n",
	       program_name, option);
      usage (stderr, 1);
    }
  else if (val < min)
    {
      fprintf (stderr, "%s: value for -%c option must be >= %ld\n",
	       program_name, option, min);
      usage (stderr, 1);
    }
  else if (max >= 0 && val > max)
    {
      fprintf (stderr, "%s: value for -%c option must be < %ld\n",
	       program_name, option, max);
      usage (stderr, 1);
    }
  return val;
}

/* Return how much of ARG_MAX is used by the environment.  */

static long
env_size (envp)
     char **envp;
{
  long len = 0;

  while (*envp)
    len += strlen (*envp++) + 1;

  return len;
}

static void
usage (stream, status)
     FILE *stream;
     int status;
{
  fprintf (stream, "\
Usage: %s [-0prtx] [-e[eof-str]] [-i[replace-str]] [-l[max-lines]]\n\
       [-n max-args] [-s max-chars] [-P max-procs] [--null] [--eof[=eof-str]]\n\
       [--replace[=replace-str]] [--max-lines[=max-lines]] [--interactive]\n\
       [--max-chars=max-chars] [--verbose] [--exit] [--max-procs=max-procs]\n\
       [--max-args=max-args] [--no-run-if-empty] [--version] [--help]\n\
       [command [initial-arguments]]\n",
	   program_name);
  exit (status);
}
