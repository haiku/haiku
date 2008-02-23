/* A tar (tape archiver) program.

   Copyright (C) 1988, 1992, 1993, 1994, 1995, 1996, 1997, 1999, 2000,
   2001, 2003, 2004, 2005, 2006, 2007 Free Software Foundation, Inc.

   Written by John Gilmore, starting 1985-08-25.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 3, or (at your option) any later
   version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
   Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#include <system.h>

#include <fnmatch.h>
#include <argp.h>
#include <argp-namefrob.h>
#include <argp-fmtstream.h>

#include <signal.h>
#if ! defined SIGCHLD && defined SIGCLD
# define SIGCHLD SIGCLD
#endif

/* The following causes "common.h" to produce definitions of all the global
   variables, rather than just "extern" declarations of them.  GNU tar does
   depend on the system loader to preset all GLOBAL variables to neutral (or
   zero) values; explicit initialization is usually not done.  */
#define GLOBAL
#include "common.h"

#include <argmatch.h>
#include <closeout.h>
#include <configmake.h>
#include <exitfail.h>
#include <getdate.h>
#include <rmt.h>
#include <rmt-command.h>
#include <prepargs.h>
#include <quotearg.h>
#include <version-etc.h>
#include <xstrtol.h>
#include <stdopen.h>

/* Local declarations.  */

#ifndef DEFAULT_ARCHIVE_FORMAT
# define DEFAULT_ARCHIVE_FORMAT GNU_FORMAT
#endif

#ifndef DEFAULT_ARCHIVE
# define DEFAULT_ARCHIVE "tar.out"
#endif

#ifndef DEFAULT_BLOCKING
# define DEFAULT_BLOCKING 20
#endif


/* Miscellaneous.  */

/* Name of option using stdin.  */
static const char *stdin_used_by;

/* Doesn't return if stdin already requested.  */
void
request_stdin (const char *option)
{
  if (stdin_used_by)
    USAGE_ERROR ((0, 0, _("Options `-%s' and `-%s' both want standard input"),
		  stdin_used_by, option));

  stdin_used_by = option;
}

extern int rpmatch (char const *response);

/* Returns true if and only if the user typed an affirmative response.  */
int
confirm (const char *message_action, const char *message_name)
{
  static FILE *confirm_file;
  static int confirm_file_EOF;
  bool status = false;

  if (!confirm_file)
    {
      if (archive == 0 || stdin_used_by)
	{
	  confirm_file = fopen (TTY_NAME, "r");
	  if (! confirm_file)
	    open_fatal (TTY_NAME);
	}
      else
	{
	  request_stdin ("-w");
	  confirm_file = stdin;
	}
    }

  fprintf (stdlis, "%s %s?", message_action, quote (message_name));
  fflush (stdlis);

  if (!confirm_file_EOF)
    {
      char *response = NULL;
      size_t response_size = 0;
      if (getline (&response, &response_size, confirm_file) < 0)
	confirm_file_EOF = 1;
      else
	status = rpmatch (response) > 0;
      free (response);
    }

  if (confirm_file_EOF)
    {
      fputc ('\n', stdlis);
      fflush (stdlis);
    }

  return status;
}

static struct fmttab {
  char const *name;
  enum archive_format fmt;
} const fmttab[] = {
  { "v7",      V7_FORMAT },
  { "oldgnu",  OLDGNU_FORMAT },
  { "ustar",   USTAR_FORMAT },
  { "posix",   POSIX_FORMAT },
#if 0 /* not fully supported yet */
  { "star",    STAR_FORMAT },
#endif
  { "gnu",     GNU_FORMAT },
  { "pax",     POSIX_FORMAT }, /* An alias for posix */
  { NULL,      0 }
};

static void
set_archive_format (char const *name)
{
  struct fmttab const *p;

  for (p = fmttab; strcmp (p->name, name) != 0; )
    if (! (++p)->name)
      USAGE_ERROR ((0, 0, _("%s: Invalid archive format"),
		    quotearg_colon (name)));

  archive_format = p->fmt;
}

const char *
archive_format_string (enum archive_format fmt)
{
  struct fmttab const *p;

  for (p = fmttab; p->name; p++)
    if (p->fmt == fmt)
      return p->name;
  return "unknown?";
}

#define FORMAT_MASK(n) (1<<(n))

static void
assert_format(unsigned fmt_mask)
{
  if ((FORMAT_MASK (archive_format) & fmt_mask) == 0)
    USAGE_ERROR ((0, 0,
		  _("GNU features wanted on incompatible archive format")));
}

const char *
subcommand_string (enum subcommand c)
{
  switch (c)
    {
    case UNKNOWN_SUBCOMMAND:
      return "unknown?";

    case APPEND_SUBCOMMAND:
      return "-r";

    case CAT_SUBCOMMAND:
      return "-A";

    case CREATE_SUBCOMMAND:
      return "-c";

    case DELETE_SUBCOMMAND:
      return "-D";

    case DIFF_SUBCOMMAND:
      return "-d";

    case EXTRACT_SUBCOMMAND:
      return "-x";

    case LIST_SUBCOMMAND:
      return "-t";

    case UPDATE_SUBCOMMAND:
      return "-u";

    default:
      abort ();
    }
}

void
tar_list_quoting_styles (argp_fmtstream_t fs, char *prefix)
{
  int i;

  for (i = 0; quoting_style_args[i]; i++)
    argp_fmtstream_printf (fs, "%s%s\n", prefix, quoting_style_args[i]);
}

void
tar_set_quoting_style (char *arg)
{
  int i;

  for (i = 0; quoting_style_args[i]; i++)
    if (strcmp (arg, quoting_style_args[i]) == 0)
      {
	set_quoting_style (NULL, i);
	return;
      }
  FATAL_ERROR ((0, 0,
		_("Unknown quoting style `%s'. Try `%s --quoting-style=help' to get a list."), arg, program_invocation_short_name));
}


/* Options.  */

enum
{
  ANCHORED_OPTION = CHAR_MAX + 1,
  ATIME_PRESERVE_OPTION,
  BACKUP_OPTION,
  CHECKPOINT_OPTION,
  DELAY_DIRECTORY_RESTORE_OPTION,
  DELETE_OPTION,
  EXCLUDE_CACHES_OPTION,
  EXCLUDE_CACHES_UNDER_OPTION,
  EXCLUDE_CACHES_ALL_OPTION,
  EXCLUDE_OPTION,
  EXCLUDE_TAG_OPTION,
  EXCLUDE_TAG_UNDER_OPTION,
  EXCLUDE_TAG_ALL_OPTION,
  EXCLUDE_VCS_OPTION,
  FORCE_LOCAL_OPTION,
  GROUP_OPTION,
  HANG_OPTION,
  IGNORE_CASE_OPTION,
  IGNORE_COMMAND_ERROR_OPTION,
  IGNORE_FAILED_READ_OPTION,
  INDEX_FILE_OPTION,
  KEEP_NEWER_FILES_OPTION,
  MODE_OPTION,
  MTIME_OPTION,
  NEWER_MTIME_OPTION,
  NO_ANCHORED_OPTION,
  NO_DELAY_DIRECTORY_RESTORE_OPTION,
  NO_IGNORE_CASE_OPTION,
  NO_IGNORE_COMMAND_ERROR_OPTION,
  NO_OVERWRITE_DIR_OPTION,
  NO_QUOTE_CHARS_OPTION,
  NO_RECURSION_OPTION,
  NO_SAME_OWNER_OPTION,
  NO_SAME_PERMISSIONS_OPTION,
  NO_UNQUOTE_OPTION,
  NO_WILDCARDS_MATCH_SLASH_OPTION,
  NO_WILDCARDS_OPTION,
  NULL_OPTION,
  NUMERIC_OWNER_OPTION,
  OCCURRENCE_OPTION,
  OLD_ARCHIVE_OPTION,
  ONE_FILE_SYSTEM_OPTION,
  OVERWRITE_DIR_OPTION,
  OVERWRITE_OPTION,
  OWNER_OPTION,
  PAX_OPTION,
  POSIX_OPTION,
  PRESERVE_OPTION,
  QUOTE_CHARS_OPTION,
  QUOTING_STYLE_OPTION,
  RECORD_SIZE_OPTION,
  RECURSION_OPTION,
  RECURSIVE_UNLINK_OPTION,
  REMOVE_FILES_OPTION,
  RESTRICT_OPTION,
  RMT_COMMAND_OPTION,
  RSH_COMMAND_OPTION,
  SAME_OWNER_OPTION,
  SHOW_DEFAULTS_OPTION,
  SHOW_OMITTED_DIRS_OPTION,
  SHOW_TRANSFORMED_NAMES_OPTION,
  SPARSE_VERSION_OPTION,
  STRIP_COMPONENTS_OPTION,
  SUFFIX_OPTION,
  TEST_LABEL_OPTION,
  TOTALS_OPTION,
  TO_COMMAND_OPTION,
  TRANSFORM_OPTION,
  UNQUOTE_OPTION,
  USAGE_OPTION,
  USE_COMPRESS_PROGRAM_OPTION,
  UTC_OPTION,
  VERSION_OPTION,
  VOLNO_FILE_OPTION,
  WILDCARDS_MATCH_SLASH_OPTION,
  WILDCARDS_OPTION
};

const char *argp_program_version = "tar (" PACKAGE_NAME ") " VERSION;
const char *argp_program_bug_address = "<" PACKAGE_BUGREPORT ">";
static char const doc[] = N_("\
GNU `tar' saves many files together into a single tape or disk archive, \
and can restore individual files from the archive.\n\
\n\
Examples:\n\
  tar -cf archive.tar foo bar  # Create archive.tar from files foo and bar.\n\
  tar -tvf archive.tar         # List all files in archive.tar verbosely.\n\
  tar -xf archive.tar          # Extract all files from archive.tar.\n")
"\v"
N_("The backup suffix is `~', unless set with --suffix or SIMPLE_BACKUP_SUFFIX.\n\
The version control may be set with --backup or VERSION_CONTROL, values are:\n\n\
  none, off       never make backups\n\
  t, numbered     make numbered backups\n\
  nil, existing   numbered if numbered backups exist, simple otherwise\n\
  never, simple   always make simple backups\n");


