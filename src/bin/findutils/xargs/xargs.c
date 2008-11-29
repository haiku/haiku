/* xargs -- build and execute command lines from standard input
   Copyright (C) 1990, 91, 92, 93, 94, 2000, 2003, 2004, 2005, 2006, 2007 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* Written by Mike Rendell <michael@cs.mun.ca>
   and David MacKenzie <djm@gnu.org>.
   Modifications by
	James Youngman
	Dmitry V. Levin
*/

#include <config.h>

# ifndef PARAMS
#  if defined PROTOTYPES || (defined __STDC__ && __STDC__)
#   define PARAMS(Args) Args
#  else
#   define PARAMS(Args) ()
#  endif
# endif

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
#include <fcntl.h>

#if defined(STDC_HEADERS)
#include <assert.h>
#endif

#if defined(HAVE_STRING_H) || defined(STDC_HEADERS)
#include <string.h>
#if !defined(STDC_HEADERS)
#include <memory.h>
#endif
#else
#include <strings.h>
#define memcpy(dest, source, count) (bcopy((source), (dest), (count)))
#endif

#ifndef _POSIX_SOURCE
#include <sys/param.h>
#endif

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#ifndef LONG_MAX
#define LONG_MAX (~(1 << (sizeof (long) * 8 - 1)))
#endif

/* The presence of unistd.h is assumed by gnulib these days, so we 
 * might as well assume it too. 
 */
#include <unistd.h>

#include <signal.h>

#if !defined(SIGCHLD) && defined(SIGCLD)
#define SIGCHLD SIGCLD
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
extern int errno;
#endif

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif
#if ENABLE_NLS
# include <libintl.h>
# define _(Text) gettext (Text)
#else
# define _(Text) Text
#define textdomain(Domain)
#define bindtextdomain(Package, Directory)
#endif
#ifdef gettext_noop
# define N_(String) gettext_noop (String)
#else
/* See locate.c for explanation as to why not use (String) */
# define N_(String) String
#endif

#include "buildcmd.h"


/* Return nonzero if S is the EOF string.  */
#define EOF_STR(s) (eof_str && *eof_str == *s && !strcmp (eof_str, s))

/* Do multibyte processing if multibyte characters are supported,
   unless multibyte sequences are search safe.  Multibyte sequences
   are search safe if searching for a substring using the byte
   comparison function 'strstr' gives no false positives.  All 8-bit
   encodings and the UTF-8 multibyte encoding are search safe, but
   the EUC encodings are not.
   BeOS uses the UTF-8 encoding exclusively, so it is search safe. */
#if defined __BEOS__
# define MULTIBYTE_IS_SEARCH_SAFE 1
#endif
#define DO_MULTIBYTE (HAVE_MBLEN && ! MULTIBYTE_IS_SEARCH_SAFE)

#if DO_MULTIBYTE
# if HAVE_MBRLEN
#  include <wchar.h>
# else
   /* Simulate mbrlen with mblen as best we can.  */
#  define mbstate_t int
#  define mbrlen(s, n, ps) mblen (s, n)
# endif
#endif

/* Not char because of type promotion; NeXT gcc can't handle it.  */
typedef int boolean;
#define		true    1
#define		false	0

#if __STDC__
#define VOID void
#else
#define VOID char
#endif

#include <xalloc.h>
#include "closein.h"
#include "gnulib-version.h"

void error PARAMS ((int status, int errnum, char *message,...));

extern char *version_string;

/* The name this program was run with.  */
char *program_name;

static FILE *input_stream;

/* Buffer for reading arguments from input.  */
static char *linebuf;

static int keep_stdin = 0;

/* Line number in stdin since the last command was executed.  */
static int lineno = 0;

static struct buildcmd_state bc_state;
static struct buildcmd_control bc_ctl;


/* If nonzero, when this string is read on stdin it is treated as
   end of file.
   IEEE Std 1003.1, 2004 Edition allows this to be NULL.
   In findutils releases up to and including 4.2.8, this was "_".
*/
static char *eof_str = NULL;

/* Number of chars in the initial args.  */
/* static int initial_argv_chars = 0; */

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
static volatile int child_error = 0;

static volatile int original_exit_value;

