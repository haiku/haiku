/* A tar (tape archiver) program.

   Copyright (C) 1988, 1992, 1993, 1994, 1995, 1996, 1997, 1999, 2000,
   2001, 2003 Free Software Foundation, Inc.

   Written by John Gilmore, starting 1985-08-25.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any later
   version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
   Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "system.h"

#include <fnmatch.h>
#include <getopt.h>

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

#include <getdate.h>
#include <localedir.h>
#include <prepargs.h>
#include <quotearg.h>
#include <xstrtol.h>

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

/* Returns true if and only if the user typed 'y' or 'Y'.  */
int
confirm (const char *message_action, const char *message_name)
{
  static FILE *confirm_file;
  static int confirm_file_EOF;

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

  {
    int reply = confirm_file_EOF ? EOF : getc (confirm_file);
    int character;

    for (character = reply;
	 character != '\n';
	 character = getc (confirm_file))
      if (character == EOF)
	{
	  confirm_file_EOF = 1;
	  fputc ('\n', stdlis);
	  fflush (stdlis);
	  break;
	}
    return reply == 'y' || reply == 'Y';
  }
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
  { NULL,	 0 }
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

static const char *
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
  if ((FORMAT_MASK(archive_format) & fmt_mask) == 0)
    USAGE_ERROR ((0, 0,
		  _("GNU features wanted on incompatible archive format")));
}



/* Options.  */

/* For long options that unconditionally set a single flag, we have getopt
   do it.  For the others, we share the code for the equivalent short
   named option, the name of which is stored in the otherwise-unused `val'
   field of the `struct option'; for long options that have no equivalent
   short option, we use non-characters as pseudo short options,
   starting at CHAR_MAX + 1 and going upwards.  */

enum
{
  ANCHORED_OPTION = CHAR_MAX + 1,
  ATIME_PRESERVE_OPTION,
  BACKUP_OPTION,
  CHECKPOINT_OPTION,
  DELETE_OPTION,
  EXCLUDE_OPTION,
  FORCE_LOCAL_OPTION,
  FORMAT_OPTION,
  GROUP_OPTION,
  IGNORE_CASE_OPTION,
  IGNORE_FAILED_READ_OPTION,
  INDEX_FILE_OPTION,
  KEEP_NEWER_FILES_OPTION,
  MODE_OPTION,
  NEWER_MTIME_OPTION,
  NO_ANCHORED_OPTION,
  NO_IGNORE_CASE_OPTION,
  NO_OVERWRITE_DIR_OPTION,
  NO_WILDCARDS_OPTION,
  NO_WILDCARDS_MATCH_SLASH_OPTION,
  NULL_OPTION,
  NUMERIC_OWNER_OPTION,
  OCCURRENCE_OPTION,
  OVERWRITE_OPTION,
  OWNER_OPTION,
  PAX_OPTION,
  POSIX_OPTION,
  PRESERVE_OPTION,
  RECORD_SIZE_OPTION,
  RECURSIVE_UNLINK_OPTION,
  REMOVE_FILES_OPTION,
  RSH_COMMAND_OPTION,
  SHOW_DEFAULTS_OPTION,
  SHOW_OMITTED_DIRS_OPTION,
  STRIP_PATH_OPTION,
  SUFFIX_OPTION,
  TOTALS_OPTION,
  USE_COMPRESS_PROGRAM_OPTION,
  UTC_OPTION,
  VOLNO_FILE_OPTION,
  WILDCARDS_OPTION,
  WILDCARDS_MATCH_SLASH_OPTION
};

/* If nonzero, display usage information and exit.  */
static int show_help;

/* If nonzero, print the version on standard output and exit.  */
static int show_version;

