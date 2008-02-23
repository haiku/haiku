/* GNU dump extensions to tar.

   Copyright (C) 1988, 1992, 1993, 1994, 1996, 1997, 1999, 2000, 2001,
   2003, 2004, 2005, 2006, 2007 Free Software Foundation, Inc.

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
#include <hash.h>
#include <quotearg.h>
#include "common.h"

/* Incremental dump specialities.  */

/* Which child files to save under a directory.  */
enum children
  {
    NO_CHILDREN,
    CHANGED_CHILDREN,
    ALL_CHILDREN
  };

#define DIRF_INIT     0x0001    /* directory structure is initialized
				   (procdir called at least once) */
#define DIRF_NFS      0x0002    /* directory is mounted on nfs */
#define DIRF_FOUND    0x0004    /* directory is found on fs */
#define DIRF_NEW      0x0008    /* directory is new (not found
				   in the previous dump) */
#define DIRF_RENAMED  0x0010    /* directory is renamed */

#define DIR_IS_INITED(d) ((d)->flags & DIRF_INIT)
#define DIR_IS_NFS(d) ((d)->flags & DIRF_NFS)
#define DIR_IS_FOUND(d) ((d)->flags & DIRF_FOUND)
#define DIR_IS_NEW(d) ((d)->flags & DIRF_NEW)
#define DIR_IS_RENAMED(d) ((d)->flags & DIRF_RENAMED)

#define DIR_SET_FLAG(d,f) (d)->flags |= (f)
#define DIR_CLEAR_FLAG(d,f) (d)->flags &= ~(f)

/* Directory attributes.  */
struct directory
  {
    struct timespec mtime;      /* Modification time */
    dev_t device_number;	/* device number for directory */
    ino_t inode_number;		/* inode number for directory */
    char *contents;             /* Directory contents */
    char *icontents;            /* Initial contents if the directory was
				   rescanned */
    enum children children;     /* What to save under this directory */
    unsigned flags;             /* See DIRF_ macros above */
    struct directory *orig;     /* If the directory was renamed, points to
				   the original directory structure */
    const char *tagfile;        /* Tag file, if the directory falls under
				   exclusion_tag_under */
    char name[1];		/* file name of directory */
  };

static Hash_table *directory_table;
static Hash_table *directory_meta_table;

#if HAVE_ST_FSTYPE_STRING
  static char const nfs_string[] = "nfs";
# define NFS_FILE_STAT(st) (strcmp ((st).st_fstype, nfs_string) == 0)
#else
# define ST_DEV_MSB(st) (~ (dev_t) 0 << (sizeof (st).st_dev * CHAR_BIT - 1))
# define NFS_FILE_STAT(st) (((st).st_dev & ST_DEV_MSB (st)) != 0)
#endif

/* Calculate the hash of a directory.  */
static size_t
hash_directory_name (void const *entry, size_t n_buckets)
{
  struct directory const *directory = entry;
  return hash_string (directory->name, n_buckets);
}

/* Compare two directories for equality of their names. */
static bool
compare_directory_names (void const *entry1, void const *entry2)
{
  struct directory const *directory1 = entry1;
  struct directory const *directory2 = entry2;
  return strcmp (directory1->name, directory2->name) == 0;
}

static size_t
hash_directory_meta (void const *entry, size_t n_buckets)
{
  struct directory const *directory = entry;
  /* FIXME: Work out a better algorytm */
  return (directory->device_number + directory->inode_number) % n_buckets;
}

/* Compare two directories for equality of their device and inode numbers. */
static bool
compare_directory_meta (void const *entry1, void const *entry2)
{
  struct directory const *directory1 = entry1;
  struct directory const *directory2 = entry2;
  return directory1->device_number == directory2->device_number
            && directory1->inode_number == directory2->inode_number;
}

/* Make a directory entry for given NAME */
static struct directory *
make_directory (const char *name)
{
  size_t namelen = strlen (name);
  size_t size = offsetof (struct directory, name) + namelen + 1;
  struct directory *directory = xmalloc (size);
  directory->contents = directory->icontents = NULL;
  directory->orig = NULL;
  directory->flags = false;
  strcpy (directory->name, name);
  if (ISSLASH (directory->name[namelen-1]))
    directory->name[namelen-1] = 0;
  directory->tagfile = NULL;
  return directory;
}

/* Create and link a new directory entry for directory NAME, having a
   device number DEV and an inode number INO, with NFS indicating
   whether it is an NFS device and FOUND indicating whether we have
   found that the directory exists.  */
static struct directory *
note_directory (char const *name, struct timespec mtime,
		dev_t dev, ino_t ino, bool nfs, bool found, char *contents)
{
  struct directory *directory = make_directory (name);

  directory->mtime = mtime;
  directory->device_number = dev;
  directory->inode_number = ino;
  directory->children = CHANGED_CHILDREN;
  if (nfs)
    DIR_SET_FLAG (directory, DIRF_NFS);
  if (found)
    DIR_SET_FLAG (directory, DIRF_FOUND);
  if (contents)
    {
      size_t size = dumpdir_size (contents);
      directory->contents = xmalloc (size);
      memcpy (directory->contents, contents, size);
    }
  else
    directory->contents = NULL;

  if (! ((directory_table
	  || (directory_table = hash_initialize (0, 0,
						 hash_directory_name,
						 compare_directory_names, 0)))
	 && hash_insert (directory_table, directory)))
    xalloc_die ();

  if (! ((directory_meta_table
	  || (directory_meta_table = hash_initialize (0, 0,
						      hash_directory_meta,
						      compare_directory_meta,
						      0)))
	 && hash_insert (directory_meta_table, directory)))
    xalloc_die ();

  return directory;
}

