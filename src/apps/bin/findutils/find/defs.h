/* defs.h -- data types and declarations.
   Copyright (C) 1990, 91, 92, 93, 94 Free Software Foundation, Inc.

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <time.h>

#include "regex.h"

#if __STDC__
# define P_(s) s
#else
# define P_(s) ()
#endif

/* Not char because of type promotion; NeXT gcc can't handle it.  */
typedef int boolean;
#define		true    1
#define		false	0

/* Pointer to function returning boolean. */
typedef boolean (*PFB)();

/* The number of seconds in a day. */
#define		DAYSECS	    86400

/* Argument structures for predicates. */

enum comparison_type
{
  COMP_GT,
  COMP_LT,
  COMP_EQ
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
  unsigned long l_val;
};

struct size_val
{
  enum comparison_type kind;
  int blocksize;
  unsigned long size;
};

struct path_arg
{
  short offset;			/* Offset in `vec' of this arg. */
  short count;			/* Number of path replacements in this arg. */
  char *origarg;		/* Arg with "{}" intact. */
};

struct exec_val
{
  struct path_arg *paths;	/* Array of args with path replacements. */
  char **vec;			/* Array of args to pass to program. */
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
};

struct predicate
{
  /* Pointer to the function that implements this predicate.  */
  PFB pred_func;

  /* Only used for debugging, but defined unconditionally so individual
     modules can be compiled with -DDEBUG.  */
  char *p_name;

  /* The type of this node.  There are two kinds.  The first is real
     predicates ("primaries") such as -perm, -print, or -exec.  The
     other kind is operators for combining predicates. */
  enum predicate_type p_type;

  /* The precedence of this node.  Only has meaning for operators. */
  enum predicate_precedence p_prec;

  /* True if this predicate node produces side effects. */
  boolean side_effects;

  /* True if this predicate node requires a stat system call to execute. */
  boolean need_stat;

  /* Information needed by the predicate processor.
     Next to each member are listed the predicates that use it. */
  union
  {
    char *str;			/* fstype [i]lname [i]name [i]path */
    struct re_pattern_buffer *regex; /* regex */
    struct exec_val exec_vec;	/* exec ok */
    struct long_val info;	/* atime ctime mtime inum links */
    struct size_val size;	/* size */
    uid_t uid;			/* user */
    gid_t gid;			/* group */
    time_t time;		/* newer */
    unsigned long perm;		/* perm */
    unsigned long type;		/* type */
    FILE *stream;		/* fprint fprint0 */
    struct format_val printf_vec; /* printf fprintf */
  } args;

  /* The next predicate in the user input sequence,
     which repesents the order in which the user supplied the
     predicates on the command line. */
  struct predicate *pred_next;

  /* The right and left branches from this node in the expression
     tree, which represents the order in which the nodes should be
     processed. */
  struct predicate *pred_left;
  struct predicate *pred_right;
};

/* find library function declarations.  */

/* dirname.c */
char *dirname P_((char *path));

/* error.c */
void error P_((int status, int errnum, char *message, ...));

/* listfile.c */
void list_file P_((char *name, char *relname, struct stat *statp, FILE *stream));
char *get_link_name P_((char *name, char *relname));

/* savedir.c */
char *savedir P_((char *dir, unsigned name_size));

/* stpcpy.c */
#if !HAVE_STPCPY
char *stpcpy P_((char *dest, const char *src));
#endif

/* xgetcwd.c */
char *xgetcwd P_((void));

/* xmalloc.c */
#if __STDC__
#define VOID void
#else
#define VOID char
#endif

VOID *xmalloc P_((size_t n));
VOID *xrealloc P_((VOID *p, size_t n));

/* xstrdup.c */
char *xstrdup P_((char *string));

/* find global function declarations.  */

/* fstype.c */
char *filesystem_type P_((char *path, char *relpath, struct stat *statp));

/* parser.c */
PFB find_parser P_((char *search_name));
boolean parse_close P_((char *argv[], int *arg_ptr));
boolean parse_open P_((char *argv[], int *arg_ptr));
boolean parse_print P_((char *argv[], int *arg_ptr));

/* pred.c */
boolean pred_amin P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_and P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_anewer P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_atime P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_close P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_cmin P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_cnewer P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_comma P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_ctime P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_empty P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_exec P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_false P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_fls P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_fprint P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_fprint0 P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_fprintf P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_fstype P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_gid P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_group P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_ilname P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_iname P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_inum P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_ipath P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_links P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_lname P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_ls P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_mmin P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_mtime P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_name P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_negate P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_newer P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_nogroup P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_nouser P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_ok P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_open P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_or P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_path P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_perm P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_print P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_print0 P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_prune P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_regex P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_size P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_true P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_type P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_uid P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_used P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_user P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
boolean pred_xtype P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr));
char *find_pred_name P_((PFB pred_func));
#ifdef DEBUG
void print_tree P_((struct predicate *node, int indent));
void print_list P_((struct predicate *node));
#endif /* DEBUG */

/* tree.c */
struct predicate *get_expr P_((struct predicate **input, int prev_prec));
boolean opt_expr P_((struct predicate **eval_treep));
boolean mark_stat P_((struct predicate *tree));

/* util.c */
char *basename P_((char *fname));
struct predicate *get_new_pred P_((void));
struct predicate *get_new_pred_chk_op P_((void));
struct predicate *insert_primary P_((boolean (*pred_func )()));
void usage P_((char *msg));

extern char *program_name;
extern struct predicate *predicates;
extern struct predicate *last_pred;
extern boolean do_dir_first;
extern int maxdepth;
extern int mindepth;
extern int curdepth;
extern time_t cur_day_start;
extern boolean full_days;
extern boolean no_leaf_check;
extern boolean stay_on_filesystem;
extern boolean stop_at_current_level;
extern boolean have_stat;
extern char *rel_pathname;
#ifndef HAVE_FCHDIR
extern char *starting_dir;
#else
extern int starting_desc;
#endif
extern int exit_status;
extern int path_length;
extern int (*xstat) ();
extern boolean dereference;
