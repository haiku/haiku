/* hashcmd.h - Common defines for hashing filenames. */

/* Copyright (C) 1993 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne Again SHell.

   Bash is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 2, or (at your option) any later
   version.

   Bash is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.

   You should have received a copy of the GNU General Public License along
   with Bash; see the file COPYING.  If not, write to the Free Software
   Foundation, 59 Temple Place, Suite 330, Boston, MA 02111 USA. */

#include "stdc.h"
#include "hashlib.h"

#define FILENAME_HASH_BUCKETS 64	/* must be power of two */

extern HASH_TABLE *hashed_filenames;

typedef struct {
  char *path;		/* The full pathname of the file. */
  int flags;
} PATH_DATA;

#define HASH_RELPATH	0x01	/* this filename is a relative pathname. */
#define HASH_CHKDOT	0x02	/* check `.' since it was earlier in $PATH */

#define pathdata(x) ((PATH_DATA *)(x)->data)

extern void phash_create __P((void));
extern void phash_flush __P((void));

extern void phash_insert __P((char *, char *, int, int));
extern int phash_remove __P((const char *));
extern char *phash_search __P((const char *));
