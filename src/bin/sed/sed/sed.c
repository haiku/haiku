#define COPYRIGHT_NOTICE "Copyright (C) 2003 Free Software Foundation, Inc."
#define BUG_ADDRESS "bonzini@gnu.org"

/*  GNU SED, a batch stream editor.
    Copyright (C) 1989,90,91,92,93,94,95,98,99,2002,2003
    Free Software Foundation, Inc.

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
    Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */


#include "sed.h"

#include <stdio.h>
#ifdef HAVE_STRINGS_H
# include <strings.h>
#else
# include <string.h>
#endif /*HAVE_STRINGS_H*/
#ifdef HAVE_MEMORY_H
# include <memory.h>
#endif

#ifndef HAVE_STRCHR
# define strchr index
# define strrchr rindex
#endif

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_LOCALE_H
# include <locale.h>
#endif
#include "getopt.h"

#ifndef BOOTSTRAP
#ifndef HAVE_STDLIB_H
 extern char *getenv P_((const char *));
#endif
#endif

#ifndef HAVE_STRTOUL
# define ATOI(x)	atoi(x)
#else
# define ATOI(x)	strtoul(x, NULL, 0)
#endif

int extended_regexp_flags = 0;

/* If set, fflush(stdout) on every line output. */
flagT unbuffered_output = FALSE;

/* If set, don't write out the line unless explicitly told to */
flagT no_default_output = FALSE;

/* If set, reset line counts on every new file. */
flagT separate_files = FALSE;

/* How do we edit files in-place? (we don't if NULL) */
char *in_place_extension = NULL;

/* Do we need to be pedantically POSIX compliant? */
flagT POSIXLY_CORRECT;

/* How long should the `l' command's output line be? */
countT lcmd_out_line_len = 70;

/* The complete compiled SED program that we are going to run: */
static struct vector *the_program = NULL;

static void usage P_((int));
static void
usage(status)
  int status;
{
  FILE *out = status ? stderr : stdout;

#ifdef REG_PERL
#define PERL_HELP _("  -R, --regexp-perl\n                 use Perl 5's regular expressions syntax in the script.\n")
#else
#define PERL_HELP ""
#endif

  fprintf(out, _("\
Usage: %s [OPTION]... {script-only-if-no-other-script} [input-file]...\n\
\n\
  -n, --quiet, --silent\n\
                 suppress automatic printing of pattern space\n\
  -e script, --expression=script\n\
                 add the script to the commands to be executed\n\
  -f script-file, --file=script-file\n\
                 add the contents of script-file to the commands to be executed\n\
  -i[suffix], --in-place[=suffix]\n\
                 edit files in place (makes backup if extension supplied)\n\
  -l N, --line-length=N\n\
                 specify the desired line-wrap length for the `l' command\n\
  -r, --regexp-extended\n\
                 use extended regular expressions in the script.\n%s\
  -s, --separate\n\
                 consider files as separate rather than as a single continuous\n\
                 long stream.\n\
  -u, --unbuffered\n\
                 load minimal amounts of data from the input files and flush\n\
                 the output buffers more often\n\
      --help     display this help and exit\n\
  -V, --version  output version information and exit\n\
\n\
If no -e, --expression, -f, or --file option is given, then the first\n\
non-option argument is taken as the sed script to interpret.  All\n\
remaining arguments are names of input files; if no input files are\n\
specified, then the standard input is read.\n\
\n"), myname, PERL_HELP);
  fprintf(out, _("E-mail bug reports to: %s .\n\
Be sure to include the word ``%s'' somewhere in the ``Subject:'' field.\n"),
	  BUG_ADDRESS, PACKAGE);

  ck_fclose (NULL);
  exit (status);
}

