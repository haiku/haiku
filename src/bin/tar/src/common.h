/* Common declarations for the tar program.

   Copyright (C) 1988, 1992, 1993, 1994, 1996, 1997, 1999, 2000, 2001,
   2003 Free Software Foundation, Inc.

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

/* Declare the GNU tar archive format.  */
#include "tar.h"

/* The checksum field is filled with this while the checksum is computed.  */
#define CHKBLANKS	"        "	/* 8 blanks, no null */

/* Some constants from POSIX are given names.  */
#define NAME_FIELD_SIZE   100
#define PREFIX_FIELD_SIZE 155
#define UNAME_FIELD_SIZE   32
#define GNAME_FIELD_SIZE   32

/* FIXME */
#define MAXOCTAL11      017777777777L
#define MAXOCTAL7       07777777



/* Some various global definitions.  */

/* Name of file to use for interacting with user.  */
#if MSDOS
# define TTY_NAME "con"
#else
# define TTY_NAME "/dev/tty"
#endif

/* GLOBAL is defined to empty in tar.c only, and left alone in other *.c
   modules.  Here, we merely set it to "extern" if it is not already set.
   GNU tar does depend on the system loader to preset all GLOBAL variables to
   neutral (or zero) values, explicit initialization is usually not done.  */
#ifndef GLOBAL
# define GLOBAL extern
#endif

/* Exit status for GNU tar.  Let's try to keep this list as simple as
   possible.  -d option strongly invites a status different for unequal
   comparison and other errors.  */
GLOBAL int exit_status;

#define TAREXIT_SUCCESS 0
#define TAREXIT_DIFFERS 1
#define TAREXIT_FAILURE 2

/* Both WARN and ERROR write a message on stderr and continue processing,
   however ERROR manages so tar will exit unsuccessfully.  FATAL_ERROR
   writes a message on stderr and aborts immediately, with another message
   line telling so.  USAGE_ERROR works like FATAL_ERROR except that the
   other message line suggests trying --help.  All four macros accept a
   single argument of the form ((0, errno, _("FORMAT"), Args...)).  errno
   is zero when the error is not being detected by the system.  */

#define WARN(Args) \
  error Args
#define ERROR(Args) \
  (error Args, exit_status = TAREXIT_FAILURE)
#define FATAL_ERROR(Args) \
  (error Args, fatal_exit ())
#define USAGE_ERROR(Args) \
  (error Args, usage (TAREXIT_FAILURE))

#include "arith.h"
#include <backupfile.h>
#include <exclude.h>
#include <full-write.h>
#include <modechange.h>
#include <quote.h>
#include <safe-read.h>

/* Log base 2 of common values.  */
#define LG_8 3
#define LG_64 6
#define LG_256 8

/* Information gleaned from the command line.  */

/* Name of this program.  */
GLOBAL const char *program_name;

/* Main command option.  */

enum subcommand
{
  UNKNOWN_SUBCOMMAND,		/* none of the following */
  APPEND_SUBCOMMAND,		/* -r */
  CAT_SUBCOMMAND,		/* -A */
  CREATE_SUBCOMMAND,		/* -c */
  DELETE_SUBCOMMAND,		/* -D */
  DIFF_SUBCOMMAND,		/* -d */
  EXTRACT_SUBCOMMAND,		/* -x */
  LIST_SUBCOMMAND,		/* -t */
  UPDATE_SUBCOMMAND		/* -u */
};

GLOBAL enum subcommand subcommand_option;

/* Selected format for output archive.  */
GLOBAL enum archive_format archive_format;

/* Either NL or NUL, as decided by the --null option.  */
GLOBAL char filename_terminator;

/* Size of each record, once in blocks, once in bytes.  Those two variables
   are always related, the second being BLOCKSIZE times the first.  They do
   not have _option in their name, even if their values is derived from
   option decoding, as these are especially important in tar.  */
GLOBAL int blocking_factor;
GLOBAL size_t record_size;

GLOBAL bool absolute_names_option;

/* Display file times in UTC */
GLOBAL bool utc_option;

/* This variable tells how to interpret newer_mtime_option, below.  If zero,
   files get archived if their mtime is not less than newer_mtime_option.
   If nonzero, files get archived if *either* their ctime or mtime is not less
   than newer_mtime_option.  */
GLOBAL int after_date_option;

GLOBAL bool atime_preserve_option;

GLOBAL bool backup_option;

/* Type of backups being made.  */
GLOBAL enum backup_type backup_type;

