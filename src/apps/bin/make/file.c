/* Target file hash table management for GNU Make.
Copyright (C) 1988, 1989, 1990, 1991, 1992, 1993, 1994, 1995, 1996, 1997,
2002 Free Software Foundation, Inc.
This file is part of GNU Make.

GNU Make is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU Make is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Make; see the file COPYING.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#include "make.h"

#include <assert.h>

#include "dep.h"
#include "filedef.h"
#include "job.h"
#include "commands.h"
#include "variable.h"
#include "debug.h"
#include "hash.h"


/* Hash table of files the makefile knows how to make.  */

static unsigned long
file_hash_1 (key)
    const void *key;
{
  return_ISTRING_HASH_1 (((struct file const *) key)->hname);
}

static unsigned long
file_hash_2 (key)
    const void *key;
{
  return_ISTRING_HASH_2 (((struct file const *) key)->hname);
}

static int
file_hash_cmp (x, y)
    const void *x;
    const void *y;
{
  return_ISTRING_COMPARE (((struct file const *) x)->hname,
			  ((struct file const *) y)->hname);
}

#ifndef	FILE_BUCKETS
#define FILE_BUCKETS	1007
#endif
static struct hash_table files;

/* Whether or not .SECONDARY with no prerequisites was given.  */
static int all_secondary = 0;

/* Access the hash table of all file records.
   lookup_file  given a name, return the struct file * for that name,
           or nil if there is none.
   enter_file   similar, but create one if there is none.  */

struct file *
lookup_file (name)
     char *name;
{
  register struct file *f;
  struct file file_key;
#if defined(VMS) && !defined(WANT_CASE_SENSITIVE_TARGETS)
  register char *lname, *ln;
#endif

  assert (*name != '\0');

  /* This is also done in parse_file_seq, so this is redundant
     for names read from makefiles.  It is here for names passed
     on the command line.  */
#ifdef VMS
# ifndef WANT_CASE_SENSITIVE_TARGETS
  {
    register char *n;
    lname = (char *) malloc (strlen (name) + 1);
    for (n = name, ln = lname; *n != '\0'; ++n, ++ln)
      *ln = isupper ((unsigned char)*n) ? tolower ((unsigned char)*n) : *n;
    *ln = '\0';
    name = lname;
  }
# endif

  while (name[0] == '[' && name[1] == ']' && name[2] != '\0')
      name += 2;
#endif
  while (name[0] == '.' && name[1] == '/' && name[2] != '\0')
    {
      name += 2;
      while (*name == '/')
	/* Skip following slashes: ".//foo" is "foo", not "/foo".  */
	++name;
    }

  if (*name == '\0')
    /* It was all slashes after a dot.  */
#ifdef VMS
    name = "[]";
#else
#ifdef _AMIGA
    name = "";
#else
    name = "./";
#endif /* AMIGA */
#endif /* VMS */

  file_key.hname = name;
  f = (struct file *) hash_find_item (&files, &file_key);
#if defined(VMS) && !defined(WANT_CASE_SENSITIVE_TARGETS)
  free (lname);
#endif
  return f;
}

struct file *
enter_file (name)
     char *name;
{
  register struct file *f;
  register struct file *new;
  register struct file **file_slot;
  struct file file_key;
#if defined(VMS) && !defined(WANT_CASE_SENSITIVE_TARGETS)
  char *lname, *ln;
#endif

  assert (*name != '\0');

#if defined(VMS) && !defined(WANT_CASE_SENSITIVE_TARGETS)
  {
    register char *n;
    lname = (char *) malloc (strlen (name) + 1);
    for (n = name, ln = lname; *n != '\0'; ++n, ++ln)
      {
        if (isupper ((unsigned char)*n))
          *ln = tolower ((unsigned char)*n);
        else
          *ln = *n;
      }

    *ln = 0;
    /* Creates a possible leak, old value of name is unreachable, but I
       currently don't know how to fix it. */
    name = lname;
  }
#endif

  file_key.hname = name;
  file_slot = (struct file **) hash_find_slot (&files, &file_key);
  f = *file_slot;
  if (! HASH_VACANT (f) && !f->double_colon)
    {
#if defined(VMS) && !defined(WANT_CASE_SENSITIVE_TARGETS)
      free(lname);
#endif
      return f;
    }

  new = (struct file *) xmalloc (sizeof (struct file));
  bzero ((char *) new, sizeof (struct file));
  new->name = new->hname = name;
  new->update_status = -1;

  if (HASH_VACANT (f))
    hash_insert_at (&files, new, file_slot);
  else
    {
      /* There is already a double-colon entry for this file.  */
      new->double_colon = f;
      while (f->prev != 0)
	f = f->prev;
      f->prev = new;
    }

  return new;
}