/* If true, print each command on stderr before executing it.  */
static boolean print_command = false; /* Option -t */

/* If true, query the user before executing each command, and only
   execute the command if the user responds affirmatively.  */
static boolean query_before_executing = false;

/* The delimiter for input arguments.   This is only consulted if the 
 * -0 or -d option had been given.
 */
static char input_delimiter = '\0';


static struct option const longopts[] =
{
  {"null", no_argument, NULL, '0'},
  {"arg-file", required_argument, NULL, 'a'},
  {"delimiter", required_argument, NULL, 'd'},
  {"eof", optional_argument, NULL, 'e'},
  {"replace", optional_argument, NULL, 'I'},
  {"max-lines", optional_argument, NULL, 'l'},
  {"max-args", required_argument, NULL, 'n'},
  {"interactive", no_argument, NULL, 'p'},
  {"no-run-if-empty", no_argument, NULL, 'r'},
  {"max-chars", required_argument, NULL, 's'},
  {"verbose", no_argument, NULL, 't'},
  {"show-limits", no_argument, NULL, 'S'},
  {"exit", no_argument, NULL, 'x'},
  {"max-procs", required_argument, NULL, 'P'},
  {"version", no_argument, NULL, 'v'},
  {"help", no_argument, NULL, 'h'},
  {NULL, no_argument, NULL, 0}
};

static int read_line PARAMS ((void));
static int read_string PARAMS ((void));
static boolean print_args PARAMS ((boolean ask));
/* static void do_exec PARAMS ((void)); */
static int xargs_do_exec (const struct buildcmd_control *cl, struct buildcmd_state *state);
static void exec_if_possible PARAMS ((void));
static void add_proc PARAMS ((pid_t pid));
static void wait_for_proc PARAMS ((boolean all));
static void wait_for_proc_all PARAMS ((void));
static long parse_num PARAMS ((char *str, int option, long min, long max, int fatal));
static void usage PARAMS ((FILE * stream));



static char 
get_char_oct_or_hex_escape(const char *s)
{
  const char * p;
  int base = 8;
  unsigned long val;
  char *endp;

  assert('\\' == s[0]);
  
  if ('x' == s[1])
    {
      /* hex */
      p = s+2;
      base = 16;
    }
  else if (isdigit(s[1]))
    {
      /* octal */
      p = s+1;
      base = 8;
    }
  else
    {
      error(1, 0,
	    _("Invalid escape sequence %s in input delimiter specification."),
	    s);
    }
  errno = 0;
  endp = (char*)p;
  val = strtoul(p, &endp, base);
  
  /* This if condition is carefully constructed to do 
   * the right thing if UCHAR_MAX has the same 
   * value as ULONG_MAX.   IF UCHAR_MAX==ULONG_MAX,
   * then val can never be greater than UCHAR_MAX.
   */
  if ((ULONG_MAX == val && ERANGE == errno)
      || (val > UCHAR_MAX))
    {
      if (16 == base)
	{
	  error(1, 0,
		_("Invalid escape sequence %s in input delimiter specification; character values must not exceed %lx."),
		s, (unsigned long)UCHAR_MAX);
	}
      else
	{
	  error(1, 0,
		_("Invalid escape sequence %s in input delimiter specification; character values must not exceed %lo."),
		s, (unsigned long)UCHAR_MAX);
	}
    }
  
  /* check for trailing garbage */
  if (0 != *endp)
    {
      error(1, 0,
	    _("Invalid escape sequence %s in input delimiter specification; trailing characters %s not recognised."),
	    s, endp);
    }
  
  return (char) val;
}


static char 
get_input_delimiter(const char *s)
{
  if (1 == strlen(s))
    {
      return s[0];
    }
  else
    {
      if ('\\' == s[0])
	{
	  /* an escape code */
	  switch (s[1])
	    {
	    case 'a':
	      return '\a';
	    case 'b':
	      return '\b';
	    case 'f':
	      return '\f';
	    case 'n':
	      return '\n';
	    case 'r':
	      return '\r';
	    case 't':
	      return'\t';
	    case 'v':
	      return '\v';
	    case '\\':
	      return '\\';
	    default:
	      return get_char_oct_or_hex_escape(s);
	    }
	}
      else
	{
	  error(1, 0,
		_("Invalid input delimiter specification %s: the delimiter must be either a single character or an escape sequence starting with \\."),
		s);
	  abort ();
	}
    }
}

