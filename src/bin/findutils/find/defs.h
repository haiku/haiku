/* defs.h -- data types and declarations.
   Copyright (C) 1990, 91, 92, 93, 94, 2000, 2001, 2003, 2004, 2005, 2006, 2007 Free Software Foundation, Inc.

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


#ifndef INC_DEFS_H
#define INC_DEFS_H 1

#include <config.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

#if defined(HAVE_STRING_H) || defined(STDC_HEADERS)
#include <string.h>
#else
#include <strings.h>
#ifndef strchr
#define strchr index
#endif
#ifndef strrchr
#define strrchr rindex
#endif
#endif

#include <errno.h>
#ifndef errno
extern int errno;
#endif

#ifdef STDC_HEADERS
#include <stdlib.h>
#endif

/* The presence of unistd.h is assumed by gnulib these days, so we 
 * might as well assume it too. 
 */
#include <unistd.h>

#include <time.h>

#if HAVE_LIMITS_H
# include <limits.h>
#endif
#ifndef CHAR_BIT
# define CHAR_BIT 8
#endif

#if HAVE_INTTYPES_H
# include <inttypes.h>
#endif

#include "regex.h"

#ifndef S_IFLNK
#define lstat stat
#endif

# ifndef PARAMS
#  if defined PROTOTYPES || (defined __STDC__ && __STDC__)
#   define PARAMS(Args) Args
#  else
#   define PARAMS(Args) ()
#  endif
# endif

int lstat PARAMS((const char *__path, struct stat *__statbuf));
int stat PARAMS((const char *__path, struct stat *__statbuf));

int optionl_stat PARAMS((const char *name, struct stat *p));
int optionp_stat PARAMS((const char *name, struct stat *p));
int optionh_stat PARAMS((const char *name, struct stat *p));

int get_statinfo PARAMS((const char *pathname, const char *name, struct stat *p));



#ifndef S_ISUID
# define S_ISUID 0004000
#endif
#ifndef S_ISGID
# define S_ISGID 0002000
#endif
#ifndef S_ISVTX
# define S_ISVTX 0001000
#endif
#ifndef S_IRUSR
# define S_IRUSR 0000400
#endif
#ifndef S_IWUSR
# define S_IWUSR 0000200
#endif
#ifndef S_IXUSR
# define S_IXUSR 0000100
#endif
#ifndef S_IRGRP
# define S_IRGRP 0000040
#endif
#ifndef S_IWGRP
# define S_IWGRP 0000020
#endif
#ifndef S_IXGRP
# define S_IXGRP 0000010
#endif
#ifndef S_IROTH
# define S_IROTH 0000004
#endif
#ifndef S_IWOTH
# define S_IWOTH 0000002
#endif
#ifndef S_IXOTH
# define S_IXOTH 0000001
#endif

#define MODE_WXUSR	(S_IWUSR | S_IXUSR)
#define MODE_R		(S_IRUSR | S_IRGRP | S_IROTH)
#define MODE_RW		(S_IWUSR | S_IWGRP | S_IWOTH | MODE_R)
#define MODE_RWX	(S_IXUSR | S_IXGRP | S_IXOTH | MODE_RW)
#define MODE_ALL	(S_ISUID | S_ISGID | S_ISVTX | MODE_RWX)

#if 1
#include <stdbool.h>
typedef bool boolean;
#else
/* Not char because of type promotion; NeXT gcc can't handle it.  */
typedef int boolean;
#define		true    1
#define		false	0
#endif

struct predicate;

/* Pointer to a predicate function. */
typedef boolean (*PRED_FUNC)(char *pathname, struct stat *stat_buf, struct predicate *pred_ptr);

/* The number of seconds in a day. */
#define		DAYSECS	    86400

/* Argument structures for predicates. */

enum comparison_type
{
  COMP_GT,
  COMP_LT,
  COMP_EQ
};

enum permissions_type
{
  PERM_AT_LEAST,
  PERM_ANY,
  PERM_EXACT
};

