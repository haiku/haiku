/* Extract files from a tar archive.

   Copyright (C) 1988, 1992, 1993, 1994, 1996, 1997, 1998, 1999, 2000,
   2001, 2003, 2004, 2005, 2006, 2007 Free Software Foundation, Inc.

   Written by John Gilmore, on 1985-11-19.

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
#include <quotearg.h>
#include <utimens.h>
#include <errno.h>
#include <xgetcwd.h>

#include "common.h"

static bool we_are_root;	/* true if our effective uid == 0 */
static mode_t newdir_umask;	/* umask when creating new directories */
static mode_t current_umask;	/* current umask (which is set to 0 if -p) */

/* Status of the permissions of a file that we are extracting.  */
enum permstatus
{
  /* This file may have existed already; its permissions are unknown.  */
  UNKNOWN_PERMSTATUS,

  /* This file was created using the permissions from the archive,
     except with S_IRWXG | S_IRWXO masked out if 0 < same_owner_option.  */
  ARCHIVED_PERMSTATUS,

  /* This is an intermediate directory; the archive did not specify
     its permissions.  */
  INTERDIR_PERMSTATUS
};

/* List of directories whose statuses we need to extract after we've
   finished extracting their subsidiary files.  If you consider each
   contiguous subsequence of elements of the form [D]?[^D]*, where [D]
   represents an element where AFTER_LINKS is nonzero and [^D]
   represents an element where AFTER_LINKS is zero, then the head
   of the subsequence has the longest name, and each non-head element
   in the prefix is an ancestor (in the directory hierarchy) of the
   preceding element.  */

struct delayed_set_stat
  {
    struct delayed_set_stat *next;
    dev_t dev;
    ino_t ino;
    mode_t mode;
    uid_t uid;
    gid_t gid;
    struct timespec atime;
    struct timespec mtime;
    size_t file_name_len;
    mode_t invert_permissions;
    enum permstatus permstatus;
    bool after_links;
    char file_name[1];
  };

static struct delayed_set_stat *delayed_set_stat_head;

/* List of links whose creation we have delayed.  */
struct delayed_link
  {
    /* The next delayed link in the list.  */
    struct delayed_link *next;

    /* The device, inode number and last-modified time of the placeholder.  */
    dev_t dev;
    ino_t ino;
    struct timespec mtime;

    /* True if the link is symbolic.  */
    bool is_symlink;

    /* The desired owner and group of the link, if it is a symlink.  */
    uid_t uid;
    gid_t gid;

    /* A list of sources for this link.  The sources are all to be
       hard-linked together.  */
    struct string_list *sources;

    /* The desired target of the desired link.  */
    char target[1];
  };

static struct delayed_link *delayed_link_head;

struct string_list
  {
    struct string_list *next;
    char string[1];
  };

/*  Set up to extract files.  */
void
extr_init (void)
{
  we_are_root = geteuid () == 0;
  same_permissions_option += we_are_root;
  same_owner_option += we_are_root;

  /* Option -p clears the kernel umask, so it does not affect proper
     restoration of file permissions.  New intermediate directories will
     comply with umask at start of program.  */

  newdir_umask = umask (0);
  if (0 < same_permissions_option)
    current_umask = 0;
  else
    {
      umask (newdir_umask);	/* restore the kernel umask */
      current_umask = newdir_umask;
    }
}

/* If restoring permissions, restore the mode for FILE_NAME from
   information given in *STAT_INFO (where *CUR_INFO gives
   the current status if CUR_INFO is nonzero); otherwise invert the
   INVERT_PERMISSIONS bits from the file's current permissions.
   PERMSTATUS specifies the status of the file's permissions.
   TYPEFLAG specifies the type of the file.  */
static void
set_mode (char const *file_name,
	  struct stat const *stat_info,
	  struct stat const *cur_info,
	  mode_t invert_permissions, enum permstatus permstatus,
	  char typeflag)
{
  mode_t mode;

  if (0 < same_permissions_option
      && permstatus != INTERDIR_PERMSTATUS)
    {
      mode = stat_info->st_mode;

      /* If we created the file and it has a mode that we set already
	 with O_CREAT, then its mode is often set correctly already.
	 But if we are changing ownership, the mode's group and and
	 other permission bits were omitted originally, so it's less
	 likely that the mode is OK now.  Also, on many hosts, some
	 directories inherit the setgid bits from their parents, so we
	 we must set directories' modes explicitly.  */
      if ((permstatus == ARCHIVED_PERMSTATUS
	   && ! (mode & ~ (0 < same_owner_option ? S_IRWXU : MODE_RWX)))
	  && typeflag != DIRTYPE
	  && typeflag != GNUTYPE_DUMPDIR)
	return;
    }
  else if (! invert_permissions)
    return;
  else
    {
      /* We must inspect a directory's current permissions, since the
	 directory may have inherited its setgid bit from its parent.

	 INVERT_PERMISSIONS happens to be nonzero only for directories
	 that we created, so there's no point optimizing this code for
	 other cases.  */
      struct stat st;
      if (! cur_info)
	{
	  if (stat (file_name, &st) != 0)
	    {
	      stat_error (file_name);
	      return;
	    }
	  cur_info = &st;
	}
      mode = cur_info->st_mode ^ invert_permissions;
    }

  if (chmod (file_name, mode) != 0)
    chmod_error_details (file_name, mode);
}

