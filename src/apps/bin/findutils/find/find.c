/* find -- search for files in a directory hierarchy
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

/* GNU find was written by Eric Decker <cire@cisco.com>,
   with enhancements by David MacKenzie <djm@gnu.ai.mit.edu>,
   Jay Plett <jay@silence.princeton.nj.us>,
   and Tim Wood <axolotl!tim@toad.com>.
   The idea for -print0 and xargs -0 came from
   Dan Bernstein <brnstnd@kramden.acf.nyu.edu>.  */

#include <config.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#else
#include <sys/file.h>
#endif
#include "defs.h"
#include "modetype.h"

#ifndef S_IFLNK
#define lstat stat
#endif

int lstat ();
int stat ();

#define apply_predicate(pathname, stat_buf_ptr, node)	\
  (*(node)->pred_func)((pathname), (stat_buf_ptr), (node))

static void process_top_path P_((char *pathname));
static int process_path P_((char *pathname, char *name, boolean leaf, char *parent));
static void process_dir P_((char *pathname, char *name, int pathlen, struct stat *statp, char *parent));
static boolean no_side_effects P_((struct predicate *pred));

/* Name this program was run with. */
char *program_name;

/* All predicates for each path to process. */
struct predicate *predicates;

/* The last predicate allocated. */
struct predicate *last_pred;

/* The root of the evaluation tree. */
static struct predicate *eval_tree;

/* If true, process directory before contents.  True unless -depth given. */
boolean do_dir_first;

/* If >=0, don't descend more than this many levels of subdirectories. */
int maxdepth;

/* If >=0, don't process files above this level. */
int mindepth;

/* Current depth; 0 means current path is a command line arg. */
int curdepth;

/* Seconds between 00:00 1/1/70 and either one day before now
   (the default), or the start of today (if -daystart is given). */
time_t cur_day_start;

/* If true, cur_day_start has been adjusted to the start of the day. */
boolean full_days;

/* If true, do not assume that files in directories with nlink == 2
   are non-directories. */
boolean no_leaf_check;

/* If true, don't cross filesystem boundaries. */
boolean stay_on_filesystem;

/* If true, don't descend past current directory.
   Can be set by -prune, -maxdepth, and -xdev/-mount. */
boolean stop_at_current_level;

#ifndef HAVE_FCHDIR
/* The full path of the initial working directory.  */
char *starting_dir;
#else
/* A file descriptor open to the initial working directory.
   Doing it this way allows us to work when the i.w.d. has
   unreadable parents.  */
int starting_desc;
#endif

/* If true, we have called stat on the current path. */
boolean have_stat;

/* The file being operated on, relative to the current directory.
   Used for stat, readlink, and opendir.  */
char *rel_pathname;

/* Length of current path. */
int path_length;

/* true if following symlinks.  Should be consistent with xstat.  */
boolean dereference;

/* Pointer to the function used to stat files. */
int (*xstat) ();

/* Status value to return to system. */
int exit_status;

#ifdef DEBUG_STAT
static int
debug_stat (file, bufp)
     char *file;
     struct stat *bufp;
{
  fprintf (stderr, "debug_stat (%s)\n", file);
  return lstat (file, bufp);
}
#endif /* DEBUG_STAT */