/* Return a directory entry for a given file NAME, or zero if none found.  */
static struct directory *
find_directory (const char *name)
{
  if (! directory_table)
    return 0;
  else
    {
      struct directory *dir = make_directory (name);
      struct directory *ret = hash_lookup (directory_table, dir);
      free (dir);
      return ret;
    }
}

/* Return a directory entry for a given combination of device and inode
   numbers, or zero if none found.  */
static struct directory *
find_directory_meta (dev_t dev, ino_t ino)
{
  if (! directory_meta_table)
    return 0;
  else
    {
      struct directory *dir = make_directory ("");
      struct directory *ret;
      dir->device_number = dev;
      dir->inode_number = ino;
      ret = hash_lookup (directory_meta_table, dir);
      free (dir);
      return ret;
    }
}

void
update_parent_directory (const char *name)
{
  struct directory *directory;
  char *p;

  p = dir_name (name);
  directory = find_directory (p);
  if (directory)
    {
      struct stat st;
      if (deref_stat (dereference_option, p, &st) != 0)
	stat_diag (name);
      else
	directory->mtime = get_stat_mtime (&st);
    }
  free (p);
}

static struct directory *
procdir (char *name_buffer, struct stat *stat_data,
	 dev_t device,
	 enum children children,
	 bool verbose,
	 char *entry)
{
  struct directory *directory;
  bool nfs = NFS_FILE_STAT (*stat_data);

  if ((directory = find_directory (name_buffer)) != NULL)
    {
      if (DIR_IS_INITED (directory))
	return directory;

      /* With NFS, the same file can have two different devices
	 if an NFS directory is mounted in multiple locations,
	 which is relatively common when automounting.
	 To avoid spurious incremental redumping of
	 directories, consider all NFS devices as equal,
	 relying on the i-node to establish differences.  */

      if (! (((DIR_IS_NFS (directory) & nfs)
	      || directory->device_number == stat_data->st_dev)
	     && directory->inode_number == stat_data->st_ino))
	{
	  /* FIXME: find_directory_meta ignores nfs */
	  struct directory *d = find_directory_meta (stat_data->st_dev,
						     stat_data->st_ino);
	  if (d)
	    {
	      if (verbose_option)
		WARN ((0, 0, _("%s: Directory has been renamed from %s"),
		       quotearg_colon (name_buffer),
		       quote_n (1, d->name)));
	      directory->orig = d;
	      DIR_SET_FLAG (directory, DIRF_RENAMED);
	      directory->children = CHANGED_CHILDREN;
	    }
	  else
	    {
	      if (verbose_option)
		WARN ((0, 0, _("%s: Directory has been renamed"),
		       quotearg_colon (name_buffer)));
	      directory->children = ALL_CHILDREN;
	      directory->device_number = stat_data->st_dev;
	      directory->inode_number = stat_data->st_ino;
	    }
	  if (nfs)
	    DIR_SET_FLAG (directory, DIRF_NFS);
	}
      else
	directory->children = CHANGED_CHILDREN;

      DIR_SET_FLAG (directory, DIRF_FOUND);
    }
  else
    {
      struct directory *d = find_directory_meta (stat_data->st_dev,
						 stat_data->st_ino);

      directory = note_directory (name_buffer,
				  get_stat_mtime(stat_data),
				  stat_data->st_dev,
				  stat_data->st_ino,
				  nfs,
				  true,
				  NULL);

      if (d)
	{
	  if (verbose)
	    WARN ((0, 0, _("%s: Directory has been renamed from %s"),
		   quotearg_colon (name_buffer),
		   quote_n (1, d->name)));
	  directory->orig = d;
	  DIR_SET_FLAG (directory, DIRF_RENAMED);
	  directory->children = CHANGED_CHILDREN;
	}
      else
	{
	  DIR_SET_FLAG (directory, DIRF_NEW);
	  if (verbose)
	    WARN ((0, 0, _("%s: Directory is new"),
		   quotearg_colon (name_buffer)));
	  directory->children =
	    (listed_incremental_option
	     || (OLDER_STAT_TIME (*stat_data, m)
		 || (after_date_option
		     && OLDER_STAT_TIME (*stat_data, c))))
	    ? ALL_CHILDREN
	    : CHANGED_CHILDREN;
	}
    }

  /* If the directory is on another device and --one-file-system was given,
     omit it... */
  if (one_file_system_option && device != stat_data->st_dev
      /* ... except if it was explicitely given in the command line */
      && !is_individual_file (name_buffer))
    directory->children = NO_CHILDREN;
  else if (children == ALL_CHILDREN)
    directory->children = ALL_CHILDREN;

  DIR_SET_FLAG (directory, DIRF_INIT);

  {
    const char *tag_file_name;
    size_t len;
    
    switch (check_exclusion_tags (name_buffer, &tag_file_name))
      {
      case exclusion_tag_all:
	/* This warning can be duplicated by code in dump_file0, but only
	   in case when the topmost directory being archived contains
	   an exclusion tag. */
	exclusion_tag_warning (name_buffer, tag_file_name,
			       _("directory not dumped"));
	if (entry)
	  *entry = 'N';
	directory->children = NO_CHILDREN;
	break;

      case exclusion_tag_contents:
	exclusion_tag_warning (name_buffer, tag_file_name,
			       _("contents not dumped"));
	directory->children = NO_CHILDREN;
	break;
	
      case exclusion_tag_under:
	exclusion_tag_warning (name_buffer, tag_file_name,
			       _("contents not dumped"));
	directory->tagfile = tag_file_name;
	break;
	
      case exclusion_tag_none:
	break;
      }
  }

  return directory;
}