static struct option long_options[] =
{
  {"absolute-names", no_argument, 0, 'P'},
  {"after-date", required_argument, 0, 'N'},
  {"anchored", no_argument, 0, ANCHORED_OPTION},
  {"append", no_argument, 0, 'r'},
  {"atime-preserve", no_argument, 0, ATIME_PRESERVE_OPTION},
  {"backup", optional_argument, 0, BACKUP_OPTION},
  {"block-number", no_argument, 0, 'R'},
  {"blocking-factor", required_argument, 0, 'b'},
  {"bzip2", no_argument, 0, 'j'},
  {"catenate", no_argument, 0, 'A'},
  {"checkpoint", no_argument, 0, CHECKPOINT_OPTION},
  {"check-links", no_argument, &check_links_option, 1},
  {"compare", no_argument, 0, 'd'},
  {"compress", no_argument, 0, 'Z'},
  {"concatenate", no_argument, 0, 'A'},
  {"confirmation", no_argument, 0, 'w'},
  /* FIXME: --selective as a synonym for --confirmation?  */
  {"create", no_argument, 0, 'c'},
  {"delete", no_argument, 0, DELETE_OPTION},
  {"dereference", no_argument, 0, 'h'},
  {"diff", no_argument, 0, 'd'},
  {"directory", required_argument, 0, 'C'},
  {"exclude", required_argument, 0, EXCLUDE_OPTION},
  {"exclude-from", required_argument, 0, 'X'},
  {"extract", no_argument, 0, 'x'},
  {"file", required_argument, 0, 'f'},
  {"files-from", required_argument, 0, 'T'},
  {"force-local", no_argument, 0, FORCE_LOCAL_OPTION},
  {"format", required_argument, 0, FORMAT_OPTION},
  {"get", no_argument, 0, 'x'},
  {"group", required_argument, 0, GROUP_OPTION},
  {"gunzip", no_argument, 0, 'z'},
  {"gzip", no_argument, 0, 'z'},
  {"help", no_argument, &show_help, 1},
  {"ignore-case", no_argument, 0, IGNORE_CASE_OPTION},
  {"ignore-failed-read", no_argument, 0, IGNORE_FAILED_READ_OPTION},
  {"ignore-zeros", no_argument, 0, 'i'},
  /* FIXME: --ignore-end as a new name for --ignore-zeros?  */
  {"incremental", no_argument, 0, 'G'},
  {"index-file", required_argument, 0, INDEX_FILE_OPTION},
  {"info-script", required_argument, 0, 'F'},
  {"interactive", no_argument, 0, 'w'},
  {"keep-newer-files", no_argument, 0, KEEP_NEWER_FILES_OPTION},
  {"keep-old-files", no_argument, 0, 'k'},
  {"label", required_argument, 0, 'V'},
  {"list", no_argument, 0, 't'},
  {"listed-incremental", required_argument, 0, 'g'},
  {"mode", required_argument, 0, MODE_OPTION},
  {"multi-volume", no_argument, 0, 'M'},
  {"new-volume-script", required_argument, 0, 'F'},
  {"newer", required_argument, 0, 'N'},
  {"newer-mtime", required_argument, 0, NEWER_MTIME_OPTION},
  {"null", no_argument, 0, NULL_OPTION},
  {"no-anchored", no_argument, 0, NO_ANCHORED_OPTION},
  {"no-ignore-case", no_argument, 0, NO_IGNORE_CASE_OPTION},
  {"no-overwrite-dir", no_argument, 0, NO_OVERWRITE_DIR_OPTION},
  {"no-wildcards", no_argument, 0, NO_WILDCARDS_OPTION},
  {"no-wildcards-match-slash", no_argument, 0, NO_WILDCARDS_MATCH_SLASH_OPTION},
  {"no-recursion", no_argument, &recursion_option, 0},
  {"no-same-owner", no_argument, &same_owner_option, -1},
  {"no-same-permissions", no_argument, &same_permissions_option, -1},
  {"numeric-owner", no_argument, 0, NUMERIC_OWNER_OPTION},
  {"occurrence", optional_argument, 0, OCCURRENCE_OPTION},
  {"old-archive", no_argument, 0, 'o'},
  {"one-file-system", no_argument, 0, 'l'},
  {"overwrite", no_argument, 0, OVERWRITE_OPTION},
  {"owner", required_argument, 0, OWNER_OPTION},
  {"pax-option", required_argument, 0, PAX_OPTION},
  {"portability", no_argument, 0, 'o'},
  {"posix", no_argument, 0, POSIX_OPTION},
  {"preserve", no_argument, 0, PRESERVE_OPTION},
  {"preserve-order", no_argument, 0, 's'},
  {"preserve-permissions", no_argument, 0, 'p'},
  {"recursion", no_argument, &recursion_option, FNM_LEADING_DIR},
  {"recursive-unlink", no_argument, 0, RECURSIVE_UNLINK_OPTION},
  {"read-full-records", no_argument, 0, 'B'},
  /* FIXME: --partial-blocks might be a synonym for --read-full-records?  */
  {"record-size", required_argument, 0, RECORD_SIZE_OPTION},
  {"remove-files", no_argument, 0, REMOVE_FILES_OPTION},
  {"rsh-command", required_argument, 0, RSH_COMMAND_OPTION},
  {"same-order", no_argument, 0, 's'},
  {"same-owner", no_argument, &same_owner_option, 1},
  {"same-permissions", no_argument, 0, 'p'},
  {"show-defaults", no_argument, 0, SHOW_DEFAULTS_OPTION},
  {"show-omitted-dirs", no_argument, 0, SHOW_OMITTED_DIRS_OPTION},
  {"sparse", no_argument, 0, 'S'},
  {"starting-file", required_argument, 0, 'K'},
  {"strip-path", required_argument, 0, STRIP_PATH_OPTION },
  {"suffix", required_argument, 0, SUFFIX_OPTION},
  {"tape-length", required_argument, 0, 'L'},
  {"to-stdout", no_argument, 0, 'O'},
  {"totals", no_argument, 0, TOTALS_OPTION},
  {"touch", no_argument, 0, 'm'},
  {"uncompress", no_argument, 0, 'Z'},
  {"ungzip", no_argument, 0, 'z'},
  {"unlink-first", no_argument, 0, 'U'},
  {"update", no_argument, 0, 'u'},
  {"utc", no_argument, 0, UTC_OPTION },
  {"use-compress-program", required_argument, 0, USE_COMPRESS_PROGRAM_OPTION},
  {"verbose", no_argument, 0, 'v'},
  {"verify", no_argument, 0, 'W'},
  {"version", no_argument, &show_version, 1},
  {"volno-file", required_argument, 0, VOLNO_FILE_OPTION},
  {"wildcards", no_argument, 0, WILDCARDS_OPTION},
  {"wildcards-match-slash", no_argument, 0, WILDCARDS_MATCH_SLASH_OPTION},
  