void
main (argc, argv)
     int argc;
     char *argv[];
{
  int i;
  PFB parse_function;		/* Pointer to who is to do the parsing. */
  struct predicate *cur_pred;
  char *predicate_name;		/* Name of predicate being parsed. */

  program_name = argv[0];

  predicates = NULL;
  last_pred = NULL;
  do_dir_first = true;
  maxdepth = mindepth = -1;
  cur_day_start = time ((time_t *) 0) - DAYSECS;
  full_days = false;
  no_leaf_check = false;
  stay_on_filesystem = false;
  exit_status = 0;
  dereference = false;
#ifdef DEBUG_STAT
  xstat = debug_stat;
#else /* !DEBUG_STAT */
  xstat = lstat;
#endif /* !DEBUG_STAT */

#ifdef DEBUG
  printf ("cur_day_start = %s", ctime (&cur_day_start));
#endif /* DEBUG */

  /* Find where in ARGV the predicates begin. */
  for (i = 1; i < argc && strchr ("-!(),", argv[i][0]) == NULL; i++)
    /* Do nothing. */ ;

  /* Enclose the expression in `( ... )' so a default -print will
     apply to the whole expression. */
  parse_open (argv, &argc);
  /* Build the input order list. */
  while (i < argc)
    {
      if (strchr ("-!(),", argv[i][0]) == NULL)
	usage ("paths must precede expression");
      predicate_name = argv[i];
      parse_function = find_parser (predicate_name);
      if (parse_function == NULL)
	error (1, 0, "invalid predicate `%s'", predicate_name);
      i++;
      if (!(*parse_function) (argv, &i))
	{
	  if (argv[i] == NULL)
	    error (1, 0, "missing argument to `%s'", predicate_name);
	  else
	    error (1, 0, "invalid argument `%s' to `%s'",
		   argv[i], predicate_name);
	}
    }
  if (predicates->pred_next == NULL)
    {
      /* No predicates that do something other than set a global variable
	 were given; remove the unneeded initial `(' and add `-print'. */
      cur_pred = predicates;
      predicates = last_pred = predicates->pred_next;
      free ((char *) cur_pred);
      parse_print (argv, &argc);
    }
  else if (!no_side_effects (predicates->pred_next))
    {
      /* One or more predicates that produce output were given;
	 remove the unneeded initial `('. */
      cur_pred = predicates;
      predicates = predicates->pred_next;
      free ((char *) cur_pred);
    }
  else
    {
      /* `( user-supplied-expression ) -print'. */
      parse_close (argv, &argc);
      parse_print (argv, &argc);
    }

#ifdef	DEBUG
  printf ("Predicate List:\n");
  print_list (predicates);
#endif /* DEBUG */

  /* Done parsing the predicates.  Build the evaluation tree. */
  cur_pred = predicates;
  eval_tree = get_expr (&cur_pred, NO_PREC);
#ifdef	DEBUG
  printf ("Eval Tree:\n");
  print_tree (eval_tree, 0);
#endif /* DEBUG */

  /* Rearrange the eval tree in optimal-predicate order. */
  opt_expr (&eval_tree);

  /* Determine the point, if any, at which to stat the file. */
  mark_stat (eval_tree);

#ifdef DEBUG
  printf ("Optimized Eval Tree:\n");
  print_tree (eval_tree, 0);
#endif /* DEBUG */

#ifndef HAVE_FCHDIR
  starting_dir = xgetcwd ();
  if (starting_dir == NULL)
    error (1, errno, "cannot get current directory");
#else
  starting_desc = open (".", O_RDONLY);
  if (starting_desc < 0)
    error (1, errno, "cannot open current directory");
#endif

  /* If no paths are given, default to ".".  */
  for (i = 1; i < argc && strchr ("-!(),", argv[i][0]) == NULL; i++)
    process_top_path (argv[i]);
  if (i == 1)
    process_top_path (".");

  exit (exit_status);
}

/* Descend PATHNAME, which is a command-line argument.  */

static void
process_top_path (pathname)
     char *pathname;
{
  struct stat stat_buf;

  curdepth = 0;
  path_length = strlen (pathname);

  /* We stat each pathname given on the command-line twice --
     once here and once in process_path.  It's not too bad, though,
     since the kernel can read the stat information out of its inode
     cache the second time.  */
  if ((*xstat) (pathname, &stat_buf) == 0 && S_ISDIR (stat_buf.st_mode))
    {
      if (chdir (pathname) < 0)
	{
	  error (0, errno, "%s", pathname);
	  exit_status = 1;
	  return;
	}
      process_path (pathname, ".", false, ".");
#ifndef HAVE_FCHDIR
      if (chdir (starting_dir) < 0)
	error (1, errno, "%s", starting_dir);
#else
      if (fchdir (starting_desc))
	error (1, errno, "cannot return to starting directory");
#endif
    }
  else
    process_path (pathname, pathname, false, ".");
}