enum predicate_type
{
  NO_TYPE,
  PRIMARY_TYPE,
  UNI_OP,
  BI_OP,
  OPEN_PAREN,
  CLOSE_PAREN
};

enum predicate_precedence
{
  NO_PREC,
  COMMA_PREC,
  OR_PREC,
  AND_PREC,
  NEGATE_PREC,
  MAX_PREC
};

struct long_val
{
  enum comparison_type kind;
  boolean negative;		/* Defined only when representing time_t.  */
  uintmax_t l_val;
};

struct perm_val
{
  enum permissions_type kind;
  mode_t val[2];
};

/* dir_id is used to support loop detection in find.c and 
 * also to support the -samefile test.
 */
struct dir_id
{
  ino_t ino;
  dev_t dev;
};

struct size_val
{
  enum comparison_type kind;
  int blocksize;
  uintmax_t size;
};

#define NEW_EXEC 1
/*
#undef NEW_EXEC 
*/

#if !defined(NEW_EXEC)
struct path_arg
{
  short offset;			/* Offset in `vec' of this arg. */
  short count;			/* Number of path replacements in this arg. */
  char *origarg;		/* Arg with "{}" intact. */
};
#endif

#include "buildcmd.h"

struct exec_val
{
#if defined(NEW_EXEC)
  /* new-style */
  boolean multiple;		/* -exec {} \+ denotes multiple argument. */
  struct buildcmd_control ctl;
  struct buildcmd_state   state;
  char **replace_vec;		/* Command arguments (for ";" style) */
  int num_args;
  boolean use_current_dir;      /* If nonzero, don't chdir to start dir */
  boolean close_stdin;		/* If true, close stdin in the child. */
#else
  struct path_arg *paths;	/* Array of args with path replacements. */
  char **vec;			/* Array of args to pass to program. */
#endif
};

/* The format string for a -printf or -fprintf is chopped into one or
   more `struct segment', linked together into a list.
   Each stretch of plain text is a segment, and
   each \c and `%' conversion is a segment. */

/* Special values for the `kind' field of `struct segment'. */
#define KIND_PLAIN 0		/* Segment containing just plain text. */
#define KIND_STOP 1		/* \c -- stop printing and flush output. */

struct segment
{
  int kind;			/* Format chars or KIND_{PLAIN,STOP}. */
  char *text;			/* Plain text or `%' format string. */
  int text_len;			/* Length of `text'. */
  struct segment *next;		/* Next segment for this predicate. */
};

struct format_val
{
  struct segment *segment;	/* Linked list of segments. */
  FILE *stream;			/* Output stream to print on. */
  boolean dest_is_tty;		/* True if the destination is a terminal. */
  struct quoting_options *quote_opts;
};

struct predicate
{
  /* Pointer to the function that implements this predicate.  */
  PRED_FUNC pred_func;

  /* Only used for debugging, but defined unconditionally so individual
     modules can be compiled with -DDEBUG.  */
  char *p_name;

  /* The type of this node.  There are two kinds.  The first is real
     predicates ("primaries") such as -perm, -print, or -exec.  The
     other kind is operators for combining predicates. */
  enum predicate_type p_type;

  /* The precedence of this node.  Only has meaning for operators. */
  enum predicate_precedence p_prec;

  /* True if this predicate node produces side effects.
     If side_effects are produced
     then optimization will not be performed */
  boolean side_effects;

  /* True if this predicate node requires default print be turned off. */
  boolean no_default_print;

  /* True if this predicate node requires a stat system call to execute. */
  boolean need_stat;

  /* True if this predicate node requires knowledge of the file type. */
  boolean need_type;

  /* Information needed by the predicate processor.
     Next to each member are listed the predicates that use it. */
  union
  {
    char *str;			/* fstype [i]lname [i]name [i]path */
    struct re_pattern_buffer *regex; /* regex */
    struct exec_val exec_vec;	/* exec ok */
    struct long_val info;	/* atime ctime gid inum links mtime
                                   size uid */
    struct size_val size;	/* size */
    uid_t uid;			/* user */
    gid_t gid;			/* group */
    time_t time;		/* newer */
    struct perm_val perm;	/* perm */
    struct dir_id   fileid;	/* samefile */
    mode_t type;		/* type */
    FILE *stream;		/* ls fls fprint0 */
    struct format_val printf_vec; /* printf fprintf fprint  */
  } args;