/* NOTE:

   Available option letters are DEIJQY and aeqy. Consider the following
   assignments:

   [For Solaris tar compatibility =/= Is it important at all?]
   e  exit immediately with a nonzero exit status if unexpected errors occur
   E  use extended headers (--format=posix)

   [q  alias for --occurrence=1 =/= this would better be used for quiet?]
   [I  same as T =/= will harm star compatibility]

   y  per-file gzip compression
   Y  per-block gzip compression */

static struct argp_option options[] = {
#define GRID 10
  {NULL, 0, NULL, 0,
   N_("Main operation mode:"), GRID },

  {"list", 't', 0, 0,
   N_("list the contents of an archive"), GRID+1 },
  {"extract", 'x', 0, 0,
   N_("extract files from an archive"), GRID+1 },
  {"get", 0, 0, OPTION_ALIAS, NULL, GRID+1 },
  {"create", 'c', 0, 0,
   N_("create a new archive"), GRID+1 },
  {"diff", 'd', 0, 0,
   N_("find differences between archive and file system"), GRID+1 },
  {"compare", 0, 0, OPTION_ALIAS, NULL, GRID+1 },
  {"append", 'r', 0, 0,
   N_("append files to the end of an archive"), GRID+1 },
  {"update", 'u', 0, 0,
   N_("only append files newer than copy in archive"), GRID+1 },
  {"catenate", 'A', 0, 0,
   N_("append tar files to an archive"), GRID+1 },
  {"concatenate", 0, 0, OPTION_ALIAS, NULL, GRID+1 },
  {"delete", DELETE_OPTION, 0, 0,
   N_("delete from the archive (not on mag tapes!)"), GRID+1 },
  {"test-label", TEST_LABEL_OPTION, NULL, 0,
   N_("test the archive volume label and exit"), GRID+1 },
#undef GRID

#define GRID 20
  {NULL, 0, NULL, 0,
   N_("Operation modifiers:"), GRID },

  {"sparse", 'S', 0, 0,
   N_("handle sparse files efficiently"), GRID+1 },
  {"sparse-version", SPARSE_VERSION_OPTION, N_("MAJOR[.MINOR]"), 0,
   N_("set version of the sparse format to use (implies --sparse)"), GRID+1},
  {"incremental", 'G', 0, 0,
   N_("handle old GNU-format incremental backup"), GRID+1 },
  {"listed-incremental", 'g', N_("FILE"), 0,
   N_("handle new GNU-format incremental backup"), GRID+1 },
  {"ignore-failed-read", IGNORE_FAILED_READ_OPTION, 0, 0,
   N_("do not exit with nonzero on unreadable files"), GRID+1 },
  {"occurrence", OCCURRENCE_OPTION, N_("NUMBER"), OPTION_ARG_OPTIONAL,
   N_("process only the NUMBERth occurrence of each file in the archive;"
      " this option is valid only in conjunction with one of the subcommands"
      " --delete, --diff, --extract or --list and when a list of files"
      " is given either on the command line or via the -T option;"
      " NUMBER defaults to 1"), GRID+1 },
  {"seek", 'n', NULL, 0,
   N_("archive is seekable"), GRID+1 },
#undef GRID

#define GRID 30
  {NULL, 0, NULL, 0,
   N_("Overwrite control:"), GRID },

  {"verify", 'W', 0, 0,
   N_("attempt to verify the archive after writing it"), GRID+1 },
  {"remove-files", REMOVE_FILES_OPTION, 0, 0,
   N_("remove files after adding them to the archive"), GRID+1 },
  {"keep-old-files", 'k', 0, 0,
   N_("don't replace existing files when extracting"), GRID+1 },
  {"keep-newer-files", KEEP_NEWER_FILES_OPTION, 0, 0,
   N_("don't replace existing files that are newer than their archive copies"), GRID+1 },
  {"overwrite", OVERWRITE_OPTION, 0, 0,
   N_("overwrite existing files when extracting"), GRID+1 },
  {"unlink-first", 'U', 0, 0,
   N_("remove each file prior to extracting over it"), GRID+1 },
  {"recursive-unlink", RECURSIVE_UNLINK_OPTION, 0, 0,
   N_("empty hierarchies prior to extracting directory"), GRID+1 },
  {"no-overwrite-dir", NO_OVERWRITE_DIR_OPTION, 0, 0,
   N_("preserve metadata of existing directories"), GRID+1 },
  {"overwrite-dir", OVERWRITE_DIR_OPTION, 0, 0,
   N_("overwrite metadata of existing directories when extracting (default)"),
   GRID+1 },
#undef GRID

#define GRID 40
  {NULL, 0, NULL, 0,
   N_("Select output stream:"), GRID },

  {"to-stdout", 'O', 0, 0,
   N_("extract files to standard output"), GRID+1 },
  {"to-command", TO_COMMAND_OPTION, N_("COMMAND"), 0,
   N_("pipe extracted files to another program"), GRID+1 },
  {"ignore-command-error", IGNORE_COMMAND_ERROR_OPTION, 0, 0,
   N_("ignore exit codes of children"), GRID+1 },
  {"no-ignore-command-error", NO_IGNORE_COMMAND_ERROR_OPTION, 0, 0,
   N_("treat non-zero exit codes of children as error"), GRID+1 },
#undef GRID

#define GRID 50
  {NULL, 0, NULL, 0,
   N_("Handling of file attributes:"), GRID },

  {"owner", OWNER_OPTION, N_("NAME"), 0,
   N_("force NAME as owner for added files"), GRID+1 },
  {"group", GROUP_OPTION, N_("NAME"), 0,
   N_("force NAME as group for added files"), GRID+1 },
  {"mtime", MTIME_OPTION, N_("DATE-OR-FILE"), 0,
   N_("set mtime for added files from DATE-OR-FILE"), GRID+1 },
  {"mode", MODE_OPTION, N_("CHANGES"), 0,
   N_("force (symbolic) mode CHANGES for added files"), GRID+1 },
  {"atime-preserve", ATIME_PRESERVE_OPTION,
   N_("METHOD"), OPTION_ARG_OPTIONAL,
   N_("preserve access times on dumped files, either by restoring the times"
      " after reading (METHOD='replace'; default) or by not setting the times"
      " in the first place (METHOD='system')"), GRID+1 },
  {"touch", 'm', 0, 0,
   N_("don't extract file modified time"), GRID+1 },
  {"same-owner", SAME_OWNER_OPTION, 0, 0,
   N_("try extracting files with the same ownership"), GRID+1 },
  {"no-same-owner", NO_SAME_OWNER_OPTION, 0, 0,
   N_("extract files as yourself"), GRID+1 },
  {"numeric-owner", NUMERIC_OWNER_OPTION, 0, 0,
   N_("always use numbers for user/group names"), GRID+1 },
  {"preserve-permissions", 'p', 0, 0,
   N_("extract information about file permissions (default for superuser)"),
   GRID+1 },
  {"same-permissions", 0, 0, OPTION_ALIAS, NULL, GRID+1 },
  {"no-same-permissions", NO_SAME_PERMISSIONS_OPTION, 0, 0,
   N_("apply the user's umask when extracting permissions from the archive (default for ordinary users)"), GRID+1 },
  {"preserve-order", 's', 0, 0,
   N_("sort names to extract to match archive"), GRID+1 },
  {"same-order", 0, 0, OPTION_ALIAS, NULL, GRID+1 },
  {"preserve", PRESERVE_OPTION, 0, 0,
   N_("same as both -p and -s"), GRID+1 },
  {"delay-directory-restore", DELAY_DIRECTORY_RESTORE_OPTION, 0, 0,
   N_("delay setting modification times and permissions of extracted"
      " directories until the end of extraction"), GRID+1 },
  {"no-delay-directory-restore", NO_DELAY_DIRECTORY_RESTORE_OPTION, 0, 0,
   N_("cancel the effect of --delay-directory-restore option"), GRID+1 },
#undef GRID

#define GRID 60
  {NULL, 0, NULL, 0,
   N_("Device selection and switching:"), GRID },

  {"file", 'f', N_("ARCHIVE"), 0,
   N_("use archive file or device ARCHIVE"), GRID+1 },
  {"force-local", FORCE_LOCAL_OPTION, 0, 0,
   N_("archive file is local even if it has a colon"), GRID+1 },
  {"rmt-command", RMT_COMMAND_OPTION, N_("COMMAND"), 0,
   N_("use given rmt COMMAND instead of rmt"), GRID+1 },
  {"rsh-command", RSH_COMMAND_OPTION, N_("COMMAND"), 0,
   N_("use remote COMMAND instead of rsh"), GRID+1 },
#ifdef DEVICE_PREFIX
  {"-[0-7][lmh]", 0, NULL, OPTION_DOC, /* It is OK, since `name' will never be
					  translated */
   N_("specify drive and density"), GRID+1 },
#endif
  {NULL, '0', NULL, OPTION_HIDDEN, NULL, GRID+1 },
  {NULL, '1', NULL, OPTION_HIDDEN, NULL, GRID+1 },
  {NULL, '2', NULL, OPTION_HIDDEN, NULL, GRID+1 },
  {NULL, '3', NULL, OPTION_HIDDEN, NULL, GRID+1 },
  {NULL, '4', NULL, OPTION_HIDDEN, NULL, GRID+1 },
  {NULL, '5', NULL, OPTION_HIDDEN, NULL, GRID+1 },
  {NULL, '6', NULL, OPTION_HIDDEN, NULL, GRID+1 },
  {NULL, '7', NULL, OPTION_HIDDEN, NULL, GRID+1 },
  {NULL, '8', NULL, OPTION_HIDDEN, NULL, GRID+1 },
  {NULL, '9', NULL, OPTION_HIDDEN, NULL, GRID+1 },