/* Info on each directory in the current tree branch, to avoid
   getting stuck in symbolic link loops.  */
struct dir_id
{
  ino_t ino;
  dev_t dev;
};
static struct dir_id *dir_ids = NULL;
/* Entries allocated in `dir_ids'.  */
static int dir_alloc = 0;
/* Index in `dir_ids' of directory currently being searched.
   This is always the last valid entry.  */
static int dir_curr = -1;
/* (Arbitrary) number of entries to grow `dir_ids' by.  */
#define DIR_ALLOC_STEP 32

/* Recursively descend path PATHNAME, applying the predicates.
   LEAF is true if PATHNAME is known to be in a directory that has no
   more unexamined subdirectories, and therefore it is not a directory.
   Knowing this allows us to avoid calling stat as long as possible for
   leaf files.

   NAME is PATHNAME relative to the current directory.  We access NAME
   but print PATHNAME.

   PARENT is the path of the parent of NAME, relative to find's
   starting directory.

   Return nonzero iff PATHNAME is a directory. */

static int
process_path (pathname, name, leaf, parent)
     char *pathname;
     char *name;
     boolean leaf;
     char *parent;
{
  struct stat stat_buf;
  static dev_t root_dev;	/* Device ID of current argument pathname. */
  int i;

  /* Assume it is a non-directory initially. */
  stat_buf.st_mode = 0;

  rel_pathname = name;

  if (leaf)
    have_stat = false;
  else
    {
      if ((*xstat) (name, &stat_buf) != 0)
	{
	  error (0, errno, "%s", pathname);
	  exit_status = 1;
	  return 0;
	}
      have_stat = true;
    }

  if (!S_ISDIR (stat_buf.st_mode))
    {
      if (curdepth >= mindepth)
	apply_predicate (pathname, &stat_buf, eval_tree);
      return 0;
    }

  /* From here on, we're working on a directory.  */

  stop_at_current_level = maxdepth >= 0 && curdepth >= maxdepth;

  /* If we've already seen this directory on this branch,
     don't descend it again.  */
  for (i = 0; i <= dir_curr; i++)
    if (stat_buf.st_ino == dir_ids[i].ino &&
	stat_buf.st_dev == dir_ids[i].dev)
      stop_at_current_level = true;

  if (dir_alloc <= ++dir_curr)
    {
      dir_alloc += DIR_ALLOC_STEP;
      dir_ids = (struct dir_id *)
	xrealloc ((char *) dir_ids, dir_alloc * sizeof (struct dir_id));
    }
  dir_ids[dir_curr].ino = stat_buf.st_ino;
  dir_ids[dir_curr].dev = stat_buf.st_dev;

  if (stay_on_filesystem)
    {
      if (curdepth == 0)
	root_dev = stat_buf.st_dev;
      else if (stat_buf.st_dev != root_dev)
	stop_at_current_level = true;
    }

  if (do_dir_first && curdepth >= mindepth)
    apply_predicate (pathname, &stat_buf, eval_tree);

  if (stop_at_current_level == false)
    /* Scan directory on disk. */
    process_dir (pathname, name, strlen (pathname), &stat_buf, parent);

  if (do_dir_first == false && curdepth >= mindepth)
    apply_predicate (pathname, &stat_buf, eval_tree);

  dir_curr--;

  return 1;
}

/* Scan directory PATHNAME and recurse through process_path for each entry.

   PATHLEN is the length of PATHNAME.

   NAME is PATHNAME relative to the current directory.

   STATP is the results of *xstat on it.

   PARENT is the path of the parent of NAME, relative to find's
   starting directory.  */