/* Rename FILE to NAME.  This is not as simple as resetting
   the `name' member, since it must be put in a new hash bucket,
   and possibly merged with an existing file called NAME.  */

void
rename_file (from_file, to_hname)
     register struct file *from_file;
     char *to_hname;
{
  rehash_file (from_file, to_hname);
  while (from_file)
    {
      from_file->name = from_file->hname;
      from_file = from_file->prev;
    }
}

/* Rehash FILE to NAME.  This is not as simple as resetting
   the `hname' member, since it must be put in a new hash bucket,
   and possibly merged with an existing file called NAME.  */

void
rehash_file (from_file, to_hname)
     register struct file *from_file;
     char *to_hname;
{
  struct file file_key;
  struct file **file_slot;
  struct file *to_file;
  struct file *deleted_file;
  struct file *f;

  file_key.hname = to_hname;
  if (0 == file_hash_cmp (from_file, &file_key))
    return;

  file_key.hname = from_file->hname;
  while (from_file->renamed != 0)
    from_file = from_file->renamed;
  if (file_hash_cmp (from_file, &file_key))
    /* hname changed unexpectedly */
    abort ();

  deleted_file = hash_delete (&files, from_file);
  if (deleted_file != from_file)
    /* from_file isn't the one stored in files */
    abort ();

  file_key.hname = to_hname;
  file_slot = (struct file **) hash_find_slot (&files, &file_key);
  to_file = *file_slot;

  from_file->hname = to_hname;
  for (f = from_file->double_colon; f != 0; f = f->prev)
    f->hname = to_hname;

  if (HASH_VACANT (to_file))
    hash_insert_at (&files, from_file, file_slot);
  else
    {
      /* TO_FILE already exists under TO_HNAME.
	 We must retain TO_FILE and merge FROM_FILE into it.  */

      if (from_file->cmds != 0)
	{
	  if (to_file->cmds == 0)
	    to_file->cmds = from_file->cmds;
	  else if (from_file->cmds != to_file->cmds)
	    {
	      /* We have two sets of commands.  We will go with the
		 one given in the rule explicitly mentioning this name,
		 but give a message to let the user know what's going on.  */
	      if (to_file->cmds->fileinfo.filenm != 0)
                error (&from_file->cmds->fileinfo,
		       _("Commands were specified for file `%s' at %s:%lu,"),
		       from_file->name, to_file->cmds->fileinfo.filenm,
		       to_file->cmds->fileinfo.lineno);
	      else
		error (&from_file->cmds->fileinfo,
		       _("Commands for file `%s' were found by implicit rule search,"),
		       from_file->name);
	      error (&from_file->cmds->fileinfo,
		     _("but `%s' is now considered the same file as `%s'."),
		     from_file->name, to_hname);
	      error (&from_file->cmds->fileinfo,
		     _("Commands for `%s' will be ignored in favor of those for `%s'."),
		     to_hname, from_file->name);
	    }
	}

      /* Merge the dependencies of the two files.  */

      if (to_file->deps == 0)
	to_file->deps = from_file->deps;
      else
	{
	  register struct dep *deps = to_file->deps;
	  while (deps->next != 0)
	    deps = deps->next;
	  deps->next = from_file->deps;
	}

      merge_variable_set_lists (&to_file->variables, from_file->variables);

      if (to_file->double_colon && from_file->is_target && !from_file->double_colon)
	fatal (NILF, _("can't rename single-colon `%s' to double-colon `%s'"),
	       from_file->name, to_hname);
      if (!to_file->double_colon  && from_file->double_colon)
	{
	  if (to_file->is_target)
	    fatal (NILF, _("can't rename double-colon `%s' to single-colon `%s'"),
		   from_file->name, to_hname);
	  else
	    to_file->double_colon = from_file->double_colon;
	}

      if (from_file->last_mtime > to_file->last_mtime)
	/* %%% Kludge so -W wins on a file that gets vpathized.  */
	to_file->last_mtime = from_file->last_mtime;

      to_file->mtime_before_update = from_file->mtime_before_update;

#define MERGE(field) to_file->field |= from_file->field
      MERGE (precious);
      MERGE (tried_implicit);
      MERGE (updating);
      MERGE (updated);
      MERGE (is_target);
      MERGE (cmd_target);
      MERGE (phony);
      MERGE (ignore_vpath);
#undef MERGE

      from_file->renamed = to_file;
    }
}