  {0, 0, 0, 0}
};

/* Print a usage message and exit with STATUS.  */
void
usage (int status)
{
  if (status != TAREXIT_SUCCESS)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      fputs (_("\
GNU `tar' saves many files together into a single tape or disk archive, and\n\
can restore individual files from the archive.\n"),
	     stdout);
      printf (_("\nUsage: %s [OPTION]... [FILE]...\n\
\n\
Examples:\n\
  %s -cf archive.tar foo bar  # Create archive.tar from files foo and bar.\n\
  %s -tvf archive.tar         # List all files in archive.tar verbosely.\n\
  %s -xf archive.tar          # Extract all files from archive.tar.\n"),
	     program_name, program_name, program_name, program_name);
      fputs (_("\
\n\
If a long option shows an argument as mandatory, then it is mandatory\n\
for the equivalent short option also.  Similarly for optional arguments.\n"),
	     stdout);
      fputs(_("\
\n\
Main operation mode:\n\
  -t, --list              list the contents of an archive\n\
  -x, --extract, --get    extract files from an archive\n\
  -c, --create            create a new archive\n\
  -d, --diff, --compare   find differences between archive and file system\n\
  -r, --append            append files to the end of an archive\n\
  -u, --update            only append files newer than copy in archive\n\
  -A, --catenate          append tar files to an archive\n\
      --concatenate       same as -A\n\
      --delete            delete from the archive (not on mag tapes!)\n"),
	    stdout);
      fputs (_("\
\n\
Operation modifiers:\n\
  -W, --verify               attempt to verify the archive after writing it\n\
      --remove-files         remove files after adding them to the archive\n\
  -k, --keep-old-files       don't replace existing files when extracting\n\
      --keep-newer-files     don't replace existing files that are newer\n\
                             than their archive copies\n\
      --overwrite            overwrite existing files when extracting\n\
      --no-overwrite-dir     preserve metadata of existing directories\n\
  -U, --unlink-first         remove each file prior to extracting over it\n\
      --recursive-unlink     empty hierarchies prior to extracting directory\n\
  -S, --sparse               handle sparse files efficiently\n\
  -O, --to-stdout            extract files to standard output\n\
  -G, --incremental          handle old GNU-format incremental backup\n\
  -g, --listed-incremental=FILE\n\
                             handle new GNU-format incremental backup\n\
      --ignore-failed-read   do not exit with nonzero on unreadable files\n\
      --occurrence[=NUM]     process only the NUMth occurrence of each file in\n\
                             the archive. This option is valid only in\n\
                             conjunction with one of the subcommands --delete,\n\
                             --diff, --extract or --list and when a list of\n\
                             files is given either on the command line or\n\
                             via -T option.\n\
                             NUM defaults to 1.\n"),
	     stdout);
      fputs (_("\
\n\
Handling of file attributes:\n\
      --owner=NAME             force NAME as owner for added files\n\
      --group=NAME             force NAME as group for added files\n\
      --mode=CHANGES           force (symbolic) mode CHANGES for added files\n\
      --atime-preserve         don't change access times on dumped files\n\
  -m, --modification-time      don't extract file modified time\n\
      --same-owner             try extracting files with the same ownership\n\
      --no-same-owner          extract files as yourself\n\
      --numeric-owner          always use numbers for user/group names\n\
  -p, --same-permissions       extract permissions information\n\
      --no-same-permissions    do not extract permissions information\n\
      --preserve-permissions   same as -p\n\
  -s, --same-order             sort names to extract to match archive\n\
      --preserve-order         same as -s\n\
      --preserve               same as both -p and -s\n"),
	     stdout);
      fputs (_("\
\n\
Device selection and switching:\n\
  -f, --file=ARCHIVE             use archive file or device ARCHIVE\n\
      --force-local              archive file is local even if has a colon\n\
      --rsh-command=COMMAND      use remote COMMAND instead of rsh\n\
  -[0-7][lmh]                    specify drive and density\n\
  -M, --multi-volume             create/list/extract multi-volume archive\n\
  -L, --tape-length=NUM          change tape after writing NUM x 1024 bytes\n\
  -F, --info-script=FILE         run script at end of each tape (implies -M)\n\
      --new-volume-script=FILE   same as -F FILE\n\
      --volno-file=FILE          use/update the volume number in FILE\n"),
	     stdout);
      fputs (_("\
\n\
Device blocking:\n\
  -b, --blocking-factor=BLOCKS   BLOCKS x 512 bytes per record\n\
      --record-size=SIZE         SIZE bytes per record, multiple of 512\n\
  -i, --ignore-zeros             ignore zeroed blocks in archive (means EOF)\n\
  -B, --read-full-records        reblock as we read (for 4.2BSD pipes)\n"),
	     stdout);
      fputs (_("\
\n\
Archive format selection:\n\
      --format=FMTNAME               create archive of the given format.\n\
                                     FMTNAME is one of the following:\n\
                                     v7        old V7 tar format\n\
                                     oldgnu    GNU format as per tar <= 1.12\n\
                                     gnu       GNU tar 1.13 format\n\
                                     ustar     POSIX 1003.1-1988 (ustar) format\n\
                                     posix     POSIX 1003.1-2001 (pax) format\n\
      --old-archive, --portability   same as --format=v7\n\
      --posix                        same as --format=posix\n\
  --pax-option keyword[[:]=value][,keyword[[:]=value], ...]\n\
                                     control pax keywords\n\
  -V, --label=NAME                   create archive with volume name NAME\n\
              PATTERN                at list/extract time, a globbing PATTERN\n\
  -j, --bzip2                        filter the archive through bzip2\n\
  -z, --gzip, --ungzip               filter the archive through gzip\n\
  -Z, --compress, --uncompress       filter the archive through compress\n\
      --use-compress-program=PROG    filter through PROG (must accept -d)\n"),
	     stdout);
      fputs (_("\
\n\
Local file selection:\n\
  -C, --directory=DIR          change to directory DIR\n\
  -T, --files-from=NAME        get names to extract or create from file NAME\n\
      --null                   -T reads null-terminated names, disable -C\n\
      --exclude=PATTERN        exclude files, given as a PATTERN\n\
  -X, --exclude-from=FILE      exclude patterns listed in FILE\n\
      --anchored               exclude patterns match file name start (default)\n\
      --no-anchored            exclude patterns match after any /\n\
      --ignore-case            exclusion ignores case\n\
      --no-ignore-case         exclusion is case sensitive (default)\n\
      --wildcards              exclude patterns use wildcards (default)\n\
      --no-wildcards           exclude patterns are plain strings\n\
      --wildcards-match-slash  exclude pattern wildcards match '/' (default)\n\
      --no-wildcards-match-slash exclude pattern wildcards do not match '/'\n\
  -P, --absolute-names         don't strip leading `/'s from file names\n\
  -h, --dereference            dump instead the files symlinks point to\n\
      --no-recursion           avoid descending automatically in directories\n\
  -l, --one-file-system        stay in local file system when creating archive\n\
  -K, --starting-file=NAME     begin at file NAME in the archive\n\
      --strip-path=NUM         strip NUM leading components from file names\n\
                               before extraction\n"),
	     stdout);
#if !MSDOS
      fputs (_("\
  -N, --newer=DATE-OR-FILE     only store files newer than DATE-OR-FILE\n\
      --newer-mtime=DATE       compare date and time when data changed only\n\
      --after-date=DATE        same as -N\n"),
	     stdout);
#endif
      fputs (_("\
      --backup[=CONTROL]       backup before removal, choose version control\n\
      --suffix=SUFFIX          backup before removal, override usual suffix\n"),
	     stdout);
      fputs (_("\
\n\
Informative output:\n\
      --help            print this help, then exit\n\
      --version         print tar program version number, then exit\n\
  -v, --verbose         verbosely list files processed\n\
      --checkpoint      print directory names while reading the archive\n\
      --check-links     print a message if not all links are dumped\n\
      --totals          print total bytes written while creating archive\n\
      --index-file=FILE send verbose output to FILE\n\
      --utc             print file modification dates in UTC\n\
  -R, --block-number    show block number within archive with each message\n\
  -w, --interactive     ask for confirmation for every action\n\
      --confirmation    same as -w\n"),
	     stdout);
      fputs (_("\
\n\
Compatibility options:\n\
  -o                                 when creating, same as --old-archive\n\
                                     when extracting, same as --no-same-owner\n"),
             stdout);
      
      fputs (_("\
\n\
The backup suffix is `~', unless set with --suffix or SIMPLE_BACKUP_SUFFIX.\n\
The version control may be set with --backup or VERSION_CONTROL, values are:\n\
\n\
  t, numbered     make numbered backups\n\
  nil, existing   numbered if numbered backups exist, simple otherwise\n\
  never, simple   always make simple backups\n"),
	     stdout);
      printf (_("\
\n\
ARCHIVE may be FILE, HOST:FILE or USER@HOST:FILE; DATE may be a textual date\n\
or a file name starting with `/' or `.', in which case the file's date is used.\n\
*This* `tar' defaults to `--format=%s -f%s -b%d'.\n"),
	      archive_format_string (DEFAULT_ARCHIVE_FORMAT),
	      DEFAULT_ARCHIVE, DEFAULT_BLOCKING);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status);
}