/* Locate NAME in the dumpdir array DUMP.
   Return pointer to the slot in the array, or NULL if not found */
const char *
dumpdir_locate (const char *dump, const char *name)
{
  if (dump)
    while (*dump)
      {
	/* Ignore 'R' (rename) and 'X' (tempname) entries, since they break
	   alphabetical ordering.
	   They normally do not occur in dumpdirs from the snapshot files,
	   but this function is also used by purge_directory, which operates
	   on a dumpdir from the archive, hence the need for this test. */
	if (!strchr ("RX", *dump))
	  {
	    int rc = strcmp (dump + 1, name);
	    if (rc == 0)
	      return dump;
	    if (rc > 1)
	      break;
	  }
	dump += strlen (dump) + 1;
      }
  return NULL;
}

/* Return size in bytes of the dumpdir array P */
size_t
dumpdir_size (const char *p)
{
  size_t totsize = 0;

  while (*p)
    {
      size_t size = strlen (p) + 1;
      totsize += size;
      p += size;
    }
  return totsize + 1;
}

static int
compare_dirnames (const void *first, const void *second)
{
  char const *const *name1 = first;
  char const *const *name2 = second;
  return strcmp (*name1, *name2);
}

/* Compare dumpdir array from DIRECTORY with directory listing DIR and
   build a new dumpdir template.

   DIR must be returned by a previous call to savedir().

   File names in DIRECTORY->contents must be sorted
   alphabetically.

   DIRECTORY->contents is replaced with the created template. Each entry is
   prefixed with ' ' if it was present in DUMP and with 'Y' otherwise. */

void
makedumpdir (struct directory *directory, const char *dir)
{
  size_t i,
         dirsize,  /* Number of elements in DIR */
         len;      /* Length of DIR, including terminating nul */
  const char *p;
  char const **array;
  char *new_dump, *new_dump_ptr;
  const char *dump;

  if (directory->children == ALL_CHILDREN)
    dump = NULL;
  else if (DIR_IS_RENAMED (directory))
    dump = directory->orig->icontents ?
              directory->orig->icontents : directory->orig->contents;
  else
    dump = directory->contents;

  /* Count the size of DIR and the number of elements it contains */
  dirsize = 0;
  len = 0;
  for (p = dir; *p; p += strlen (p) + 1, dirsize++)
    len += strlen (p) + 2;
  len++;

  /* Create a sorted directory listing */
  array = xcalloc (dirsize, sizeof array[0]);
  for (i = 0, p = dir; *p; p += strlen (p) + 1, i++)
    array[i] = p;

  qsort (array, dirsize, sizeof (array[0]), compare_dirnames);

  /* Prepare space for new dumpdir */
  new_dump = xmalloc (len);
  new_dump_ptr = new_dump;

  /* Fill in the dumpdir template */
  for (i = 0; i < dirsize; i++)
    {
      const char *loc = dumpdir_locate (dump, array[i]);
      if (loc)
	{
	  if (directory->tagfile)
	    *new_dump_ptr = strcmp (directory->tagfile, array[i]) == 0 ?
		               ' ' : 'I';
	  else
	    *new_dump_ptr = ' ';
	  new_dump_ptr++;
	  dump = loc + strlen (loc) + 1;
	}
      else if (directory->tagfile)
	*new_dump_ptr++ = strcmp (directory->tagfile, array[i]) == 0 ?
		               ' ' : 'I';
      else
	*new_dump_ptr++ = 'Y'; /* New entry */

      /* Copy the file name */
      for (p = array[i]; (*new_dump_ptr++ = *p++); )
	;
    }
  *new_dump_ptr = 0;
  directory->icontents = directory->contents;
  directory->contents = new_dump;
  free (array);
}

/* Recursively scan the given directory. */
static char *
scan_directory (char *dir, dev_t device)
{
  char *dirp = savedir (dir);	/* for scanning directory */
  char *name_buffer;		/* directory, `/', and directory member */
  size_t name_buffer_size;	/* allocated size of name_buffer, minus 2 */
  size_t name_length;		/* used length in name_buffer */
  struct stat stat_data;
  struct directory *directory;
  
  if (! dirp)
    savedir_error (dir);

  name_buffer_size = strlen (dir) + NAME_FIELD_SIZE;
  name_buffer = xmalloc (name_buffer_size + 2);
  strcpy (name_buffer, dir);
  if (! ISSLASH (dir[strlen (dir) - 1]))
    strcat (name_buffer, "/");
  name_length = strlen (name_buffer);

  if (deref_stat (dereference_option, name_buffer, &stat_data))
    {
      stat_diag (name_buffer);
      /* FIXME: used to be
           children = CHANGED_CHILDREN;
	 but changed to: */
      free (name_buffer);
      free (dirp);
      return NULL;
    }

  directory = procdir (name_buffer, &stat_data, device, NO_CHILDREN, false,
		       NULL);
  
  if (dirp && directory->children != NO_CHILDREN)
    {
      char  *entry;	/* directory entry being scanned */
      size_t entrylen;	/* length of directory entry */

      makedumpdir (directory, dirp);

      for (entry = directory->contents;
	   (entrylen = strlen (entry)) != 0;
	   entry += entrylen + 1)
	{
	  if (name_buffer_size <= entrylen - 1 + name_length)
	    {
	      do
		name_buffer_size += NAME_FIELD_SIZE;
	      while (name_buffer_size <= entrylen - 1 + name_length);
	      name_buffer = xrealloc (name_buffer, name_buffer_size + 2);
	    }
	  strcpy (name_buffer + name_length, entry + 1);

	  if (*entry == 'I') /* Ignored entry */
	    *entry = 'N';
	  else if (excluded_name (name_buffer))
	    *entry = 'N';
	  else
	    {
	      if (deref_stat (dereference_option, name_buffer, &stat_data))
		{
		  stat_diag (name_buffer);
		  *entry = 'N';
		  continue;
		}

	      if (S_ISDIR (stat_data.st_mode))
		{
		  *entry = 'D';
		  procdir (name_buffer, &stat_data, device,
			   directory->children,
			   verbose_option, entry);
		}

	      else if (one_file_system_option && device != stat_data.st_dev)
		*entry = 'N';

	      else if (*entry == 'Y')
		/* New entry, skip further checks */;

	      /* FIXME: if (S_ISHIDDEN (stat_data.st_mode))?? */

	      else if (OLDER_STAT_TIME (stat_data, m)
		       && (!after_date_option
			   || OLDER_STAT_TIME (stat_data, c)))
		*entry = 'N';
	      else
		*entry = 'Y';
	    }
	}
    }

  free (name_buffer);
  if (dirp)
    free (dirp);

  return directory->contents;
}