static void 
noop (void)
{
  /* does nothing. */
}

static void 
fail_due_to_env_size (void)
{
  error (1, 0, _("environment is too large for exec"));
}


int
main (int argc, char **argv)
{
  int optc;
  int show_limits = 0;			/* --show-limits */
  int always_run_command = 1;
  char *input_file = "-"; /* "-" is stdin */
  char *default_cmd = "/bin/echo";
  int (*read_args) PARAMS ((void)) = read_line;
  void (*act_on_init_result)(void) = noop;
  enum BC_INIT_STATUS bcstatus;

  program_name = argv[0];
  original_exit_value = 0;
  
#ifdef HAVE_SETLOCALE
  setlocale (LC_ALL, "");
#endif
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);
  atexit (close_stdin);
  atexit (wait_for_proc_all);

  bcstatus = bc_init_controlinfo(&bc_ctl);
  /* The bc_init_controlinfo call may have determined that the 
   * environment is too big.  In that case, we will fail with 
   * an error message after processing the command-line options,
   * as "xargs --help" should still work even if the environment is 
   * too big.
   *
   * Some of the argument processing depends on the contents of 
   * bc_ctl, which will be in an undefined state if bc_init_controlinfo()
   * failed.
   */
  if (BC_INIT_ENV_TOO_BIG == bcstatus)
    {
      act_on_init_result = fail_due_to_env_size;
    }
  else
    {
      /* IEEE Std 1003.1, 2003 specifies that the combined argument and 
       * environment list shall not exceed {ARG_MAX}-2048 bytes.  It also 
       * specifies that it shall be at least LINE_MAX.
       */
#if defined(ARG_MAX)
      assert(bc_ctl.arg_max <= (ARG_MAX-2048));
#endif
#ifdef LINE_MAX
      assert(bc_ctl.arg_max >= LINE_MAX);
#endif
      
      bc_ctl.exec_callback = xargs_do_exec;
      
      /* Start with a reasonable default size, though this can be
       * adjusted via the -s option.
       */
      bc_use_sensible_arg_max(&bc_ctl);
    }
  
  while ((optc = getopt_long (argc, argv, "+0a:E:e::i::I:l::L:n:prs:txP:d:",
			      longopts, (int *) 0)) != -1)
    {
      switch (optc)
	{
	case '0':
	  read_args = read_string;
	  input_delimiter = '\0';
	  break;

	case 'd':
	  read_args = read_string;
	  input_delimiter = get_input_delimiter(optarg);
	  break;

	case 'E':		/* POSIX */
	case 'e':		/* deprecated */
	  if (optarg && (strlen(optarg) > 0))
	    eof_str = optarg;
	  else
	    eof_str = 0;
	  break;

	case 'h':
	  usage (stdout);
	  return 0;

	case 'I':		/* POSIX */
	case 'i':		/* deprecated */
	  if (optarg)
	    bc_ctl.replace_pat = optarg;
	  else
	    bc_ctl.replace_pat = "{}";
	  /* -i excludes -n -l.  */
	  bc_ctl.args_per_exec = 0;
	  bc_ctl.lines_per_exec = 0;
	  break;

	case 'L':		/* POSIX */
	  bc_ctl.lines_per_exec = parse_num (optarg, 'L', 1L, -1L, 1);
	  /* -L excludes -i -n.  */
	  bc_ctl.args_per_exec = 0;
	  bc_ctl.replace_pat = NULL;
	  break;

	case 'l':		/* deprecated */
	  if (optarg)
	    bc_ctl.lines_per_exec = parse_num (optarg, 'l', 1L, -1L, 1);
	  else
	    bc_ctl.lines_per_exec = 1;
	  /* -l excludes -i -n.  */
	  bc_ctl.args_per_exec = 0;
	  bc_ctl.replace_pat = NULL;
	  break;

	case 'n':
	  bc_ctl.args_per_exec = parse_num (optarg, 'n', 1L, -1L, 1);
	  /* -n excludes -i -l.  */
	  bc_ctl.lines_per_exec = 0;
	  if (bc_ctl.args_per_exec == 1 && bc_ctl.replace_pat)
	    /* ignore -n1 in '-i -n1' */
	    bc_ctl.args_per_exec = 0;
	  else
	    bc_ctl.replace_pat = NULL;
	  break;

	  /* The POSIX standard specifies that it is not an error 
	   * for the -s option to specify a size that the implementation 
	   * cannot support - in that case, the relevant limit is used.
	   */
	case 's':
	  {
	    size_t arg_size;
	    act_on_init_result();
	    arg_size = parse_num (optarg, 's', 1L,
				  bc_ctl.posix_arg_size_max, 0);
	    if (arg_size > bc_ctl.posix_arg_size_max)
	      {
		error (0, 0,
		       _("warning: value %ld for -s option is too large, "
			 "using %ld instead"),
		       arg_size, bc_ctl.posix_arg_size_max);
		arg_size = bc_ctl.posix_arg_size_max;
	      }
	    bc_ctl.arg_max = arg_size;
	  }
	  break;

	case 'S':
	  show_limits = true;
	  break;

	case 't':
	  print_command = true;
	  break;

	case 'x':
	  bc_ctl.exit_if_size_exceeded = true;
	  break;

	case 'p':
	  query_before_executing = true;
	  print_command = true;
	  break;

	case 'r':
	  always_run_command = 0;
	  break;

	case 'P':
	  proc_max = parse_num (optarg, 'P', 0L, -1L, 1);
	  break;

        case 'a':
          input_file = optarg;
          break;

	case 'v':
	  printf (_("GNU xargs version %s\n"), version_string);
	  printf (_("Built using GNU gnulib version %s\n"), gnulib_version);
	  return 0;

	default:
	  usage (stderr);
	  return 1;
	}
    }

  /* If we had deferred failing due to problems in bc_init_controlinfo(),
   * do it now.
   *
   * We issue this error message after processing command line 
   * arguments so that it is possible to use "xargs --help" even if
   * the environment is too large. 
   */
  act_on_init_result();
  assert(BC_INIT_OK == bcstatus);

  if (0 == strcmp (input_file, "-"))
    {
      input_stream = stdin;
    }
  else
    {
      keep_stdin = 1;		/* see prep_child_for_exec() */
      input_stream = fopen (input_file, "r");
      if (NULL == input_stream)
	{
	  error (1, errno,
		 _("Cannot open input file `%s'"),
		 input_file);
	}
    }

  if (bc_ctl.replace_pat || bc_ctl.lines_per_exec)
    bc_ctl.exit_if_size_exceeded = true;

  if (optind == argc)
    {
      optind = 0;
      argc = 1;
      argv = &default_cmd;
    }

  /* We want to be able to print size_t values as unsigned long, so if
   * the cast isn't value-preserving, we have a problem.  This isn't a
   * problem in C89, because size_t was known to be no wider than
   * unsigned long.  In C99 this is no longer the case, but there are
   * special C99 ways to print such values.  Unfortunately this
   * program tries to work on both C89 and C99 systems.
   */