/* Parse the options for tar.  */

/* Available option letters are DEHIJQY and aenqy.  Some are reserved:

   e  exit immediately with a nonzero exit status if unexpected errors occur
   E  use extended headers (draft POSIX headers, that is)
   I  same as T (for compatibility with Solaris tar)
   n  the archive is quickly seekable, so don't worry about random seeks
   q  stop after extracting the first occurrence of the named file
   y  per-file gzip compression
   Y  per-block gzip compression */

#define OPTION_STRING \
  "-01234567ABC:F:GIK:L:MN:OPRST:UV:WX:Zb:cdf:g:hijklmoprstuvwxyz"

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
  if (use_compress_program_option && strcmp (use_compress_program_option, string) != 0)
    USAGE_ERROR ((0, 0, _("Conflicting compression options")));

  use_compress_program_option = string;
}

static void
decode_options (int argc, char **argv)
{
  int optchar;			/* option letter */
  int input_files;		/* number of input files */
  char const *textual_date_option = 0;
  char const *backup_suffix_string;
  char const *version_control_string = 0;
  int exclude_options = EXCLUDE_WILDCARDS;
  bool o_option = 0;
  int pax_option = 0;

  /* Set some default option values.  */

  subcommand_option = UNKNOWN_SUBCOMMAND;
  archive_format = DEFAULT_FORMAT;
  blocking_factor = DEFAULT_BLOCKING;
  record_size = DEFAULT_BLOCKING * BLOCKSIZE;
  excluded = new_exclude ();
  newer_mtime_option = TYPE_MINIMUM (time_t);
  recursion_option = FNM_LEADING_DIR;

  owner_option = -1;
  group_option = -1;

  backup_suffix_string = getenv ("SIMPLE_BACKUP_SUFFIX");

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
      const char *cursor;	/* cursor in OPTION_STRING */

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
	  buffer[1] = *letter;
	  *out++ = xstrdup (buffer);
	  cursor = strchr (OPTION_STRING, *letter);
	  if (cursor && cursor[1] == ':')
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

  input_files = 0;

  prepend_default_options (getenv ("TAR_OPTIONS"), &argc, &argv);

  while (optchar = getopt_long (argc, argv, OPTION_STRING, long_options, 0),
	 optchar != -1)
    switch (optchar)
      {
      case '?':
	usage (TAREXIT_FAILURE);

      case 0:
	break;

      case 1:
	/* File name or non-parsed option, because of RETURN_IN_ORDER
	   ordering triggered by the leading dash in OPTION_STRING.  */

	name_add (optarg);
	input_files++;
	break;

      case 'A':
	set_subcommand_option (CAT_SUBCOMMAND);
	break;

      case 'b':
	{
	  uintmax_t u;
	  if (! (xstrtoumax (optarg, 0, 10, &u, "") == LONGINT_OK
		 && u == (blocking_factor = u)
		 && 0 < blocking_factor
		 && u == (record_size = u * BLOCKSIZE) / BLOCKSIZE))
	    USAGE_ERROR ((0, 0, "%s: %s", quotearg_colon (optarg),
			  _("Invalid blocking factor")));
	}
	break;

      case 'B':
	/* Try to reblock input records.  For reading 4.2BSD pipes.  */

	/* It would surely make sense to exchange -B and -R, but it seems
	   that -B has been used for a long while in Sun tar ans most
	   BSD-derived systems.  This is a consequence of the block/record
	   terminology confusion.  */

	read_full_records_option = true;
	break;

      case 'c':
	set_subcommand_option (CREATE_SUBCOMMAND);
	break;

      case 'C':
	name_add ("-C");
	name_add (optarg);
	break;

      case 'd':
	set_subcommand_option (DIFF_SUBCOMMAND);
	break;

      case 'f':
	if (archive_names == allocated_archive_names)
	  {
	    allocated_archive_names *= 2;
	    archive_name_array =
	      xrealloc (archive_name_array,
			sizeof (const char *) * allocated_archive_names);
	  }
	archive_name_array[archive_names++] = optarg;
	break;

      case 'F':
	/* Since -F is only useful with -M, make it implied.  Run this
	   script at the end of each tape.  */

	info_script_option = optarg;
	multi_volume_option = true;
	break;

      case 'g':
	listed_incremental_option = optarg;
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
	addname (optarg, 0);
	break;

      case 'l':
	/* When dumping directories, don't dump files/subdirectories
	   that are on other filesystems.  */

	one_file_system_option = true;
	break;

      case 'L':
	{
	  uintmax_t u;
	  if (xstrtoumax (optarg, 0, 10, &u, "") != LONGINT_OK)
	    USAGE_ERROR ((0, 0, "%s: %s", quotearg_colon (optarg),
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

#if !MSDOS
      case 'N':
	after_date_option = true;
	/* Fall through.  */

      case NEWER_MTIME_OPTION:
	if (newer_mtime_option != TYPE_MINIMUM (time_t))
	  USAGE_ERROR ((0, 0, _("More than one threshold date")));

	if (FILESYSTEM_PREFIX_LEN (optarg) != 0
	    || ISSLASH (*optarg)
	    || *optarg == '.')
	  {
	    struct stat st;
	    if (deref_stat (dereference_option, optarg, &st) != 0)
	      {
		stat_error (optarg);
		USAGE_ERROR ((0, 0, _("Date file not found")));
	      }
	    newer_mtime_option = st.st_mtime;
	  }
	else
	  {
	    newer_mtime_option = get_date (optarg, 0);
	    if (newer_mtime_option == (time_t) -1)
	      WARN ((0, 0, _("Substituting %s for unknown date format %s"),
		     tartime (newer_mtime_option), quote (optarg)));
	    else
	      textual_date_option = optarg;
	  }

	break;
#endif /* not MSDOS */

      case 'o':
	o_option = true;
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
	   that -B has been used for a long while in Sun tar ans most
	   BSD-derived systems.  This is a consequence of the block/record
	   terminology confusion.  */

	block_number_option = true;
	break;

      case 's':
	/* Names to extr are sorted.  */

	same_order_option = true;
	break;

      case 'S':
	sparse_option = true;
	break;

      case 't':
	set_subcommand_option (LIST_SUBCOMMAND);
	verbose_option++;
	break;

      case 'T':
	files_from_option = optarg;
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
	volume_label_option = optarg;
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
	if (add_exclude_file (add_exclude, excluded, optarg,
			      exclude_options | recursion_option, '\n')
	    != 0)
	  {
	    int e = errno;
	    FATAL_ERROR ((0, e, "%s", quotearg_colon (optarg)));
	  }
	break;

      case 'y':
	USAGE_ERROR ((0, 0,
		      _("Warning: the -y option is not supported;"
			" perhaps you meant -j?")));
	break;

      case 'z':
	set_use_compress_program_option ("gzip");
	break;

      case 'Z':
	set_use_compress_program_option ("compress");
	break;

      case ANCHORED_OPTION:
	exclude_options |= EXCLUDE_ANCHORED;
	break;

      case ATIME_PRESERVE_OPTION:
	atime_preserve_option = true;
	break;

      case CHECKPOINT_OPTION:
	checkpoint_option = true;
	break;

      case BACKUP_OPTION:
	backup_option = true;
	if (optarg)
	  version_control_string = optarg;
	break;

      case DELETE_OPTION:
	set_subcommand_option (DELETE_SUBCOMMAND);
	break;

      case EXCLUDE_OPTION:
	add_exclude (excluded, optarg, exclude_options | recursion_option);
	break;

      case FORCE_LOCAL_OPTION:
	force_local_option = true;
	break;

      case FORMAT_OPTION:
	set_archive_format (optarg);
	break;
	
      case INDEX_FILE_OPTION:
	index_file_name = optarg;
	break;

      case IGNORE_CASE_OPTION:
	exclude_options |= FNM_CASEFOLD;
	break;

      case IGNORE_FAILED_READ_OPTION:
	ignore_failed_read_option = true;
	break;

      case KEEP_NEWER_FILES_OPTION:
	old_files_option = KEEP_NEWER_FILES;
	break;
	
      case GROUP_OPTION:
	if (! (strlen (optarg) < GNAME_FIELD_SIZE
	       && gname_to_gid (optarg, &group_option)))
	  {
	    uintmax_t g;
	    if (xstrtoumax (optarg, 0, 10, &g, "") == LONGINT_OK
		&& g == (gid_t) g)
	      group_option = g;
	    else
	      FATAL_ERROR ((0, 0, "%s: %s", quotearg_colon (optarg),
			    _("%s: Invalid group")));
	  }
	break;

      case MODE_OPTION:
	mode_option
	  = mode_compile (optarg,
			  MODE_MASK_EQUALS | MODE_MASK_PLUS | MODE_MASK_MINUS);
	if (mode_option == MODE_INVALID)
	  FATAL_ERROR ((0, 0, _("Invalid mode given on option")));
	if (mode_option == MODE_MEMORY_EXHAUSTED)
	  xalloc_die ();
	break;

      case NO_ANCHORED_OPTION:
	exclude_options &= ~ EXCLUDE_ANCHORED;
	break;

      case NO_IGNORE_CASE_OPTION:
	exclude_options &= ~ FNM_CASEFOLD;
	break;

      case NO_OVERWRITE_DIR_OPTION:
	old_files_option = NO_OVERWRITE_DIR_OLD_FILES;
	break;

      case NO_WILDCARDS_OPTION:
	exclude_options &= ~ EXCLUDE_WILDCARDS;
	break;

      case NO_WILDCARDS_MATCH_SLASH_OPTION:
	exclude_options |= FNM_FILE_NAME;
	break;

      case NULL_OPTION:
	filename_terminator = '\0';
	break;

      case NUMERIC_OWNER_OPTION:
	numeric_owner_option = true;
	break;

      case OCCURRENCE_OPTION:
	if (!optarg)
	  occurrence_option = 1;
	else
	  {
	    uintmax_t u;
	    if (xstrtoumax (optarg, 0, 10, &u, "") == LONGINT_OK)
	      occurrence_option = u;
	    else
	      FATAL_ERROR ((0, 0, "%s: %s", quotearg_colon (optarg),
			    _("Invalid number")));
	  }
	break;
	
      case OVERWRITE_OPTION:
	old_files_option = OVERWRITE_OLD_FILES;
	break;

      case OWNER_OPTION:
	if (! (strlen (optarg) < UNAME_FIELD_SIZE
	       && uname_to_uid (optarg, &owner_option)))
	  {
	    uintmax_t u;
	    if (xstrtoumax (optarg, 0, 10, &u, "") == LONGINT_OK
		&& u == (uid_t) u)
	      owner_option = u;
	    else
	      FATAL_ERROR ((0, 0, "%s: %s", quotearg_colon (optarg),
			    _("Invalid owner")));
	  }
	break;

      case PAX_OPTION:
	pax_option++;
	xheader_set_option (optarg);
	break;
			    
      case POSIX_OPTION:
	set_archive_format ("posix");
	break;

      case PRESERVE_OPTION:
	same_permissions_option = true;
	same_order_option = true;
	break;

      case RECORD_SIZE_OPTION:
	{
	  uintmax_t u;
	  if (! (xstrtoumax (optarg, 0, 10, &u, "") == LONGINT_OK
		 && u == (size_t) u))
	    USAGE_ERROR ((0, 0, "%s: %s", quotearg_colon (optarg),
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

      case RSH_COMMAND_OPTION:
	rsh_command_option = optarg;
	break;

      case SHOW_DEFAULTS_OPTION:
	printf ("--format=%s -f%s -b%d\n",
		archive_format_string (DEFAULT_ARCHIVE_FORMAT),
		DEFAULT_ARCHIVE, DEFAULT_BLOCKING);
	exit(0);

      case STRIP_PATH_OPTION:
	{
	  uintmax_t u;
	  if (! (xstrtoumax (optarg, 0, 10, &u, "") == LONGINT_OK
		 && u == (size_t) u))
	    USAGE_ERROR ((0, 0, "%s: %s", quotearg_colon (optarg),
			  _("Invalid number of elements")));
	  strip_path_elements = u;
	}
	break;
	
      case SUFFIX_OPTION:
	backup_option = true;
	backup_suffix_string = optarg;
	break;

      case TOTALS_OPTION:
	totals_option = true;
	break;
	
      case USE_COMPRESS_PROGRAM_OPTION:
	set_use_compress_program_option (optarg);
	break;

      case VOLNO_FILE_OPTION:
	volno_file_option = optarg;
	break;

      case WILDCARDS_OPTION:
	exclude_options |= EXCLUDE_WILDCARDS;
	break;

      case WILDCARDS_MATCH_SLASH_OPTION:
	exclude_options &= ~ FNM_FILE_NAME;
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
	  int device = optchar - '0';
	  int density;
	  static char buf[sizeof DEVICE_PREFIX + 10];
	  char *cursor;

	  density = getopt_long (argc, argv, "lmh", 0, 0);
	  strcpy (buf, DEVICE_PREFIX);
	  cursor = buf + strlen (buf);

#ifdef DENSITY_LETTER

	  sprintf (cursor, "%d%c", device, density);

#else /* not DENSITY_LETTER */

	  switch (density)
	    {
	    case 'l':
#ifdef LOW_NUM
	      device += LOW_NUM;
#endif
	      break;

	    case 'm':
#ifdef MID_NUM
	      device += MID_NUM;
#else
	      device += 8;
#endif
	      break;

	    case 'h':
#ifdef HGH_NUM
	      device += HGH_NUM;
#else
	      device += 16;
#endif
	      break;

	    default:
	      usage (TAREXIT_FAILURE);
	    }
	  sprintf (cursor, "%d", device);

#endif /* not DENSITY_LETTER */

	  if (archive_names == allocated_archive_names)
	    {
	      allocated_archive_names *= 2;
	      archive_name_array =
		xrealloc (archive_name_array,
			  sizeof (const char *) * allocated_archive_names);
	    }
	  archive_name_array[archive_names++] = strdup (buf);
	}
	break;

#else /* not DEVICE_PREFIX */

	USAGE_ERROR ((0, 0,
		      _("Options `-[0-7][lmh]' not supported by *this* tar")));

#endif /* not DEVICE_PREFIX */
      }

  /* Special handling for 'o' option:

     GNU tar used to say "output old format".
     UNIX98 tar says don't chown files after extracting (we use
     "--no-same-owner" for this).

     The old GNU tar semantics is retained when used with --create
     option, otherwise UNIX98 semantics is assumed */

  if (o_option)
    {
      if (subcommand_option == CREATE_SUBCOMMAND)
	{
	  /* GNU Tar <= 1.13 compatibility */
	  set_archive_format ("v7");
	}
      else
	{
	  /* UNIX98 compatibility */
	  same_owner_option = 1;
	}
    }

  /* Handle operands after any "--" argument.  */
  for (; optind < argc; optind++)
    {
      name_add (argv[optind]);
      input_files++;
    }

  /* Process trivial options.  */

  if (show_version)
    {
      printf ("tar (%s) %s\n%s\n", PACKAGE_NAME, PACKAGE_VERSION,
	      "Copyright (C) 2003 Free Software Foundation, Inc.");
      puts (_("\
This program comes with NO WARRANTY, to the extent permitted by law.\n\
You may redistribute it under the terms of the GNU General Public License;\n\
see the file named COPYING for details."));

      puts (_("Written by John Gilmore and Jay Fenlason."));

      exit (TAREXIT_SUCCESS);
    }

  if (show_help)
    usage (TAREXIT_SUCCESS);

  /* Derive option values and check option consistency.  */

  if (archive_format == DEFAULT_FORMAT)
    {
      if (pax_option)
	archive_format = POSIX_FORMAT;
      else
	archive_format = DEFAULT_ARCHIVE_FORMAT;
    }
  
  if (volume_label_option && subcommand_option == CREATE_SUBCOMMAND)
    assert_format (FORMAT_MASK (OLDGNU_FORMAT)
		   | FORMAT_MASK (GNU_FORMAT));


  if (incremental_option || multi_volume_option)
    assert_format (FORMAT_MASK (OLDGNU_FORMAT) | FORMAT_MASK (GNU_FORMAT));
		   
  if (sparse_option)
    assert_format (FORMAT_MASK (OLDGNU_FORMAT)
		   | FORMAT_MASK (GNU_FORMAT)
		   | FORMAT_MASK (POSIX_FORMAT));
  
  if (occurrence_option)
    {
      if (!input_files && !files_from_option)
	USAGE_ERROR ((0, 0,
		      _("--occurrence is meaningless without a file list")));
      if (subcommand_option != DELETE_SUBCOMMAND
	  && subcommand_option != DIFF_SUBCOMMAND
	  && subcommand_option != EXTRACT_SUBCOMMAND
	  && subcommand_option != LIST_SUBCOMMAND)
	    USAGE_ERROR ((0, 0,
			  _("--occurrence cannot be used in the requested operation mode")));
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
      && newer_mtime_option != TYPE_MINIMUM (time_t))
    USAGE_ERROR ((0, 0,
		  _("Cannot combine --listed-incremental with --newer")));

  if (volume_label_option)
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
      if (subcommand_option == UPDATE_SUBCOMMAND)
	USAGE_ERROR ((0, 0, _("Cannot update compressed archives")));
    }

  /* It is no harm to use --pax-option on non-pax archives in archive
     reading mode. It may even be useful, since it allows to override
     file attributes from tar headers. Therefore I allow such usage.
     --gray */
  if (pax_option
      && archive_format != POSIX_FORMAT
      && (subcommand_option != EXTRACT_SUBCOMMAND
	  || subcommand_option != DIFF_SUBCOMMAND
	  || subcommand_option != LIST_SUBCOMMAND))
    USAGE_ERROR ((0, 0, _("--pax-option can be used only on POSIX archives")));
  
  /* If ready to unlink hierarchies, so we are for simpler files.  */
  if (recursive_unlink_option)
    old_files_option = UNLINK_FIRST_OLD_FILES;

  /* Forbid using -c with no input files whatsoever.  Check that `-f -',
     explicit or implied, is used correctly.  */

  switch (subcommand_option)
    {
    case CREATE_SUBCOMMAND:
      if (input_files == 0 && !files_from_option)
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

  archive_name_cursor = archive_name_array;

  /* Prepare for generating backup names.  */

  if (backup_suffix_string)
    simple_backup_suffix = xstrdup (backup_suffix_string);

  if (backup_option)
    backup_type = xget_version ("--backup", version_control_string);

  if (verbose_option && textual_date_option)
    {
      char const *treated_as = tartime (newer_mtime_option);
      if (strcmp (textual_date_option, treated_as) != 0)
	WARN ((0, 0, _("Treating date `%s' as %s"),
	       textual_date_option, treated_as));
    }
}


/* Tar proper.  */

/* Main routine for tar.  */
int
main (int argc, char **argv)
{
#if HAVE_CLOCK_GETTIME
  if (clock_gettime (CLOCK_REALTIME, &start_timespec) != 0)
#endif
    start_time = time (0);
  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  exit_status = TAREXIT_SUCCESS;
  filename_terminator = '\n';
  set_quoting_style (0, escape_quoting_style);

  /* Pre-allocate a few structures.  */

  allocated_archive_names = 10;
  archive_name_array =
    xmalloc (sizeof (const char *) * allocated_archive_names);
  archive_names = 0;

#ifdef SIGCHLD
  /* System V fork+wait does not work if SIGCHLD is ignored.  */
  signal (SIGCHLD, SIG_DFL);
#endif

  init_names ();

  /* Decode options.  */

  decode_options (argc, argv);
  name_init (argc, argv);

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
      name_close ();

      if (totals_option)
	print_total_written ();
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

  if (check_links_option)
      check_links ();

  if (volno_file_option)
    closeout_volume_number ();

  /* Dispose of allocated memory, and return.  */

  free (archive_name_array);
  name_term ();

  if (stdlis != stderr && (ferror (stdlis) || fclose (stdlis) != 0))
    FATAL_ERROR ((0, 0, _("Error in writing to standard output")));
  if (exit_status == TAREXIT_FAILURE)
    error (0, 0, _("Error exit delayed from previous errors"));
  if (ferror (stderr) || fclose (stderr) != 0)
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
  memset (st, 0, sizeof (*st));
}