  {"multi-volume", 'M', 0, 0,
   N_("create/list/extract multi-volume archive"), GRID+1 },
  {"tape-length", 'L', N_("NUMBER"), 0,
   N_("change tape after writing NUMBER x 1024 bytes"), GRID+1 },
  {"info-script", 'F', N_("NAME"), 0,
   N_("run script at end of each tape (implies -M)"), GRID+1 },
  {"new-volume-script", 0, 0, OPTION_ALIAS, NULL, GRID+1 },
  {"volno-file", VOLNO_FILE_OPTION, N_("FILE"), 0,
   N_("use/update the volume number in FILE"), GRID+1 },
#undef GRID

#define GRID 70
  {NULL, 0, NULL, 0,
   N_("Device blocking:"), GRID },

  {"blocking-factor", 'b', N_("BLOCKS"), 0,
   N_("BLOCKS x 512 bytes per record"), GRID+1 },
  {"record-size", RECORD_SIZE_OPTION, N_("NUMBER"), 0,
   N_("NUMBER of bytes per record, multiple of 512"), GRID+1 },
  {"ignore-zeros", 'i', 0, 0,
   N_("ignore zeroed blocks in archive (means EOF)"), GRID+1 },
  {"read-full-records", 'B', 0, 0,
   N_("reblock as we read (for 4.2BSD pipes)"), GRID+1 },
#undef GRID

#define GRID 80
  {NULL, 0, NULL, 0,
   N_("Archive format selection:"), GRID },

  {"format", 'H', N_("FORMAT"), 0,
   N_("create archive of the given format"), GRID+1 },

  {NULL, 0, NULL, 0, N_("FORMAT is one of the following:"), GRID+2 },
  {"  v7", 0, NULL, OPTION_DOC|OPTION_NO_TRANS, N_("old V7 tar format"),
   GRID+3 },
  {"  oldgnu", 0, NULL, OPTION_DOC|OPTION_NO_TRANS,
   N_("GNU format as per tar <= 1.12"), GRID+3 },
  {"  gnu", 0, NULL, OPTION_DOC|OPTION_NO_TRANS,
   N_("GNU tar 1.13.x format"), GRID+3 },
  {"  ustar", 0, NULL, OPTION_DOC|OPTION_NO_TRANS,
   N_("POSIX 1003.1-1988 (ustar) format"), GRID+3 },
  {"  pax", 0, NULL, OPTION_DOC|OPTION_NO_TRANS,
   N_("POSIX 1003.1-2001 (pax) format"), GRID+3 },
  {"  posix", 0, NULL, OPTION_DOC|OPTION_NO_TRANS, N_("same as pax"), GRID+3 },

  {"old-archive", OLD_ARCHIVE_OPTION, 0, 0, /* FIXME */
   N_("same as --format=v7"), GRID+8 },
  {"portability", 0, 0, OPTION_ALIAS, NULL, GRID+8 },
  {"posix", POSIX_OPTION, 0, 0,
   N_("same as --format=posix"), GRID+8 },
  {"pax-option", PAX_OPTION, N_("keyword[[:]=value][,keyword[[:]=value]]..."), 0,
   N_("control pax keywords"), GRID+8 },
  {"label", 'V', N_("TEXT"), 0,
   N_("create archive with volume name TEXT; at list/extract time, use TEXT as a globbing pattern for volume name"), GRID+8 },
  {"bzip2", 'j', 0, 0,
   N_("filter the archive through bzip2"), GRID+8 },
  {"gzip", 'z', 0, 0,
   N_("filter the archive through gzip"), GRID+8 },
  {"gunzip", 0, 0, OPTION_ALIAS, NULL, GRID+8 },
  {"ungzip", 0, 0, OPTION_ALIAS, NULL, GRID+8 },
  {"compress", 'Z', 0, 0,
   N_("filter the archive through compress"), GRID+8 },
  {"uncompress", 0, 0, OPTION_ALIAS, NULL, GRID+8 },
  {"use-compress-program", USE_COMPRESS_PROGRAM_OPTION, N_("PROG"), 0,
   N_("filter through PROG (must accept -d)"), GRID+8 },
#undef GRID

#define GRID 90
  {NULL, 0, NULL, 0,
   N_("Local file selection:"), GRID },

  {"add-file", ARGP_KEY_ARG, N_("FILE"), 0,
   N_("add given FILE to the archive (useful if its name starts with a dash)"), GRID+1 },
  {"directory", 'C', N_("DIR"), 0,
   N_("change to directory DIR"), GRID+1 },
  {"files-from", 'T', N_("FILE"), 0,
   N_("get names to extract or create from FILE"), GRID+1 },
  {"null", NULL_OPTION, 0, 0,
   N_("-T reads null-terminated names, disable -C"), GRID+1 },
  {"unquote", UNQUOTE_OPTION, 0, 0,
   N_("unquote filenames read with -T (default)"), GRID+1 },
  {"no-unquote", NO_UNQUOTE_OPTION, 0, 0,
   N_("do not unquote filenames read with -T"), GRID+1 },
  {"exclude", EXCLUDE_OPTION, N_("PATTERN"), 0,
   N_("exclude files, given as a PATTERN"), GRID+1 },
  {"exclude-from", 'X', N_("FILE"), 0,
   N_("exclude patterns listed in FILE"), GRID+1 },
  {"exclude-caches", EXCLUDE_CACHES_OPTION, 0, 0,
   N_("exclude contents of directories containing CACHEDIR.TAG, "
      "except for the tag file itself"), GRID+1 },
  {"exclude-caches-under", EXCLUDE_CACHES_UNDER_OPTION, 0, 0,
   N_("exclude everything under directories containing CACHEDIR.TAG"),
   GRID+1 },
  {"exclude-caches-all", EXCLUDE_CACHES_ALL_OPTION, 0, 0,
   N_("exclude directories containing CACHEDIR.TAG"), GRID+1 },
  {"exclude-tag", EXCLUDE_TAG_OPTION, N_("FILE"), 0,
   N_("exclude contents of directories containing FILE, except"
      " for FILE itself"), GRID+1 },
  {"exclude-tag-under", EXCLUDE_TAG_UNDER_OPTION, N_("FILE"), 0,
   N_("exclude everything under directories containing FILE"), GRID+1 },
  {"exclude-tag-all", EXCLUDE_TAG_ALL_OPTION, N_("FILE"), 0,
   N_("exclude directories containing FILE"), GRID+1 },
  {"exclude-vcs", EXCLUDE_VCS_OPTION, NULL, 0,
   N_("exclude version control system directories"), GRID+1 },
  {"no-recursion", NO_RECURSION_OPTION, 0, 0,
   N_("avoid descending automatically in directories"), GRID+1 },
  {"one-file-system", ONE_FILE_SYSTEM_OPTION, 0, 0,
   N_("stay in local file system when creating archive"), GRID+1 },
  {"recursion", RECURSION_OPTION, 0, 0,
   N_("recurse into directories (default)"), GRID+1 },
  {"absolute-names", 'P', 0, 0,
   N_("don't strip leading `/'s from file names"), GRID+1 },
  {"dereference", 'h', 0, 0,
   N_("follow symlinks; archive and dump the files they point to"), GRID+1 },
  {"starting-file", 'K', N_("MEMBER-NAME"), 0,
   N_("begin at member MEMBER-NAME in the archive"), GRID+1 },
  {"newer", 'N', N_("DATE-OR-FILE"), 0,
   N_("only store files newer than DATE-OR-FILE"), GRID+1 },
  {"after-date", 0, 0, OPTION_ALIAS, NULL, GRID+1 },
  {"newer-mtime", NEWER_MTIME_OPTION, N_("DATE"), 0,
   N_("compare date and time when data changed only"), GRID+1 },
  {"backup", BACKUP_OPTION, N_("CONTROL"), OPTION_ARG_OPTIONAL,
   N_("backup before removal, choose version CONTROL"), GRID+1 },
  {"suffix", SUFFIX_OPTION, N_("STRING"), 0,
   N_("backup before removal, override usual suffix ('~' unless overridden by environment variable SIMPLE_BACKUP_SUFFIX)"), GRID+1 },
#undef GRID

#define GRID 92
  {NULL, 0, NULL, 0,
   N_("File name transformations:"), GRID },
  {"strip-components", STRIP_COMPONENTS_OPTION, N_("NUMBER"), 0,
   N_("strip NUMBER leading components from file names on extraction"),
   GRID+1 },
  {"transform", TRANSFORM_OPTION, N_("EXPRESSION"), 0,
   N_("use sed replace EXPRESSION to transform file names"), GRID+1 },
#undef GRID

#define GRID 95
  {NULL, 0, NULL, 0,
   N_("File name matching options (affect both exclude and include patterns):"),
   GRID },
  {"ignore-case", IGNORE_CASE_OPTION, 0, 0,
   N_("ignore case"), GRID+1 },
  {"anchored", ANCHORED_OPTION, 0, 0,
   N_("patterns match file name start"), GRID+1 },
  {"no-anchored", NO_ANCHORED_OPTION, 0, 0,
   N_("patterns match after any `/' (default for exclusion)"), GRID+1 },
  {"no-ignore-case", NO_IGNORE_CASE_OPTION, 0, 0,
   N_("case sensitive matching (default)"), GRID+1 },
  {"wildcards", WILDCARDS_OPTION, 0, 0,
   N_("use wildcards (default for exclusion)"), GRID+1 },
  {"no-wildcards", NO_WILDCARDS_OPTION, 0, 0,
   N_("verbatim string matching"), GRID+1 },
  {"no-wildcards-match-slash", NO_WILDCARDS_MATCH_SLASH_OPTION, 0, 0,
   N_("wildcards do not match `/'"), GRID+1 },
  {"wildcards-match-slash", WILDCARDS_MATCH_SLASH_OPTION, 0, 0,
   N_("wildcards match `/' (default for exclusion)"), GRID+1 },
#undef GRID

#define GRID 100
  {NULL, 0, NULL, 0,
   N_("Informative output:"), GRID },

