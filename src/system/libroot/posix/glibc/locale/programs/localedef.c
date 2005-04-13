/* Copyright (C) 1995-1999, 2000, 2001, 2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1995.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <argp.h>
#include <errno.h>
#include <fcntl.h>
#include <libintl.h>
#include <locale.h>
#include <mcheck.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "error.h"
#include "charmap.h"
#include "locfile.h"

/* Undefine the following line in the production version.  */
/* #define NDEBUG 1 */
#include <assert.h>


/* List of copied locales.  */
struct copy_def_list_t *copy_list;

/* If this is defined be POSIX conform.  */
int posix_conformance;

/* If not zero give a lot more messages.  */
int verbose;

/* If not zero suppress warnings and information messages.  */
int be_quiet;

/* If not zero, produce old-style hash table instead of 3-level access tables.  */
int oldstyle_tables;

/* If not zero force output even if warning were issued.  */
static int force_output;

/* Prefix for output files.  */
const char *output_prefix;

/* Name of the character map file.  */
static const char *charmap_file;

/* Name of the locale definition file.  */
static const char *input_file;

/* Name of the repertoire map file.  */
const char *repertoire_global;

/* List of all locales.  */
static struct localedef_t *locales;


/* Name and version of program.  */
static void print_version (FILE *stream, struct argp_state *state);
void (*argp_program_version_hook) (FILE *, struct argp_state *) = print_version;

#define OPT_POSIX 1
#define OPT_QUIET 2
#define OPT_OLDSTYLE 3
#define OPT_PREFIX 4

/* Definitions of arguments for argp functions.  */
static const struct argp_option options[] =
{
  { NULL, 0, NULL, 0, N_("Input Files:") },
  { "charmap", 'f', "FILE", 0,
    N_("Symbolic character names defined in FILE") },
  { "inputfile", 'i', "FILE", 0, N_("Source definitions are found in FILE") },
  { "repertoire-map", 'u', "FILE", 0,
    N_("FILE contains mapping from symbolic names to UCS4 values") },

  { NULL, 0, NULL, 0, N_("Output control:") },
  { "force", 'c', NULL, 0,
    N_("Create output even if warning messages were issued") },
  { "old-style", OPT_OLDSTYLE, NULL, 0, N_("Create old-style tables") },
  { "prefix", OPT_PREFIX, "PATH", 0, N_("Optional output file prefix") },
  { "posix", OPT_POSIX, NULL, 0, N_("Be strictly POSIX conform") },
  { "quiet", OPT_QUIET, NULL, 0,
    N_("Suppress warnings and information messages") },
  { "verbose", 'v', NULL, 0, N_("Print more messages") },
  { NULL, 0, NULL, 0, NULL }
};

/* Short description of program.  */
static const char doc[] = N_("Compile locale specification");

/* Strings for arguments in help texts.  */
static const char args_doc[] = N_("NAME");

/* Prototype for option handler.  */
static error_t parse_opt (int key, char *arg, struct argp_state *state);

/* Function to print some extra text in the help message.  */
static char *more_help (int key, const char *text, void *input);

/* Data structure to communicate with argp functions.  */
static struct argp argp =
{
  options, parse_opt, args_doc, doc, NULL, more_help
};


/* Prototypes for global functions.  */
extern void *xmalloc (size_t __n);

/* Prototypes for local functions.  */
static void error_print (void);
static const char *construct_output_path (char *path);
static const char *normalize_codeset (const char *codeset, size_t name_len);