int
main(argc, argv)
  int argc;
  char **argv;
{
#ifdef REG_PERL
#define SHORTOPTS "shnrRuVe:f:l:i::"
#else
#define SHORTOPTS "shnruVe:f:l:i::"
#endif

  static struct option longopts[] = {
    {"regexp-extended", 0, NULL, 'r'},
#ifdef REG_PERL
    {"regexp-perl", 0, NULL, 'R'},
#endif
    {"expression", 1, NULL, 'e'},
    {"file", 1, NULL, 'f'},
    {"in-place", 2, NULL, 'i'},
    {"line-length", 1, NULL, 'l'},
    {"quiet", 0, NULL, 'n'},
    {"silent", 0, NULL, 'n'},
    {"separate", 0, NULL, 's'},
    {"unbuffered", 0, NULL, 'u'},
    {"version", 0, NULL, 'V'},
    {"help", 0, NULL, 'h'},
    {NULL, 0, NULL, 0}
  };

  int opt;
  int return_code;

  initialize_main(&argc, &argv);

#if ENABLE_NLS
#if HAVE_SETLOCALE
  /* Set locale according to user's wishes.  */
  setlocale (LC_ALL, "");
#endif

  /* Tell program which translations to use and where to find.  */
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);
#endif

  POSIXLY_CORRECT = (getenv("POSIXLY_CORRECT") != NULL);

  /* If environment variable `COLS' is set, use its value for
     the baseline setting of `lcmd_out_line_len'.  The "-1"
     is to avoid gratuitous auto-line-wrap on ttys.
   */
  {
    const char *p = getenv("COLS");
    if (p)
      {
	countT t = ATOI(p);
	if (1 < t)
	  lcmd_out_line_len = t-1;
      }
  }

  myname = *argv;
  while ((opt = getopt_long(argc, argv, SHORTOPTS, longopts, NULL)) != EOF)
    {
      switch (opt)
	{
	case 'n':
	  no_default_output = TRUE;
	  break;
	case 'e':
	  the_program = compile_string(the_program, optarg, strlen(optarg));
	  break;
	case 'f':
	  the_program = compile_file(the_program, optarg);
	  break;

	case 'i':
	  separate_files = TRUE;
	  if (optarg == NULL)
	    in_place_extension = "*";
	  else if (strchr(optarg, '*') == NULL)
	    {
	      in_place_extension = xmalloc (strlen(optarg) + 2);
	      in_place_extension[0] = '*';
	      strcpy (in_place_extension + 1, optarg);
	    }
	  else
	    in_place_extension = ck_strdup(optarg);
	  break;

	case 'l':
	  lcmd_out_line_len = ATOI(optarg);
	  break;
	case 'r':
	  if (extended_regexp_flags)
	    usage(4);
	  extended_regexp_flags = REG_EXTENDED;
	  break;

#ifdef REG_PERL
	case 'R':
	  if (extended_regexp_flags)
	    usage(4);
	  extended_regexp_flags = REG_PERL;
	  break;
#endif

	case 's':
	  separate_files = TRUE;
	  break;

	case 'u':
	  unbuffered_output = TRUE;
	  break;

	case 'V':
#ifdef REG_PERL
	  fprintf(stdout, _("super-sed version %s\n"), VERSION);
	  fprintf(stdout, _("based on GNU sed version 3.02.80\n\n"));
#else
	  fprintf(stdout, _("GNU sed version %s\n"), VERSION);
#endif
	  fprintf(stdout, _("%s\n\
This is free software; see the source for copying conditions.  There is NO\n\
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE,\n\
to the extent permitted by law.\n\
"), COPYRIGHT_NOTICE);
	  ck_fclose (NULL);
	  exit (0);
	case 'h':
	  usage(0);
	default:
	  usage(4);
	}
    }

  if (!the_program)
    {
      if (optind < argc)
	{
	  char *arg = argv[optind++];
	  the_program = compile_string(the_program, arg, strlen(arg));
	}
      else
	usage(4);
    }
  check_final_program(the_program);

  return_code = process_files(the_program, argv+optind);

  finish_program(the_program);
  ck_fclose(NULL);

  return return_code;
}