GLOBAL bool block_number_option;

GLOBAL bool checkpoint_option;

/* Specified name of compression program, or "gzip" as implied by -z.  */
GLOBAL const char *use_compress_program_option;

GLOBAL bool dereference_option;

/* Print a message if not all links are dumped */
GLOBAL int check_links_option;

/* Patterns that match file names to be excluded.  */
GLOBAL struct exclude *excluded;

/* Specified file containing names to work on.  */
GLOBAL const char *files_from_option;

GLOBAL bool force_local_option;

/* Specified value to be put into tar file in place of stat () results, or
   just -1 if such an override should not take place.  */
GLOBAL gid_t group_option;

GLOBAL bool ignore_failed_read_option;

GLOBAL bool ignore_zeros_option;

GLOBAL bool incremental_option;

/* Specified name of script to run at end of each tape change.  */
GLOBAL const char *info_script_option;

GLOBAL bool interactive_option;

/* If nonzero, extract only Nth occurrence of each named file */
GLOBAL uintmax_t occurrence_option;

enum old_files
{
  DEFAULT_OLD_FILES,          /* default */
  NO_OVERWRITE_DIR_OLD_FILES, /* --no-overwrite-dir */
  OVERWRITE_OLD_FILES,        /* --overwrite */
  UNLINK_FIRST_OLD_FILES,     /* --unlink-first */
  KEEP_OLD_FILES,             /* --keep-old-files */
  KEEP_NEWER_FILES,           /* --keep-newer-files */
};
GLOBAL enum old_files old_files_option;

/* Specified file name for incremental list.  */
GLOBAL const char *listed_incremental_option;

/* Specified mode change string.  */
GLOBAL struct mode_change *mode_option;

GLOBAL bool multi_volume_option;

/* The same variable hold the time, whether mtime or ctime.  Just fake a
   non-existing option, for making the code clearer, elsewhere.  */
#define newer_ctime_option newer_mtime_option

/* Specified threshold date and time.  Files having an older time stamp
   do not get archived (also see after_date_option above).  */
GLOBAL time_t newer_mtime_option;

/* Zero if there is no recursion, otherwise FNM_LEADING_DIR.  */
GLOBAL int recursion_option;

GLOBAL bool numeric_owner_option;

GLOBAL bool one_file_system_option;

/* Specified value to be put into tar file in place of stat () results, or
   just -1 if such an override should not take place.  */
GLOBAL uid_t owner_option;

GLOBAL bool recursive_unlink_option;

GLOBAL bool read_full_records_option;

GLOBAL bool remove_files_option;

/* Specified remote shell command.  */
GLOBAL const char *rsh_command_option;

GLOBAL bool same_order_option;

/* If positive, preserve ownership when extracting.  */
GLOBAL int same_owner_option;

/* If positive, preserve permissions when extracting.  */
GLOBAL int same_permissions_option;

/* When set, strip the given number of path elements from the file name
   before extracting */
GLOBAL size_t strip_path_elements;

GLOBAL bool show_omitted_dirs_option;

GLOBAL bool sparse_option;

GLOBAL bool starting_file_option;

/* Specified maximum byte length of each tape volume (multiple of 1024).  */
GLOBAL tarlong tape_length_option;

GLOBAL bool to_stdout_option;

GLOBAL bool totals_option;

GLOBAL bool touch_option;

/* Count how many times the option has been set, multiple setting yields
   more verbose behavior.  Value 0 means no verbosity, 1 means file name
   only, 2 means file name and all attributes.  More than 2 is just like 2.  */
GLOBAL int verbose_option;

GLOBAL bool verify_option;

/* Specified name of file containing the volume number.  */
GLOBAL const char *volno_file_option;

/* Specified value or pattern.  */
GLOBAL const char *volume_label_option;

/* Other global variables.  */

/* File descriptor for archive file.  */
GLOBAL int archive;

/* Nonzero when outputting to /dev/null.  */
GLOBAL bool dev_null_output;

/* Timestamp for when we started execution.  */
#if HAVE_CLOCK_GETTIME
  GLOBAL struct timespec start_timespec;
# define start_time (start_timespec.tv_sec)
#else
  GLOBAL time_t start_time;
#endif

GLOBAL struct tar_stat_info current_stat_info;

/* List of tape drive names, number of such tape drives, allocated number,
   and current cursor in list.  */
GLOBAL const char **archive_name_array;
GLOBAL int archive_names;
GLOBAL int allocated_archive_names;
GLOBAL const char **archive_name_cursor;

