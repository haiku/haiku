/* Handle so called `shell archives'.
   Copyright (C) 1994, 1995 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/* Unpackage one or more shell archive files.  The `unshar' program is a
   filter which removes the front part of a file and passes the rest to
   the `sh' command.  It understands phrases like "cut here", and also
   knows about shell comment characters and the Unix commands `echo',
   `cat', and `sed'.  */

#include "system.h"
#include "getopt.h"

/* Buffer size for holding a file name.  FIXME: No fix limit in GNU... */
#define NAME_BUFFER_SIZE 1024

/* Buffer size for shell process input.  */
#define SHELL_BUFFER_SIZE 8196

#define EOL '\n'

/* The name this program was run with. */
const char *program_name;

/* If non-zero, display usage information and exit.  */
static int show_help = 0;

/* If non-zero, print the version on standard output and exit.  */
static int show_version = 0;

static int pass_c_flag = 0;
static int continue_reading = 0;
static const char *exit_string = "exit 0";
static size_t exit_string_length;
static char *current_directory;

/*-------------------------------------------------------------------.
| Match the leftmost part of a string.  Returns 1 if initial	     |
| characters of DATA match PATTERN exactly; else 0.  This was	     |
| formerly a function.  But because we always have a constant string |
| as the seconf argument and the length of the second argument is a  |
| lot of shorter than the buffer the first argument is pointing at,  |
| we simply use `memcmp'.  And one more point: even if the `memcmp'  |
| function does not work correct for 8 bit characters it does not    |
| matter here.  We are only interested in equal or not equal	     |
| information.							     |
`-------------------------------------------------------------------*/

#define starting_with(data, pattern)					    \
  (memcmp (data, pattern, sizeof (pattern) - 1) == 0)


/*-------------------------------------------------------------------------.
| For a DATA string and a PATTERN containing one or more embedded	   |
| asterisks (matching any number of characters), return non-zero if the	   |
| match succeeds, and set RESULT_ARRAY[I] to the characters matched by the |
| I'th *.								   |
`-------------------------------------------------------------------------*/

static int
matched_by (data, pattern, result_array)
     const char *data;
     const char *pattern;
     char **result_array;
{
  const char *pattern_cursor = NULL;
  const char *data_cursor = NULL;
  char *result_cursor = NULL;
  int number_of_results = 0;

  while (1)
    if (*pattern == '*')
      {
	pattern_cursor = ++pattern;
	data_cursor = data;
	result_cursor = result_array[number_of_results++];
	*result_cursor = '\0';
      }
    else if (*data == *pattern)
      {
	if (*pattern == '\0')
	  /* The pattern matches.  */
	  return 1;

	pattern++;
	data++;
      }
    else
      {
	if (*data == '\0')
	  /* The pattern fails: no more data.  */
	  return 0;

	if (pattern_cursor == NULL)
	  /* The pattern fails: no star to adjust.  */
	  return 0;

	/* Restart pattern after star.  */

	pattern = pattern_cursor;
	*result_cursor++ = *data_cursor;
	*result_cursor = '\0';

	/* Rescan after copied char.  */

	data = ++data_cursor;
      }
}

/*------------------------------------------------------------------------.
| Associated with a given file NAME, position FILE at the start of the	  |
| shell command portion of a shell archive file.  Scan file from position |
| START.								  |
`------------------------------------------------------------------------*/