static void
process_dir (pathname, name, pathlen, statp, parent)
     char *pathname;
     char *name;
     int pathlen;
     struct stat *statp;
     char *parent;
{
  char *name_space;		/* Names of files in PATHNAME. */
  int subdirs_left;		/* Number of unexamined subdirs in PATHNAME. */

  subdirs_left = statp->st_nlink - 2; /* Account for name and ".". */

  errno = 0;
  /* On some systems (VAX 4.3BSD+NFS), NFS mount points have st_size < 0.  */
  name_space = savedir (name, statp->st_size > 0 ? statp->st_size : 512);
  if (name_space == NULL)
    {
      if (errno)
	{
	  error (0, errno, "%s", pathname);
	  exit_status = 1;
	}
      else
	error (1, 0, "virtual memory exhausted");
    }
  else
    {
      register char *namep;	/* Current point in `name_space'. */
      char *cur_path;		/* Full path of each file to process. */
      char *cur_name;		/* Base name of each file to process. */
      unsigned cur_path_size;	/* Bytes allocated for `cur_path'. */
      register unsigned file_len; /* Length of each path to process. */
      register unsigned pathname_len; /* PATHLEN plus trailing '/'. */

      if (pathname[pathlen - 1] == '/')
	pathname_len = pathlen + 1; /* For '\0'; already have '/'. */
      else
	pathname_len = pathlen + 2; /* For '/' and '\0'. */
      cur_path_size = 0;
      cur_path = NULL;

      if (strcmp (name, ".") && chdir (name) < 0)
	{
	  error (0, errno, "%s", pathname);
	  exit_status = 1;
	  return;
	}

      for (namep = name_space; *namep; namep += file_len - pathname_len + 1)
	{
	  /* Append this directory entry's name to the path being searched. */
	  file_len = pathname_len + strlen (namep);
	  if (file_len > cur_path_size)
	    {
	      while (file_len > cur_path_size)
		cur_path_size += 1024;
	      if (cur_path)
		free (cur_path);
	      cur_path = xmalloc (cur_path_size);
	      strcpy (cur_path, pathname);
	      cur_path[pathname_len - 2] = '/';
	    }
	  cur_name = cur_path + pathname_len - 1;
	  strcpy (cur_name, namep);

	  curdepth++;
	  if (!no_leaf_check)
	    /* Normal case optimization.
	       On normal Unix filesystems, a directory that has no
	       subdirectories has two links: its name, and ".".  Any
	       additional links are to the ".." entries of its
	       subdirectories.  Once we have processed as many
	       subdirectories as there are additional links, we know
	       that the rest of the entries are non-directories --
	       in other words, leaf files. */
	    subdirs_left -= process_path (cur_path, cur_name,
					  subdirs_left == 0, pathname);
	  else
	    /* There might be weird (e.g., CD-ROM or MS-DOS) filesystems
	       mounted, which don't have Unix-like directory link counts. */
	    process_path (cur_path, cur_name, false, pathname);
	  curdepth--;
	}

      if (strcmp (name, "."))
	{
	  if (!dereference)
	    {
	      if (chdir ("..") < 0)
		/* We could go back and do the next command-line arg instead,
		   maybe using longjmp.  */
		error (1, errno, "%s", parent);
	    }
	  else
	    {
#ifndef HAVE_FCHDIR
	      if (chdir (starting_dir) || chdir (parent))
		error (1, errno, "%s", parent);
#else
	      if (fchdir (starting_desc) || chdir (parent))
		error (1, errno, "%s", parent);
#endif
	    }
	}

      if (cur_path)
	free (cur_path);
      free (name_space);
    }
}

/* Return true if there are no side effects in any of the predicates in
   predicate list PRED, false if there are any. */

static boolean
no_side_effects (pred)
     struct predicate *pred;
{
  while (pred != NULL)
    {
      if (pred->side_effects)
	return (false);
      pred = pred->pred_next;
    }
  return (true);
}