  {"verbose", 'v', 0, 0,
   N_("verbosely list files processed"), GRID+1 },
  {"checkpoint", CHECKPOINT_OPTION, N_("[.]NUMBER"), OPTION_ARG_OPTIONAL,
   N_("display progress messages every NUMBERth record (default 10)"),
   GRID+1 },
  {"check-links", 'l', 0, 0,
   N_("print a message if not all links are dumped"), GRID+1 },
  {"totals", TOTALS_OPTION, N_("SIGNAL"), OPTION_ARG_OPTIONAL,
   N_("print total bytes after processing the archive; "
      "with an argument - print total bytes when this SIGNAL is delivered; "
      "Allowed signals are: SIGHUP, SIGQUIT, SIGINT, SIGUSR1 and SIGUSR2; "
      "the names without SIG prefix are also accepted"), GRID+1 },
  {"utc", UTC_OPTION, 0, 0,
   N_("print file modification dates in UTC"), GRID+1 },
  {"index-file", INDEX_FILE_OPTION, N_("FILE"), 0,
   N_("send verbose output to FILE"), GRID+1 },
  {"block-number", 'R', 0, 0,
   N_("show block number within archive with each message"), GRID+1 },
  {"interactive", 'w', 0, 0,
   N_("ask for confirmation for every action"), GRID+1 },
  {"confirmation", 0, 0, OPTION_ALIAS, NULL, GRID+1 },
  {"show-defaults", SHOW_DEFAULTS_OPTION, 0, 0,
   N_("show tar defaults"), GRID+1 },
  {"show-omitted-dirs", SHOW_OMITTED_DIRS_OPTION, 0, 0,
   N_("when listing or extracting, list each directory that does not match search criteria"), GRID+1 },
  {"show-transformed-names", SHOW_TRANSFORMED_NAMES_OPTION, 0, 0,
   N_("show file or archive names after transformation"),
   GRID+1 },
  {"show-stored-names", 0, 0, OPTION_ALIAS, NULL, GRID+1 },
  {"quoting-style", QUOTING_STYLE_OPTION, N_("STYLE"), 0,
   N_("set name quoting style; see below for valid STYLE values"), GRID+1 },
  {"quote-chars", QUOTE_CHARS_OPTION, N_("STRING"), 0,
   N_("additionally quote characters from STRING"), GRID+1 },
  {"no-quote-chars", NO_QUOTE_CHARS_OPTION, N_("STRING"), 0,
   N_("disable quoting for characters from STRING"), GRID+1 },
#undef GRID

#define GRID 110
  {NULL, 0, NULL, 0,
   N_("Compatibility options:"), GRID },

  {NULL, 'o', 0, 0,
   N_("when creating, same as --old-archive; when extracting, same as --no-same-owner"), GRID+1 },
#undef GRID

#define GRID 120
  {NULL, 0, NULL, 0,
   N_("Other options:"), GRID },

  {"restrict", RESTRICT_OPTION, 0, 0,
   N_("disable use of some potentially harmful options"), -1 },

  {"help",  '?', 0, 0,  N_("give this help list"), -1},
  {"usage", USAGE_OPTION, 0, 0,  N_("give a short usage message"), -1},
  {"version", VERSION_OPTION, 0, 0,  N_("print program version"), -1},
  /* FIXME -V (--label) conflicts with the default short option for
     --version */
  {"HANG",	  HANG_OPTION,    "SECS", OPTION_ARG_OPTIONAL | OPTION_HIDDEN,
   N_("hang for SECS seconds (default 3600)"), 0},
#undef GRID

  {0, 0, 0, 0, 0, 0}
};

static char const *const atime_preserve_args[] =
{
  "replace", "system", NULL
};

static enum atime_preserve const atime_preserve_types[] =
{
  replace_atime_preserve, system_atime_preserve
};

/* Make sure atime_preserve_types has as much entries as atime_preserve_args
   (minus 1 for NULL guard) */
ARGMATCH_VERIFY (atime_preserve_args, atime_preserve_types);

/* Wildcard matching settings */
enum wildcards
  {
    default_wildcards, /* For exclusion == enable_wildcards,
			  for inclusion == disable_wildcards */
    disable_wildcards,
    enable_wildcards
  };

struct tar_args        /* Variables used during option parsing */
{
  struct textual_date *textual_date; /* Keeps the arguments to --newer-mtime
					and/or --date option if they are
					textual dates */
  enum wildcards wildcards;        /* Wildcard settings (--wildcards/
				      --no-wildcards) */
  int matching_flags;              /* exclude_fnmatch options */
  int include_anchored;            /* Pattern anchoring options used for
				      file inclusion */
  bool o_option;                   /* True if -o option was given */
  bool pax_option;                 /* True if --pax-option was given */
  char const *backup_suffix_string;   /* --suffix option argument */
  char const *version_control_string; /* --backup option argument */
  bool input_files;                /* True if some input files where given */
};


#define MAKE_EXCL_OPTIONS(args) \
 ((((args)->wildcards != disable_wildcards) ? EXCLUDE_WILDCARDS : 0) \
  | (args)->matching_flags \
  | recursion_option)

#define MAKE_INCL_OPTIONS(args) \
 ((((args)->wildcards == enable_wildcards) ? EXCLUDE_WILDCARDS : 0) \
  | (args)->include_anchored \
  | (args)->matching_flags \
  | recursion_option)

void
exclude_vcs_files ()
{
  int i;
  static char *vcs_file[] = {
    /* CVS: */
    "CVS",
    ".cvsignore",
    /* RCS: */
    "RCS",
    /* SCCS: */
    "SCCS",
    /* SVN: */
    ".svn",
    /* git: */
    ".git",
    ".gitignore",
    /* Arch: */
    ".arch-ids",
    "{arch}",
    "=RELEASE-ID",
    "=meta-update",
    "=update",
    NULL
  };

  for (i = 0; vcs_file[i]; i++)
    add_exclude (excluded, vcs_file[i], 0);
}


#ifdef REMOTE_SHELL
# define DECL_SHOW_DEFAULT_SETTINGS(stream, printer)                      \
{                                                                         \
  printer (stream,                                                        \
	   "--format=%s -f%s -b%d --quoting-style=%s --rmt-command=%s",   \
	   archive_format_string (DEFAULT_ARCHIVE_FORMAT),                \
	   DEFAULT_ARCHIVE, DEFAULT_BLOCKING,                             \
	   quoting_style_args[DEFAULT_QUOTING_STYLE],                     \
	   DEFAULT_RMT_COMMAND);                                          \
  printer (stream, " --rsh-command=%s", REMOTE_SHELL);                    \
  printer (stream, "\n");                                                 \
}
#else
# define DECL_SHOW_DEFAULT_SETTINGS(stream, printer)                      \
{                                                                         \
  printer (stream,                                                        \
	   "--format=%s -f%s -b%d --quoting-style=%s --rmt-command=%s",   \
	   archive_format_string (DEFAULT_ARCHIVE_FORMAT),                \
	   DEFAULT_ARCHIVE, DEFAULT_BLOCKING,                             \
	   quoting_style_args[DEFAULT_QUOTING_STYLE],                     \
	   DEFAULT_RMT_COMMAND);                                          \
  printer (stream, "\n");                                                 \
}
#endif

static void
show_default_settings (FILE *fp)
     DECL_SHOW_DEFAULT_SETTINGS(fp, fprintf)

static void
show_default_settings_fs (argp_fmtstream_t fs)
     DECL_SHOW_DEFAULT_SETTINGS(fs, argp_fmtstream_printf)

static void
set_subcommand_option (enum subcommand subcommand)
{
  if (subcommand_option != UNKNOWN_SUBCOMMAND
      && subcommand_option != subcommand)
    USAGE_ERROR ((0, 0,
		  _("You may not specify more than one `-Acdtrux' option")));

  subcommand_option = subcommand;
}

static void
set_use_compress_program_option (const char *string)
{
  if (use_compress_program_option
      && strcmp (use_compress_program_option, string) != 0)
    USAGE_ERROR ((0, 0, _("Conflicting compression options")));

  use_compress_program_option = string;
}

static RETSIGTYPE
sigstat (int signo)
{
  compute_duration ();
  print_total_stats ();
#ifndef HAVE_SIGACTION
  signal (signo, sigstat);
#endif
}

static void
stat_on_signal (int signo)
{
#ifdef HAVE_SIGACTION
  struct sigaction act;
  act.sa_handler = sigstat;
  sigemptyset (&act.sa_mask);
  act.sa_flags = 0;
  sigaction (signo, &act, NULL);
#else
  signal (signo, sigstat);
#endif
}

void
set_stat_signal (const char *name)
{
  static struct sigtab
  {
    char *name;
    int signo;
  } sigtab[] = {
    { "SIGUSR1", SIGUSR1 },
    { "USR1", SIGUSR1 },
    { "SIGUSR2", SIGUSR2 },
    { "USR2", SIGUSR2 },
    { "SIGHUP", SIGHUP },
    { "HUP", SIGHUP },
    { "SIGINT", SIGINT },
    { "INT", SIGINT },
    { "SIGQUIT", SIGQUIT },
    { "QUIT", SIGQUIT }
  };
  struct sigtab *p;

  for (p = sigtab; p < sigtab + sizeof (sigtab) / sizeof (sigtab[0]); p++)
    if (strcmp (p->name, name) == 0)
      {
	stat_on_signal (p->signo);
	return;
      }
  FATAL_ERROR ((0, 0, _("Unknown signal name: %s"), name));
}


struct textual_date
{
  struct textual_date *next;
  struct timespec *ts;
  const char *option;
  const char *date;
};

static void
get_date_or_file (struct tar_args *args, const char *option,
		  const char *str, struct timespec *ts)
{
  if (FILE_SYSTEM_PREFIX_LEN (str) != 0
      || ISSLASH (*str)
      || *str == '.')
    {
      struct stat st;
      if (deref_stat (dereference_option, str, &st) != 0)
	{
	  stat_error (str);
	  USAGE_ERROR ((0, 0, _("Date sample file not found")));
	}
      *ts = get_stat_mtime (&st);
    }
  else
    {
      if (! get_date (ts, str, NULL))
	{
	  WARN ((0, 0, _("Substituting %s for unknown date format %s"),
		 tartime (*ts, false), quote (str)));
	  ts->tv_nsec = 0;
	}
      else
	{
	  struct textual_date *p = xmalloc (sizeof (*p));
	  p->ts = ts;
	  p->option = option;
	  p->date = str;
	  p->next = args->textual_date;
	  args->textual_date = p;
	}
    }
}