/* Check time after successfully setting FILE_NAME's time stamp to T.  */
static void
check_time (char const *file_name, struct timespec t)
{
  if (t.tv_sec <= 0)
    WARN ((0, 0, _("%s: implausibly old time stamp %s"),
	   file_name, tartime (t, true)));
  else if (timespec_cmp (volume_start_time, t) < 0)
    {
      struct timespec now;
      gettime (&now);
      if (timespec_cmp (now, t) < 0)
	{
	  char buf[TIMESPEC_STRSIZE_BOUND];
	  struct timespec diff;
	  diff.tv_sec = t.tv_sec - now.tv_sec;
	  diff.tv_nsec = t.tv_nsec - now.tv_nsec;
	  if (diff.tv_nsec < 0)
	    {
	      diff.tv_nsec += BILLION;
	      diff.tv_sec--;
	    }
	  WARN ((0, 0, _("%s: time stamp %s is %s s in the future"),
		 file_name, tartime (t, true), code_timespec (diff, buf)));
	}
    }
}

/* Restore stat attributes (owner, group, mode and times) for
   FILE_NAME, using information given in *ST.
   If CUR_INFO is nonzero, *CUR_INFO is the
   file's current status.
   If not restoring permissions, invert the
   INVERT_PERMISSIONS bits from the file's current permissions.
   PERMSTATUS specifies the status of the file's permissions.
   TYPEFLAG specifies the type of the file.  */

/* FIXME: About proper restoration of symbolic link attributes, we still do
   not have it right.  Pretesters' reports tell us we need further study and
   probably more configuration.  For now, just use lchown if it exists, and
   punt for the rest.  Sigh!  */

static void
set_stat (char const *file_name,
	  struct tar_stat_info const *st,
	  struct stat const *cur_info,
	  mode_t invert_permissions, enum permstatus permstatus,
	  char typeflag)
{
  if (typeflag != SYMTYPE)
    {
      /* We do the utime before the chmod because some versions of utime are
	 broken and trash the modes of the file.  */

      if (! touch_option && permstatus != INTERDIR_PERMSTATUS)
	{
	  /* We set the accessed time to `now', which is really the time we
	     started extracting files, unless incremental_option is used, in
	     which case .st_atime is used.  */

	  /* FIXME: incremental_option should set ctime too, but how?  */

	  struct timespec ts[2];
	  if (incremental_option)
	    ts[0] = st->atime;
	  else
	    ts[0] = start_time;
	  ts[1] = st->mtime;

	  if (utimens (file_name, ts) != 0)
	    utime_error (file_name);
	  else
	    {
	      check_time (file_name, ts[0]);
	      check_time (file_name, ts[1]);
	    }
	}

      /* Some systems allow non-root users to give files away.  Once this
	 done, it is not possible anymore to change file permissions.
	 However, setting file permissions now would be incorrect, since
	 they would apply to the wrong user, and there would be a race
	 condition.  So, don't use systems that allow non-root users to
	 give files away.  */
    }

  if (0 < same_owner_option && permstatus != INTERDIR_PERMSTATUS)
    {
      /* When lchown exists, it should be used to change the attributes of
	 the symbolic link itself.  In this case, a mere chown would change
	 the attributes of the file the symbolic link is pointing to, and
	 should be avoided.  */
      int chown_result = 1;

      if (typeflag == SYMTYPE)
	{
#if HAVE_LCHOWN
	  chown_result = lchown (file_name, st->stat.st_uid, st->stat.st_gid);
#endif
	}
      else
	{
	  chown_result = chown (file_name, st->stat.st_uid, st->stat.st_gid);
	}

      if (chown_result == 0)
	{
	  /* Changing the owner can flip st_mode bits in some cases, so
	     ignore cur_info if it might be obsolete now.  */
	  if (cur_info
	      && cur_info->st_mode & S_IXUGO
	      && cur_info->st_mode & (S_ISUID | S_ISGID))
	    cur_info = NULL;
	}
      else if (chown_result < 0)
	chown_error_details (file_name,
			     st->stat.st_uid, st->stat.st_gid);
    }

  if (typeflag != SYMTYPE)
    set_mode (file_name, &st->stat, cur_info,
	      invert_permissions, permstatus, typeflag);
}