char *
get_directory_contents (char *dir, dev_t device)
{
  return scan_directory (dir, device);
}


static void
obstack_code_rename (struct obstack *stk, char *from, char *to)
{
  char *s;

  s = from[0] == 0 ? from :
                     safer_name_suffix (from, false, absolute_names_option);
  obstack_1grow (stk, 'R');
  obstack_grow (stk, s, strlen (s) + 1);

  s = to[0] == 0 ? to:
                   safer_name_suffix (to, false, absolute_names_option);
  obstack_1grow (stk, 'T');
  obstack_grow (stk, s, strlen (s) + 1);
}

static bool
rename_handler (void *data, void *proc_data)
{
  struct directory *dir = data;
  struct obstack *stk = proc_data;

  if (DIR_IS_RENAMED (dir))
    {
      struct directory *prev, *p;

      /* Detect eventual cycles and clear DIRF_RENAMED flag, so these entries
	 are ignored when hit by this function next time.
	 If the chain forms a cycle, prev points to the entry DIR is renamed
	 from. In this case it still retains DIRF_RENAMED flag, which will be
	 cleared in the `else' branch below */
      for (prev = dir; prev && prev->orig != dir; prev = prev->orig)
	DIR_CLEAR_FLAG (prev, DIRF_RENAMED);

      if (prev == NULL)
	{
	  for (p = dir; p && p->orig; p = p->orig)
	    obstack_code_rename (stk, p->orig->name, p->name);
	}
      else
	{
	  char *temp_name;

	  DIR_CLEAR_FLAG (prev, DIRF_RENAMED);

	  /* Break the cycle by using a temporary name for one of its
	     elements.
	     First, create a temp name stub entry. */
	  temp_name = dir_name (dir->name);
	  obstack_1grow (stk, 'X');
	  obstack_grow (stk, temp_name, strlen (temp_name) + 1);

	  obstack_code_rename (stk, dir->name, "");

	  for (p = dir; p != prev; p = p->orig)
	    obstack_code_rename (stk, p->orig->name, p->name);

	  obstack_code_rename (stk, "", prev->name);
	}
    }
  return true;
}

const char *
append_incremental_renames (const char *dump)
{
  struct obstack stk;
  size_t size;

  if (directory_table == NULL)
    return dump;

  obstack_init (&stk);
  if (dump)
    {
      size = dumpdir_size (dump) - 1;
      obstack_grow (&stk, dump, size);
    }
  else
    size = 0;

  hash_do_for_each (directory_table, rename_handler, &stk);
  if (obstack_object_size (&stk) != size)
    {
      obstack_1grow (&stk, 0);
      dump = obstack_finish (&stk);
    }
  else
    obstack_free (&stk, NULL);
  return dump;
}



static FILE *listed_incremental_stream;

/* Version of incremental format snapshots (directory files) used by this
   tar. Currently it is supposed to be a single decimal number. 0 means
   incremental snapshots as per tar version before 1.15.2.

   The current tar version supports incremental versions from
   0 up to TAR_INCREMENTAL_VERSION, inclusive.
   It is able to create only snapshots of TAR_INCREMENTAL_VERSION */

#define TAR_INCREMENTAL_VERSION 2