/* Remove all nonprecious intermediate files.
   If SIG is nonzero, this was caused by a fatal signal,
   meaning that a different message will be printed, and
   the message will go to stderr rather than stdout.  */

void
remove_intermediates (sig)
     int sig;
{
  register struct file **file_slot;
  register struct file **file_end;
  int doneany = 0;

  /* If there's no way we will ever remove anything anyway, punt early.  */
  if (question_flag || touch_flag || all_secondary)
    return;

  if (sig && just_print_flag)
    return;

  file_slot = (struct file **) files.ht_vec;
  file_end = file_slot + files.ht_size;
  for ( ; file_slot < file_end; file_slot++)
    if (! HASH_VACANT (*file_slot))
      {
	register struct file *f = *file_slot;
	if (f->intermediate && (f->dontcare || !f->precious)
	    && !f->secondary && !f->cmd_target)
	  {
	    int status;
	    if (f->update_status == -1)
	      /* If nothing would have created this file yet,
		 don't print an "rm" command for it.  */
	      continue;
	    if (just_print_flag)
	      status = 0;
	    else
	      {
		status = unlink (f->name);
		if (status < 0 && errno == ENOENT)
		  continue;
	      }
	    if (!f->dontcare)
	      {
		if (sig)
		  error (NILF, _("*** Deleting intermediate file `%s'"), f->name);
		else
		  {
		    if (! doneany)
		      DB (DB_BASIC, (_("Removing intermediate files...\n")));
		    if (!silent_flag)
		      {
			if (! doneany)
			  {
			    fputs ("rm ", stdout);
			    doneany = 1;
			  }
			else
			  putchar (' ');
			fputs (f->name, stdout);
			fflush (stdout);
		      }
		  }
		if (status < 0)
		  perror_with_name ("unlink: ", f->name);
	      }
	  }
      }

  if (doneany && !sig)
    {
      putchar ('\n');
      fflush (stdout);
    }
}

/* For each dependency of each file, make the `struct dep' point
   at the appropriate `struct file' (which may have to be created).

   Also mark the files depended on by .PRECIOUS, .PHONY, .SILENT,
   and various other special targets.  */