/* Remember to restore stat attributes (owner, group, mode and times)
   for the directory FILE_NAME, using information given in *ST,
   once we stop extracting files into that directory.
   If not restoring permissions, remember to invert the
   INVERT_PERMISSIONS bits from the file's current permissions.
   PERMSTATUS specifies the status of the file's permissions.

   NOTICE: this works only if the archive has usual member order, i.e.
   directory, then the files in that directory. Incremental archive have
   somewhat reversed order: first go subdirectories, then all other
   members. To help cope with this case the variable
   delay_directory_restore_option is set by prepare_to_extract.

   If an archive was explicitely created so that its member order is
   reversed, some directory timestamps can be restored incorrectly,
   e.g.:
       tar --no-recursion -cf archive dir dir/file1 foo dir/file2
*/
static void
delay_set_stat (char const *file_name, struct tar_stat_info const *st,
		mode_t invert_permissions, enum permstatus permstatus)
{
  size_t file_name_len = strlen (file_name);
  struct delayed_set_stat *data =
    xmalloc (offsetof (struct delayed_set_stat, file_name)
	     + file_name_len + 1);
  data->next = delayed_set_stat_head;
  data->dev = st->stat.st_dev;
  data->ino = st->stat.st_ino;
  data->mode = st->stat.st_mode;
  data->uid = st->stat.st_uid;
  data->gid = st->stat.st_gid;
  data->atime = st->atime;
  data->mtime = st->mtime;
  data->file_name_len = file_name_len;
  data->invert_permissions = invert_permissions;
  data->permstatus = permstatus;
  data->after_links = 0;
  strcpy (data->file_name, file_name);
  delayed_set_stat_head = data;
}

/* Update the delayed_set_stat info for an intermediate directory
   created within the file name of DIR.  The intermediate directory turned
   out to be the same as this directory, e.g. due to ".." or symbolic
   links.  *DIR_STAT_INFO is the status of the directory.  */
static void
repair_delayed_set_stat (char const *dir,
			 struct stat const *dir_stat_info)
{
  struct delayed_set_stat *data;
  for (data = delayed_set_stat_head;  data;  data = data->next)
    {
      struct stat st;
      if (stat (data->file_name, &st) != 0)
	{
	  stat_error (data->file_name);
	  return;
	}

      if (st.st_dev == dir_stat_info->st_dev
	  && st.st_ino == dir_stat_info->st_ino)
	{
	  data->dev = current_stat_info.stat.st_dev;
	  data->ino = current_stat_info.stat.st_ino;
	  data->mode = current_stat_info.stat.st_mode;
	  data->uid = current_stat_info.stat.st_uid;
	  data->gid = current_stat_info.stat.st_gid;
	  data->atime = current_stat_info.atime;
	  data->mtime = current_stat_info.mtime;
	  data->invert_permissions =
	    ((current_stat_info.stat.st_mode ^ st.st_mode)
	     & MODE_RWX & ~ current_umask);
	  data->permstatus = ARCHIVED_PERMSTATUS;
	  return;
	}
    }

  ERROR ((0, 0, _("%s: Unexpected inconsistency when making directory"),
	  quotearg_colon (dir)));
}

/* After a file/link/directory creation has failed, see if
   it's because some required directory was not present, and if so,
   create all required directories.  Return non-zero if a directory
   was created.  */
static int
make_directories (char *file_name)
{
  char *cursor0 = file_name + FILE_SYSTEM_PREFIX_LEN (file_name);
  char *cursor;	        	/* points into the file name */
  int did_something = 0;	/* did we do anything yet? */
  int mode;
  int invert_permissions;
  int status;

  for (cursor = cursor0; *cursor; cursor++)
    {
      if (! ISSLASH (*cursor))
	continue;

      /* Avoid mkdir of empty string, if leading or double '/'.  */

      if (cursor == cursor0 || ISSLASH (cursor[-1]))
	continue;

      /* Avoid mkdir where last part of file name is "." or "..".  */

      if (cursor[-1] == '.'
	  && (cursor == cursor0 + 1 || ISSLASH (cursor[-2])
	      || (cursor[-2] == '.'
		  && (cursor == cursor0 + 2 || ISSLASH (cursor[-3])))))
	continue;

      *cursor = '\0';		/* truncate the name there */
      mode = MODE_RWX & ~ newdir_umask;
      invert_permissions = we_are_root ? 0 : MODE_WXUSR & ~ mode;
      status = mkdir (file_name, mode ^ invert_permissions);

      if (status == 0)
	{
	  /* Create a struct delayed_set_stat even if
	     invert_permissions is zero, because
	     repair_delayed_set_stat may need to update the struct.  */
	  delay_set_stat (file_name,
			  &current_stat_info,
			  invert_permissions, INTERDIR_PERMSTATUS);

	  print_for_mkdir (file_name, cursor - file_name, mode);
	  did_something = 1;

	  *cursor = '/';
	  continue;
	}

      *cursor = '/';

      if (errno == EEXIST)
	continue;	        /* Directory already exists.  */
      else if ((errno == ENOSYS /* Automounted dirs on Solaris return
				   this. Reported by Warren Hyde
				   <Warren.Hyde@motorola.com> */
	       || ERRNO_IS_EACCES)  /* Turbo C mkdir gives a funny errno.  */
	       && access (file_name, W_OK) == 0)
	continue;

      /* Some other error in the mkdir.  We return to the caller.  */
      break;
    }

  return did_something;		/* tell them to retry if we made one */
}

