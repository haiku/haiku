#define COPYRIGHT_NOTICE "Copyright (C) 1998 Free Software Foundation, Inc."
#define BUG_ADDRESS "bug-gnu-utils@gnu.org"

/*  GNU SED, a batch stream editor.
    Copyright (C) 1989, 1990, 1991, 1992, 1993, 1994, 1995, 1998 \
    Free Software Foundation, Inc."

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


#include "config.h"
#include <stdio.h>

#ifndef HAVE_STRING_H
# include <strings.h>
#else
# include <string.h>
#endif

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif

#ifdef HAVE_MMAP
# ifdef HAVE_UNISTD_H
#  include <unistd.h>
# endif
# include <sys/types.h>
# include <sys/mman.h>
# ifndef MAP_FAILED
#  define MAP_FAILED	((char *)-1)	/* what a stupid concept... */
# endif
# include <sys/stat.h>
# ifndef S_ISREG
#  define S_ISREG(m)	(((m)&S_IFMT) == S_IFMT)
# endif
#endif /* HAVE_MMAP */

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#include "regex-sed.h"
#include "getopt.h"
#include "basicdefs.h"
#include "utils.h"
#include "sed.h"

#ifndef HAVE_STDLIB_H
 extern char *getenv P_((const char *));
#endif

#ifdef STUB_FROM_RX_LIBRARY_USAGE
extern void print_rexp P_((void));
extern int rx_basic_unfaniverse_delay;
#endif /*STUB_FROM_RX_LIBRARY_USAGE*/

static void usage P_((int));

flagT use_extended_syntax_p = 0;

flagT rx_testing = 0;

/* If set, don't write out the line unless explicitly told to */
flagT no_default_output = 0;

/* Do we need to be pedantically POSIX compliant? */
flagT POSIXLY_CORRECT;

static void
usage(status)
  int status;
{
  FILE *out = status ? stderr : stdout;

  fprintf(out, "\
Usage: %s [OPTION]... {script-only-if-no-other-script} [input-file]...\n\
\n\
  -n, --quiet, --silent\n\
                 suppress automatic printing of pattern space\n\
  -e script, --expression=script\n\
                 add the script to the commands to be executed\n\
  -f script-file, --file=script-file\n\
                 add the contents of script-file to the commands to be executed\n\
      --help     display this help and exit\n\
  -V, --version  output version information and exit\n\
\n\
If no -e, --expression, -f, or --file option is given, then the first\n\
non-option argument is taken as the sed script to interpret.  All\n\
remaining arguments are names of input files; if no input files are\n\
specified, then the standard input is read.\n\
\n", myname);
  fprintf(out, "E-mail bug reports to: %s .\n\
Be sure to include the word ``%s'' somewhere in the ``Subject:'' field.\n",
	  BUG_ADDRESS, PACKAGE);
  exit(status);
}

int
main(argc, argv)
  int argc;
  char **argv;
{
  static struct option longopts[] = {
    {"rxtest", 0, NULL, 'r'},
    {"expression", 1, NULL, 'e'},
    {"file", 1, NULL, 'f'},
    {"quiet", 0, NULL, 'n'},
    {"silent", 0, NULL, 'n'},
    {"version", 0, NULL, 'V'},
    {"help", 0, NULL, 'h'},
    {NULL, 0, NULL, 0}
  };

  /* The complete compiled SED program that we are going to run: */
  struct vector *the_program = NULL;
  int opt;
  flagT bad_input;	/* If this variable is non-zero at exit, one or
			   more of the input files couldn't be opened. */

  POSIXLY_CORRECT = (getenv("POSIXLY_CORRECT") != NULL);
#ifdef STUB_FROM_RX_LIBRARY_USAGE
  if (!rx_default_cache)
    print_rexp();
  /* Allow roughly a megabyte of cache space. */
  rx_default_cache->bytes_allowed = 1<<20;
  rx_basic_unfaniverse_delay = 512 * 4;
#endif /*STUB_FROM_RX_LIBRARY_USAGE*/

  myname = *argv;
  while ((opt = getopt_long(argc, argv, "hnrVe:f:", longopts, NULL)) != EOF)
    {
      switch (opt)
	{
	case 'n':
	  no_default_output = 1;
	  break;
	case 'e':
	  the_program = compile_string(the_program, optarg);
	  break;
	case 'f':
	  the_program = compile_file(the_program, optarg);
	  break;

	case 'r':
	  use_extended_syntax_p = 1;
	  rx_testing = 1;
	  break;
	case 'V':
	  fprintf(stdout, "GNU %s version %s\n\n", PACKAGE, VERSION);
	  fprintf(stdout, "%s\n\
This is free software; see the source for copying conditions.  There is NO\n\
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE,\n\
to the extent permitted by law.\n\
", COPYRIGHT_NOTICE);
	  exit(0);
	case 'h':
	  usage(0);
	default:
	  usage(4);
	}
    }

  if (!the_program)
    {
      if (optind < argc)
	the_program = compile_string(the_program, argv[optind++]);
      else
	usage(4);
    }
  check_final_program(the_program);

  bad_input = process_files(the_program, argv+optind);

  close_all_files();
  ck_fclose(stdout);
  if (bad_input)
    exit(2);
  return 0;
}


/* Attempt to mmap() a file.  On failure, just return a zero,
   otherwise set *base and *len and return non-zero.  */
flagT
map_file(fp, base, len)
  FILE *fp;
  VOID **base;
  size_t *len;
{
#ifdef HAVE_MMAP
  struct stat s;
  VOID *nbase;

  if (fstat(fileno(fp), &s) == 0
      && S_ISREG(s.st_mode)
      && s.st_size == CAST(size_t)s.st_size)
    {
      /* "As if" the whole file was read into memory at this moment... */
      nbase = VCAST(VOID *)mmap(NULL, CAST(size_t)s.st_size, PROT_READ,
				MAP_PRIVATE, fileno(fp), CAST(off_t)0);
      if (nbase != MAP_FAILED)
	{
	  *base = nbase;
	  *len =  s.st_size;
	  return 1;
	}
    }
#endif /* HAVE_MMAP */

  return 0;
}

/* Attempt to munmap() a memory region. */
void
unmap_file(base, len)
  VOID *base;
  size_t len;
{
#ifdef HAVE_MMAP
  if (base)
    munmap(VCAST(caddr_t)base, len);
#endif
}