void
snap_deps ()
{
  register struct file *f;
  register struct file *f2;
  register struct dep *d;
  register struct file **file_slot_0;
  register struct file **file_slot;
  register struct file **file_end;

  /* Enter each dependency name as a file.  */
  /* We must use hash_dump (), because within this loop
     we might add new files to the table, possibly causing
     an in-situ table expansion.  */
  file_slot_0 = (struct file **) hash_dump (&files, 0, 0);
  file_end = file_slot_0 + files.ht_fill;
  for (file_slot = file_slot_0; file_slot < file_end; file_slot++)
    for (f2 = *file_slot; f2 != 0; f2 = f2->prev)
      for (d = f2->deps; d != 0; d = d->next)
	if (d->name != 0)
	  {
	    d->file = lookup_file (d->name);
	    if (d->file == 0)
	      d->file = enter_file (d->name);
	    else
	      free (d->name);
	    d->name = 0;
	  }
  free (file_slot_0);

  for (f = lookup_file (".PRECIOUS"); f != 0; f = f->prev)
    for (d = f->deps; d != 0; d = d->next)
      for (f2 = d->file; f2 != 0; f2 = f2->prev)
	f2->precious = 1;

  for (f = lookup_file (".LOW_RESOLUTION_TIME"); f != 0; f = f->prev)
    for (d = f->deps; d != 0; d = d->next)
      for (f2 = d->file; f2 != 0; f2 = f2->prev)
	f2->low_resolution_time = 1;

  for (f = lookup_file (".PHONY"); f != 0; f = f->prev)
    for (d = f->deps; d != 0; d = d->next)
      for (f2 = d->file; f2 != 0; f2 = f2->prev)
	{
	  /* Mark this file as phony and nonexistent.  */
	  f2->phony = 1;
	  f2->last_mtime = NONEXISTENT_MTIME;
	  f2->mtime_before_update = NONEXISTENT_MTIME;
	}

  for (f = lookup_file (".INTERMEDIATE"); f != 0; f = f->prev)
    {
      /* .INTERMEDIATE with deps listed
	 marks those deps as intermediate files.  */
      for (d = f->deps; d != 0; d = d->next)
	for (f2 = d->file; f2 != 0; f2 = f2->prev)
	  f2->intermediate = 1;
      /* .INTERMEDIATE with no deps does nothing.
	 Marking all files as intermediates is useless
	 since the goal targets would be deleted after they are built.  */
    }

  for (f = lookup_file (".SECONDARY"); f != 0; f = f->prev)
    {
      /* .SECONDARY with deps listed
	 marks those deps as intermediate files
	 in that they don't get rebuilt if not actually needed;
	 but unlike real intermediate files,
	 these are not deleted after make finishes.  */
      if (f->deps)
        for (d = f->deps; d != 0; d = d->next)
          for (f2 = d->file; f2 != 0; f2 = f2->prev)
            f2->intermediate = f2->secondary = 1;
      /* .SECONDARY with no deps listed marks *all* files that way.  */
      else
        all_secondary = 1;
    }

  f = lookup_file (".EXPORT_ALL_VARIABLES");
  if (f != 0 && f->is_target)
    export_all_variables = 1;

  f = lookup_file (".IGNORE");
  if (f != 0 && f->is_target)
    {
      if (f->deps == 0)
	ignore_errors_flag = 1;
      else
	for (d = f->deps; d != 0; d = d->next)
	  for (f2 = d->file; f2 != 0; f2 = f2->prev)
	    f2->command_flags |= COMMANDS_NOERROR;
    }

  f = lookup_file (".SILENT");
  if (f != 0 && f->is_target)
    {
      if (f->deps == 0)
	silent_flag = 1;
      else
	for (d = f->deps; d != 0; d = d->next)
	  for (f2 = d->file; f2 != 0; f2 = f2->prev)
	    f2->command_flags |= COMMANDS_SILENT;
    }

  f = lookup_file (".POSIX");
  if (f != 0 && f->is_target)
    posix_pedantic = 1;

  f = lookup_file (".NOTPARALLEL");
  if (f != 0 && f->is_target)
    not_parallel = 1;
}

/* Set the `command_state' member of FILE and all its `also_make's.  */

void
set_command_state (file, state)
     struct file *file;
     int state;
{
  struct dep *d;

  file->command_state = state;

  for (d = file->also_make; d != 0; d = d->next)
    d->file->command_state = state;
}

/* Convert an external file timestamp to internal form.  */