/* Read incremental snapshot formats 0 and 1 */
static void
read_incr_db_01 (int version, const char *initbuf)
{
  int n;
  uintmax_t u;
  time_t sec;
  long int nsec;
  char *buf = 0;
  size_t bufsize;
  char *ebuf;
  long lineno = 1;

  if (version == 1)
    {
      if (getline (&buf, &bufsize, listed_incremental_stream) <= 0)
	{
	  read_error (listed_incremental_option);
	  free (buf);
	  return;
	}
      ++lineno;
    }
  else
    {
      buf = strdup (initbuf);
      bufsize = strlen (buf) + 1;
    }

  sec = TYPE_MINIMUM (time_t);
  nsec = -1;
  errno = 0;
  u = strtoumax (buf, &ebuf, 10);
  if (!errno && TYPE_MAXIMUM (time_t) < u)
    errno = ERANGE;
  if (errno || buf == ebuf)
    ERROR ((0, errno, "%s:%ld: %s",
	    quotearg_colon (listed_incremental_option),
	    lineno,
	    _("Invalid time stamp")));
  else
    {
      sec = u;

      if (version == 1 && *ebuf)
	{
	  char const *buf_ns = ebuf + 1;
	  errno = 0;
	  u = strtoumax (buf_ns, &ebuf, 10);
	  if (!errno && BILLION <= u)
	    errno = ERANGE;
	  if (errno || buf_ns == ebuf)
	    {
	      ERROR ((0, errno, "%s:%ld: %s",
		      quotearg_colon (listed_incremental_option),
		      lineno,
		      _("Invalid time stamp")));
	      sec = TYPE_MINIMUM (time_t);
	    }
	  else
	    nsec = u;
	}
      else
	{
	  /* pre-1 incremental format does not contain nanoseconds */
	  nsec = 0;
	}
    }
  newer_mtime_option.tv_sec = sec;
  newer_mtime_option.tv_nsec = nsec;


  while (0 < (n = getline (&buf, &bufsize, listed_incremental_stream)))
    {
      dev_t dev;
      ino_t ino;
      bool nfs = buf[0] == '+';
      char *strp = buf + nfs;
      struct timespec mtime;

      lineno++;

      if (buf[n - 1] == '\n')
	buf[n - 1] = '\0';

      if (version == 1)
	{
	  errno = 0;
	  u = strtoumax (strp, &ebuf, 10);
	  if (!errno && TYPE_MAXIMUM (time_t) < u)
	    errno = ERANGE;
	  if (errno || strp == ebuf || *ebuf != ' ')
	    {
	      ERROR ((0, errno, "%s:%ld: %s",
		      quotearg_colon (listed_incremental_option), lineno,
		      _("Invalid modification time (seconds)")));
	      sec = (time_t) -1;
	    }
	  else
	    sec = u;
	  strp = ebuf;

	  errno = 0;
	  u = strtoumax (strp, &ebuf, 10);
	  if (!errno && BILLION <= u)
	    errno = ERANGE;
	  if (errno || strp == ebuf || *ebuf != ' ')
	    {
	      ERROR ((0, errno, "%s:%ld: %s",
		      quotearg_colon (listed_incremental_option), lineno,
		      _("Invalid modification time (nanoseconds)")));
	      nsec = -1;
	    }
	  else
	    nsec = u;
	  mtime.tv_sec = sec;
	  mtime.tv_nsec = nsec;
	  strp = ebuf;
	}
      else
	memset (&mtime, 0, sizeof mtime);

      errno = 0;
      u = strtoumax (strp, &ebuf, 10);
      if (!errno && TYPE_MAXIMUM (dev_t) < u)
	errno = ERANGE;
      if (errno || strp == ebuf || *ebuf != ' ')
	{
	  ERROR ((0, errno, "%s:%ld: %s",
		  quotearg_colon (listed_incremental_option), lineno,
		  _("Invalid device number")));
	  dev = (dev_t) -1;
	}
      else
	dev = u;
      strp = ebuf;

      errno = 0;
      u = strtoumax (strp, &ebuf, 10);
      if (!errno && TYPE_MAXIMUM (ino_t) < u)
	errno = ERANGE;
      if (errno || strp == ebuf || *ebuf != ' ')
	{
	  ERROR ((0, errno, "%s:%ld: %s",
		  quotearg_colon (listed_incremental_option), lineno,
		  _("Invalid inode number")));
	  ino = (ino_t) -1;
	}
      else
	ino = u;
      strp = ebuf;

      strp++;
      unquote_string (strp);
      note_directory (strp, mtime, dev, ino, nfs, false, NULL);
    }
  free (buf);
}

/* Read a nul-terminated string from FP and store it in STK.
   Store the number of bytes read (including nul terminator) in PCOUNT.

   Return the last character read or EOF on end of file. */
static int
read_obstack (FILE *fp, struct obstack *stk, size_t *pcount)
{
  int c;
  size_t i;

  for (i = 0, c = getc (fp); c != EOF && c != 0; c = getc (fp), i++)
    obstack_1grow (stk, c);
  obstack_1grow (stk, 0);

  *pcount = i;
  return c;
}

/* Read from file FP a nul-terminated string and convert it to
   intmax_t.  Return the resulting value in PVAL.  Assume '-' has
   already been read.

   Throw a fatal error if the string cannot be converted or if the
   converted value is less than MIN_VAL.  */

static void
read_negative_num (FILE *fp, intmax_t min_val, intmax_t *pval)
{
  int c;
  size_t i;
  char buf[INT_BUFSIZE_BOUND (intmax_t)];
  char *ep;
  buf[0] = '-';

  for (i = 1; ISDIGIT (c = getc (fp)); i++)
    {
      if (i == sizeof buf - 1)
	FATAL_ERROR ((0, 0, _("Field too long while reading snapshot file")));
      buf[i] = c;
    }

  if (c < 0)
    {
      if (ferror (fp))
	FATAL_ERROR ((0, errno, _("Read error in snapshot file")));
      else
	FATAL_ERROR ((0, 0, _("Unexpected EOF in snapshot file")));
    }

  buf[i] = 0;
  errno = 0;
  *pval = strtoimax (buf, &ep, 10);
  if (c || errno || *pval < min_val)
    FATAL_ERROR ((0, errno, _("Unexpected field value in snapshot file")));
}

/* Read from file FP a nul-terminated string and convert it to
   uintmax_t.  Return the resulting value in PVAL.  Assume C has
   already been read.

   Throw a fatal error if the string cannot be converted or if the
   converted value exceeds MAX_VAL.

   Return the last character read or EOF on end of file. */