int
main (int argc, char *argv[])
{
  const char *output_path;
  int cannot_write_why;
  struct charmap_t *charmap;
  struct localedef_t global;
  int remaining;

  /* Set initial values for global variables.  */
  copy_list = NULL;
  posix_conformance = getenv ("POSIXLY_CORRECT") != NULL;
  error_print_progname = error_print;

  /* Set locale.  Do not set LC_ALL because the other categories must
     not be affected (according to POSIX.2).  */
  setlocale (LC_MESSAGES, "");
  setlocale (LC_CTYPE, "");

  /* Initialize the message catalog.  */
  textdomain (_libc_intl_domainname);

  /* Parse and process arguments.  */
  argp_err_exit_status = 4;
  argp_parse (&argp, argc, argv, 0, &remaining, NULL);

  /* POSIX.2 requires to be verbose about missing characters in the
     character map.  */
  verbose |= posix_conformance;

  if (argc - remaining != 1)
    {
      /* We need exactly one non-option parameter.  */
      argp_help (&argp, stdout, ARGP_HELP_SEE | ARGP_HELP_EXIT_ERR,
		 program_invocation_short_name);
      exit (4);
    }

  /* The parameter describes the output path of the constructed files.
     If the described files cannot be written return a NULL pointer.  */
  output_path  = construct_output_path (argv[remaining]);
  cannot_write_why = errno;

  /* Now that the parameters are processed we have to reset the local
     ctype locale.  (P1003.2 4.35.5.2)  */
  setlocale (LC_CTYPE, "POSIX");

  /* Look whether the system really allows locale definitions.  POSIX
     defines error code 3 for this situation so I think it must be
     a fatal error (see P1003.2 4.35.8).  */
  if (sysconf (_SC_2_LOCALEDEF) < 0)
    error (3, 0, _("FATAL: system does not define `_POSIX2_LOCALEDEF'"));

  /* Process charmap file.  */
  charmap = charmap_read (charmap_file, verbose, be_quiet, 1);

  /* Add the first entry in the locale list.  */
  memset (&global, '\0', sizeof (struct localedef_t));
  global.name = input_file ?: "/dev/stdin";
  global.needed = ALL_LOCALES;
  locales = &global;

  /* Now read the locale file.  */
  if (locfile_read (&global, charmap) != 0)
    error (4, errno, _("cannot open locale definition file `%s'"), input_file);

  /* Perhaps we saw some `copy' instructions.  */
  while (1)
    {
      struct localedef_t *runp = locales;

      while (runp != NULL && (runp->needed & runp->avail) == runp->needed)
	runp = runp->next;

      if (runp == NULL)
	/* Everything read.  */
	break;

      if (locfile_read (runp, charmap) != 0)
	error (4, errno, _("cannot open locale definition file `%s'"),
	       runp->name);
    }

  /* Check the categories we processed in source form.  */
  check_all_categories (locales, charmap);

  /* We are now able to write the data files.  If warning were given we
     do it only if it is explicitly requested (--force).  */
  if (error_message_count == 0 || force_output != 0)
    {
      if (cannot_write_why != 0)
	error (4, cannot_write_why, _("cannot write output files to `%s'"),
	       output_path);
      else
	write_all_categories (locales, charmap, output_path);
    }
  else
    error (4, 0, _("no output file produced because warning were issued"));

  /* This exit status is prescribed by POSIX.2 4.35.7.  */
  exit (error_message_count != 0);
}


/* Handle program arguments.  */
static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case OPT_QUIET:
      be_quiet = 1;
      break;
    case OPT_POSIX:
      posix_conformance = 1;
      break;
    case OPT_OLDSTYLE:
      oldstyle_tables = 1;
      break;
    case OPT_PREFIX:
      output_prefix = arg;
      break;
    case 'c':
      force_output = 1;
      break;
    case 'f':
      charmap_file = arg;
      break;
    case 'i':
      input_file = arg;
      break;
    case 'u':
      repertoire_global = arg;
      break;
    case 'v':
      verbose = 1;
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}


static char *
more_help (int key, const char *text, void *input)
{
  char *cp;

  switch (key)
    {
    case ARGP_KEY_HELP_EXTRA:
      /* We print some extra information.  */
      asprintf (&cp, gettext ("\
System's directory for character maps : %s\n\
                       repertoire maps: %s\n\
                       locale path    : %s\n\
%s"),
		CHARMAP_PATH, REPERTOIREMAP_PATH, LOCALE_PATH, gettext ("\
Report bugs using the `glibcbug' script to <bugs@gnu.org>.\n"));
      return cp;
    default:
      break;
    }
  return (char *) text;
}

/* Print the version information.  */
static void
print_version (FILE *stream, struct argp_state *state)
{
  fprintf (stream, "localedef (GNU %s) %s\n", PACKAGE, VERSION);
  fprintf (stream, gettext ("\
Copyright (C) %s Free Software Foundation, Inc.\n\
This is free software; see the source for copying conditions.  There is NO\n\
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\
"), "2002");
  fprintf (stream, gettext ("Written by %s.\n"), "Ulrich Drepper");
}


/* The address of this function will be assigned to the hook in the error
   functions.  */
static void
error_print (void)
{
}


/* The parameter to localedef describes the output path.  If it does
   contain a '/' character it is a relative path.  Otherwise it names the
   locale this definition is for.  */