FILE_TIMESTAMP
file_timestamp_cons (fname, s, ns)
     char const *fname;
     time_t s;
     int ns;
{
  int offset = ORDINARY_MTIME_MIN + (FILE_TIMESTAMP_HI_RES ? ns : 0);
  FILE_TIMESTAMP product = (FILE_TIMESTAMP) s << FILE_TIMESTAMP_LO_BITS;
  FILE_TIMESTAMP ts = product + offset;

  if (! (s <= FILE_TIMESTAMP_S (ORDINARY_MTIME_MAX)
	 && product <= ts && ts <= ORDINARY_MTIME_MAX))
    {
      char buf[FILE_TIMESTAMP_PRINT_LEN_BOUND + 1];
      ts = s <= OLD_MTIME ? ORDINARY_MTIME_MIN : ORDINARY_MTIME_MAX;
      file_timestamp_sprintf (buf, ts);
      error (NILF, _("%s: Timestamp out of range; substituting %s"),
	     fname ? fname : _("Current time"), buf);
    }

  return ts;
}

/* Return the current time as a file timestamp, setting *RESOLUTION to
   its resolution.  */
FILE_TIMESTAMP
file_timestamp_now (resolution)
     int *resolution;
{
  int r;
  time_t s;
  int ns;

  /* Don't bother with high-resolution clocks if file timestamps have
     only one-second resolution.  The code below should work, but it's
     not worth the hassle of debugging it on hosts where it fails.  */
#if FILE_TIMESTAMP_HI_RES
# if HAVE_CLOCK_GETTIME && defined CLOCK_REALTIME
  {
    struct timespec timespec;
    if (clock_gettime (CLOCK_REALTIME, &timespec) == 0)
      {
	r = 1;
	s = timespec.tv_sec;
	ns = timespec.tv_nsec;
	goto got_time;
      }
  }
# endif
# if HAVE_GETTIMEOFDAY
  {
    struct timeval timeval;
    if (gettimeofday (&timeval, 0) == 0)
      {
	r = 1000;
	s = timeval.tv_sec;
	ns = timeval.tv_usec * 1000;
	goto got_time;
      }
  }
# endif
#endif

  r = 1000000000;
  s = time ((time_t *) 0);
  ns = 0;

 got_time:
  *resolution = r;
  return file_timestamp_cons (0, s, ns);
}

/* Place into the buffer P a printable representation of the file
   timestamp TS.  */
void
file_timestamp_sprintf (p, ts)
     char *p;
     FILE_TIMESTAMP ts;
{
  time_t t = FILE_TIMESTAMP_S (ts);
  struct tm *tm = localtime (&t);

  if (tm)
    sprintf (p, "%04d-%02d-%02d %02d:%02d:%02d",
	     tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
	     tm->tm_hour, tm->tm_min, tm->tm_sec);
  else if (t < 0)
    sprintf (p, "%ld", (long) t);
  else
    sprintf (p, "%lu", (unsigned long) t);
  p += strlen (p);

  /* Append nanoseconds as a fraction, but remove trailing zeros.
     We don't know the actual timestamp resolution, since clock_getres
     applies only to local times, whereas this timestamp might come
     from a remote filesystem.  So removing trailing zeros is the
     best guess that we can do.  */
  sprintf (p, ".%09d", FILE_TIMESTAMP_NS (ts));
  p += strlen (p) - 1;
  while (*p == '0')
    p--;
  p += *p != '.';

  *p = '\0';
}

/* Print the data base of files.  */

static void
print_file (f)
     struct file *f;
{
  struct dep *d;
  struct dep *ood = 0;

  putchar ('\n');
  if (!f->is_target)
    puts (_("# Not a target:"));
  printf ("%s:%s", f->name, f->double_colon ? ":" : "");

  /* Print all normal dependencies; note any order-only deps.  */
  for (d = f->deps; d != 0; d = d->next)
    if (! d->ignore_mtime)
      printf (" %s", dep_name (d));
    else if (! ood)
      ood = d;

  /* Print order-only deps, if we have any.  */
  if (ood)
    {
      printf (" | %s", dep_name (ood));
      for (d = ood->next; d != 0; d = d->next)
        if (d->ignore_mtime)
          printf (" %s", dep_name (d));
    }

  putchar ('\n');