  /* The next predicate in the user input sequence,
     which represents the order in which the user supplied the
     predicates on the command line. */
  struct predicate *pred_next;

  /* The right and left branches from this node in the expression
     tree, which represents the order in which the nodes should be
     processed. */
  struct predicate *pred_left;
  struct predicate *pred_right;

  const struct parser_table* parser_entry;
};

/* find.c. */
int get_info PARAMS((const char *pathname, const char *name, struct stat *p, struct predicate *pred_ptr));
int following_links(void);


/* find library function declarations.  */

/* dirname.c */
char *dirname PARAMS((char *path));

/* error.c */
void error PARAMS((int status, int errnum, char *message, ...));

/* listfile.c */
void list_file PARAMS((char *name, char *relname, struct stat *statp, time_t current_time, int output_block_size, FILE *stream));
char *get_link_name PARAMS((char *name, char *relname));

/* stpcpy.c */
#if !HAVE_STPCPY
char *stpcpy PARAMS((char *dest, const char *src));
#endif

/* xgetcwd.c */
char *xgetcwd PARAMS((void));

/* xmalloc.c */
#if __STDC__
#define VOID void
#else
#define VOID char
#endif

/* find global function declarations.  */

/* find.c */
/* SymlinkOption represents the choice of 
 * -P, -L or -P (default) on the command line.
 */
enum SymlinkOption 
  {
    SYMLINK_NEVER_DEREF,	/* Option -P */
    SYMLINK_ALWAYS_DEREF,	/* Option -L */
    SYMLINK_DEREF_ARGSONLY	/* Option -H */
  };
extern enum SymlinkOption symlink_handling; /* defined in find.c. */

void set_follow_state PARAMS((enum SymlinkOption opt));
void cleanup(void);

/* fstype.c */
char *filesystem_type PARAMS((const struct stat *statp, const char *path));
char * get_mounted_filesystems (void);
dev_t * get_mounted_devices PARAMS((size_t *));



enum arg_type
  {
    ARG_OPTION,			/* regular options like -maxdepth */
    ARG_POSITIONAL_OPTION,	/* options whose position is important (-follow) */
    ARG_TEST,			/* a like -name */
    ARG_PUNCTUATION,		/* like -o or ( */
    ARG_ACTION			/* like -print */
  };


struct parser_table;
/* Pointer to a parser function. */
typedef boolean (*PARSE_FUNC)(const struct parser_table *p,
			      char *argv[], int *arg_ptr);
struct parser_table
{
  enum arg_type type;
  char *parser_name;
  PARSE_FUNC parser_func;
  PRED_FUNC    pred_func;
};

/* parser.c */
const struct parser_table* find_parser PARAMS((char *search_name));
boolean parse_open  PARAMS((const struct parser_table* entry, char *argv[], int *arg_ptr));
boolean parse_close PARAMS((const struct parser_table* entry, char *argv[], int *arg_ptr));
boolean parse_print PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
void pred_sanity_check PARAMS((const struct predicate *predicates));
void parse_begin_user_args PARAMS((char **args, int argno, const struct predicate *last, const struct predicate *predicates));
void parse_end_user_args PARAMS((char **args, int argno, const struct predicate *last, const struct predicate *predicates));
boolean parse_openparen              PARAMS((const struct parser_table* entry, char *argv[], int *arg_ptr));
boolean parse_closeparen             PARAMS((const struct parser_table* entry, char *argv[], int *arg_ptr));

/* pred.c */
boolean pred_amin PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_and PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_anewer PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_atime PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_closeparen PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_cmin PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_cnewer PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_comma PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_ctime PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_delete PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_empty PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_exec PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_execdir PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_false PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_fls PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_fprint PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_fprint0 PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_fprintf PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_fstype PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_gid PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_group PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_ilname PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_iname PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_inum PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_ipath PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_links PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_lname PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_ls PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_mmin PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_mtime PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_name PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_negate PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_newer PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_nogroup PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_nouser PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_ok PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_okdir PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_openparen PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_or PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_path PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_perm PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_print PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_print0 PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_prune PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_quit PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_regex PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_samefile PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_size PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_true PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_type PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_uid PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_used PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_user PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_xtype PARAMS((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));