static bool
file_newer_p (const char *file_name, struct tar_stat_info *tar_stat)
{
  struct stat st;

  if (stat (file_name, &st))
    {
      stat_warn (file_name);
      /* Be on the safe side: if the file does exist assume it is newer */
      return errno != ENOENT;
    }
  if (!S_ISDIR (st.st_mode)
      && tar_timespec_cmp (tar_stat->mtime, get_stat_mtime (&st)) <= 0)
    {
      return true;
    }
  return false;
}

/* Attempt repairing what went wrong with the extraction.  Delete an
   already existing file or create missing intermediate directories.
   Return nonzero if we somewhat increased our chances at a successful
   extraction.  errno is properly restored on zero return.  */
static int
maybe_recoverable (char *file_name, int *interdir_made)
{
  int e = errno;

  if (*interdir_made)
    return 0;

  switch (errno)
    {
    case EEXIST:
      /* Remove an old file, if the options allow this.  */

      switch (old_files_option)
	{
	case KEEP_OLD_FILES:
	  return 0;

	case KEEP_NEWER_FILES:
	  if (file_newer_p (file_name, &current_stat_info))
	    {
	      errno = e;
	      return 0;
	    }
	  /* FALL THROUGH */

	case DEFAULT_OLD_FILES:
	case NO_OVERWRITE_DIR_OLD_FILES:
	case OVERWRITE_OLD_FILES:
	  {
	    int r = remove_any_file (file_name, ORDINARY_REMOVE_OPTION);
	    errno = EEXIST;
	    return r;
	  }

	case UNLINK_FIRST_OLD_FILES:
	  break;
	}

    case ENOENT:
      /* Attempt creating missing intermediate directories.  */
      if (! make_directories (file_name))
	{
	  errno = ENOENT;
	  return 0;
	}
      *interdir_made = 1;
      return 1;

    default:
      /* Just say we can't do anything about it...  */

      return 0;
    }
}

/* Fix the statuses of all directories whose statuses need fixing, and
   which are not ancestors of FILE_NAME.  If AFTER_LINKS is
   nonzero, do this for all such directories; otherwise, stop at the
   first directory that is marked to be fixed up only after delayed
   links are applied.  */
static void
apply_nonancestor_delayed_set_stat (char const *file_name, bool after_links)
{
  size_t file_name_len = strlen (file_name);
  bool check_for_renamed_directories = 0;

  while (delayed_set_stat_head)
    {
      struct delayed_set_stat *data = delayed_set_stat_head;
      bool skip_this_one = 0;
      struct stat st;
      struct stat const *cur_info = 0;

      check_for_renamed_directories |= data->after_links;

      if (after_links < data->after_links
	  || (data->file_name_len < file_name_len
	      && file_name[data->file_name_len]
	      && (ISSLASH (file_name[data->file_name_len])
		  || ISSLASH (file_name[data->file_name_len - 1]))
	      && memcmp (file_name, data->file_name, data->file_name_len) == 0))
	break;

      if (check_for_renamed_directories)
	{
	  cur_info = &st;
	  if (stat (data->file_name, &st) != 0)
	    {
	      stat_error (data->file_name);
	      skip_this_one = 1;
	    }
	  else if (! (st.st_dev == data->dev && st.st_ino == data->ino))
	    {
	      ERROR ((0, 0,
		      _("%s: Directory renamed before its status could be extracted"),
		      quotearg_colon (data->file_name)));
	      skip_this_one = 1;
	    }
	}

      if (! skip_this_one)
	{
	  struct tar_stat_info sb;
	  sb.stat.st_mode = data->mode;
	  sb.stat.st_uid = data->uid;
	  sb.stat.st_gid = data->gid;
	  sb.atime = data->atime;
	  sb.mtime = data->mtime;
	  set_stat (data->file_name, &sb, cur_info,
		    data->invert_permissions, data->permstatus, DIRTYPE);
	}

      delayed_set_stat_head = data->next;
      free (data);
    }
}



/* Extractor functions for various member types */

