/*-
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)stat.h	8.9 (Berkeley) 8/17/94
 */

#ifndef _SYS_STAT_H_
#define	_SYS_STAT_H_

//#include <sys/time.h>

/**
 * @file stat.h
 * @brief Stat structures and defines
 */
 
/**
 * @defgroup OpenBeOS_POSIX POSIX Headers
 * @brief Headers to provide the POSIX layer
 */
 
/**
 * @defgroup Stat sys/stat.h
 * @brief Structures and prototypes for stat operations
 * @ingroup OpenBeOS_POSIX
 * @{
 */
 
/* XXX - some types we don't yet have in sys/types.h (because we don't yet
 *       have a sys/types.h and these aren't in ktypes.h!)
 */

#ifdef _KERNEL_MODE_
struct ostat {
	mode_t    st_mode;              /* inode protection mode */
	ino_t     st_ino;               /* inode's number */
	uint16    st_dev;		/* inode's device */
	nlink_t	  st_nlink;		/* number of hard links */
	uid_t     st_uid;		/* user ID of the file's owner */
	gid_t     st_gid;		/* group ID of the file's group */
	off_t     st_size;		/* file size, in bytes */
	time_t    st_atime;             /* time of last access */
	time_t    st_mtime;             /* time of last data modification */
	time_t    st_ctime;             /* time of last file status change */
	int64     st_blocks;            /* blocks allocated for file */
	int32	  st_blksize;		/* optimal blocksize for I/O */
};
#endif /* _KERNEL_MODE_ */

/* XXX - The stat structure, as defined by POSIX.
 *       Just implement what we can for the time being.
 */   
struct stat {
	mode_t    st_mode;              /* File mode */
	ino_t     st_ino;               /* vnode_id */
//	dev_t	  st_dev;               /* inode's device */
	nlink_t	  st_nlink;             /* number of hard links */
	uid_t	  st_uid;               /* user ID of the file's owner */
	gid_t	  st_gid;               /* group ID of the file's group */
	off_t     st_size;              /* file size, in bytes */
	time_t    st_atime;             /* time of last access */
	time_t    st_mtime;             /* time of last data modification */
	time_t    st_ctime;             /* time of last file status change */
	int64     st_blocks;            /* blocks allocated for file */
	uint32    st_blksize;           /* optimal blocksize for I/O */
};

/**
 * @defgroup POSIX_filemodes File mode defines
 * @brief Defines of the file modes we allow
 * @ingroup Stat
 *@{
 */
/** set user id on execution */
#define	S_ISUID	0004000
/** set group id on execution */
#define	S_ISGID	0002000			
/** sticky bit */
#define S_ISTXT 0001000

/** 
 * @def S_IRWXU RWX mask for owner
 * @def S_IRUSR R for owner
 * @def S_IWUSR W for owner
 * @def S_IXUSR X for owner
 */
#define	S_IRWXU	0000700			
#define	S_IRUSR	0000400			
#define	S_IWUSR	0000200			
#define	S_IXUSR	0000100			

/* @def S_IRWXG RWX mask for group
 * @def S_IWGRP W for group
 * @def S_IRGRP R for group
 * @def S_IXGRP X for group
 */
#define	S_IRWXG	0000070			
#define	S_IRGRP	0000040			
#define	S_IWGRP	0000020			
#define	S_IXGRP	0000010			

#define	S_IRWXO	0000007			/* RWX mask for other */
#define	S_IROTH	0000004			/* R for other */
#define	S_IWOTH	0000002			/* W for other */
#define	S_IXOTH	0000001			/* X for other */

#define S_IFMT   0170000
#define S_IFSOCK 0140000
#define S_IFIFO  0010000
#define S_IFCHR  0020000
#define S_IFDIR  0040000
#define S_IFBLK  0060000
#define S_IFREG  0100000
#define S_IFLNK  0120000
#define S_IFWHT  0160000
#define S_ISVTX  0001000

#define	S_ISDIR(m)	((m & 0170000) == 0040000)	/* directory */
#define	S_ISCHR(m)	((m & 0170000) == 0020000)	/* char special */
#define	S_ISBLK(m)	((m & 0170000) == 0060000)	/* block special */
#define	S_ISREG(m)	((m & 0170000) == 0100000)	/* regular file */
#define	S_ISFIFO(m)	((m & 0170000) == 0010000)	/* fifo */
#define S_ISLNK(m)      ((m & 0170000) == 0120000)      /* symbolic link */
#define S_ISSOCK(m)     ((m & 0170000) == 0140000)/* socket */
#define S_ISWHT(m)((m & 0170000) == 0160000)/* whiteout */  

/** 00777 */
#define ACCESSPERMS (S_IRWXU | S_IRWXG | S_IRWXO)
/** 07777 */
#define ALLPERMS    (S_ISUID|S_ISGID|S_ISTXT|S_IRWXU|S_IRWXG|S_IRWXO)
/** 00666 */
#define	DEFFILEMODE	(S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)

/** @} */
/*
 * Definitions of flags stored in file flags word.
 *
 * Super-user and owner changeable flags.
 */
#define	UF_SETTABLE	0x0000ffff	/* mask of owner changeable flags */
#define	UF_NODUMP	0x00000001	/* do not dump file */
#define	UF_IMMUTABLE	0x00000002	/* file may not be changed */
#define	UF_APPEND	0x00000004	/* writes to file may only append */
#define	UF_OPAQUE	0x00000008	/* directory is opaque wrt. union */
/*
 * Super-user changeable flags.
 */
#define	SF_SETTABLE	0xffff0000	/* mask of superuser changeable flags */
#define	SF_ARCHIVED	0x00010000	/* file is archived */
#define	SF_IMMUTABLE	0x00020000	/* file may not be changed */
#define	SF_APPEND	0x00040000	/* writes to file may only append */

#ifdef _KERNEL_MODE
/*
 * Shorthand abbreviations of above.
 */
#define	OPAQUE		(UF_OPAQUE)
#define	APPEND		(UF_APPEND | SF_APPEND)
#define	IMMUTABLE	(UF_IMMUTABLE | SF_IMMUTABLE)
#endif /* _KERNEL_MODE */

#ifndef _KERNEL_MODE_
  //#include <sys/cdefs.h>

  /** @fn int chmod(const char *path, mode_t mode)
   * Changes the file protection modes to those given by mode
   *
   * @ref POISX_filemodes
   */
  int	chmod (const char *, mode_t);
  /** @fn int fstat(int fd, struct stat *stat)
   * get file information on a file identified by descriptor fd
   * 
   * @ref stat
   */
  int	fstat (int, struct stat *);
  //int	mknod (const char *, mode_t, dev_t);
  int	mkdir (const char *, mode_t);
  int	mkfifo (const char *, mode_t);
  /** @fn int stat(const char *path, struct stat *stat)
   * Get file information for file path and store it in the structure
   * stat
   *
   * @ref stat
   */
  int	stat (const char *, struct stat *);
  mode_t	umask (mode_t);
  int	fchmod (int, mode_t);
  int	lstat (const char *, struct stat *);

#endif

/** @} */

#endif /* !_SYS_STAT_H_ */
