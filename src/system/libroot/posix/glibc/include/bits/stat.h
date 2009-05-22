/* Copyright (C) 1992, 1996, 1997, 2000 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#ifndef _SYS_STAT_H
# error "Never include <bits/stat.h> directly; use <sys/stat.h> instead."
#endif

/* This structure needs to be defined in accordance with the
   implementation of __stat, __fstat, and __lstat.  */

#include <bits/types.h>

#if 0
/* Structure describing file characteristics.  */
struct stat
  {
    /* These are the members that POSIX.1 requires.  */

    __mode_t st_mode;		/* File mode.  */
#ifndef __USE_FILE_OFFSET64
    __ino_t st_ino;		/* File serial number.  */
#else
    __ino64_t st_ino;		/* File serial number.	*/
#endif
    __dev_t st_dev;		/* Device containing the file.  */
    __nlink_t st_nlink;		/* Link count.  */

    __uid_t st_uid;		/* User ID of the file's owner.  */
    __gid_t st_gid;		/* Group ID of the file's group.  */
#ifndef __USE_FILE_OFFSET64
    __off_t st_size;		/* Size of file, in bytes.  */
#else
    __off64_t st_size;		/* Size of file, in bytes.  */
#endif

    __time_t st_atime;		/* Time of last access.  */
    __time_t st_mtime;		/* Time of last modification.  */
    __time_t st_ctime;		/* Time of last status change.  */

    /* This should be defined if there is a `st_blksize' member.  */
#undef	_STATBUF_ST_BLKSIZE
  };
#endif

/* Encoding of the file mode.  These are the standard Unix values,
   but POSIX.1 does not specify what values should be used.  */

#define	__S_IFMT	0170000	/* These bits determine file type.  */

/* File types.  */
#define	__S_IFDIR	0040000	/* Directory.  */
#define	__S_IFCHR	0020000	/* Character device.  */
#define	__S_IFBLK	0060000	/* Block device.  */
#define	__S_IFREG	0100000	/* Regular file.  */
#define	__S_IFIFO	0010000	/* FIFO.  */

/* POSIX.1b objects.  */
#define __S_TYPEISMQ(buf) 0
#define __S_TYPEISSEM(buf) 0
#define __S_TYPEISSHM(buf) 0

/* Protection bits.  */

#define	__S_ISUID	04000	/* Set user ID on execution.  */
#define	__S_ISGID	02000	/* Set group ID on execution.  */
#define	__S_IREAD	0400	/* Read by owner.  */
#define	__S_IWRITE	0200	/* Write by owner.  */
#define	__S_IEXEC	0100	/* Execute by owner.  */

#if 0
#ifdef __USE_LARGEFILE64
struct stat64
  {
    __mode_t st_mode;		/* File mode.  */
    __ino64_t st_ino;		/* File serial number.	*/
    __dev_t st_dev;		/* Device.  */
    __nlink_t st_nlink;		/* Link count.  */

    __uid_t st_uid;		/* User ID of the file's owner.	*/
    __gid_t st_gid;		/* Group ID of the file's group.*/
    __off64_t st_size;		/* Size of file, in bytes.  */

    __time_t st_atime;		/* Time of last access.  */
    __time_t st_mtime;		/* Time of last modification.  */
    __time_t st_ctime;		/* Time of last status change.  */
  };
#endif
#endif