  if (f->precious)
    puts (_("#  Precious file (prerequisite of .PRECIOUS)."));
  if (f->phony)
    puts (_("#  Phony target (prerequisite of .PHONY)."));
  if (f->cmd_target)
    puts (_("#  Command-line target."));
  if (f->dontcare)
    puts (_("#  A default or MAKEFILES makefile."));
  puts (f->tried_implicit
        ? _("#  Implicit rule search has been done.")
        : _("#  Implicit rule search has not been done."));
  if (f->stem != 0)
    printf (_("#  Implicit/static pattern stem: `%s'\n"), f->stem);
  if (f->intermediate)
    puts (_("#  File is an intermediate prerequisite."));
  if (f->also_make != 0)
    {
      fputs (_("#  Also makes:"), stdout);
      for (d = f->also_make; d != 0; d = d->next)
	printf (" %s", dep_name (d));
      putchar ('\n');
    }
  if (f->last_mtime == UNKNOWN_MTIME)
    puts (_("#  Modification time never checked."));
  else if (f->last_mtime == NONEXISTENT_MTIME)
    puts (_("#  File does not exist."));
  else if (f->last_mtime == OLD_MTIME)
    puts (_("#  File is very old."));
  else
    {
      char buf[FILE_TIMESTAMP_PRINT_LEN_BOUND + 1];
      file_timestamp_sprintf (buf, f->last_mtime);
      printf (_("#  Last modified %s\n"), buf);
    }
  puts (f->updated
        ? _("#  File has been updated.") : _("#  File has not been updated."));
  switch (f->command_state)
    {
    case cs_running:
      puts (_("#  Commands currently running (THIS IS A BUG)."));
      break;
    case cs_deps_running:
      puts (_("#  Dependencies commands running (THIS IS A BUG)."));
      break;
    case cs_not_started:
    case cs_finished:
      switch (f->update_status)
	{
	case -1:
	  break;
	case 0:
	  puts (_("#  Successfully updated."));
	  break;
	case 1:
	  assert (question_flag);
	  puts (_("#  Needs to be updated (-q is set)."));
	  break;
	case 2:
	  puts (_("#  Failed to be updated."));
	  break;
	default:
	  puts (_("#  Invalid value in `update_status' member!"));
	  fflush (stdout);
	  fflush (stderr);
	  abort ();
	}
      break;
    default:
      puts (_("#  Invalid value in `command_state' member!"));
      fflush (stdout);
      fflush (stderr);
      abort ();
    }

  if (f->variables != 0)
    print_file_variables (f);

  if (f->cmds != 0)
    print_commands (f->cmds);
}

void
print_file_data_base ()
{
  puts (_("\n# Files"));

  hash_map (&files, print_file);

  fputs (_("\n# files hash-table stats:\n# "), stdout);
  hash_print_stats (&files, stdout);
}

#define EXPANSION_INCREMENT(_l)  ((((_l) / 500) + 1) * 500)

char *
build_target_list (value)
     char *value;
{
  static unsigned long last_targ_count = 0;

  if (files.ht_fill != last_targ_count)
    {
      unsigned long max = EXPANSION_INCREMENT (strlen (value));
      unsigned long len;
      char *p;
      struct file **fp = (struct file **) files.ht_vec;
      struct file **end = &fp[files.ht_size];

      /* Make sure we have at least MAX bytes in the allocated buffer.  */
      value = xrealloc (value, max);

      p = value;
      len = 0;
      for (; fp < end; ++fp)
        if (!HASH_VACANT (*fp) && (*fp)->is_target)
          {
            struct file *f = *fp;
            int l = strlen (f->name);

            len += l + 1;
            if (len > max)
              {
                unsigned long off = p - value;

                max += EXPANSION_INCREMENT (l + 1);
                value = xrealloc (value, max);
                p = &value[off];
              }

            bcopy (f->name, p, l);
            p += l;
            *(p++) = ' ';
          }
      *(p-1) = '\0';

      last_targ_count = files.ht_fill;
    }

  return value;
}

void
init_hash_files ()
{
  hash_init (&files, 1000, file_hash_1, file_hash_2, file_hash_cmp);
}

/* EOF */