/* Output index file name.  */
GLOBAL char const *index_file_name;

/* Structure for keeping track of filenames and lists thereof.  */
struct name
  {
    struct name *next;
    size_t length;		/* cached strlen(name) */
    uintmax_t found_count;	/* number of times a matching file has
				   been found */
    int isdir;
    char firstch;		/* first char is literally matched */
    char regexp;		/* this name is a regexp, not literal */
    int change_dir;		/* set with the -C option */
    char const *dir_contents;	/* for incremental_option */
    char fake;			/* dummy entry */
    char name[1];
  };

/* Obnoxious test to see if dimwit is trying to dump the archive.  */
GLOBAL dev_t ar_dev;
GLOBAL ino_t ar_ino;


/* Declarations for each module.  */

/* FIXME: compare.c should not directly handle the following variable,
   instead, this should be done in buffer.c only.  */

enum access_mode
{
  ACCESS_READ,
  ACCESS_WRITE,
  ACCESS_UPDATE
};
extern enum access_mode access_mode;

/* Module buffer.c.  */

extern FILE *stdlis;
extern char *save_name;
extern off_t save_sizeleft;
extern off_t save_totsize;
extern bool write_archive_to_stdout;

size_t available_space_after (union block *);
off_t current_block_ordinal (void);
void close_archive (void);
void closeout_volume_number (void);
union block *find_next_block (void);
void flush_read (void);
void flush_write (void);
void flush_archive (void);
void init_volume_number (void);
void open_archive (enum access_mode);
void print_total_written (void);
void reset_eof (void);
void set_next_block_after (union block *);
void clear_read_error_count (void);
void xclose (int fd);
void archive_write_error (ssize_t) __attribute__ ((noreturn));
void archive_read_error (void);

/* Module create.c.  */

enum dump_status
  {
    dump_status_ok,
    dump_status_short,
    dump_status_fail,
    dump_status_not_implemented
  };

bool file_dumpable_p (struct tar_stat_info *stat);
void create_archive (void);
void pad_archive (off_t size_left);
void dump_file (char *, int, dev_t);
union block *start_header (struct tar_stat_info *st);
void finish_header (struct tar_stat_info *, union block *, off_t);
void simple_finish_header (union block *header);
union block *start_private_header (const char *name, size_t size);
void write_eot (void);
void check_links (void);

#define GID_TO_CHARS(val, where) gid_to_chars (val, where, sizeof (where))
#define MAJOR_TO_CHARS(val, where) major_to_chars (val, where, sizeof (where))
#define MINOR_TO_CHARS(val, where) minor_to_chars (val, where, sizeof (where))
#define MODE_TO_CHARS(val, where) mode_to_chars (val, where, sizeof (where))
#define OFF_TO_CHARS(val, where) off_to_chars (val, where, sizeof (where))
#define SIZE_TO_CHARS(val, where) size_to_chars (val, where, sizeof (where))
#define TIME_TO_CHARS(val, where) time_to_chars (val, where, sizeof (where))
#define UID_TO_CHARS(val, where) uid_to_chars (val, where, sizeof (where))
#define UINTMAX_TO_CHARS(val, where) uintmax_to_chars (val, where, sizeof (where))
#define UNAME_TO_CHARS(name,buf) string_to_chars (name, buf, sizeof(buf))
#define GNAME_TO_CHARS(name,buf) string_to_chars (name, buf, sizeof(buf))

void gid_to_chars (gid_t, char *, size_t);
void major_to_chars (major_t, char *, size_t);
void minor_to_chars (minor_t, char *, size_t);
void mode_to_chars (mode_t, char *, size_t);
void off_to_chars (off_t, char *, size_t);
void size_to_chars (size_t, char *, size_t);
void time_to_chars (time_t, char *, size_t);
void uid_to_chars (uid_t, char *, size_t);
void uintmax_to_chars (uintmax_t, char *, size_t);
void string_to_chars (char *, char *, size_t);

/* Module diffarch.c.  */

extern bool now_verifying;

void diff_archive (void);
void diff_init (void);
void verify_volume (void);

/* Module extract.c.  */

extern bool we_are_root;
void extr_init (void);
void extract_archive (void);
void extract_finish (void);
void fatal_exit (void) __attribute__ ((noreturn));

/* Module delete.c.  */

void delete_archive_members (void);

/* Module incremen.c.  */

char *get_directory_contents (char *, dev_t);
void read_directory_file (void);
void write_directory_file (void);
void gnu_restore (char const *);