#if defined(SIZE_MAX)
# if SIZE_MAX > ULONG_MAX
# error "I'm not sure how to print size_t values on your system"
# endif
#else
  /* Without SIZE_MAX (i.e. limits.h) this is probably 
   * close to the best we can do.
   */
  assert(sizeof(size_t) <= sizeof(unsigned long));
#endif

  if (show_limits)
    {
      fprintf(stderr,
	      _("Your environment variables take up %lu bytes\n"),
	      (unsigned long)bc_size_of_environment());
      fprintf(stderr,
	      _("POSIX lower and upper limits on argument length: %lu, %lu\n"),
	      (unsigned long)bc_ctl.posix_arg_size_min,
	      (unsigned long)bc_ctl.posix_arg_size_max);
      fprintf(stderr,
	      _("Maximum length of command we could actually use: %ld\n"),
	      (unsigned long)(bc_ctl.posix_arg_size_max -
			      bc_size_of_environment()));
      fprintf(stderr,
	      _("Size of command buffer we are actually using: %lu\n"),
	      (unsigned long)bc_ctl.arg_max);

      if (isatty(STDIN_FILENO))
	{
	  fprintf(stderr,
		  "\n"
		  "Execution of xargs will continue now, and it will "
		  "try to read its input and run commands; if this is "
		  "not what you wanted to happen, please type the "
		  "end-of-file keystroke.\n");
	}
    }
  
  linebuf = (char *) xmalloc (bc_ctl.arg_max + 1);
  bc_state.argbuf = (char *) xmalloc (bc_ctl.arg_max + 1);

  /* Make sure to listen for the kids.  */
  signal (SIGCHLD, SIG_DFL);

  if (!bc_ctl.replace_pat)
    {
      for (; optind < argc; optind++)
	bc_push_arg (&bc_ctl, &bc_state,
		     argv[optind], strlen (argv[optind]) + 1,
		     NULL, 0,
		     initial_args);
      initial_args = false;
      bc_ctl.initial_argc = bc_state.cmd_argc;
      bc_state.cmd_initial_argv_chars = bc_state.cmd_argv_chars;

      while ((*read_args) () != -1)
	if (bc_ctl.lines_per_exec && lineno >= bc_ctl.lines_per_exec)
	  {
	    xargs_do_exec (&bc_ctl, &bc_state);
	    lineno = 0;
	  }

      /* SYSV xargs seems to do at least one exec, even if the
         input is empty.  */
      if (bc_state.cmd_argc != bc_ctl.initial_argc
	  || (always_run_command && procs_executed == 0))
	xargs_do_exec (&bc_ctl, &bc_state);

    }
  else
    {
      int i;
      size_t len;
      size_t *arglen = (size_t *) xmalloc (sizeof (size_t) * argc);

      for (i = optind; i < argc; i++)
	arglen[i] = strlen(argv[i]);
      bc_ctl.rplen = strlen (bc_ctl.replace_pat);
      while ((len = (*read_args) ()) != -1)
	{
	  /* Don't do insert on the command name.  */
	  bc_clear_args(&bc_ctl, &bc_state);
	  bc_state.cmd_argv_chars = 0; /* begin at start of buffer */
	  
	  bc_push_arg (&bc_ctl, &bc_state,
		       argv[optind], arglen[optind] + 1,
		       NULL, 0,
		       initial_args);
	  len--;
	  initial_args = false;
	  
	  for (i = optind + 1; i < argc; i++)
	    bc_do_insert (&bc_ctl, &bc_state,
			  argv[i], arglen[i],
			  NULL, 0,
			  linebuf, len,
			  initial_args);
	  xargs_do_exec (&bc_ctl, &bc_state);
	}
    }

  original_exit_value = child_error;
  return child_error;
}