static void
report_textual_dates (struct tar_args *args)
{
  struct textual_date *p;
  for (p = args->textual_date; p; )
    {
      struct textual_date *next = p->next;
      char const *treated_as = tartime (*p->ts, true);
      if (strcmp (p->date, treated_as) != 0)
	WARN ((0, 0, _("Option %s: Treating date `%s' as %s"),
	       p->option, p->date, treated_as));
      free (p);
      p = next;
    }
}


static volatile int _argp_hang;

enum read_file_list_state  /* Result of reading file name from the list file */
  {
    file_list_success,     /* OK, name read successfully */
    file_list_end,         /* End of list file */
    file_list_zero,        /* Zero separator encountered where it should not */
    file_list_skip         /* Empty (zero-length) entry encountered, skip it */
  };

/* Read from FP a sequence of characters up to FILENAME_TERMINATOR and put them
   into STK.
 */
static enum read_file_list_state
read_name_from_file (FILE *fp, struct obstack *stk)
{
  int c;
  size_t counter = 0;

  for (c = getc (fp); c != EOF && c != filename_terminator; c = getc (fp))
    {
      if (c == 0)
	{
	  /* We have read a zero separator. The file possibly is
	     zero-separated */
	  return file_list_zero;
	}
      obstack_1grow (stk, c);
      counter++;
    }

  if (counter == 0 && c != EOF)
    return file_list_skip;

  obstack_1grow (stk, 0);

  return (counter == 0 && c == EOF) ? file_list_end : file_list_success;
}


static bool files_from_option;  /* When set, tar will not refuse to create
				   empty archives */
static struct obstack argv_stk; /* Storage for additional command line options
				   read using -T option */

/* Prevent recursive inclusion of the same file */
struct file_id_list
{
  struct file_id_list *next;
  ino_t ino;
  dev_t dev;
};

static struct file_id_list *file_id_list;

static void
add_file_id (const char *filename)
{
  struct file_id_list *p;
  struct stat st;

  if (stat (filename, &st))
    stat_fatal (filename);
  for (p = file_id_list; p; p = p->next)
    if (p->ino == st.st_ino && p->dev == st.st_dev)
      {
	FATAL_ERROR ((0, 0, _("%s: file list already read"),
		      quotearg_colon (filename)));
      }
  p = xmalloc (sizeof *p);
  p->next = file_id_list;
  p->ino = st.st_ino;
  p->dev = st.st_dev;
  file_id_list = p;
}

/* Default density numbers for [0-9][lmh] device specifications */

#ifndef LOW_DENSITY_NUM
# define LOW_DENSITY_NUM 0
#endif

#ifndef MID_DENSITY_NUM
# define MID_DENSITY_NUM 8
#endif

#ifndef HIGH_DENSITY_NUM
# define HIGH_DENSITY_NUM 16
#endif

static void
update_argv (const char *filename, struct argp_state *state)
{
  FILE *fp;
  size_t count = 0, i;
  char *start, *p;
  char **new_argv;
  size_t new_argc;
  bool is_stdin = false;
  enum read_file_list_state read_state;

  if (!strcmp (filename, "-"))
    {
      is_stdin = true;
      request_stdin ("-T");
      fp = stdin;
    }
  else
    {
      add_file_id (filename);
      if ((fp = fopen (filename, "r")) == NULL)
	open_fatal (filename);
    }

  while ((read_state = read_name_from_file (fp, &argv_stk)) != file_list_end)
    {
      switch (read_state)
	{
	case file_list_success:
	  count++;
	  break;

	case file_list_end: /* won't happen, just to pacify gcc */
	  break;

	case file_list_zero:
	  {
	    size_t size;

	    WARN ((0, 0, N_("%s: file name read contains nul character"),
		   quotearg_colon (filename)));

	    /* Prepare new stack contents */
	    size = obstack_object_size (&argv_stk);
	    p = obstack_finish (&argv_stk);
	    for (; size > 0; size--, p++)
	      if (*p)
		obstack_1grow (&argv_stk, *p);
	      else
		obstack_1grow (&argv_stk, '\n');
	    obstack_1grow (&argv_stk, 0);
	    count = 1;
	    /* Read rest of files using new filename terminator */
	    filename_terminator = 0;
	    break;
	  }

	case file_list_skip:
	  break;
	}
    }

  if (!is_stdin)
    fclose (fp);

  if (count == 0)
    return;

  start = obstack_finish (&argv_stk);

  if (filename_terminator == 0)
    for (p = start; *p; p += strlen (p) + 1)
      if (p[0] == '-')
	count++;

  new_argc = state->argc + count;
  new_argv = xmalloc (sizeof (state->argv[0]) * (new_argc + 1));
  memcpy (new_argv, state->argv, sizeof (state->argv[0]) * (state->argc + 1));
  state->argv = new_argv;
  memmove (&state->argv[state->next + count], &state->argv[state->next],
	   (state->argc - state->next + 1) * sizeof (state->argv[0]));

  state->argc = new_argc;

  for (i = state->next, p = start; *p; p += strlen (p) + 1, i++)
    {
      if (filename_terminator == 0 && p[0] == '-')
	state->argv[i++] = "--add-file";
      state->argv[i] = p;
    }
}