static int
read_unsigned_num (int c, FILE *fp, uintmax_t max_val, uintmax_t *pval)
{
  size_t i;
  char buf[UINTMAX_STRSIZE_BOUND], *ep;

  for (i = 0; ISDIGIT (c); i++)
    {
      if (i == sizeof buf - 1)
	FATAL_ERROR ((0, 0, _("Field too long while reading snapshot file")));
      buf[i] = c;
      c = getc (fp);
    }

  if (c < 0)
    {
      if (ferror (fp))
	FATAL_ERROR ((0, errno, _("Read error in snapshot file")));
      else if (i == 0)
	return c;
      else
	FATAL_ERROR ((0, 0, _("Unexpected EOF in snapshot file")));
    }

  buf[i] = 0;
  errno = 0;
  *pval = strtoumax (buf, &ep, 10);
  if (c || errno || max_val < *pval)
    FATAL_ERROR ((0, errno, _("Unexpected field value in snapshot file")));
  return c;
}

/* Read from file FP a nul-terminated string and convert it to
   uintmax_t.  Return the resulting value in PVAL.

   Throw a fatal error if the string cannot be converted or if the
   converted value exceeds MAX_VAL.

   Return the last character read or EOF on end of file. */

static int
read_num (FILE *fp, uintmax_t max_val, uintmax_t *pval)
{
  return read_unsigned_num (getc (fp), fp, max_val, pval);
}

/* Read from FP two NUL-terminated strings representing a struct
   timespec.  Return the resulting value in PVAL.

   Throw a fatal error if the string cannot be converted.  */

static void
read_timespec (FILE *fp, struct timespec *pval)
{
  int c = getc (fp);
  intmax_t i;
  uintmax_t u;

  if (c == '-')
    {
      read_negative_num (fp, TYPE_MINIMUM (time_t), &i);
      c = 0;
      pval->tv_sec = i;
    }
  else
    {
      c = read_unsigned_num (c, fp, TYPE_MAXIMUM (time_t), &u);
      pval->tv_sec = u;
    }

  if (c || read_num (fp, BILLION - 1, &u))
    FATAL_ERROR ((0, 0, "%s: %s",
		  quotearg_colon (listed_incremental_option),
		  _("Unexpected EOF in snapshot file")));
  pval->tv_nsec = u;
}

/* Read incremental snapshot format 2 */
static void
read_incr_db_2 ()
{
  uintmax_t u;
  struct obstack stk;

  obstack_init (&stk);

  read_timespec (listed_incremental_stream, &newer_mtime_option);

  for (;;)
    {
      struct timespec mtime;
      dev_t dev;
      ino_t ino;
      bool nfs;
      char *name;
      char *content;
      size_t s;

      if (read_num (listed_incremental_stream, 1, &u))
	return; /* Normal return */

      nfs = u;

      read_timespec (listed_incremental_stream, &mtime);

      if (read_num (listed_incremental_stream, TYPE_MAXIMUM (dev_t), &u))
	break;
      dev = u;

      if (read_num (listed_incremental_stream, TYPE_MAXIMUM (ino_t), &u))
	break;
      ino = u;

      if (read_obstack (listed_incremental_stream, &stk, &s))
	break;

      name = obstack_finish (&stk);

      while (read_obstack (listed_incremental_stream, &stk, &s) == 0 && s > 1)
	;
      if (getc (listed_incremental_stream) != 0)
	FATAL_ERROR ((0, 0, "%s: %s",
		      quotearg_colon (listed_incremental_option),
		      _("Missing record terminator")));

      content = obstack_finish (&stk);
      note_directory (name, mtime, dev, ino, nfs, false, content);
      obstack_free (&stk, content);
    }
  FATAL_ERROR ((0, 0, "%s: %s",
		quotearg_colon (listed_incremental_option),
		_("Unexpected EOF in snapshot file")));
}

/* Read incremental snapshot file (directory file).
   If the file has older incremental version, make sure that it is processed
   correctly and that tar will use the most conservative backup method among
   possible alternatives (i.e. prefer ALL_CHILDREN over CHANGED_CHILDREN,
   etc.) This ensures that the snapshots are updated to the recent version
   without any loss of data. */
void
read_directory_file (void)
{
  int fd;
  char *buf = 0;
  size_t bufsize;

  /* Open the file for both read and write.  That way, we can write
     it later without having to reopen it, and don't have to worry if
     we chdir in the meantime.  */
  fd = open (listed_incremental_option, O_RDWR | O_CREAT, MODE_RW);
  if (fd < 0)
    {
      open_error (listed_incremental_option);
      return;
    }

  listed_incremental_stream = fdopen (fd, "r+");
  if (! listed_incremental_stream)
    {
      open_error (listed_incremental_option);
      close (fd);
      return;
    }

  if (0 < getline (&buf, &bufsize, listed_incremental_stream))
    {
      char *ebuf;
      uintmax_t incremental_version;

      if (strncmp (buf, PACKAGE_NAME, sizeof PACKAGE_NAME - 1) == 0)
	{
	  ebuf = buf + sizeof PACKAGE_NAME - 1;
	  if (*ebuf++ != '-')
	    ERROR((1, 0, _("Bad incremental file format")));
	  for (; *ebuf != '-'; ebuf++)
	    if (!*ebuf)
	      ERROR((1, 0, _("Bad incremental file format")));

	  incremental_version = strtoumax (ebuf + 1, NULL, 10);
	}
      else
	incremental_version = 0;

      switch (incremental_version)
	{
	case 0:
	case 1:
	  read_incr_db_01 (incremental_version, buf);
	  break;

	case TAR_INCREMENTAL_VERSION:
	  read_incr_db_2 ();
	  break;

	default:
	  ERROR ((1, 0, _("Unsupported incremental format version: %"PRIuMAX),
		  incremental_version));
	}

    }

  if (ferror (listed_incremental_stream))
    read_error (listed_incremental_option);
  if (buf)
    free (buf);
}

