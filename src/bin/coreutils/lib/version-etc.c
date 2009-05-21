/* Print --version and bug-reporting information in a consistent format.
   Copyright (C) 1999-2009 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Written by Jim Meyering. */

#include <config.h>

/* Specification.  */
#include "version-etc.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#if USE_UNLOCKED_IO
# include "unlocked-io.h"
#endif

#include "gettext.h"
#define _(msgid) gettext (msgid)

enum { COPYRIGHT_YEAR = 2009 };

/* Like version_etc, below, but with the NULL-terminated author list
   provided via a variable of type va_list.  */
void
version_etc_va (FILE *stream,
		const char *command_name, const char *package,
		const char *version, va_list authors)
{
  size_t n_authors;

  /* Count the number of authors.  */
  {
    va_list tmp_authors;

    va_copy (tmp_authors, authors);

    n_authors = 0;
    while (va_arg (tmp_authors, const char *) != NULL)
      ++n_authors;
  }

  if (command_name)
    fprintf (stream, "%s (%s) %s\n", command_name, package, version);
  else
    fprintf (stream, "%s %s\n", package, version);

  /* TRANSLATORS: Translate "(C)" to the copyright symbol
     (C-in-a-circle), if this symbol is available in the user's
     locale.  Otherwise, do not translate "(C)"; leave it as-is.  */
  fprintf (stream, version_etc_copyright, _("(C)"), COPYRIGHT_YEAR);

  fputs (_("\
\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n\
\n\
"),
	 stream);

  switch (n_authors)
    {
    case 0:
      /* The caller must provide at least one author name.  */
      abort ();
    case 1:
      /* TRANSLATORS: %s denotes an author name.  */
      vfprintf (stream, _("Written by %s.\n"), authors);
      break;
    case 2:
      /* TRANSLATORS: Each %s denotes an author name.  */
      vfprintf (stream, _("Written by %s and %s.\n"), authors);
      break;
    case 3:
      /* TRANSLATORS: Each %s denotes an author name.  */
      vfprintf (stream, _("Written by %s, %s, and %s.\n"), authors);
      break;
    case 4:
      /* TRANSLATORS: Each %s denotes an author name.
	 You can use line breaks, estimating that each author name occupies
	 ca. 16 screen columns and that a screen line has ca. 80 columns.  */
      vfprintf (stream, _("Written by %s, %s, %s,\nand %s.\n"), authors);
      break;
    case 5:
      /* TRANSLATORS: Each %s denotes an author name.
	 You can use line breaks, estimating that each author name occupies
	 ca. 16 screen columns and that a screen line has ca. 80 columns.  */
      vfprintf (stream, _("Written by %s, %s, %s,\n%s, and %s.\n"), authors);
      break;
    case 6:
      /* TRANSLATORS: Each %s denotes an author name.
	 You can use line breaks, estimating that each author name occupies
	 ca. 16 screen columns and that a screen line has ca. 80 columns.  */
      vfprintf (stream, _("Written by %s, %s, %s,\n%s, %s, and %s.\n"),
		authors);
      break;
    case 7:
      /* TRANSLATORS: Each %s denotes an author name.
	 You can use line breaks, estimating that each author name occupies
	 ca. 16 screen columns and that a screen line has ca. 80 columns.  */
      vfprintf (stream, _("Written by %s, %s, %s,\n%s, %s, %s, and %s.\n"),
		authors);
      break;
    case 8:
      /* TRANSLATORS: Each %s denotes an author name.
	 You can use line breaks, estimating that each author name occupies
	 ca. 16 screen columns and that a screen line has ca. 80 columns.  */
      vfprintf (stream, _("\
Written by %s, %s, %s,\n%s, %s, %s, %s,\nand %s.\n"),
		authors);
      break;
    case 9:
      /* TRANSLATORS: Each %s denotes an author name.
	 You can use line breaks, estimating that each author name occupies
	 ca. 16 screen columns and that a screen line has ca. 80 columns.  */
      vfprintf (stream, _("\
Written by %s, %s, %s,\n%s, %s, %s, %s,\n%s, and %s.\n"),
		authors);
      break;
    default:
      /* 10 or more authors.  Use an abbreviation, since the human reader
	 will probably not want to read the entire list anyway.  */
      /* TRANSLATORS: Each %s denotes an author name.
	 You can use line breaks, estimating that each author name occupies
	 ca. 16 screen columns and that a screen line has ca. 80 columns.  */
      vfprintf (stream, _("\
Written by %s, %s, %s,\n%s, %s, %s, %s,\n%s, %s, and others.\n"),
		authors);
      break;
    }
  va_end (authors);
}


/* Display the --version information the standard way.

   If COMMAND_NAME is NULL, the PACKAGE is asumed to be the name of
   the program.  The formats are therefore:

   PACKAGE VERSION

   or

   COMMAND_NAME (PACKAGE) VERSION.

   The author names are passed as separate arguments, with an additional
   NULL argument at the end.  */
void
version_etc (FILE *stream,
	     const char *command_name, const char *package,
	     const char *version, /* const char *author1, ...*/ ...)
{
  va_list authors;

  va_start (authors, version);
  version_etc_va (stream, command_name, package, version, authors);
}

void
emit_bug_reporting_address (void)
{
  /* TRANSLATORS: The placeholder indicates the bug-reporting address
     for this package.  Please add _another line_ saying
     "Report translation bugs to <...>\n" with the address for translation
     bugs (typically your translation team's web or email address).  */
  printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
  printf (_("%s home page: <http://www.gnu.org/software/%s/>.\n"),
	  PACKAGE_NAME, PACKAGE);
  fputs (_("General help using GNU software: <http://www.gnu.org/gethelp/>.\n"),
         stdout);
}