static void
tar_help (struct argp_state *state)
{
  argp_fmtstream_t fs;
  state->flags |= ARGP_NO_EXIT;
  argp_state_help (state, state->out_stream,
		   ARGP_HELP_STD_HELP & ~ARGP_HELP_BUG_ADDR);
  /* FIXME: use struct uparams.rmargin (from argp-help.c) instead of 79 */
  fs = argp_make_fmtstream (state->out_stream, 0, 79, 0);

  argp_fmtstream_printf (fs, "\n%s\n\n",
		       _("Valid arguments for --quoting-style options are:"));
  tar_list_quoting_styles (fs, "  ");

  argp_fmtstream_puts (fs, _("\n*This* tar defaults to:\n"));
  show_default_settings_fs (fs);
  argp_fmtstream_putc (fs, '\n');
  argp_fmtstream_printf (fs, _("Report bugs to %s.\n"),
			 argp_program_bug_address);
  argp_fmtstream_free (fs);
}

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct tar_args *args = state->input;

  switch (key)
    {
    case ARGP_KEY_ARG:
      /* File name or non-parsed option, because of ARGP_IN_ORDER */
      name_add_name (arg, MAKE_INCL_OPTIONS (args));
      args->input_files = true;
      break;

    case 'A':
      set_subcommand_option (CAT_SUBCOMMAND);
      break;

    case 'b':
      {
	uintmax_t u;
	if (! (xstrtoumax (arg, 0, 10, &u, "") == LONGINT_OK
	       && u == (blocking_factor = u)
	       && 0 < blocking_factor
	       && u == (record_size = u * BLOCKSIZE) / BLOCKSIZE))
	  USAGE_ERROR ((0, 0, "%s: %s", quotearg_colon (arg),
			_("Invalid blocking factor")));
      }
      break;

    case 'B':
      /* Try to reblock input records.  For reading 4.2BSD pipes.  */

      /* It would surely make sense to exchange -B and -R, but it seems
	 that -B has been used for a long while in Sun tar and most
	 BSD-derived systems.  This is a consequence of the block/record
	 terminology confusion.  */

      read_full_records_option = true;
      break;

    case 'c':
      set_subcommand_option (CREATE_SUBCOMMAND);
      break;

    case 'C':
      name_add_dir (arg);
      break;

    case 'd':
      set_subcommand_option (DIFF_SUBCOMMAND);
      break;

    case 'f':
      if (archive_names == allocated_archive_names)
	archive_name_array = x2nrealloc (archive_name_array,
					 &allocated_archive_names,
					 sizeof (archive_name_array[0]));

      archive_name_array[archive_names++] = arg;
      break;

    case 'F':
      /* Since -F is only useful with -M, make it implied.  Run this
	 script at the end of each tape.  */

      info_script_option = arg;
      multi_volume_option = true;
      break;

    case 'g':
      listed_incremental_option = arg;
      after_date_option = true;
      /* Fall through.  */

    case 'G':
      /* We are making an incremental dump (FIXME: are we?); save
	 directories at the beginning of the archive, and include in each
	 directory its contents.  */

      incremental_option = true;
      break;

    case 'h':
      /* Follow symbolic links.  */
      dereference_option = true;
      break;

    case 'i':
      /* Ignore zero blocks (eofs).  This can't be the default,
	 because Unix tar writes two blocks of zeros, then pads out
	 the record with garbage.  */

      ignore_zeros_option = true;
      break;

    case 'I':
      USAGE_ERROR ((0, 0,
		    _("Warning: the -I option is not supported;"
		      " perhaps you meant -j or -T?")));
      break;

    case 'j':
      set_use_compress_program_option ("bzip2");
      break;

    case 'k':
      /* Don't replace existing files.  */
      old_files_option = KEEP_OLD_FILES;
      break;

    case 'K':
      starting_file_option = true;
      addname (arg, 0);
      break;

    case ONE_FILE_SYSTEM_OPTION:
      /* When dumping directories, don't dump files/subdirectories
	 that are on other filesystems. */
      one_file_system_option = true;
      break;

    case 'l':
      check_links_option = 1;
      break;

    case 'L':
      {
	uintmax_t u;
	if (xstrtoumax (arg, 0, 10, &u, "") != LONGINT_OK)
	  USAGE_ERROR ((0, 0, "%s: %s", quotearg_colon (arg),
			_("Invalid tape length")));
	tape_length_option = 1024 * (tarlong) u;
	multi_volume_option = true;
      }
      break;

    case 'm':
      touch_option = true;
      break;

    case 'M':
      /* Make multivolume archive: when we can't write any more into
	 the archive, re-open it, and continue writing.  */

      multi_volume_option = true;
      break;

    case MTIME_OPTION:
      get_date_or_file (args, "--mtime", arg, &mtime_option);
      set_mtime_option = true;
      break;

    case 'n':
      seekable_archive = true;
      break;

    case 'N':
      after_date_option = true;
      /* Fall through.  */

    case NEWER_MTIME_OPTION:
      if (NEWER_OPTION_INITIALIZED (newer_mtime_option))
	USAGE_ERROR ((0, 0, _("More than one threshold date")));
      get_date_or_file (args,
			key == NEWER_MTIME_OPTION ? "--newer-mtime"
			  : "--after-date", arg, &newer_mtime_option);
      break;

    case 'o':
      args->o_option = true;
      break;

    case 'O':
      to_stdout_option = true;
      break;

    case 'p':
      same_permissions_option = true;
      break;

    case 'P':
      absolute_names_option = true;
      break;

    case 'r':
      set_subcommand_option (APPEND_SUBCOMMAND);
      break;

    case 'R':
      /* Print block numbers for debugging bad tar archives.  */

      /* It would surely make sense to exchange -B and -R, but it seems
	 that -B has been used for a long while in Sun tar and most
	 BSD-derived systems.  This is a consequence of the block/record
	 terminology confusion.  */

      block_number_option = true;
      break;

    case 's':
      /* Names to extract are sorted.  */

      same_order_option = true;
      break;

    case 'S':
      sparse_option = true;
      break;

    case SPARSE_VERSION_OPTION:
      sparse_option = true;
      {
	char *p;
	tar_sparse_major = strtoul (arg, &p, 10);
	if (*p)
	  {
	    if (*p != '.')
	      USAGE_ERROR ((0, 0, _("Invalid sparse version value")));
	    tar_sparse_minor = strtoul (p + 1, &p, 10);
	    if (*p)
	      USAGE_ERROR ((0, 0, _("Invalid sparse version value")));
	  }
      }
      break;

    case 't':
      set_subcommand_option (LIST_SUBCOMMAND);
      verbose_option++;
      break;

    case TEST_LABEL_OPTION:
      set_subcommand_option (LIST_SUBCOMMAND);
      test_label_option = true;
      break;

    case 'T':
      update_argv (arg, state);
      /* Indicate we've been given -T option. This is for backward
	 compatibility only, so that `tar cfT archive /dev/null will
	 succeed */
      files_from_option = true;
      break;

    case 'u':
      set_subcommand_option (UPDATE_SUBCOMMAND);
      break;

    case 'U':
      old_files_option = UNLINK_FIRST_OLD_FILES;
      break;

    case UTC_OPTION:
      utc_option = true;
      break;

    case 'v':
      verbose_option++;
      break;

    case 'V':
      volume_label_option = arg;
      break;

    case 'w':
      interactive_option = true;
      break;

    case 'W':
      verify_option = true;
      break;

    case 'x':
      set_subcommand_option (EXTRACT_SUBCOMMAND);
      break;

    case 'X':
      if (add_exclude_file (add_exclude, excluded, arg,
			    MAKE_EXCL_OPTIONS (args), '\n')
	  != 0)
	{
	  int e = errno;
	  FATAL_ERROR ((0, e, "%s", quotearg_colon (arg)));
	}
      break;

    case 'z':
      set_use_compress_program_option ("gzip");
      break;

    case 'Z':
      set_use_compress_program_option ("compress");
      break;

    case ANCHORED_OPTION:
      args->matching_flags |= EXCLUDE_ANCHORED;
      break;

    case ATIME_PRESERVE_OPTION:
      atime_preserve_option =
	(arg
	 ? XARGMATCH ("--atime-preserve", arg,
		      atime_preserve_args, atime_preserve_types)
	 : replace_atime_preserve);
      if (! O_NOATIME && atime_preserve_option == system_atime_preserve)
	FATAL_ERROR ((0, 0,
		      _("--atime-preserve='system' is not supported"
			" on this platform")));
      break;

    case CHECKPOINT_OPTION:
      if (arg)
	{
	  char *p;

	  if (*arg == '.')
	    {
	      checkpoint_style = checkpoint_dot;
	      arg++;
	    }
	  checkpoint_option = strtoul (arg, &p, 0);
	  if (*p)
	    FATAL_ERROR ((0, 0,
			  _("--checkpoint value is not an integer")));
	}
      else
	checkpoint_option = 10;
      break;

    case BACKUP_OPTION:
      backup_option = true;
      if (arg)
	args->version_control_string = arg;
      break;

    case DELAY_DIRECTORY_RESTORE_OPTION:
      delay_directory_restore_option = true;
      break;

    case NO_DELAY_DIRECTORY_RESTORE_OPTION:
      delay_directory_restore_option = false;
      break;

    case DELETE_OPTION:
      set_subcommand_option (DELETE_SUBCOMMAND);
      break;

    case EXCLUDE_OPTION:
      add_exclude (excluded, arg, MAKE_EXCL_OPTIONS (args));
      break;

    case EXCLUDE_CACHES_OPTION:
      add_exclusion_tag ("CACHEDIR.TAG", exclusion_tag_contents,
			 cachedir_file_p);
      break;

    case EXCLUDE_CACHES_UNDER_OPTION:
      add_exclusion_tag ("CACHEDIR.TAG", exclusion_tag_under,
			 cachedir_file_p);
      break;

    case EXCLUDE_CACHES_ALL_OPTION:
      add_exclusion_tag ("CACHEDIR.TAG", exclusion_tag_all,
			 cachedir_file_p);
      break;

    case EXCLUDE_TAG_OPTION:
      add_exclusion_tag (arg, exclusion_tag_contents, NULL);
      break;

    case EXCLUDE_TAG_UNDER_OPTION:
      add_exclusion_tag (arg, exclusion_tag_under, NULL);
      break;

    case EXCLUDE_TAG_ALL_OPTION:
      add_exclusion_tag (arg, exclusion_tag_all, NULL);
      break;

    case EXCLUDE_VCS_OPTION:
      exclude_vcs_files ();
      break;
      
    case FORCE_LOCAL_OPTION:
      force_local_option = true;
      break;

    case 'H':
      set_archive_format (arg);
      break;

    case INDEX_FILE_OPTION:
      index_file_name = arg;
      break;

    case IGNORE_CASE_OPTION:
      args->matching_flags |= FNM_CASEFOLD;
      break;

    case IGNORE_COMMAND_ERROR_OPTION:
      ignore_command_error_option = true;
      break;

    case IGNORE_FAILED_READ_OPTION:
      ignore_failed_read_option = true;
      break;

    case KEEP_NEWER_FILES_OPTION:
      old_files_option = KEEP_NEWER_FILES;
      break;

    case GROUP_OPTION:
      if (! (strlen (arg) < GNAME_FIELD_SIZE
	     && gname_to_gid (arg, &group_option)))
	{
	  uintmax_t g;
	  if (xstrtoumax (arg, 0, 10, &g, "") == LONGINT_OK
	      && g == (gid_t) g)
	    group_option = g;
	  else
	    FATAL_ERROR ((0, 0, "%s: %s", quotearg_colon (arg),
			  _("%s: Invalid group")));
	}
      break;

    case MODE_OPTION:
      mode_option = mode_compile (arg);
      if (!mode_option)
	FATAL_ERROR ((0, 0, _("Invalid mode given on option")));
      initial_umask = umask (0);
      umask (initial_umask);
      break;

    case NO_ANCHORED_OPTION:
      args->include_anchored = 0; /* Clear the default for comman line args */
      args->matching_flags &= ~ EXCLUDE_ANCHORED;
      break;

    case NO_IGNORE_CASE_OPTION:
      args->matching_flags &= ~ FNM_CASEFOLD;
      break;

    case NO_IGNORE_COMMAND_ERROR_OPTION:
      ignore_command_error_option = false;
      break;

    case NO_OVERWRITE_DIR_OPTION:
      old_files_option = NO_OVERWRITE_DIR_OLD_FILES;
      break;

    case NO_QUOTE_CHARS_OPTION:
      for (;*arg; arg++)
	set_char_quoting (NULL, *arg, 0);
      break;

    case NO_WILDCARDS_OPTION:
      args->wildcards = disable_wildcards;
      break;

    case NO_WILDCARDS_MATCH_SLASH_OPTION:
      args->matching_flags |= FNM_FILE_NAME;
      break;

    case NULL_OPTION:
      filename_terminator = '\0';
      break;

    case NUMERIC_OWNER_OPTION:
      numeric_owner_option = true;
      break;

    case OCCURRENCE_OPTION:
      if (!arg)
	occurrence_option = 1;
      else
	{
	  uintmax_t u;
	  if (xstrtoumax (arg, 0, 10, &u, "") == LONGINT_OK)
	    occurrence_option = u;
	  else
	    FATAL_ERROR ((0, 0, "%s: %s", quotearg_colon (arg),
			  _("Invalid number")));
	}
      break;

    case OVERWRITE_DIR_OPTION:
      old_files_option = DEFAULT_OLD_FILES;
      break;

    case OVERWRITE_OPTION:
      old_files_option = OVERWRITE_OLD_FILES;
      break;

    case OWNER_OPTION:
      if (! (strlen (arg) < UNAME_FIELD_SIZE
	     && uname_to_uid (arg, &owner_option)))
	{
	  uintmax_t u;
	  if (xstrtoumax (arg, 0, 10, &u, "") == LONGINT_OK
	      && u == (uid_t) u)
	    owner_option = u;
	  else
	    FATAL_ERROR ((0, 0, "%s: %s", quotearg_colon (arg),
			  _("Invalid owner")));
	}
      break;

    case QUOTE_CHARS_OPTION:
      for (;*arg; arg++)
	set_char_quoting (NULL, *arg, 1);
      break;

    case QUOTING_STYLE_OPTION:
      tar_set_quoting_style (arg);
      break;

    case PAX_OPTION:
      args->pax_option = true;
      xheader_set_option (arg);
      break;

    case POSIX_OPTION:
      set_archive_format ("posix");
      break;

    case PRESERVE_OPTION:
      /* FIXME: What it is good for? */
      same_permissions_option = true;
      same_order_option = true;
      break;

    case RECORD_SIZE_OPTION:
      {
	uintmax_t u;
	if (! (xstrtoumax (arg, 0, 10, &u, "") == LONGINT_OK
	       && u == (size_t) u))
	  USAGE_ERROR ((0, 0, "%s: %s", quotearg_colon (arg),
			_("Invalid record size")));
	record_size = u;
	if (record_size % BLOCKSIZE != 0)
	  USAGE_ERROR ((0, 0, _("Record size must be a multiple of %d."),
			BLOCKSIZE));
	blocking_factor = record_size / BLOCKSIZE;
      }
      break;

    case RECURSIVE_UNLINK_OPTION:
      recursive_unlink_option = true;
      break;

    case REMOVE_FILES_OPTION:
      remove_files_option = true;
      break;

    case RESTRICT_OPTION:
      restrict_option = true;
      break;

    case RMT_COMMAND_OPTION:
      rmt_command = arg;
      break;

    case RSH_COMMAND_OPTION:
      rsh_command_option = arg;
      break;

    case SHOW_DEFAULTS_OPTION:
      show_default_settings (stdout);
      close_stdout ();
      exit (0);

    case STRIP_COMPONENTS_OPTION:
      {
	uintmax_t u;
	if (! (xstrtoumax (arg, 0, 10, &u, "") == LONGINT_OK
	       && u == (size_t) u))
	  USAGE_ERROR ((0, 0, "%s: %s", quotearg_colon (arg),
			_("Invalid number of elements")));
	strip_name_components = u;
      }
      break;

    case SHOW_OMITTED_DIRS_OPTION:
      show_omitted_dirs_option = true;
      break;

    case SHOW_TRANSFORMED_NAMES_OPTION:
      show_transformed_names_option = true;
      break;

    case SUFFIX_OPTION:
      backup_option = true;
      args->backup_suffix_string = arg;
      break;

    case TO_COMMAND_OPTION:
      if (to_command_option)
        USAGE_ERROR ((0, 0, _("Only one --to-command option allowed")));
      to_command_option = arg;
      break;

    case TOTALS_OPTION:
      if (arg)
	set_stat_signal (arg);
      else
	totals_option = true;
      break;

    case TRANSFORM_OPTION:
      set_transform_expr (arg);
      break;

    case USE_COMPRESS_PROGRAM_OPTION:
      set_use_compress_program_option (arg);
      break;

    case VOLNO_FILE_OPTION:
      volno_file_option = arg;
      break;

    case WILDCARDS_OPTION:
      args->wildcards = enable_wildcards;
      break;

    case WILDCARDS_MATCH_SLASH_OPTION:
      args->matching_flags &= ~ FNM_FILE_NAME;
      break;

    case NO_RECURSION_OPTION:
      recursion_option = 0;
      break;

    case NO_SAME_OWNER_OPTION:
      same_owner_option = -1;
      break;

    case NO_SAME_PERMISSIONS_OPTION:
      same_permissions_option = -1;
      break;

    case RECURSION_OPTION:
      recursion_option = FNM_LEADING_DIR;
      break;

    case SAME_OWNER_OPTION:
      same_owner_option = 1;
      break;

    case UNQUOTE_OPTION:
      unquote_option = true;
      break;

    case NO_UNQUOTE_OPTION:
      unquote_option = false;
      break;

    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':

#ifdef DEVICE_PREFIX
      {
	int device = key - '0';
	int density;
	static char buf[sizeof DEVICE_PREFIX + 10];
	char *cursor;

	if (arg[1])
	  argp_error (state, _("Malformed density argument: %s"), quote (arg));

	strcpy (buf, DEVICE_PREFIX);
	cursor = buf + strlen (buf);

#ifdef DENSITY_LETTER

	sprintf (cursor, "%d%c", device, arg[0]);

#else /* not DENSITY_LETTER */

	switch (arg[0])
	  {
	  case 'l':
	    device += LOW_DENSITY_NUM;
	    break;

	  case 'm':
	    device += MID_DENSITY_NUM;
	    break;

	  case 'h':
	    device += HIGH_DENSITY_NUM;
	    break;

	  default:
	    argp_error (state, _("Unknown density: `%c'"), arg[0]);
	  }
	sprintf (cursor, "%d", device);

#endif /* not DENSITY_LETTER */

	if (archive_names == allocated_archive_names)
	  archive_name_array = x2nrealloc (archive_name_array,
					   &allocated_archive_names,
					   sizeof (archive_name_array[0]));
	archive_name_array[archive_names++] = xstrdup (buf);
      }
      break;

#else /* not DEVICE_PREFIX */

      argp_error (state,
		  _("Options `-[0-7][lmh]' not supported by *this* tar"));

#endif /* not DEVICE_PREFIX */

    case '?':
      tar_help (state);
      close_stdout ();
      exit (0);

    case USAGE_OPTION:
      argp_state_help (state, state->out_stream, ARGP_HELP_USAGE);
      close_stdout ();
      exit (0);

    case VERSION_OPTION:
      version_etc (state->out_stream, "tar", PACKAGE_NAME, VERSION,
		   "John Gilmore", "Jay Fenlason", (char *) NULL);
      close_stdout ();
      exit (0);

    case HANG_OPTION:
      _argp_hang = atoi (arg ? arg : "3600");
      while (_argp_hang-- > 0)
	sleep (1);
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp argp = {
  options,
  parse_opt,
  N_("[FILE]..."),
  doc,
  NULL,
  NULL,
  NULL
};

void
usage (int status)
{
  argp_help (&argp, stderr, ARGP_HELP_SEE, (char*) program_name);
  close_stdout ();
  exit (status);
}

/* Parse the options for tar.  */

static struct argp_option *
find_argp_option (struct argp_option *o, int letter)
{
  for (;
       !(o->name == NULL
	 && o->key == 0
	 && o->arg == 0
	 && o->flags == 0
	 && o->doc == NULL); o++)
    if (o->key == letter)
      return o;
  return NULL;
}

static void
decode_options (int argc, char **argv)
{
  int idx;
  struct tar_args args;

  /* Set some default option values.  */
  args.textual_date = NULL;
  args.wildcards = default_wildcards;
  args.matching_flags = 0;
  args.include_anchored = EXCLUDE_ANCHORED;
  args.o_option = false;
  args.pax_option = false;
  args.backup_suffix_string = getenv ("SIMPLE_BACKUP_SUFFIX");
  args.version_control_string = 0;
  args.input_files = false;

  subcommand_option = UNKNOWN_SUBCOMMAND;
  archive_format = DEFAULT_FORMAT;
  blocking_factor = DEFAULT_BLOCKING;
  record_size = DEFAULT_BLOCKING * BLOCKSIZE;
  excluded = new_exclude ();
  newer_mtime_option.tv_sec = TYPE_MINIMUM (time_t);
  newer_mtime_option.tv_nsec = -1;
  recursion_option = FNM_LEADING_DIR;
  unquote_option = true;
  tar_sparse_major = 1;
  tar_sparse_minor = 0;

  owner_option = -1;
  group_option = -1;

  /* Convert old-style tar call by exploding option element and rearranging
     options accordingly.  */

  if (argc > 1 && argv[1][0] != '-')
    {
      int new_argc;		/* argc value for rearranged arguments */
      char **new_argv;		/* argv value for rearranged arguments */
      char *const *in;		/* cursor into original argv */
      char **out;		/* cursor into rearranged argv */
      const char *letter;	/* cursor into old option letters */
      char buffer[3];		/* constructed option buffer */

      /* Initialize a constructed option.  */

      buffer[0] = '-';
      buffer[2] = '\0';

      /* Allocate a new argument array, and copy program name in it.  */

      new_argc = argc - 1 + strlen (argv[1]);
      new_argv = xmalloc ((new_argc + 1) * sizeof (char *));
      in = argv;
      out = new_argv;
      *out++ = *in++;

      /* Copy each old letter option as a separate option, and have the
	 corresponding argument moved next to it.  */

      for (letter = *in++; *letter; letter++)
	{
	  struct argp_option *opt;

	  buffer[1] = *letter;
	  *out++ = xstrdup (buffer);
	  opt = find_argp_option (options, *letter);
	  if (opt && opt->arg)
	    {
	      if (in < argv + argc)
		*out++ = *in++;
	      else
		USAGE_ERROR ((0, 0, _("Old option `%c' requires an argument."),
			      *letter));
	    }
	}

      /* Copy all remaining options.  */

      while (in < argv + argc)
	*out++ = *in++;
      *out = 0;

      /* Replace the old option list by the new one.  */

      argc = new_argc;
      argv = new_argv;
    }

  /* Parse all options and non-options as they appear.  */

  prepend_default_options (getenv ("TAR_OPTIONS"), &argc, &argv);

  if (argp_parse (&argp, argc, argv, ARGP_IN_ORDER|ARGP_NO_HELP,
		  &idx, &args))
    exit (TAREXIT_FAILURE);


  /* Special handling for 'o' option:

     GNU tar used to say "output old format".
     UNIX98 tar says don't chown files after extracting (we use
     "--no-same-owner" for this).

     The old GNU tar semantics is retained when used with --create
     option, otherwise UNIX98 semantics is assumed */

  if (args.o_option)
    {
      if (subcommand_option == CREATE_SUBCOMMAND)
	{
	  /* GNU Tar <= 1.13 compatibility */
	  set_archive_format ("v7");
	}
      else
	{
	  /* UNIX98 compatibility */
	  same_owner_option = -1;
	}
    }

  /* Handle operands after any "--" argument.  */
  for (; idx < argc; idx++)
    {
      name_add_name (argv[idx], MAKE_INCL_OPTIONS (&args));
      args.input_files = true;
    }

  /* Warn about implicit use of the wildcards in command line arguments.
     See TODO */
  warn_regex_usage = args.wildcards == default_wildcards;

  /* Derive option values and check option consistency.  */

  if (archive_format == DEFAULT_FORMAT)
    {
      if (args.pax_option)
	archive_format = POSIX_FORMAT;
      else
	archive_format = DEFAULT_ARCHIVE_FORMAT;
    }

  if ((volume_label_option && subcommand_option == CREATE_SUBCOMMAND)
      || incremental_option
      || multi_volume_option
      || sparse_option)
    assert_format (FORMAT_MASK (OLDGNU_FORMAT)
		   | FORMAT_MASK (GNU_FORMAT)
		   | FORMAT_MASK (POSIX_FORMAT));

  if (occurrence_option)
    {
      if (!args.input_files)
	USAGE_ERROR ((0, 0,
		      _("--occurrence is meaningless without a file list")));
      if (subcommand_option != DELETE_SUBCOMMAND
	  && subcommand_option != DIFF_SUBCOMMAND
	  && subcommand_option != EXTRACT_SUBCOMMAND
	  && subcommand_option != LIST_SUBCOMMAND)
	    USAGE_ERROR ((0, 0,
			  _("--occurrence cannot be used in the requested operation mode")));
    }

  if (seekable_archive && subcommand_option == DELETE_SUBCOMMAND)
    {
      /* The current code in delete.c is based on the assumption that
	 skip_member() reads all data from the archive. So, we should
	 make sure it won't use seeks. On the other hand, the same code
	 depends on the ability to backspace a record in the archive,
	 so setting seekable_archive to false is technically incorrect.
         However, it is tested only in skip_member(), so it's not a
	 problem. */
      seekable_archive = false;
    }

  if (archive_names == 0)
    {
      /* If no archive file name given, try TAPE from the environment, or
	 else, DEFAULT_ARCHIVE from the configuration process.  */

      archive_names = 1;
      archive_name_array[0] = getenv ("TAPE");
      if (! archive_name_array[0])
	archive_name_array[0] = DEFAULT_ARCHIVE;
    }

  /* Allow multiple archives only with `-M'.  */

  if (archive_names > 1 && !multi_volume_option)
    USAGE_ERROR ((0, 0,
		  _("Multiple archive files require `-M' option")));

  if (listed_incremental_option
      && NEWER_OPTION_INITIALIZED (newer_mtime_option))
    USAGE_ERROR ((0, 0,
		  _("Cannot combine --listed-incremental with --newer")));

  if (volume_label_option)
    {
      if (archive_format == GNU_FORMAT || archive_format == OLDGNU_FORMAT)
	{
	  size_t volume_label_max_len =
	    (sizeof current_header->header.name
	     - 1 /* for trailing '\0' */
	     - (multi_volume_option
		? (sizeof " Volume "
		   - 1 /* for null at end of " Volume " */
		   + INT_STRLEN_BOUND (int) /* for volume number */
		   - 1 /* for sign, as 0 <= volno */)
		: 0));
	  if (volume_label_max_len < strlen (volume_label_option))
	    USAGE_ERROR ((0, 0,
			  ngettext ("%s: Volume label is too long (limit is %lu byte)",
				    "%s: Volume label is too long (limit is %lu bytes)",
				    volume_label_max_len),
			  quotearg_colon (volume_label_option),
			  (unsigned long) volume_label_max_len));
	}
      /* else FIXME
	 Label length in PAX format is limited by the volume size. */
    }

  if (verify_option)
    {
      if (multi_volume_option)
	USAGE_ERROR ((0, 0, _("Cannot verify multi-volume archives")));
      if (use_compress_program_option)
	USAGE_ERROR ((0, 0, _("Cannot verify compressed archives")));
    }

  if (use_compress_program_option)
    {
      if (multi_volume_option)
	USAGE_ERROR ((0, 0, _("Cannot use multi-volume compressed archives")));
      if (subcommand_option == UPDATE_SUBCOMMAND
	  || subcommand_option == APPEND_SUBCOMMAND
	  || subcommand_option == DELETE_SUBCOMMAND)
	USAGE_ERROR ((0, 0, _("Cannot update compressed archives")));
      if (subcommand_option == CAT_SUBCOMMAND)
	USAGE_ERROR ((0, 0, _("Cannot concatenate compressed archives")));
    }

  /* It is no harm to use --pax-option on non-pax archives in archive
     reading mode. It may even be useful, since it allows to override
     file attributes from tar headers. Therefore I allow such usage.
     --gray */
  if (args.pax_option
      && archive_format != POSIX_FORMAT
      && (subcommand_option != EXTRACT_SUBCOMMAND
	  || subcommand_option != DIFF_SUBCOMMAND
	  || subcommand_option != LIST_SUBCOMMAND))
    USAGE_ERROR ((0, 0, _("--pax-option can be used only on POSIX archives")));

  /* If ready to unlink hierarchies, so we are for simpler files.  */
  if (recursive_unlink_option)
    old_files_option = UNLINK_FIRST_OLD_FILES;


  if (test_label_option)
    {
      /* --test-label is silent if the user has specified the label name to
	 compare against. */
      if (!args.input_files)
	verbose_option++;
    }
  else if (utc_option)
    verbose_option = 2;

  /* Forbid using -c with no input files whatsoever.  Check that `-f -',
     explicit or implied, is used correctly.  */

  switch (subcommand_option)
    {
    case CREATE_SUBCOMMAND:
      if (!args.input_files && !files_from_option)
	USAGE_ERROR ((0, 0,
		      _("Cowardly refusing to create an empty archive")));
      break;

    case EXTRACT_SUBCOMMAND:
    case LIST_SUBCOMMAND:
    case DIFF_SUBCOMMAND:
      for (archive_name_cursor = archive_name_array;
	   archive_name_cursor < archive_name_array + archive_names;
	   archive_name_cursor++)
	if (!strcmp (*archive_name_cursor, "-"))
	  request_stdin ("-f");
      break;

    case CAT_SUBCOMMAND:
    case UPDATE_SUBCOMMAND:
    case APPEND_SUBCOMMAND:
      for (archive_name_cursor = archive_name_array;
	   archive_name_cursor < archive_name_array + archive_names;
	   archive_name_cursor++)
	if (!strcmp (*archive_name_cursor, "-"))
	  USAGE_ERROR ((0, 0,
			_("Options `-Aru' are incompatible with `-f -'")));

    default:
      break;
    }

  /* Initialize stdlis */
  if (index_file_name)
    {
      stdlis = fopen (index_file_name, "w");
      if (! stdlis)
	open_error (index_file_name);
    }
  else
    stdlis = to_stdout_option ? stderr : stdout;

  archive_name_cursor = archive_name_array;

  /* Prepare for generating backup names.  */

  if (args.backup_suffix_string)
    simple_backup_suffix = xstrdup (args.backup_suffix_string);

  if (backup_option)
    {
      backup_type = xget_version ("--backup", args.version_control_string);
      /* No backup is needed either if explicitely disabled or if
	 the extracted files are not being written to disk. */
      if (backup_type == no_backups || EXTRACT_OVER_PIPE)
	backup_option = false;
    }

  if (verbose_option)
    report_textual_dates (&args);
}


/* Tar proper.  */

/* Main routine for tar.  */
int
main (int argc, char **argv)
{
  set_start_time ();
  program_name = argv[0];

  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  exit_failure = TAREXIT_FAILURE;
  exit_status = TAREXIT_SUCCESS;
  filename_terminator = '\n';
  set_quoting_style (0, DEFAULT_QUOTING_STYLE);

  /* Make sure we have first three descriptors available */
  stdopen ();

  /* Pre-allocate a few structures.  */

  allocated_archive_names = 10;
  archive_name_array =
    xmalloc (sizeof (const char *) * allocated_archive_names);
  archive_names = 0;

  obstack_init (&argv_stk);

#ifdef SIGCHLD
  /* System V fork+wait does not work if SIGCHLD is ignored.  */
  signal (SIGCHLD, SIG_DFL);
#endif

  /* Decode options.  */

  decode_options (argc, argv);

  name_init ();

  /* Main command execution.  */

  if (volno_file_option)
    init_volume_number ();

  switch (subcommand_option)
    {
    case UNKNOWN_SUBCOMMAND:
      USAGE_ERROR ((0, 0,
		    _("You must specify one of the `-Acdtrux' options")));

    case CAT_SUBCOMMAND:
    case UPDATE_SUBCOMMAND:
    case APPEND_SUBCOMMAND:
      update_archive ();
      break;

    case DELETE_SUBCOMMAND:
      delete_archive_members ();
      break;

    case CREATE_SUBCOMMAND:
      create_archive ();
      break;

    case EXTRACT_SUBCOMMAND:
      extr_init ();
      read_and (extract_archive);

      /* FIXME: should extract_finish () even if an ordinary signal is
	 received.  */
      extract_finish ();

      break;

    case LIST_SUBCOMMAND:
      read_and (list_archive);
      break;

    case DIFF_SUBCOMMAND:
      diff_init ();
      read_and (diff_archive);
      break;
    }

  if (totals_option)
    print_total_stats ();

  if (check_links_option)
    check_links ();

  if (volno_file_option)
    closeout_volume_number ();

  /* Dispose of allocated memory, and return.  */

  free (archive_name_array);
  name_term ();

  if (exit_status == TAREXIT_FAILURE)
    error (0, 0, _("Error exit delayed from previous errors"));

  if (stdlis == stdout)
    close_stdout ();
  else if (ferror (stderr) || fclose (stderr) != 0)
    exit_status = TAREXIT_FAILURE;

  return exit_status;
}

void
tar_stat_init (struct tar_stat_info *st)
{
  memset (st, 0, sizeof (*st));
}

void
tar_stat_destroy (struct tar_stat_info *st)
{
  free (st->orig_file_name);
  free (st->file_name);
  free (st->link_name);
  free (st->uname);
  free (st->gname);
  free (st->sparse_map);
  free (st->dumpdir);
  xheader_destroy (&st->xhdr);
  memset (st, 0, sizeof (*st));
}

/* Format mask for all available formats that support nanosecond
   timestamp resolution. */
#define NS_PRECISION_FORMAT_MASK FORMAT_MASK (POSIX_FORMAT)

/* Same as timespec_cmp, but ignore nanoseconds if current archive
   format does not provide sufficient resolution.  */
int
tar_timespec_cmp (struct timespec a, struct timespec b)
{
  if (!(FORMAT_MASK (current_format) & NS_PRECISION_FORMAT_MASK))
    a.tv_nsec = b.tv_nsec = 0;
  return timespec_cmp (a, b);
}