static int
extract_dir (char *file_name, int typeflag)
{
  int status;
  mode_t mode;
  int interdir_made = 0;

  /* Save 'root device' to avoid purging mount points. */
  if (one_file_system_option && root_device == 0)
    {
      struct stat st;
      char *dir = xgetcwd ();

      if (deref_stat (true, dir, &st))
	stat_diag (dir);
      else
	root_device = st.st_dev;
      free (dir);
    }

  if (incremental_option)
    /* Read the entry and delete files that aren't listed in the archive.  */
    purge_directory (file_name);
  else if (typeflag == GNUTYPE_DUMPDIR)
    skip_member ();

  mode = current_stat_info.stat.st_mode | (we_are_root ? 0 : MODE_WXUSR);
  if (0 < same_owner_option || current_stat_info.stat.st_mode & ~ MODE_RWX)
    mode &= S_IRWXU;

  while ((status = mkdir (file_name, mode)))
    {
      if (errno == EEXIST
	  && (interdir_made
	      || old_files_option == DEFAULT_OLD_FILES
	      || old_files_option == OVERWRITE_OLD_FILES))
	{
	  struct stat st;
	  if (stat (file_name, &st) == 0)
	    {
	      if (interdir_made)
		{
		  repair_delayed_set_stat (file_name, &st);
		  return 0;
		}
	      if (S_ISDIR (st.st_mode))
		{
		  mode = st.st_mode;
		  break;
		}
	    }
	  errno = EEXIST;
	}

      if (maybe_recoverable (file_name, &interdir_made))
	continue;

      if (errno != EEXIST)
	{
	  mkdir_error (file_name);
	  return 1;
	}
      break;
    }

  if (status == 0
      || old_files_option == DEFAULT_OLD_FILES
      || old_files_option == OVERWRITE_OLD_FILES)
    {
      if (status == 0)
	delay_set_stat (file_name, &current_stat_info,
			((mode ^ current_stat_info.stat.st_mode)
			 & MODE_RWX & ~ current_umask),
			ARCHIVED_PERMSTATUS);
      else /* For an already existing directory, invert_perms must be 0 */
	delay_set_stat (file_name, &current_stat_info,
			0,
			UNKNOWN_PERMSTATUS);
    }
  return status;
}


static int
open_output_file (char *file_name, int typeflag, mode_t mode)
{
  int fd;
  int openflag = (O_WRONLY | O_BINARY | O_CREAT
		  | (old_files_option == OVERWRITE_OLD_FILES
		     ? O_TRUNC
		     : O_EXCL));

#if O_CTG
  /* Contiguous files (on the Masscomp) have to specify the size in
     the open call that creates them.  */

  if (typeflag == CONTTYPE)
    fd = open (file_name, openflag | O_CTG, mode, current_stat_info.stat.st_size);
  else
    fd = open (file_name, openflag, mode);

#else /* not O_CTG */
  if (typeflag == CONTTYPE)
    {
      static int conttype_diagnosed;

      if (!conttype_diagnosed)
	{
	  conttype_diagnosed = 1;
	  WARN ((0, 0, _("Extracting contiguous files as regular files")));
	}
    }
  fd = open (file_name, openflag, mode);

#endif /* not O_CTG */

  return fd;
}

static int
extract_file (char *file_name, int typeflag)
{
  int fd;
  off_t size;
  union block *data_block;
  int status;
  size_t count;
  size_t written;
  int interdir_made = 0;
  mode_t mode = current_stat_info.stat.st_mode & MODE_RWX & ~ current_umask;
  mode_t invert_permissions =
    0 < same_owner_option ? mode & (S_IRWXG | S_IRWXO) : 0;

  /* FIXME: deal with protection issues.  */

  if (to_stdout_option)
    fd = STDOUT_FILENO;
  else if (to_command_option)
    {
      fd = sys_exec_command (file_name, 'f', &current_stat_info);
      if (fd < 0)
	{
	  skip_member ();
	  return 0;
	}
    }
  else
    {
      do
	fd = open_output_file (file_name, typeflag, mode ^ invert_permissions);
      while (fd < 0 && maybe_recoverable (file_name, &interdir_made));

      if (fd < 0)
	{
	  skip_member ();
	  open_error (file_name);
	  return 1;
	}
    }

  mv_begin (&current_stat_info);
  if (current_stat_info.is_sparse)
    sparse_extract_file (fd, &current_stat_info, &size);
  else
    for (size = current_stat_info.stat.st_size; size > 0; )
      {
	mv_size_left (size);

	/* Locate data, determine max length writeable, write it,
	   block that we have used the data, then check if the write
	   worked.  */

	data_block = find_next_block ();
	if (! data_block)
	  {
	    ERROR ((0, 0, _("Unexpected EOF in archive")));
	    break;		/* FIXME: What happens, then?  */
	  }

	written = available_space_after (data_block);

	if (written > size)
	  written = size;
	errno = 0;
	count = full_write (fd, data_block->buffer, written);
	size -= written;

	set_next_block_after ((union block *)
			      (data_block->buffer + written - 1));
	if (count != written)
	  {
	    if (!to_command_option)
	      write_error_details (file_name, count, written);
	    /* FIXME: shouldn't we restore from backup? */
	    break;
	  }
      }

  skip_file (size);

  mv_end ();

  /* If writing to stdout, don't try to do anything to the filename;
     it doesn't exist, or we don't want to touch it anyway.  */

  if (to_stdout_option)
    return 0;

  status = close (fd);
  if (status < 0)
    close_error (file_name);

  if (to_command_option)
    sys_wait_command ();
  else
    set_stat (file_name, &current_stat_info, NULL, invert_permissions,
	      (old_files_option == OVERWRITE_OLD_FILES ?
	       UNKNOWN_PERMSTATUS : ARCHIVED_PERMSTATUS),
	      typeflag);

  return status;
}