/* Module list.c.  */

enum read_header
{
  HEADER_STILL_UNREAD,		/* for when read_header has not been called */
  HEADER_SUCCESS,		/* header successfully read and checksummed */
  HEADER_SUCCESS_EXTENDED,	/* likewise, but we got an extended header */
  HEADER_ZERO_BLOCK,		/* zero block where header expected */
  HEADER_END_OF_FILE,		/* true end of file while header expected */
  HEADER_FAILURE		/* ill-formed header, or bad checksum */
};

struct xheader
{
  struct obstack *stk;
  size_t size;
  char *buffer;
};

GLOBAL struct xheader extended_header;
extern union block *current_header;
extern enum archive_format current_format;
extern size_t recent_long_name_blocks;
extern size_t recent_long_link_blocks;

void decode_header (union block *, struct tar_stat_info *,
		    enum archive_format *, int);
#define STRINGIFY_BIGINT(i, b) \
  stringify_uintmax_t_backwards ((uintmax_t) (i), (b) + UINTMAX_STRSIZE_BOUND)
char *stringify_uintmax_t_backwards (uintmax_t, char *);
char const *tartime (time_t);

#define GID_FROM_HEADER(where) gid_from_header (where, sizeof (where))
#define MAJOR_FROM_HEADER(where) major_from_header (where, sizeof (where))
#define MINOR_FROM_HEADER(where) minor_from_header (where, sizeof (where))
#define MODE_FROM_HEADER(where) mode_from_header (where, sizeof (where))
#define OFF_FROM_HEADER(where) off_from_header (where, sizeof (where))
#define SIZE_FROM_HEADER(where) size_from_header (where, sizeof (where))
#define TIME_FROM_HEADER(where) time_from_header (where, sizeof (where))
#define UID_FROM_HEADER(where) uid_from_header (where, sizeof (where))
#define UINTMAX_FROM_HEADER(where) uintmax_from_header (where, sizeof (where))

gid_t gid_from_header (const char *, size_t);
major_t major_from_header (const char *, size_t);
minor_t minor_from_header (const char *, size_t);
mode_t mode_from_header (const char *, size_t);
off_t off_from_header (const char *, size_t);
size_t size_from_header (const char *, size_t);
time_t time_from_header (const char *, size_t);
uid_t uid_from_header (const char *, size_t);
uintmax_t uintmax_from_header (const char *, size_t);

void list_archive (void);
void print_for_mkdir (char *, int, mode_t);
void print_header (struct tar_stat_info *, off_t);
void read_and (void (*) (void));
enum read_header read_header (bool);
void skip_file (off_t);
void skip_member (void);

/* Module mangle.c.  */

void extract_mangle (void);

/* Module misc.c.  */

void assign_string (char **, const char *);
char *quote_copy_string (const char *);
int unquote_string (char *);

size_t dot_dot_prefix_len (char const *);

enum remove_option
{
  ORDINARY_REMOVE_OPTION,
  RECURSIVE_REMOVE_OPTION,
  WANT_DIRECTORY_REMOVE_OPTION
};
int remove_any_file (const char *, enum remove_option);
bool maybe_backup_file (const char *, int);
void undo_last_backup (void);

int deref_stat (bool, char const *, struct stat *);

int chdir_arg (char const *);
void chdir_do (int);

void decode_mode (mode_t, char *);

void chdir_fatal (char const *) __attribute__ ((noreturn));
void chmod_error_details (char const *, mode_t);
void chown_error_details (char const *, uid_t, gid_t);
void close_error (char const *);
void close_warn (char const *);
void close_diag (char const *name);
void exec_fatal (char const *) __attribute__ ((noreturn));
void link_error (char const *, char const *);
void mkdir_error (char const *);
void mkfifo_error (char const *);
void mknod_error (char const *);
void open_error (char const *);
void open_fatal (char const *) __attribute__ ((noreturn));
void open_warn (char const *);
void open_diag (char const *name);
void read_error (char const *);
void read_error_details (char const *, off_t, size_t);
void read_fatal (char const *) __attribute__ ((noreturn));
void read_fatal_details (char const *, off_t, size_t);
void read_warn_details (char const *, off_t, size_t);
void read_diag_details (char const *name, off_t offset, size_t size);
void readlink_error (char const *);
void readlink_warn (char const *);
void readlink_diag (char const *name);
void savedir_error (char const *);
void savedir_warn (char const *);
void savedir_diag (char const *name);
void seek_error (char const *);
void seek_error_details (char const *, off_t);
void seek_warn (char const *);
void seek_warn_details (char const *, off_t);
void seek_diag_details (char const *, off_t);
void stat_error (char const *);
void stat_warn (char const *);
void stat_diag (char const *name);
void symlink_error (char const *, char const *);
void truncate_error (char const *);
void truncate_warn (char const *);
void unlink_error (char const *);
void utime_error (char const *);
void waitpid_error (char const *);
void write_error (char const *);
void write_error_details (char const *, ssize_t, size_t);
void write_fatal (char const *) __attribute__ ((noreturn));
void write_fatal_details (char const *, ssize_t, size_t)
     __attribute__ ((noreturn));

