/* pred.c -- execute the expression tree.
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

#include <config.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fnmatch.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
#include "defs.h"
#include "modetype.h"
#include "wait.h"

#if !defined(SIGCHLD) && defined(SIGCLD)
#define SIGCHLD SIGCLD
#endif

#ifndef _POSIX_VERSION
struct passwd *getpwuid ();
struct group *getgrgid ();
#endif

#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif

#ifdef CLOSEDIR_VOID
/* Fake a return value. */
#define CLOSEDIR(d) (closedir (d), 0)
#else
#define CLOSEDIR(d) closedir (d)
#endif

#ifndef S_IFLNK
#define lstat stat
#endif

/* Extract or fake data from a `struct stat'.
   ST_NBLOCKS: Number of 512-byte blocks in the file
   (including indirect blocks).
   HP-UX, perhaps uniquely, counts st_blocks in 1024-byte units.
   This workaround loses when mixing HP-UX and 4BSD filesystems, though.  */
#ifdef _POSIX_SOURCE
# define ST_NBLOCKS(statp) (((statp)->st_size + 512 - 1) / 512)
#else
# ifndef HAVE_ST_BLOCKS
#  define ST_NBLOCKS(statp) (st_blocks ((statp)->st_size))
# else
#  if defined(hpux) || defined(__hpux__)
#   define ST_NBLOCKS(statp) ((statp)->st_blocks * 2)
#  else
#   define ST_NBLOCKS(statp) ((statp)->st_blocks)
#  endif
# endif
#endif

int lstat ();
int stat ();

static boolean insert_lname P_((char *pathname, struct stat *stat_buf, struct predicate *pred_ptr, boolean ignore_case));
static boolean launch P_((struct predicate *pred_ptr));
static char *format_date P_((time_t when, int kind));

#ifdef	DEBUG
struct pred_assoc
{
  PFB pred_func;
  char *pred_name;
};

struct pred_assoc pred_table[] =
{
  {pred_amin, "amin    "},
  {pred_and, "and     "},
  {pred_anewer, "anewer  "},
  {pred_atime, "atime   "},
  {pred_close, ")       "},
  {pred_amin, "cmin    "},
  {pred_cnewer, "cnewer  "},
  {pred_comma, ",       "},
  {pred_ctime, "ctime   "},
  {pred_empty, "empty   "},
  {pred_exec, "exec    "},
  {pred_false, "false   "},
  {pred_fprint, "fprint  "},
  {pred_fprint0, "fprint0 "},
  {pred_fprintf, "fprintf "},
  {pred_fstype, "fstype  "},
  {pred_gid, "gid     "},
  {pred_group, "group   "},
  {pred_ilname, "ilname  "},
  {pred_iname, "iname   "},
  {pred_inum, "inum    "},
  {pred_ipath, "ipath   "},
  {pred_links, "links   "},
  {pred_lname, "lname   "},
  {pred_ls, "ls      "},
  {pred_amin, "mmin    "},
  {pred_mtime, "mtime   "},
  {pred_name, "name    "},
  {pred_negate, "not     "},
  {pred_newer, "newer   "},
  {pred_nogroup, "nogroup "},
  {pred_nouser, "nouser  "},
  {pred_ok, "ok      "},
  {pred_open, "(       "},
  {pred_or, "or      "},
  {pred_path, "path    "},
  {pred_perm, "perm    "},
  {pred_print, "print   "},
  {pred_print0, "print0  "},
  {pred_prune, "prune   "},
  {pred_regex, "regex   "},
  {pred_size, "size    "},
  {pred_true, "true    "},
  {pred_type, "type    "},
  {pred_uid, "uid     "},
  {pred_used, "used    "},
  {pred_user, "user    "},
  {pred_xtype, "xtype   "},
  {0, "none    "}
};

struct op_assoc
{
  short type;
  char *type_name;
};

struct op_assoc type_table[] =
{
  {NO_TYPE, "no          "},
  {PRIMARY_TYPE, "primary      "},
  {UNI_OP, "uni_op      "},
  {BI_OP, "bi_op       "},
  {OPEN_PAREN, "open_paren  "},
  {CLOSE_PAREN, "close_paren "},
  {-1, "unknown     "}
};