/* Create a placeholder file with name FILE_NAME, which will be
   replaced after other extraction is done by a symbolic link if
   IS_SYMLINK is true, and by a hard link otherwise.  Set
   *INTERDIR_MADE if an intermediate directory is made in the
   process.  */

static int
create_placeholder_file (char *file_name, bool is_symlink, int *interdir_made)
{
  int fd;
  struct stat st;

  while ((fd = open (file_name, O_WRONLY | O_CREAT | O_EXCL, 0)) < 0)
    if (! maybe_recoverable (file_name, interdir_made))
      break;

  if (fd < 0)
    open_error (file_name);
  else if (fstat (fd, &st) != 0)
    {
      stat_error (file_name);
      close (fd);
    }
  else if (close (fd) != 0)
    close_error (file_name);
  else
    {
      struct delayed_set_stat *h;
      struct delayed_link *p =
	xmalloc (offsetof (struct delayed_link, target)
		 + strlen (current_stat_info.link_name)
		 + 1);
      p->next = delayed_link_head;
      delayed_link_head = p;
      p->dev = st.st_dev;
      p->ino = st.st_ino;
      p->mtime = get_stat_mtime (&st);
      p->is_symlink = is_symlink;
      if (is_symlink)
	{
	  p->uid = current_stat_info.stat.st_uid;
	  p->gid = current_stat_info.stat.st_gid;
	}
      p->sources = xmalloc (offsetof (struct string_list, string)
			    + strlen (file_name) + 1);
      p->sources->next = 0;
      strcpy (p->sources->string, file_name);
      strcpy (p->target, current_stat_info.link_name);

      h = delayed_set_stat_head;
      if (h && ! h->after_links
	  && strncmp (file_name, h->file_name, h->file_name_len) == 0
	  && ISSLASH (file_name[h->file_name_len])
	  && (last_component (file_name) == file_name + h->file_name_len + 1))
	{
	  do
	    {
	      h->after_links = 1;

	      if (stat (h->file_name, &st) != 0)
		stat_error (h->file_name);
	      else
		{
		  h->dev = st.st_dev;
		  h->ino = st.st_ino;
		}
	    }
	  while ((h = h->next) && ! h->after_links);
	}

      return 0;
    }

  return -1;
}

static int
extract_link (char *file_name, int typeflag)
{
  int interdir_made = 0;
  char const *link_name;

  transform_member_name (&current_stat_info.link_name, xform_link);
  link_name = current_stat_info.link_name;
  
  if (! absolute_names_option && contains_dot_dot (link_name))
    return create_placeholder_file (file_name, false, &interdir_made);

  do
    {
      struct stat st1, st2;
      int e;
      int status = link (link_name, file_name);
      e = errno;

      if (status == 0)
	{
	  struct delayed_link *ds = delayed_link_head;
	  if (ds && lstat (link_name, &st1) == 0)
	    for (; ds; ds = ds->next)
	      if (ds->dev == st1.st_dev
		  && ds->ino == st1.st_ino
		  && timespec_cmp (ds->mtime, get_stat_mtime (&st1)) == 0)
		{
		  struct string_list *p =  xmalloc (offsetof (struct string_list, string)
						    + strlen (file_name) + 1);
		  strcpy (p->string, file_name);
		  p->next = ds->sources;
		  ds->sources = p;
		  break;
		}
	  return 0;
	}
      else if ((e == EEXIST && strcmp (link_name, file_name) == 0)
	       || (lstat (link_name, &st1) == 0
		   && lstat (file_name, &st2) == 0
		   && st1.st_dev == st2.st_dev
		   && st1.st_ino == st2.st_ino))
	return 0;

      errno = e;
    }
  while (maybe_recoverable (file_name, &interdir_made));

  if (!(incremental_option && errno == EEXIST))
    {
      link_error (link_name, file_name);
      return 1;
    }
  return 0;
}

static int
extract_symlink (char *file_name, int typeflag)
{
#ifdef HAVE_SYMLINK
  int status;
  int interdir_made = 0;

  transform_member_name (&current_stat_info.link_name, xform_symlink);

  if (! absolute_names_option
      && (IS_ABSOLUTE_FILE_NAME (current_stat_info.link_name)
	  || contains_dot_dot (current_stat_info.link_name)))
    return create_placeholder_file (file_name, true, &interdir_made);

  while ((status = symlink (current_stat_info.link_name, file_name)))
    if (!maybe_recoverable (file_name, &interdir_made))
      break;

  if (status == 0)
    set_stat (file_name, &current_stat_info, NULL, 0, 0, SYMTYPE);
  else
    symlink_error (current_stat_info.link_name, file_name);
  return status;

#else
  static int warned_once;

  if (!warned_once)
    {
      warned_once = 1;
      WARN ((0, 0, _("Attempting extraction of symbolic links as hard links")));
    }
  return extract_link (file_name, typeflag);
#endif
}