int launch PARAMS((const struct buildcmd_control *ctl,
		   struct buildcmd_state *buildstate));


char *find_pred_name PARAMS((PRED_FUNC pred_func));



#ifdef DEBUG
void print_tree PARAMS((FILE*, struct predicate *node, int indent));
void print_list PARAMS((FILE*, struct predicate *node));
void print_optlist PARAMS((FILE *fp, struct predicate *node));
#endif /* DEBUG */

/* tree.c */
struct predicate *
get_expr PARAMS((struct predicate **input, short int prev_prec));
boolean opt_expr PARAMS((struct predicate **eval_treep));
boolean mark_stat PARAMS((struct predicate *tree));
boolean mark_type PARAMS((struct predicate *tree));

/* util.c */
struct predicate *get_new_pred PARAMS((const struct parser_table *entry));
struct predicate *get_new_pred_chk_op PARAMS((const struct parser_table *entry));
struct predicate *insert_primary PARAMS((const struct parser_table *entry));
struct predicate *insert_primary_withpred PARAMS((const struct parser_table *entry, PRED_FUNC fptr));
void usage PARAMS((char *msg));

extern char *program_name;
extern struct predicate *predicates;
extern struct predicate *last_pred;

struct options
{
  /* If true, process directory before contents.  True unless -depth given. */
  boolean do_dir_first;
  
  /* If >=0, don't descend more than this many levels of subdirectories. */
  int maxdepth;
  
  /* If >=0, don't process files above this level. */
  int mindepth;
  
  /* If true, do not assume that files in directories with nlink == 2
     are non-directories. */
  boolean no_leaf_check;
  
  /* If true, don't cross filesystem boundaries. */
  boolean stay_on_filesystem;
  
  /* If true, we ignore the problem where we find that a directory entry 
   * no longer exists by the time we get around to processing it.
   */
  boolean ignore_readdir_race;
  
/* If true, we issue warning messages
 */
  boolean warnings;
  time_t start_time;		/* Time at start of execution.  */
  
  /* Seconds between 00:00 1/1/70 and either one day before now
     (the default), or the start of today (if -daystart is given). */
  time_t cur_day_start;
  
  /* If true, cur_day_start has been adjusted to the start of the day. */
  boolean full_days;
  
  int output_block_size;	/* Output block size.  */
  
  enum SymlinkOption symlink_handling;
  
  
  /* Pointer to the function used to stat files. */
  int (*xstat) (const char *name, struct stat *statbuf);


  /* Indicate if we can implement safely_chdir() using the O_NOFOLLOW 
   * flag to open(2). 
   */
  boolean open_nofollow_available;

  /* The variety of regular expression that we support.
   * The default is POSIX Basic Regular Expressions, but this 
   * can be changed with the positional option, -regextype.
   */
  int regex_options;
};
extern struct options options;


struct state
{
  /* Current depth; 0 means current path is a command line arg. */
  int curdepth;
  
  /* If true, we have called stat on the current path. */
  boolean have_stat;
  
  /* If true, we know the type of the current path. */
  boolean have_type;
  mode_t type;			/* this is the actual type */
  
  /* The file being operated on, relative to the current directory.
     Used for stat, readlink, remove, and opendir.  */
  char *rel_pathname;

  /* Length of current path. */
  int path_length;

  /* If true, don't descend past current directory.
     Can be set by -prune, -maxdepth, and -xdev/-mount. */
  boolean stop_at_current_level;
  
  /* Status value to return to system. */
  int exit_status;
};
extern struct state state;

extern char const *starting_dir;
extern int starting_desc;
#if ! defined HAVE_FCHDIR && ! defined fchdir
# define fchdir(fd) (-1)
#endif

#endif