/* Read a line of arguments from the input and add them to the list of
   arguments to pass to the command.  Ignore blank lines and initial blanks.
   Single and double quotes and backslashes quote metacharacters and blanks
   as they do in the shell.
   Return -1 if eof (either physical or logical) is reached,
   otherwise the length of the last string read (including the null).  */

static int
read_line (void)
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
  char *endbuf = linebuf + bc_ctl.arg_max - bc_state.cmd_initial_argv_chars - 1;

  if (eof)
    return -1;
  while (1)
    {
      prevc = c;
      c = getc (input_stream);
      if (c == EOF)
	{
	  /* COMPAT: SYSV seems to ignore stuff on a line that
	     ends without a \n; we don't.  */
	  eof = true;
	  if (p == linebuf)
	    return -1;
	  *p++ = '\0';
	  len = p - linebuf;
	  if (state == QUOTE)
	    {
	      exec_if_possible ();
	      error (1, 0, _("unmatched %s quote; by default quotes are special to xargs unless you use the -0 option"),
		     quotc == '"' ? _("double") : _("single"));
	    }
	  if (first && EOF_STR (linebuf))
	    return -1;
	  if (!bc_ctl.replace_pat)
	    bc_push_arg (&bc_ctl, &bc_state,
			 linebuf, len,
			 NULL, 0,
			 initial_args);
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
	      if (!bc_ctl.replace_pat)
		bc_push_arg (&bc_ctl, &bc_state,
			     linebuf, len,
			     NULL, 0,
			     initial_args);
	      return len;
	    }
	  if (!bc_ctl.replace_pat && ISSPACE (c))
	    {
	      *p++ = '\0';
	      len = p - linebuf;
	      if (EOF_STR (linebuf))
		{
		  eof = true;
		  return first ? -1 : len;
		}
	      bc_push_arg (&bc_ctl, &bc_state,
			   linebuf, len,
			   NULL, 0,
			   initial_args);
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
	    {
	      exec_if_possible ();
	      error (1, 0, _("unmatched %s quote; by default quotes are special to xargs unless you use the -0 option"),
		     quotc == '"' ? _("double") : _("single"));
	    }
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
#if 1
      if (p >= endbuf)
        {
	  exec_if_possible ();
	  error (1, 0, _("argument line too long"));
	}
      *p++ = c;
#else
      append_char_to_buf(&linebuf, &endbuf, &p, c);
#endif
    }
}