#if S_IFCHR || S_IFBLK
static int
extract_node (char *file_name, int typeflag)
{
  int status;
  int interdir_made = 0;
  mode_t mode = current_stat_info.stat.st_mode & ~ current_umask;
  mode_t invert_permissions =
    0 < same_owner_option ? mode & (S_IRWXG | S_IRWXO) : 0;

  do
    status = mknod (file_name, mode ^ invert_permissions,
		    current_stat_info.stat.st_rdev);
  while (status && maybe_recoverable (file_name, &interdir_made));

  if (status != 0)
    mknod_error (file_name);
  else
    set_stat (file_name, &current_stat_info, NULL, invert_permissions,
	      ARCHIVED_PERMSTATUS, typeflag);
  return status;
}
#endif

#if HAVE_MKFIFO || defined mkfifo
static int
extract_fifo (char *file_name, int typeflag)
{
  int status;
  int interdir_made = 0;
  mode_t mode = current_stat_info.stat.st_mode & ~ current_umask;
  mode_t invert_permissions =
    0 < same_owner_option ? mode & (S_IRWXG | S_IRWXO) : 0;

  while ((status = mkfifo (file_name, mode)) != 0)
    if (!maybe_recoverable (file_name, &interdir_made))
      break;

  if (status == 0)
    set_stat (file_name, &current_stat_info, NULL, invert_permissions,
	      ARCHIVED_PERMSTATUS, typeflag);
  else
    mkfifo_error (file_name);
  return status;
}
#endif

static int
extract_volhdr (char *file_name, int typeflag)
{
  if (verbose_option)
    fprintf (stdlis, _("Reading %s\n"), quote (current_stat_info.file_name));
  skip_member ();
  return 0;
}

static int
extract_failure (char *file_name, int typeflag)
{
  return 1;
}

typedef int (*tar_extractor_t) (char *file_name, int typeflag);



/* Prepare to extract a file. Find extractor function.
   Return zero if extraction should not proceed.  */

static int
prepare_to_extract (char const *file_name, int typeflag, tar_extractor_t *fun)
{
  int rc = 1;

  if (EXTRACT_OVER_PIPE)
    rc = 0;

  /* Select the extractor */
  switch (typeflag)
    {
    case GNUTYPE_SPARSE:
      *fun = extract_file;
      rc = 1;
      break;

    case AREGTYPE:
    case REGTYPE:
    case CONTTYPE:
      /* Appears to be a file.  But BSD tar uses the convention that a slash
	 suffix means a directory.  */
      if (current_stat_info.had_trailing_slash)
	*fun = extract_dir;
      else
	{
	  *fun = extract_file;
	  rc = 1;
	}
      break;

    case SYMTYPE:
      *fun = extract_symlink;
      break;

    case LNKTYPE:
      *fun = extract_link;
      break;

#if S_IFCHR
    case CHRTYPE:
      current_stat_info.stat.st_mode |= S_IFCHR;
      *fun = extract_node;
      break;
#endif

#if S_IFBLK
    case BLKTYPE:
      current_stat_info.stat.st_mode |= S_IFBLK;
      *fun = extract_node;
      break;
#endif

#if HAVE_MKFIFO || defined mkfifo
    case FIFOTYPE:
      *fun = extract_fifo;
      break;
#endif

    case DIRTYPE:
    case GNUTYPE_DUMPDIR:
      *fun = extract_dir;
      if (current_stat_info.is_dumpdir)
	delay_directory_restore_option = true;
      break;

    case GNUTYPE_VOLHDR:
      *fun = extract_volhdr;
      break;

    case GNUTYPE_MULTIVOL:
      ERROR ((0, 0,
	      _("%s: Cannot extract -- file is continued from another volume"),
	      quotearg_colon (current_stat_info.file_name)));
      *fun = extract_failure;
      break;

    case GNUTYPE_LONGNAME:
    case GNUTYPE_LONGLINK:
      ERROR ((0, 0, _("Unexpected long name header")));
      *fun = extract_failure;
      break;

    default:
      WARN ((0, 0,
	     _("%s: Unknown file type `%c', extracted as normal file"),
	     quotearg_colon (file_name), typeflag));
      *fun = extract_file;
    }

  /* Determine whether the extraction should proceed */
  if (rc == 0)
    return 0;

  switch (old_files_option)
    {
    case UNLINK_FIRST_OLD_FILES:
      if (!remove_any_file (file_name,
                            recursive_unlink_option ? RECURSIVE_REMOVE_OPTION
                                                      : ORDINARY_REMOVE_OPTION)
	  && errno && errno != ENOENT)
	{
	  unlink_error (file_name);
	  return 0;
	}
      break;

    case KEEP_NEWER_FILES:
      if (file_newer_p (file_name, &current_stat_info))
	{
	  WARN ((0, 0, _("Current %s is newer or same age"),
		 quote (file_name)));
	  return 0;
	}
      break;

    default:
      break;
    }

  return 1;
}