/* Output incremental data for the directory ENTRY to the file DATA.
   Return nonzero if successful, preserving errno on write failure.  */
static bool
write_directory_file_entry (void *entry, void *data)
{
  struct directory const *directory = entry;
  FILE *fp = data;

  if (DIR_IS_FOUND (directory))
    {
      char buf[UINTMAX_STRSIZE_BOUND];
      char *s;

      s = DIR_IS_NFS (directory) ? "1" : "0";
      fwrite (s, 2, 1, fp);
      s = (TYPE_SIGNED (time_t)
	   ? imaxtostr (directory->mtime.tv_sec, buf)
	   : umaxtostr (directory->mtime.tv_sec, buf));
      fwrite (s, strlen (s) + 1, 1, fp);
      s = umaxtostr (directory->mtime.tv_nsec, buf);
      fwrite (s, strlen (s) + 1, 1, fp);
      s = umaxtostr (directory->device_number, buf);
      fwrite (s, strlen (s) + 1, 1, fp);
      s = umaxtostr (directory->inode_number, buf);
      fwrite (s, strlen (s) + 1, 1, fp);

      fwrite (directory->name, strlen (directory->name) + 1, 1, fp);
      if (directory->contents)
	{
	  char *p;
	  for (p = directory->contents; *p; p += strlen (p) + 1)
	    {
	      if (strchr ("YND", *p))
		fwrite (p, strlen (p) + 1, 1, fp);
	    }
	}
      fwrite ("\0\0", 2, 1, fp);
    }

  return ! ferror (fp);
}

void
write_directory_file (void)
{
  FILE *fp = listed_incremental_stream;
  char buf[UINTMAX_STRSIZE_BOUND];
  char *s;

  if (! fp)
    return;

  if (fseek (fp, 0L, SEEK_SET) != 0)
    seek_error (listed_incremental_option);
  if (sys_truncate (fileno (fp)) != 0)
    truncate_error (listed_incremental_option);

  fprintf (fp, "%s-%s-%d\n", PACKAGE_NAME, PACKAGE_VERSION,
	   TAR_INCREMENTAL_VERSION);

  s = (TYPE_SIGNED (time_t)
       ? imaxtostr (start_time.tv_sec, buf)
       : umaxtostr (start_time.tv_sec, buf));
  fwrite (s, strlen (s) + 1, 1, fp);
  s = umaxtostr (start_time.tv_nsec, buf);
  fwrite (s, strlen (s) + 1, 1, fp);

  if (! ferror (fp) && directory_table)
    hash_do_for_each (directory_table, write_directory_file_entry, fp);

  if (ferror (fp))
    write_error (listed_incremental_option);
  if (fclose (fp) != 0)
    close_error (listed_incremental_option);
}


/* Restoration of incremental dumps.  */

static void
get_gnu_dumpdir (struct tar_stat_info *stat_info)
{
  size_t size;
  size_t copied;
  union block *data_block;
  char *to;
  char *archive_dir;

  size = stat_info->stat.st_size;

  archive_dir = xmalloc (size);
  to = archive_dir;

  set_next_block_after (current_header);
  mv_begin (stat_info);

  for (; size > 0; size -= copied)
    {
      mv_size_left (size);
      data_block = find_next_block ();
      if (!data_block)
	ERROR ((1, 0, _("Unexpected EOF in archive")));
      copied = available_space_after (data_block);
      if (copied > size)
	copied = size;
      memcpy (to, data_block->buffer, copied);
      to += copied;
      set_next_block_after ((union block *)
			    (data_block->buffer + copied - 1));
    }

  mv_end ();

  stat_info->dumpdir = archive_dir;
  stat_info->skipped = true; /* For skip_member() and friends
				to work correctly */
}

/* Return T if STAT_INFO represents a dumpdir archive member.
   Note: can invalidate current_header. It happens if flush_archive()
   gets called within get_gnu_dumpdir() */
bool
is_dumpdir (struct tar_stat_info *stat_info)
{
  if (stat_info->is_dumpdir && !stat_info->dumpdir)
    get_gnu_dumpdir (stat_info);
  return stat_info->is_dumpdir;
}

static bool
dumpdir_ok (char *dumpdir)
{
  char *p;
  int has_tempdir = 0;
  int expect = 0;

  for (p = dumpdir; *p; p += strlen (p) + 1)
    {
      if (expect && *p != expect)
	{
	  ERROR ((0, 0,
		  _("Malformed dumpdir: expected '%c' but found %#3o"),
		  expect, *p));
	  return false;
	}
      switch (*p)
	{
	case 'X':
	  if (has_tempdir)
	    {
	      ERROR ((0, 0,
		      _("Malformed dumpdir: 'X' duplicated")));
	      return false;
	    }
	  else
	    has_tempdir = 1;
	  break;

	case 'R':
	  if (p[1] == 0)
	    {
	      if (!has_tempdir)
		{
		  ERROR ((0, 0,
			  _("Malformed dumpdir: empty name in 'R'")));
		  return false;
		}
	      else
		has_tempdir = 0;
	    }
	  expect = 'T';
	  break;

	case 'T':
	  if (expect != 'T')
	    {
	      ERROR ((0, 0,
		      _("Malformed dumpdir: 'T' not preceeded by 'R'")));
	      return false;
	    }
	  if (p[1] == 0 && !has_tempdir)
	    {
	      ERROR ((0, 0,
		      _("Malformed dumpdir: empty name in 'T'")));
	      return false;
	    }
	  expect = 0;
	  break;

	case 'N':
	case 'Y':
	case 'D':
	  break;

	default:
	  /* FIXME: bail out? */
	  break;
	}
    }

  if (expect)
    {
      ERROR ((0, 0,
	      _("Malformed dumpdir: expected '%c' but found end of data"),
	      expect));
      return false;
    }

  if (has_tempdir)
    WARN ((0, 0, _("Malformed dumpdir: 'X' never used")));

  return true;
}