/* Read a null-terminated string from the input and add it to the list of
   arguments to pass to the command.
   Return -1 if eof (either physical or logical) is reached,
   otherwise the length of the string read (including the null).  */

static int
read_string (void)
{
  static boolean eof = false;
  int len;
  char *p = linebuf;
  /* Including the NUL, the args must not grow past this point.  */
  char *endbuf = linebuf + bc_ctl.arg_max - bc_state.cmd_initial_argv_chars - 1;

  if (eof)
    return -1;
  while (1)
    {
      int c = getc (input_stream);
      if (c == EOF)
	{
	  eof = true;
	  if (p == linebuf)
	    return -1;
	  *p++ = '\0';
	  len = p - linebuf;
	  if (!bc_ctl.replace_pat)
	    bc_push_arg (&bc_ctl, &bc_state,
			 linebuf, len,
			 NULL, 0,
			 initial_args);
	  return len;
	}
      if (c == input_delimiter)
	{
	  lineno++;		/* For -l.  */
	  *p++ = '\0';
	  len = p - linebuf;
	  if (!bc_ctl.replace_pat)
	    bc_push_arg (&bc_ctl, &bc_state,
			 linebuf, len,
			 NULL, 0,
			 initial_args);
	  return len;
	}
      if (p >= endbuf)
        {
	  exec_if_possible ();
	  error (1, 0, _("argument line too long"));
	}
      *p++ = c;
    }
}

/* Print the arguments of the command to execute.
   If ASK is nonzero, prompt the user for a response, and
   if the user responds affirmatively, return true;
   otherwise, return false.  */

static boolean
print_args (boolean ask)
{
  int i;

  for (i = 0; i < bc_state.cmd_argc - 1; i++)
    fprintf (stderr, "%s ", bc_state.cmd_argv[i]);
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


/* Close stdin and attach /dev/null to it.
 * This resolves Savannah bug #3992.
 */
static void
prep_child_for_exec (void)
{
  if (!keep_stdin)
    {
      const char inputfile[] = "/dev/null";
      /* fprintf(stderr, "attaching stdin to /dev/null\n"); */
      
      close(0);
      if (open(inputfile, O_RDONLY) < 0)
	{
	  /* This is not entirely fatal, since 
	   * executing the child with a closed
	   * stdin is almost as good as executing it
	   * with its stdin attached to /dev/null.
	   */
	  error (0, errno, "%s", inputfile);
	}
    }
}


/* Execute the command that has been built in `cmd_argv'.  This may involve
   waiting for processes that were previously executed.  */

static int
xargs_do_exec (const struct buildcmd_control *ctl, struct buildcmd_state *state)
{
  pid_t child;

  bc_push_arg (&bc_ctl, &bc_state,
	       (char *) NULL, 0,
	       NULL, 0,
	       false); /* Null terminate the arg list.  */
  
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
	  error (1, errno, _("cannot fork"));

	case 0:		/* Child.  */
	  prep_child_for_exec();
	  execvp (bc_state.cmd_argv[0], bc_state.cmd_argv);
	  error (0, errno, "%s", bc_state.cmd_argv[0]);
	  _exit (errno == ENOENT ? 127 : 126);
	  /*NOTREACHED*/
	}
      add_proc (child);
    }

  bc_clear_args(&bc_ctl, &bc_state);
  return 1;			/* Success */
}