static int
find_archive (name, file, start)
     const char *name;
     FILE *file;
     off_t start;
{
  char buffer[BUFSIZ];
  off_t position;

  /* Results from star matcher.  */

  static char res1[BUFSIZ], res2[BUFSIZ], res3[BUFSIZ], res4[BUFSIZ];
  static char *result[] = {res1, res2, res3, res4};

  fseek (file, start, 0);

  while (1)
    {

      /* Record position of the start of this line.  */

      position = ftell (file);

      /* Read next line, fail if no more and no previous process.  */

      if (!fgets (buffer, BUFSIZ, file))
	{
	  if (!start)
	    error (0, 0, _("Found no shell commands in %s"), name);
	  return 0;
	}

      /* Bail out if we see C preprocessor commands or C comments.  */

      if (starting_with (buffer, "#include")
	  || starting_with (buffer, "# include")
	  || starting_with (buffer, "#define")
	  || starting_with (buffer, "# define")
	  || starting_with (buffer, "#ifdef")
	  || starting_with (buffer, "# ifdef")
	  || starting_with (buffer, "#ifndef")
	  || starting_with (buffer, "# ifndef")
	  || starting_with (buffer, "/*"))
	{
	  error (0, 0, _("%s looks like raw C code, not a shell archive"),
		 name);
	  return 0;
	}

      /* Does this line start with a shell command or comment.  */

      if (starting_with (buffer, "#")
	  || starting_with (buffer, ":")
	  || starting_with (buffer, "echo ")
	  || starting_with (buffer, "sed ")
	  || starting_with (buffer, "cat ")
	  || starting_with (buffer, "if "))
	{
	  fseek (file, position, 0);
	  return 1;
	}

      /* Does this line say "Cut here".  */

      if (matched_by (buffer, "*CUT*HERE*", result) ||
	  matched_by (buffer, "*cut*here*", result) ||
	  matched_by (buffer, "*TEAR*HERE*", result) ||
	  matched_by (buffer, "*tear*here*", result) ||
	  matched_by (buffer, "*CUT*CUT*", result) ||
	  matched_by (buffer, "*cut*cut*", result))
	{

	  /* Read next line after "cut here", skipping blank lines.  */

	  while (1)
	    {
	      position = ftell (file);

	      if (!fgets (buffer, BUFSIZ, file))
		{
		  error (0, 0, _("Found no shell commands after `cut' in %s"),
			 name);
		  return 0;
		}

	      if (*buffer != '\n')
		break;
	    }

	  /* Win if line starts with a comment character of lower case
	     letter.  */

	  if (*buffer == '#' || *buffer == ':'
	      || (('a' <= *buffer) && ('z' >= *buffer)))
	    {
	      fseek (file, position, 0);
	      return 1;
	    }

	  /* Cut here message lied to us.  */

	  error (0, 0, _("%s is probably not a shell archive"), name);
	  error (0, 0, _("The `cut' line was followed by: %s"), buffer);
	  return 0;
	}
    }
}

/*-----------------------------------------------------------------.
| Unarchive a shar file provided on file NAME.  The file itself is |
| provided on the already opened FILE.				   |
`-----------------------------------------------------------------*/

static void
unarchive_shar_file (name, file)
     const char *name;
     FILE *file;
{
  char buffer[SHELL_BUFFER_SIZE];
  FILE *shell_process;
  off_t current_position = 0;
  char *more_to_read;

  while (find_archive (name, file, current_position))
    {
      printf ("%s:\n", name);
      shell_process = popen (pass_c_flag ? "sh -s - -c" : "sh", "w");
      if (!shell_process)
	error (EXIT_FAILURE, errno, _("Starting `sh' process"));

      if (!continue_reading)
	{
	  size_t len;

	  while ((len = fread (buffer, 1, SHELL_BUFFER_SIZE, file)) != 0)
	    fwrite (buffer, 1, len, shell_process);
#if 0
	  /* Don't know whether a test is appropriate here.  */
	  if (ferror (shell_process) != 0)
	    fwrite (buffer, length, 1, shell_process);
#endif
	  pclose (shell_process);
	  break;
	}
      else
	{
	  while (more_to_read = fgets (buffer, SHELL_BUFFER_SIZE, file),
		 more_to_read != NULL)
	    {
	      fputs (buffer, shell_process);
	      if (!strncmp (exit_string, buffer, exit_string_length))
		break;
	    }
	  pclose (shell_process);

	  if (more_to_read)
	    current_position = ftell (file);
	  else
	    break;
	}
    }
}

/*-----------------------------.
| Explain how to use program.  |
`-----------------------------*/