/* Examine the directories under directory_name and delete any
   files that were not there at the time of the back-up. */
static bool
try_purge_directory (char const *directory_name)
{
  char *current_dir;
  char *cur, *arc, *p;
  char *temp_stub = NULL;

  if (!is_dumpdir (&current_stat_info))
    return false;

  current_dir = savedir (directory_name);

  if (!current_dir)
    /* The directory doesn't exist now.  It'll be created.  In any
       case, we don't have to delete any files out of it.  */
    return false;

  /* Verify if dump directory is sane */
  if (!dumpdir_ok (current_stat_info.dumpdir))
    return false;

  /* Process renames */
  for (arc = current_stat_info.dumpdir; *arc; arc += strlen (arc) + 1)
    {
      if (*arc == 'X')
	{
#define TEMP_DIR_TEMPLATE "tar.XXXXXX"
	  size_t len = strlen (arc + 1);
	  temp_stub = xrealloc (temp_stub, len + 1 + sizeof TEMP_DIR_TEMPLATE);
	  memcpy (temp_stub, arc + 1, len);
	  temp_stub[len] = '/';
	  memcpy (temp_stub + len + 1, TEMP_DIR_TEMPLATE,
		  sizeof TEMP_DIR_TEMPLATE);
	  if (!mkdtemp (temp_stub))
	    {
	      ERROR ((0, errno,
		      _("Cannot create temporary directory using template %s"),
		      quote (temp_stub)));
	      free (temp_stub);
	      free (current_dir);
	      return false;
	    }
	}
      else if (*arc == 'R')
	{
	  char *src, *dst;
	  src = arc + 1;
	  arc += strlen (arc) + 1;
	  dst = arc + 1;

	  /* Ensure that neither source nor destination are absolute file
	     names (unless permitted by -P option), and that they do not
	     contain dubious parts (e.g. ../).

	     This is an extra safety precaution. Besides, it might be
	     necessary to extract from archives created with tar versions
	     prior to 1.19. */
	  
	  if (*src)
	    src = safer_name_suffix (src, false, absolute_names_option);
	  if (*dst)
	    dst = safer_name_suffix (dst, false, absolute_names_option);
	  
	  if (*src == 0)
	    src = temp_stub;
	  else if (*dst == 0)
	    dst = temp_stub;

	  if (!rename_directory (src, dst))
	    {
	      free (temp_stub);
	      free (current_dir);
	      /* FIXME: Make sure purge_directory(dst) will return
		 immediately */
	      return false;
	    }
	}
    }

  free (temp_stub);

  /* Process deletes */
  p = NULL;
  for (cur = current_dir; *cur; cur += strlen (cur) + 1)
    {
      const char *entry;
      struct stat st;
      if (p)
	free (p);
      p = new_name (directory_name, cur);

      if (deref_stat (false, p, &st))
	{
	  if (errno != ENOENT) /* FIXME: Maybe keep a list of renamed
				  dirs and check it here? */
	    {
	      stat_diag (p);
	      WARN ((0, 0, _("%s: Not purging directory: unable to stat"),
		     quotearg_colon (p)));
	    }
	  continue;
	}

      if (!(entry = dumpdir_locate (current_stat_info.dumpdir, cur))
	  || (*entry == 'D' && !S_ISDIR (st.st_mode))
	  || (*entry == 'Y' && S_ISDIR (st.st_mode)))
	{
	  if (one_file_system_option && st.st_dev != root_device)
	    {
	      WARN ((0, 0,
		     _("%s: directory is on a different device: not purging"),
		     quotearg_colon (p)));
	      continue;
	    }

	  if (! interactive_option || confirm ("delete", p))
	    {
	      if (verbose_option)
		fprintf (stdlis, _("%s: Deleting %s\n"),
			 program_name, quote (p));
	      if (! remove_any_file (p, RECURSIVE_REMOVE_OPTION))
		{
		  int e = errno;
		  ERROR ((0, e, _("%s: Cannot remove"), quotearg_colon (p)));
		}
	    }
	}
    }
  free (p);

  free (current_dir);
  return true;
}

void
purge_directory (char const *directory_name)
{
  if (!try_purge_directory (directory_name))
    skip_member ();
}

void
list_dumpdir (char *buffer, size_t size)
{
  int state = 0;
  while (size)
    {
      switch (*buffer)
	{
	case 'Y':
	case 'N':
	case 'D':
	case 'R':
	case 'T':
	case 'X':
	  fprintf (stdlis, "%c", *buffer);
	  if (state == 0)
	    {
	      fprintf (stdlis, " ");
	      state = 1;
	    }
	  buffer++;
	  size--;
	  break;

	case 0:
	  fputc ('\n', stdlis);
	  buffer++;
	  size--;
	  state = 0;
	  break;

	default:
	  fputc (*buffer, stdlis);
	  buffer++;
	  size--;
	}
    }
}