struct prec_assoc
{
  short prec;
  char *prec_name;
};

struct prec_assoc prec_table[] =
{
  {NO_PREC, "no      "},
  {COMMA_PREC, "comma   "},
  {OR_PREC, "or      "},
  {AND_PREC, "and     "},
  {NEGATE_PREC, "negate  "},
  {MAX_PREC, "max     "},
  {-1, "unknown "}
};
#endif	/* DEBUG */

/* Predicate processing routines.
 
   PATHNAME is the full pathname of the file being checked.
   *STAT_BUF contains information about PATHNAME.
   *PRED_PTR contains information for applying the predicate.
 
   Return true if the file passes this predicate, false if not. */

boolean
pred_amin (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  switch (pred_ptr->args.info.kind)
    {
    case COMP_GT:
      if (stat_buf->st_atime > (time_t) pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_LT:
      if (stat_buf->st_atime < (time_t) pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_EQ:
      if ((stat_buf->st_atime >= (time_t) pred_ptr->args.info.l_val)
	  && (stat_buf->st_atime < (time_t) pred_ptr->args.info.l_val + 60))
	return (true);
      break;
    }
  return (false);
}

boolean
pred_and (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  if (pred_ptr->pred_left == NULL
      || (*pred_ptr->pred_left->pred_func) (pathname, stat_buf,
					    pred_ptr->pred_left))
    {
      /* Check whether we need a stat here. */
      if (pred_ptr->need_stat)
	{
	  if (!have_stat && (*xstat) (rel_pathname, stat_buf) != 0)
	    {
	      error (0, errno, "%s", pathname);
	      exit_status = 1;
	      return (false);
	    }
	  have_stat = true;
	}
      return ((*pred_ptr->pred_right->pred_func) (pathname, stat_buf,
						  pred_ptr->pred_right));
    }
  else
    return (false);
}

boolean
pred_anewer (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  if (stat_buf->st_atime > pred_ptr->args.time)
    return (true);
  return (false);
}

boolean
pred_atime (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  switch (pred_ptr->args.info.kind)
    {
    case COMP_GT:
      if (stat_buf->st_atime > (time_t) pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_LT:
      if (stat_buf->st_atime < (time_t) pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_EQ:
      if ((stat_buf->st_atime >= (time_t) pred_ptr->args.info.l_val)
	  && (stat_buf->st_atime < (time_t) pred_ptr->args.info.l_val
	      + DAYSECS))
	return (true);
      break;
    }
  return (false);
}

boolean
pred_close (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  return (true);
}

boolean
pred_cmin (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  switch (pred_ptr->args.info.kind)
    {
    case COMP_GT:
      if (stat_buf->st_ctime > (time_t) pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_LT:
      if (stat_buf->st_ctime < (time_t) pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_EQ:
      if ((stat_buf->st_ctime >= (time_t) pred_ptr->args.info.l_val)
	  && (stat_buf->st_ctime < (time_t) pred_ptr->args.info.l_val + 60))
	return (true);
      break;
    }
  return (false);
}

boolean
pred_cnewer (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  if (stat_buf->st_ctime > pred_ptr->args.time)
    return (true);
  return (false);
}

boolean
pred_comma (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  if (pred_ptr->pred_left != NULL)
    (*pred_ptr->pred_left->pred_func) (pathname, stat_buf,
				       pred_ptr->pred_left);
  /* Check whether we need a stat here. */
  if (pred_ptr->need_stat)
    {
      if (!have_stat && (*xstat) (rel_pathname, stat_buf) != 0)
	{
	  error (0, errno, "%s", pathname);
	  exit_status = 1;
	  return (false);
	}
      have_stat = true;
    }
  return ((*pred_ptr->pred_right->pred_func) (pathname, stat_buf,
					      pred_ptr->pred_right));
}

boolean
pred_ctime (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  switch (pred_ptr->args.info.kind)
    {
    case COMP_GT:
      if (stat_buf->st_ctime > (time_t) pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_LT:
      if (stat_buf->st_ctime < (time_t) pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_EQ:
      if ((stat_buf->st_ctime >= (time_t) pred_ptr->args.info.l_val)
	  && (stat_buf->st_ctime < (time_t) pred_ptr->args.info.l_val
	      + DAYSECS))
	return (true);
      break;
    }
  return (false);
}

boolean
pred_empty (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  if (S_ISDIR (stat_buf->st_mode))
    {
      DIR *d;
      struct dirent *dp;
      boolean empty = true;

      errno = 0;
      d = opendir (rel_pathname);
      if (d == NULL)
	{
	  error (0, errno, "%s", pathname);
	  exit_status = 1;
	  return (false);
	}
      for (dp = readdir (d); dp; dp = readdir (d))
	{
	  if (dp->d_name[0] != '.'
	      || (dp->d_name[1] != '\0'
		  && (dp->d_name[1] != '.' || dp->d_name[2] != '\0')))
	    {
	      empty = false;
	      break;
	    }
	}
      if (CLOSEDIR (d))
	{
	  error (0, errno, "%s", pathname);
	  exit_status = 1;
	  return (false);
	}
      return (empty);
    }
  else if (S_ISREG (stat_buf->st_mode))
    return (stat_buf->st_size == 0);
  else
    return (false);
}

boolean
pred_exec (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  int i;
  int path_pos;
  struct exec_val *execp;	/* Pointer for efficiency. */

  execp = &pred_ptr->args.exec_vec;

  /* Replace "{}" with the real path in each affected arg. */
  for (path_pos = 0; execp->paths[path_pos].offset >= 0; path_pos++)
    {
      register char *from, *to;

      i = execp->paths[path_pos].offset;
      execp->vec[i] =
	xmalloc (strlen (execp->paths[path_pos].origarg) + 1
		 + (strlen (pathname) - 2) * execp->paths[path_pos].count);
      for (from = execp->paths[path_pos].origarg, to = execp->vec[i]; *from; )
	if (from[0] == '{' && from[1] == '}')
	  {
	    to = stpcpy (to, pathname);
	    from += 2;
	  }
	else
	  *to++ = *from++;
      *to = *from;		/* Copy null. */
    }

  i = launch (pred_ptr);

  /* Free the temporary args. */
  for (path_pos = 0; execp->paths[path_pos].offset >= 0; path_pos++)
    free (execp->vec[execp->paths[path_pos].offset]);

  return (i);
}

boolean
pred_false (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  return (false);
}

boolean
pred_fls (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  list_file (pathname, rel_pathname, stat_buf, pred_ptr->args.stream);
  return (true);
}

boolean
pred_fprint (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  fputs (pathname, pred_ptr->args.stream);
  putc ('\n', pred_ptr->args.stream);
  return (true);
}

boolean
pred_fprint0 (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  fputs (pathname, pred_ptr->args.stream);
  putc (0, pred_ptr->args.stream);
  return (true);
}

boolean
pred_fprintf (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  FILE *fp = pred_ptr->args.printf_vec.stream;
  struct segment *segment;
  char *cp;

  for (segment = pred_ptr->args.printf_vec.segment; segment;
       segment = segment->next)
    {
      if (segment->kind & 0xff00) /* Component of date. */
	{
	  time_t t;

	  switch (segment->kind & 0xff)
	    {
	    case 'A':
	      t = stat_buf->st_atime;
	      break;
	    case 'C':
	      t = stat_buf->st_ctime;
	      break;
	    case 'T':
	      t = stat_buf->st_mtime;
	      break;
	    default:
	      abort ();
	    }
	  fprintf (fp, segment->text,
		   format_date (t, (segment->kind >> 8) & 0xff));
	  continue;
	}

      switch (segment->kind)
	{
	case KIND_PLAIN:	/* Plain text string (no % conversion). */
	  fwrite (segment->text, 1, segment->text_len, fp);
	  break;
	case KIND_STOP:		/* Terminate argument and flush output. */
	  fwrite (segment->text, 1, segment->text_len, fp);
	  fflush (fp);
	  return (true);
	case 'a':		/* atime in `ctime' format. */
	  cp = ctime (&stat_buf->st_atime);
	  cp[24] = '\0';
	  fprintf (fp, segment->text, cp);
	  break;
	case 'b':		/* size in 512-byte blocks */
	  fprintf (fp, segment->text, ST_NBLOCKS (stat_buf));
	  break;
	case 'c':		/* ctime in `ctime' format */
	  cp = ctime (&stat_buf->st_ctime);
	  cp[24] = '\0';
	  fprintf (fp, segment->text, cp);
	  break;
	case 'd':		/* depth in search tree */
	  fprintf (fp, segment->text, curdepth);
	  break;
	case 'f':		/* basename of path */
	  cp = strrchr (pathname, '/');
	  if (cp)
	    cp++;
	  else
	    cp = pathname;
	  fprintf (fp, segment->text, cp);
	  break;
	case 'F':		/* filesystem type */
	  fprintf (fp, segment->text,
		   filesystem_type (pathname, rel_pathname, stat_buf));
	  break;
	case 'g':		/* group name */
	  {
	    struct group *g;

	    g = getgrgid (stat_buf->st_gid);
	    if (g)
	      {
		segment->text[segment->text_len] = 's';
		fprintf (fp, segment->text, g->gr_name);
		break;
	      }
	    /* else fallthru */
	  }
	case 'G':		/* GID number */
	  segment->text[segment->text_len] = 'u';
	  fprintf (fp, segment->text, stat_buf->st_gid);
	  break;
	case 'h':		/* leading directories part of path */
	  {
	    char cc;

	    cp = strrchr (pathname, '/');
	    if (cp == NULL)	/* No leading directories. */
	      break;
	    cc = *cp;
	    *cp = '\0';
	    fprintf (fp, segment->text, pathname);
	    *cp = cc;
	    break;
	  }
	case 'H':		/* ARGV element file was found under */
	  {
	    char cc = pathname[path_length];

	    pathname[path_length] = '\0';
	    fprintf (fp, segment->text, pathname);
	    pathname[path_length] = cc;
	    break;
	  }
	case 'i':		/* inode number */
	  fprintf (fp, segment->text, stat_buf->st_ino);
	  break;
	case 'k':		/* size in 1K blocks */
	  fprintf (fp, segment->text, (ST_NBLOCKS (stat_buf) + 1) / 2);
	  break;
	case 'l':		/* object of symlink */
#ifdef S_ISLNK
	  {
	    char *linkname = 0;

	    if (S_ISLNK (stat_buf->st_mode))
	      {
		linkname = get_link_name (pathname, rel_pathname);
		if (linkname == 0)
		  exit_status = 1;
	      }
	    if (linkname)
	      {
		fprintf (fp, segment->text, linkname);
		free (linkname);
	      }
	    else
	      fprintf (fp, segment->text, "");
	  }
#endif				/* S_ISLNK */
	  break;
	case 'm':		/* mode as octal number (perms only) */
	  fprintf (fp, segment->text, stat_buf->st_mode & 07777);
	  break;
	case 'n':		/* number of links */
	  fprintf (fp, segment->text, stat_buf->st_nlink);
	  break;
	case 'p':		/* pathname */
	  fprintf (fp, segment->text, pathname);
	  break;
	case 'P':		/* pathname with ARGV element stripped */
	  if (curdepth)
	    {
	      cp = pathname + path_length;
	      if (*cp == '/')
		/* Move past the slash between the ARGV element
		   and the rest of the pathname.  But if the ARGV element
		   ends in a slash, we didn't add another, so we've
		   already skipped past it.  */
		cp++;
	    }
	  else
	    cp = "";
	  fprintf (fp, segment->text, cp);
	  break;
	case 's':		/* size in bytes */
	  fprintf (fp, segment->text, stat_buf->st_size);
	  break;
	case 't':		/* mtime in `ctime' format */
	  cp = ctime (&stat_buf->st_mtime);
	  cp[24] = '\0';
	  fprintf (fp, segment->text, cp);
	  break;
	case 'u':		/* user name */
	  {
	    struct passwd *p;

	    p = getpwuid (stat_buf->st_uid);
	    if (p)
	      {
		segment->text[segment->text_len] = 's';
		fprintf (fp, segment->text, p->pw_name);
		break;
	      }
	    /* else fallthru */
	  }
	case 'U':		/* UID number */
	  segment->text[segment->text_len] = 'u';
	  fprintf (fp, segment->text, stat_buf->st_uid);
	  break;
	}
    }
  return (true);
}

boolean
pred_fstype (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  if (strcmp (filesystem_type (pathname, rel_pathname, stat_buf),
	      pred_ptr->args.str) == 0)
    return (true);
  return (false);
}

boolean
pred_gid (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  switch (pred_ptr->args.info.kind)
    {
    case COMP_GT:
      if (stat_buf->st_gid > pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_LT:
      if (stat_buf->st_gid < pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_EQ:
      if (stat_buf->st_gid == pred_ptr->args.info.l_val)
	return (true);
      break;
    }
  return (false);
}

boolean
pred_group (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  if (pred_ptr->args.gid == stat_buf->st_gid)
    return (true);
  else
    return (false);
}

boolean
pred_ilname (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  return insert_lname (pathname, stat_buf, pred_ptr, true);
}

boolean
pred_iname (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  char *base;

  base = basename (pathname);
  if (fnmatch (pred_ptr->args.str, base, FNM_PERIOD | FNM_CASEFOLD) == 0)
    return (true);
  return (false);
}

boolean
pred_inum (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  switch (pred_ptr->args.info.kind)
    {
    case COMP_GT:
      if (stat_buf->st_ino > pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_LT:
      if (stat_buf->st_ino < pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_EQ:
      if (stat_buf->st_ino == pred_ptr->args.info.l_val)
	return (true);
      break;
    }
  return (false);
}

boolean
pred_ipath (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  if (fnmatch (pred_ptr->args.str, pathname, FNM_CASEFOLD) == 0)
    return (true);
  return (false);
}

boolean
pred_links (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  switch (pred_ptr->args.info.kind)
    {
    case COMP_GT:
      if (stat_buf->st_nlink > pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_LT:
      if (stat_buf->st_nlink < pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_EQ:
      if (stat_buf->st_nlink == pred_ptr->args.info.l_val)
	return (true);
      break;
    }
  return (false);
}

boolean
pred_lname (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  return insert_lname (pathname, stat_buf, pred_ptr, false);
}

static boolean
insert_lname (pathname, stat_buf, pred_ptr, ignore_case)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
     boolean ignore_case;
{
  boolean ret = false;
#ifdef S_ISLNK
  if (S_ISLNK (stat_buf->st_mode))
    {
      char *linkname = get_link_name (pathname, rel_pathname);
      if (linkname)
	{
	  if (fnmatch (pred_ptr->args.str, linkname,
		       ignore_case ? FNM_CASEFOLD : 0) == 0)
	    ret = true;
	  free (linkname);
	}
    }
#endif /* S_ISLNK */
  return (ret);
}

boolean
pred_ls (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  list_file (pathname, rel_pathname, stat_buf, stdout);
  return (true);
}

boolean
pred_mmin (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  switch (pred_ptr->args.info.kind)
    {
    case COMP_GT:
      if (stat_buf->st_mtime > (time_t) pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_LT:
      if (stat_buf->st_mtime < (time_t) pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_EQ:
      if ((stat_buf->st_mtime >= (time_t) pred_ptr->args.info.l_val)
	  && (stat_buf->st_mtime < (time_t) pred_ptr->args.info.l_val + 60))
	return (true);
      break;
    }
  return (false);
}

boolean
pred_mtime (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  switch (pred_ptr->args.info.kind)
    {
    case COMP_GT:
      if (stat_buf->st_mtime > (time_t) pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_LT:
      if (stat_buf->st_mtime < (time_t) pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_EQ:
      if ((stat_buf->st_mtime >= (time_t) pred_ptr->args.info.l_val)
	  && (stat_buf->st_mtime < (time_t) pred_ptr->args.info.l_val
	      + DAYSECS))
	return (true);
      break;
    }
  return (false);
}

boolean
pred_name (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  char *base;

  base = basename (pathname);
  if (fnmatch (pred_ptr->args.str, base, FNM_PERIOD) == 0)
    return (true);
  return (false);
}

boolean
pred_negate (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  /* Check whether we need a stat here. */
  if (pred_ptr->need_stat)
    {
      if (!have_stat && (*xstat) (rel_pathname, stat_buf) != 0)
	{
	  error (0, errno, "%s", pathname);
	  exit_status = 1;
	  return (false);
	}
      have_stat = true;
    }
  return (!(*pred_ptr->pred_right->pred_func) (pathname, stat_buf,
					      pred_ptr->pred_right));
}

boolean
pred_newer (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  if (stat_buf->st_mtime > pred_ptr->args.time)
    return (true);
  return (false);
}

boolean
pred_nogroup (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
#ifdef CACHE_IDS
  extern char *gid_unused;

  return gid_unused[(unsigned) stat_buf->st_gid];
#else
  return getgrgid (stat_buf->st_gid) == NULL;
#endif
}

boolean
pred_nouser (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
#ifdef CACHE_IDS
  extern char *uid_unused;

  return uid_unused[(unsigned) stat_buf->st_uid];
#else
  return getpwuid (stat_buf->st_uid) == NULL;
#endif
}

boolean
pred_ok (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  int i, yes;
  
  fflush (stdout);
  fprintf (stderr, "< %s ... %s > ? ",
	   pred_ptr->args.exec_vec.vec[0], pathname);
  fflush (stderr);
  i = getchar ();
  yes = (i == 'y' || i == 'Y');
  while (i != EOF && i != '\n')
    i = getchar ();
  if (!yes)
    return (false);
  return pred_exec (pathname, stat_buf, pred_ptr);
}

boolean
pred_open (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  return (true);
}

boolean
pred_or (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  if (pred_ptr->pred_left == NULL
      || !(*pred_ptr->pred_left->pred_func) (pathname, stat_buf,
					     pred_ptr->pred_left))
    {
      /* Check whether we need a stat here. */
      if (pred_ptr->need_stat)
	{
	  if (!have_stat && (*xstat) (rel_pathname, stat_buf) != 0)
	    {
	      error (0, errno, "%s", pathname);
	      exit_status = 1;
	      return (false);
	    }
	  have_stat = true;
	}
      return ((*pred_ptr->pred_right->pred_func) (pathname, stat_buf,
						  pred_ptr->pred_right));
    }
  else
    return (true);
}

boolean
pred_path (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  if (fnmatch (pred_ptr->args.str, pathname, 0) == 0)
    return (true);
  return (false);
}

boolean
pred_perm (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  if (pred_ptr->args.perm & 010000)
    {
      /* Magic flag set in parse_perm:
	 true if at least the given bits are set. */
      if ((stat_buf->st_mode & 07777 & pred_ptr->args.perm)
	  == (pred_ptr->args.perm & 07777))
	return (true);
    }
  else if (pred_ptr->args.perm & 020000)
    {
      /* Magic flag set in parse_perm:
	 true if any of the given bits are set. */
      if ((stat_buf->st_mode & 07777) & pred_ptr->args.perm)
	return (true);
    }
  else
    {
      /* True if exactly the given bits are set. */
      if ((stat_buf->st_mode & 07777) == pred_ptr->args.perm)
	return (true);
    }
  return (false);
}

boolean
pred_print (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  puts (pathname);
  return (true);
}

boolean
pred_print0 (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  fputs (pathname, stdout);
  putc (0, stdout);
  return (true);
}

boolean
pred_prune (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  stop_at_current_level = true;
  return (do_dir_first);	/* This is what SunOS find seems to do. */
}

boolean
pred_regex (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  int len = strlen (pathname);
  if (re_match (pred_ptr->args.regex, pathname, len, 0,
		(struct re_registers *) NULL) == len)
    return (true);
  return (false);
}

boolean
pred_size (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  unsigned long f_val;

  f_val = (stat_buf->st_size + pred_ptr->args.size.blocksize - 1)
    / pred_ptr->args.size.blocksize;
  switch (pred_ptr->args.size.kind)
    {
    case COMP_GT:
      if (f_val > pred_ptr->args.size.size)
	return (true);
      break;
    case COMP_LT:
      if (f_val < pred_ptr->args.size.size)
	return (true);
      break;
    case COMP_EQ:
      if (f_val == pred_ptr->args.size.size)
	return (true);
      break;
    }
  return (false);
}

boolean
pred_true (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  return (true);
}

boolean
pred_type (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  unsigned long mode = stat_buf->st_mode;
  unsigned long type = pred_ptr->args.type;

#ifndef S_IFMT
  /* POSIX system; check `mode' the slow way. */
  if ((S_ISBLK (mode) && type == S_IFBLK)
      || (S_ISCHR (mode) && type == S_IFCHR)
      || (S_ISDIR (mode) && type == S_IFDIR)
      || (S_ISREG (mode) && type == S_IFREG)
#ifdef S_IFLNK
      || (S_ISLNK (mode) && type == S_IFLNK)
#endif
#ifdef S_IFIFO
      || (S_ISFIFO (mode) && type == S_IFIFO)
#endif
#ifdef S_IFSOCK
      || (S_ISSOCK (mode) && type == S_IFSOCK)
#endif
      )
#else /* S_IFMT */
  /* Unix system; check `mode' the fast way. */
  if ((mode & S_IFMT) == type)
#endif /* S_IFMT */
    return (true);
  else
    return (false);
}

boolean
pred_uid (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  switch (pred_ptr->args.info.kind)
    {
    case COMP_GT:
      if (stat_buf->st_uid > pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_LT:
      if (stat_buf->st_uid < pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_EQ:
      if (stat_buf->st_uid == pred_ptr->args.info.l_val)
	return (true);
      break;
    }
  return (false);
}

boolean
pred_used (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  time_t delta;

  delta = stat_buf->st_atime - stat_buf->st_ctime; /* Use difftime? */
  switch (pred_ptr->args.info.kind)
    {
    case COMP_GT:
      if (delta > (time_t) pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_LT:
      if (delta < (time_t) pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_EQ:
      if ((delta >= (time_t) pred_ptr->args.info.l_val)
	  && (delta < (time_t) pred_ptr->args.info.l_val + DAYSECS))
	return (true);
      break;
    }
  return (false);
}

boolean
pred_user (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  if (pred_ptr->args.uid == stat_buf->st_uid)
    return (true);
  else
    return (false);
}

boolean
pred_xtype (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct stat *stat_buf;
     struct predicate *pred_ptr;
{
  struct stat sbuf;
  int (*ystat) ();

  ystat = xstat == lstat ? stat : lstat;
  if ((*ystat) (rel_pathname, &sbuf) != 0)
    {
      if (ystat == stat && errno == ENOENT)
	/* Mimic behavior of ls -lL. */
	return (pred_type (pathname, stat_buf, pred_ptr));
      error (0, errno, "%s", pathname);
      exit_status = 1;
      return (false);
    }
  return (pred_type (pathname, &sbuf, pred_ptr));
}

/*  1) fork to get a child; parent remembers the child pid
    2) child execs the command requested
    3) parent waits for child; checks for proper pid of child

    Possible returns:

    ret		errno	status(h)   status(l)

    pid		x	signal#	    0177	stopped
    pid		x	exit arg    0		term by _exit
    pid		x	0	    signal #	term by signal
    -1		EINTR				parent got signal
    -1		other				some other kind of error

    Return true only if the pid matches, status(l) is
    zero, and the exit arg (status high) is 0.
    Otherwise return false, possibly printing an error message. */

static boolean
launch (pred_ptr)
     struct predicate *pred_ptr;
{
  int status;
  pid_t wait_ret, child_pid;
  struct exec_val *execp;	/* Pointer for efficiency. */
  static int first_time = 1;

  execp = &pred_ptr->args.exec_vec;

  /* Make sure output of command doesn't get mixed with find output. */
  fflush (stdout);
  fflush (stderr);

  /* Make sure to listen for the kids.  */
  if (first_time)
    {
      first_time = 0;
      signal (SIGCHLD, SIG_DFL);
    }

  child_pid = fork ();
  if (child_pid == -1)
    error (1, errno, "cannot fork");
  if (child_pid == 0)
    {
      /* We be the child. */
#ifndef HAVE_FCHDIR
      if (chdir (starting_dir) < 0)
	{
	  error (0, errno, "%s", starting_dir);
	  _exit (1);
	}
#else
      if (fchdir (starting_desc) < 0)
	{
	  error (0, errno, "cannot return to starting directory");
	  _exit (1);
	}
#endif
      execvp (execp->vec[0], execp->vec);
      error (0, errno, "%s", execp->vec[0]);
      _exit (1);
    }

  wait_ret = wait (&status);
  if (wait_ret == -1)
    {
      error (0, errno, "error waiting for %s", execp->vec[0]);
      exit_status = 1;
      return (false);
    }
  if (wait_ret != child_pid)
    {
      error (0, 0, "wait got pid %d, expected pid %d", wait_ret, child_pid);
      exit_status = 1;
      return (false);
    }
  if (WIFSTOPPED (status))
    {
      error (0, 0, "%s stopped by signal %d", 
	     execp->vec[0], WSTOPSIG (status));
      exit_status = 1;
      return (false);
    }
  if (WIFSIGNALED (status))
    {
      error (0, 0, "%s terminated by signal %d",
	     execp->vec[0], WTERMSIG (status));
      exit_status = 1;
      return (false);
    }
  return (!WEXITSTATUS (status));
}

/* Return a static string formatting the time WHEN according to the
   strftime format character KIND.  */

static char *
format_date (when, kind)
     time_t when;
     int kind;
{
  static char fmt[3];
  static char buf[64];		/* More than enough space. */

  if (kind == '@')
    {
      sprintf (buf, "%ld", when);
      return (buf);
    }
  else
    {
      fmt[0] = '%';
      fmt[1] = kind;
      fmt[2] = '\0';
      if (strftime (buf, sizeof (buf), fmt, localtime (&when)))
	return (buf);
    }
  return "";
}

#ifdef	DEBUG
/* Return a pointer to the string representation of 
   the predicate function PRED_FUNC. */

char *
find_pred_name (pred_func)
     PFB pred_func;
{
  int i;

  for (i = 0; pred_table[i].pred_func != 0; i++)
    if (pred_table[i].pred_func == pred_func)
      break;
  return (pred_table[i].pred_name);
}

static char *
type_name (type)
     short type;
{
  int i;

  for (i = 0; type_table[i].type != (short) -1; i++)
    if (type_table[i].type == type)
      break;
  return (type_table[i].type_name);
}

static char *
prec_name (prec)
     short prec;
{
  int i;

  for (i = 0; prec_table[i].prec != (short) -1; i++)
    if (prec_table[i].prec == prec)
      break;
  return (prec_table[i].prec_name);
}

/* Walk the expression tree NODE to stdout.
   INDENT is the number of levels to indent the left margin. */

void
print_tree (node, indent)
     struct predicate *node;
     int indent;
{
  int i;

  if (node == NULL)
    return;
  for (i = 0; i < indent; i++)
    printf ("    ");
  printf ("pred = %s type = %s prec = %s addr = %x\n",
	  find_pred_name (node->pred_func),
	  type_name (node->p_type), prec_name (node->p_prec), node);
  for (i = 0; i < indent; i++)
    printf ("    ");
  printf ("left:\n");
  print_tree (node->pred_left, indent + 1);
  for (i = 0; i < indent; i++)
    printf ("    ");
  printf ("right:\n");
  print_tree (node->pred_right, indent + 1);
}

/* Copy STR into BUF and trim blanks from the end of BUF.
   Return BUF. */

static char *
blank_rtrim (str, buf)
     char *str;
     char *buf;
{
  int i;

  if (str == NULL)
    return (NULL);
  strcpy (buf, str);
  i = strlen (buf) - 1;
  while ((i >= 0) && ((buf[i] == ' ') || buf[i] == '\t'))
    i--;
  buf[++i] = '\0';
  return (buf);
}

/* Print out the predicate list starting at NODE. */

void
print_list (node)
     struct predicate *node;
{
  struct predicate *cur;
  char name[256];

  cur = node;
  while (cur != NULL)
    {
      printf ("%s ", blank_rtrim (find_pred_name (cur->pred_func), name));
      cur = cur->pred_next;
    }
  printf ("\n");
}
#endif	/* DEBUG */