static void
usage (status)
     int status;
{
  if (status != EXIT_SUCCESS)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("Usage: %s [OPTION]... [FILE]...\n"), program_name);
      fputs (_("\
Mandatory arguments to long options are mandatory for short options too.\n\
\n\
  -d, --directory=DIRECTORY   change to DIRECTORY before unpacking\n\
  -c, --overwrite             pass -c to shar script for overwriting files\n\
  -e, --exit-0                same as `--split-at=\"exit 0\"'\n\
  -E, --split-at=STRING       split concatenated shars after STRING\n\
  -f, --force                 same as `-c'\n\
      --help                  display this help and exit\n\
      --version               output version information and exit\n\
\n\
If no FILE, standard input is read.\n"),
	     stdout);
    }
  exit (status);
}

/*--------------------------------------.
| Decode options and launch execution.  |
`--------------------------------------*/

static const struct option long_options[] =
{
  {"directory", required_argument, NULL, 'd'},
  {"exit-0", no_argument, NULL, 'e'},
  {"force", no_argument, NULL, 'f'},
  {"overwrite", no_argument, NULL, 'c'},
  {"split-at", required_argument, NULL, 'E'},

  {"help", no_argument, &show_help, 1},
  {"version", no_argument, &show_version, 1},

  { NULL, 0, NULL, 0 },
};

int
main (argc, argv)
     int argc;
     char *const *argv;
{
  size_t size_read;
  FILE *file;
  char name_buffer[NAME_BUFFER_SIZE];
  char copy_buffer[NAME_BUFFER_SIZE];
  int optchar;

  program_name = argv[0];
  setlocale (LC_ALL, "");

  /* Set the text message domain.  */
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

#ifdef __MSDOS__
  setbuf (stdout, NULL);
  setbuf (stderr, NULL);
#endif

  if (current_directory = xgetcwd (), !current_directory)
    error (EXIT_FAILURE, errno, _("Cannot get current directory name"));

  /* Process options.  */

  while (optchar = getopt_long (argc, argv, "E:cd:ef", long_options, NULL),
	 optchar != EOF)
    switch (optchar)
      {
      case '\0':
	break;

      case 'c':
      case 'f':
	pass_c_flag = 1;
	break;

      case 'd':
	if (chdir (optarg) == -1)
	  error (2, 0, _("Cannot chdir to `%s'"), optarg);
	break;

      case 'E':
	exit_string = optarg;
	/* Fall through.  */

      case 'e':
	continue_reading = 1;
	exit_string_length = strlen (exit_string);
	break;

      default:
	usage (EXIT_FAILURE);
      }

  if (show_version)
    {
      printf ("%s - GNU %s %s\n", program_name, PACKAGE, VERSION);
      exit (EXIT_SUCCESS);
    }

  if (show_help)
    usage (EXIT_SUCCESS);

  if (optind < argc)
    for (; optind < argc; optind++)
      {
	if (argv[optind][0] == '/')
	  stpcpy (name_buffer, argv[optind]);
	else
	  {
	    char *cp = stpcpy (name_buffer, current_directory);
	    *cp++ = '/';
	    stpcpy (cp, argv[optind]);
	  }
	if (file = fopen (name_buffer, "r"), !file)
	  error (EXIT_FAILURE, errno, name_buffer);
	unarchive_shar_file (name_buffer, file);
	fclose (file);
      }
  else
    {
      sprintf (name_buffer, "/tmp/unsh.%05d", (int) getpid ());
      unlink (name_buffer);

      if (file = fopen (name_buffer, "w+"), !file)
	error (EXIT_FAILURE, errno, name_buffer);
#ifndef __MSDOS__
      unlink (name_buffer);	/* will be deleted on fclose */
#endif

      while (size_read = fread (copy_buffer, 1, sizeof (copy_buffer), stdin),
	     size_read != 0)
	fwrite (copy_buffer, size_read, 1, file);
      rewind (file);

      unarchive_shar_file (_("standard input"), file);

      fclose (file);
#ifdef __MSDOS__
      unlink (name_buffer);
#endif
    }

  exit (EXIT_SUCCESS);
}