static const char *
construct_output_path (char *path)
{
  const char *normal = NULL;
  char *result;
  char *endp;

  if (strchr (path, '/') == NULL)
    {
      /* This is a system path.  First examine whether the locale name
	 contains a reference to the codeset.  This should be
	 normalized.  */
      char *startp;
      size_t n;

      startp = path;
      /* We must be prepared for finding a CEN name or a location of
	 the introducing `.' where it is not possible anymore.  */
      while (*startp != '\0' && *startp != '@' && *startp != '.'
	     && *startp != '+' && *startp != ',')
	++startp;
      if (*startp == '.')
	{
	  /* We found a codeset specification.  Now find the end.  */
	  endp = ++startp;
	  while (*endp != '\0' && *endp != '@')
	    ++endp;

	  if (endp > startp)
	    normal = normalize_codeset (startp, endp - startp);
	}
      else
	/* This is to keep gcc quiet.  */
	endp = NULL;

      /* We put an additional '\0' at the end of the string because at
	 the end of the function we need another byte for the trailing
	 '/'.  */
      if (normal == NULL)
	n = asprintf (&result, "%s%s/%s%c",
		      output_prefix ?: "", LOCALEDIR, path, '\0');
      else
	n = asprintf (&result, "%s%s/%.*s%s%s%c",
		      output_prefix ?: "", LOCALEDIR,
		      (int) (startp - path), path, normal, endp, '\0');

      endp = result + n - 1;
    }
  else
    {
      /* This is a user path.  Please note the additional byte in the
	 memory allocation.  */
      size_t len = strlen (path) + 1;
      result = xmalloc (len + 1);
      endp = mempcpy (result, path, len) - 1;
    }

  errno = 0;

  if (euidaccess (result, W_OK) == -1)
    /* Perhaps the directory does not exist now.  Try to create it.  */
    if (errno == ENOENT)
      {
	errno = 0;
	mkdir (result, 0777);
      }

  *endp++ = '/';
  *endp = '\0';

  return result;
}


/* Normalize codeset name.  There is no standard for the codeset
   names.  Normalization allows the user to use any of the common
   names.  */
static const char *
normalize_codeset (codeset, name_len)
     const char *codeset;
     size_t name_len;
{
  int len = 0;
  int only_digit = 1;
  char *retval;
  char *wp;
  size_t cnt;

  for (cnt = 0; cnt < name_len; ++cnt)
    if (isalnum (codeset[cnt]))
      {
	++len;

	if (isalpha (codeset[cnt]))
	  only_digit = 0;
      }

  retval = (char *) malloc ((only_digit ? 3 : 0) + len + 1);

  if (retval != NULL)
    {
      if (only_digit)
	wp = stpcpy (retval, "iso");
      else
	wp = retval;

      for (cnt = 0; cnt < name_len; ++cnt)
	if (isalpha (codeset[cnt]))
	  *wp++ = tolower (codeset[cnt]);
	else if (isdigit (codeset[cnt]))
	  *wp++ = codeset[cnt];

      *wp = '\0';
    }

  return (const char *) retval;
}


struct localedef_t *
add_to_readlist (int locale, const char *name, const char *repertoire_name,
		 int generate, struct localedef_t *copy_locale)
{
  struct localedef_t *runp = locales;

  while (runp != NULL && strcmp (name, runp->name) != 0)
    runp = runp->next;

  if (runp == NULL)
    {
      /* Add a new entry at the end.  */
      struct localedef_t *newp;

      assert (generate == 1);

      newp = xcalloc (1, sizeof (struct localedef_t));
      newp->name = name;
      newp->repertoire_name = repertoire_name;

      if (locales == NULL)
	runp = locales = newp;
      else
	{
	  runp = locales;
	  while (runp->next != NULL)
	    runp = runp->next;
	  runp = runp->next = newp;
	}
    }

  if (generate && (runp->needed & (1 << locale)) != 0)
    error (5, 0, _("circular dependencies between locale definitions"));

  if (copy_locale != NULL)
    {
      if (runp->categories[locale].generic != NULL)
	error (5, 0, _("cannot add already read locale `%s' a second time"),
	       name);
      else
	runp->categories[locale].generic =
	  copy_locale->categories[locale].generic;
    }

  runp->needed |= 1 << locale;

  return runp;
}


struct localedef_t *
find_locale (int locale, const char *name, const char *repertoire_name,
	     struct charmap_t *charmap)
{
  struct localedef_t *result;

  /* Find the locale, but do not generate it since this would be a bug.  */
  result = add_to_readlist (locale, name, repertoire_name, 0, NULL);

  assert (result != NULL);

  if ((result->avail & (1 << locale)) == 0
      && locfile_read (result, charmap) != 0)
    error (4, errno, _("cannot open locale definition file `%s'"),
	   result->name);

  return result;
}


struct localedef_t *
load_locale (int locale, const char *name, const char *repertoire_name,
	     struct charmap_t *charmap, struct localedef_t *copy_locale)
{
  struct localedef_t *result;

  /* Generate the locale if it does not exist.  */
  result = add_to_readlist (locale, name, repertoire_name, 1, copy_locale);

  assert (result != NULL);

  if ((result->avail & (1 << locale)) == 0
      && locfile_read (result, charmap) != 0)
    error (4, errno, _("cannot open locale definition file `%s'"),
	   result->name);

  return result;
}

static void
turn_on_mcheck (void)
{
  /* Enable `malloc' debugging.  */
  mcheck (NULL);
  /* Use the following line for a more thorough but much slower testing.  */
  /* mcheck_pedantic (NULL); */
}

void (*__malloc_initialize_hook) (void) = turn_on_mcheck;