/* Execute the command if possible.  */

static void
exec_if_possible (void)
{
  if (bc_ctl.replace_pat || initial_args ||
      bc_state.cmd_argc == bc_ctl.initial_argc || bc_ctl.exit_if_size_exceeded)
    return;
  xargs_do_exec (&bc_ctl, &bc_state);
}

/* Add the process with id PID to the list of processes that have
   been executed.  */

static void
add_proc (pid_t pid)
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
wait_for_proc (boolean all)
{
  while (procs_executing)
    {
      int i, status;

      do
	{
	  pid_t pid;

	  while ((pid = wait (&status)) == (pid_t) -1)
	    if (errno != EINTR)
	      error (1, errno, _("error waiting for child process"));

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
	error (124, 0, _("%s: exited with status 255; aborting"), bc_state.cmd_argv[0]);
      if (WIFSTOPPED (status))
	error (125, 0, _("%s: stopped by signal %d"), bc_state.cmd_argv[0], WSTOPSIG (status));
      if (WIFSIGNALED (status))
	error (125, 0, _("%s: terminated by signal %d"), bc_state.cmd_argv[0], WTERMSIG (status));
      if (WEXITSTATUS (status) != 0)
	child_error = 123;

      if (!all)
	break;
    }
}

/* Wait for all child processes to finish.  */

static void
wait_for_proc_all (void)
{
  static boolean waiting = false;
  
  if (waiting)
    return;

  waiting = true;
  wait_for_proc (true);
  waiting = false;
  
  if (original_exit_value != child_error)
    {
      /* wait_for_proc() changed the value of child_error().  This
       * function is registered via atexit(), and so may have been
       * called from exit().  We now know that the original value
       * passed to exit() is no longer the exit status we require.
       * The POSIX standard states that the behaviour if exit() is
       * called more than once is undefined.  Therefore we now have to
       * exit with _exit() instead of exit().
       */
      _exit(child_error);
    }
  
}

/* Return the value of the number represented in STR.
   OPTION is the command line option to which STR is the argument.
   If the value does not fall within the boundaries MIN and MAX,
   Print an error message mentioning OPTION.  If FATAL is true, 
   we also exit. */

static long
parse_num (char *str, int option, long int min, long int max, int fatal)
{
  char *eptr;
  long val;

  val = strtol (str, &eptr, 10);
  if (eptr == str || *eptr)
    {
      fprintf (stderr, _("%s: invalid number for -%c option\n"),
	       program_name, option);
      usage (stderr);
      exit(1);
    }
  else if (val < min)
    {
      fprintf (stderr, _("%s: value for -%c option should be >= %ld\n"),
	       program_name, option, min);
      if (fatal)
	{
	  usage (stderr);
	  exit(1);
	}
      else
	{
	  val = min;
	}
    }
  else if (max >= 0 && val > max)
    {
      fprintf (stderr, _("%s: value for -%c option should be < %ld\n"),
	       program_name, option, max);
      if (fatal)
	{
	  usage (stderr);
	  exit(1);
	}
      else
	{
	  val = max;
	}
    }
  return val;
}

/* Return how much of ARG_MAX is used by the environment.  */

static void
usage (FILE *stream)
{
  fprintf (stream, _("\
Usage: %s [-0prtx] [--interactive] [--null] [-d|--delimiter=delim]\n\
       [-E eof-str] [-e[eof-str]]  [--eof[=eof-str]]\n\
       [-L max-lines] [-l[max-lines]] [--max-lines[=max-lines]]\n\
       [-I replace-str] [-i[replace-str]] [--replace[=replace-str]]\n\
       [-n max-args] [--max-args=max-args]\n\
       [-s max-chars] [--max-chars=max-chars]\n\
       [-P max-procs]  [--max-procs=max-procs] [--show-limits]\n\
       [--verbose] [--exit] [--no-run-if-empty] [--arg-file=file]\n\
       [--version] [--help] [command [initial-arguments]]\n"),
	   program_name);
  fputs (_("\nReport bugs to <bug-findutils@gnu.org>.\n"), stream);
}