/* Extract a file from the archive.  */
void
extract_archive (void)
{
  char typeflag;
  tar_extractor_t fun;

  set_next_block_after (current_header);
  decode_header (current_header, &current_stat_info, &current_format, 1);
  if (!current_stat_info.file_name[0]
      || (interactive_option
	  && !confirm ("extract", current_stat_info.file_name)))
    {
      skip_member ();
      return;
    }

  /* Print the block from current_header and current_stat.  */
  if (verbose_option)
    print_header (&current_stat_info, -1);

  /* Restore stats for all non-ancestor directories, unless
     it is an incremental archive.
     (see NOTICE in the comment to delay_set_stat above) */
  if (!delay_directory_restore_option)
    apply_nonancestor_delayed_set_stat (current_stat_info.file_name, 0);

  /* Take a safety backup of a previously existing file.  */

  if (backup_option)
    if (!maybe_backup_file (current_stat_info.file_name, 0))
      {
	int e = errno;
	ERROR ((0, e, _("%s: Was unable to backup this file"),
		quotearg_colon (current_stat_info.file_name)));
	skip_member ();
	return;
      }

  /* Extract the archive entry according to its type.  */
  /* KLUDGE */
  typeflag = sparse_member_p (&current_stat_info) ?
                  GNUTYPE_SPARSE : current_header->header.typeflag;

  if (prepare_to_extract (current_stat_info.file_name, typeflag, &fun))
    {
      if (fun && (*fun) (current_stat_info.file_name, typeflag)
	  && backup_option)
	undo_last_backup ();
    }
  else
    skip_member ();

}

/* Extract the symbolic links whose final extraction were delayed.  */
static void
apply_delayed_links (void)
{
  struct delayed_link *ds;

  for (ds = delayed_link_head; ds; )
    {
      struct string_list *sources = ds->sources;
      char const *valid_source = 0;

      for (sources = ds->sources; sources; sources = sources->next)
	{
	  char const *source = sources->string;
	  struct stat st;

	  /* Make sure the placeholder file is still there.  If not,
	     don't create a link, as the placeholder was probably
	     removed by a later extraction.  */
	  if (lstat (source, &st) == 0
	      && st.st_dev == ds->dev
	      && st.st_ino == ds->ino
	      && timespec_cmp (get_stat_mtime (&st), ds->mtime) == 0)
	    {
	      /* Unlink the placeholder, then create a hard link if possible,
		 a symbolic link otherwise.  */
	      if (unlink (source) != 0)
		unlink_error (source);
	      else if (valid_source && link (valid_source, source) == 0)
		;
	      else if (!ds->is_symlink)
		{
		  if (link (ds->target, source) != 0)
		    link_error (ds->target, source);
		}
	      else if (symlink (ds->target, source) != 0)
		symlink_error (ds->target, source);
	      else
		{
		  struct tar_stat_info st1;
		  st1.stat.st_uid = ds->uid;
		  st1.stat.st_gid = ds->gid;
		  set_stat (source, &st1, NULL, 0, 0, SYMTYPE);
		  valid_source = source;
		}
	    }
	}

      for (sources = ds->sources; sources; )
	{
	  struct string_list *next = sources->next;
	  free (sources);
	  sources = next;
	}

      {
	struct delayed_link *next = ds->next;
	free (ds);
	ds = next;
      }
    }

  delayed_link_head = 0;
}

/* Finish the extraction of an archive.  */
void
extract_finish (void)
{
  /* First, fix the status of ordinary directories that need fixing.  */
  apply_nonancestor_delayed_set_stat ("", 0);

  /* Then, apply delayed links, so that they don't affect delayed
     directory status-setting for ordinary directories.  */
  apply_delayed_links ();

  /* Finally, fix the status of directories that are ancestors
     of delayed links.  */
  apply_nonancestor_delayed_set_stat ("", 1);
}

bool
rename_directory (char *src, char *dst)
{
  if (rename (src, dst))
    {
      int e = errno;

      switch (e)
	{
	case ENOENT:
	  if (make_directories (dst))
	    {
	      if (rename (src, dst) == 0)
		return true;
	      e = errno;
	    }
	  break;

	case EXDEV:
	  /* FIXME: Fall back to recursive copying */

	default:
	  break;
	}

      ERROR ((0, e, _("Cannot rename %s to %s"),
	      quote_n (0, src),
	      quote_n (1, dst)));
      return false;
    }
  return true;
}

void
fatal_exit (void)
{
  extract_finish ();
  error (TAREXIT_FAILURE, 0, _("Error is not recoverable: exiting now"));
  abort ();
}

void
xalloc_die (void)
{
  error (0, 0, "%s", _("memory exhausted"));
  fatal_exit ();
}