pid_t xfork (void);
void xpipe (int[2]);

/* Module names.c.  */

extern struct name *gnu_list_name;

void gid_to_gname (gid_t, char **gname);
int gname_to_gid (char *gname, gid_t *);
void uid_to_uname (uid_t, char **uname);
int uname_to_uid (char *uname, uid_t *);

void init_names (void);
void name_add (const char *);
void name_init (int, char *const *);
void name_term (void);
char *name_next (int);
void name_close (void);
void name_gather (void);
struct name *addname (char const *, int);
int name_match (const char *);
void names_notfound (void);
void collect_and_sort_names (void);
struct name *name_scan (const char *);
char *name_from_list (void);
void blank_name_list (void);
char *new_name (const char *, const char *);
char *safer_name_suffix (char const *, bool);
size_t stripped_prefix_len (char const *file_name, size_t num);
bool all_names_found (struct tar_stat_info *);

bool excluded_name (char const *);

void add_avoided_name (char const *);
bool is_avoided_name (char const *);

bool contains_dot_dot (char const *);

#define ISFOUND(c) ((occurrence_option == 0) ? (c)->found_count : \
                    (c)->found_count == occurrence_option)
#define WASFOUND(c) ((occurrence_option == 0) ? (c)->found_count : \
                     (c)->found_count >= occurrence_option)

/* Module tar.c.  */

int confirm (const char *, const char *);
void request_stdin (const char *);

void tar_stat_init (struct tar_stat_info *st);
void tar_stat_destroy (struct tar_stat_info *st);
void usage (int) __attribute__ ((noreturn));

/* Module update.c.  */

extern char *output_start;

void update_archive (void);

/* Module xheader.c.  */

void xheader_decode (struct tar_stat_info *);
void xheader_decode_global (void);
void xheader_store (char const *, struct tar_stat_info const *, void *);
void xheader_read (union block *, size_t);
void xheader_write (char type, char *name, struct xheader *xhdr);
void xheader_write_global (void);
void xheader_finish (struct xheader *);
void xheader_destroy (struct xheader *);
char *xheader_xhdr_name (struct tar_stat_info *st);
char *xheader_ghdr_name (void);
void xheader_set_option (char *string);

/* Module system.c */

void sys_stat_nanoseconds (struct tar_stat_info *stat);
void sys_detect_dev_null_output (void);
void sys_save_archive_dev_ino (void);
void sys_drain_input_pipe (void);
void sys_wait_for_child (pid_t);
void sys_spawn_shell (void);
bool sys_compare_uid (struct stat *a, struct stat *b);
bool sys_compare_gid (struct stat *a, struct stat *b);
bool sys_file_is_archive (struct tar_stat_info *p);
bool sys_compare_links (struct stat *link_data, struct stat *stat_data);
int sys_truncate (int fd);
void sys_reset_uid_gid (void);
pid_t sys_child_open_for_compress (void);
pid_t sys_child_open_for_uncompress (void);
ssize_t sys_write_archive_buffer (void);
bool sys_get_archive_stat (void);
void sys_reset_uid_gid (void);

/* Module compare.c */
void report_difference (struct tar_stat_info *st, const char *message, ...);

/* Module sparse.c */
bool sparse_file_p (struct tar_stat_info *stat);
bool sparse_member_p (struct tar_stat_info *stat);
bool sparse_fixup_header (struct tar_stat_info *stat);
enum dump_status sparse_dump_file (int fd, struct tar_stat_info *stat);
enum dump_status sparse_extract_file (int fd, struct tar_stat_info *stat, off_t *size);
enum dump_status sparse_skip_file (struct tar_stat_info *stat);
bool sparse_diff_file (int fd, struct tar_stat_info *stat);

/* Module utf8.c */
bool string_ascii_p (const char *str);
bool utf8_convert(bool to_utf, const char *input, char **output);
