/*
 * Copyright 2002-2007, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FSSH_FCNTL_H
#define _FSSH_FCNTL_H


#include "fssh_types.h"


/* commands that can be passed to fcntl() */
#define	FSSH_F_DUPFD		0x0001		/* duplicate fd */
#define	FSSH_F_GETFD		0x0002		/* get fd flags */
#define	FSSH_F_SETFD		0x0004		/* set fd flags */
#define	FSSH_F_GETFL		0x0008		/* get file status flags and access mode */
#define	FSSH_F_SETFL		0x0010		/* set file status flags */
#define FSSH_F_GETLK		0x0020		/* get locking information */
#define FSSH_F_SETLK		0x0080		/* set locking information */
#define FSSH_F_SETLKW		0x0100		/* as above, but waits if blocked */

/* advisory locking types */
#define FSSH_F_RDLCK		0x0040		/* read or shared lock */
#define FSSH_F_UNLCK		0x0200		/* unlock */
#define FSSH_F_WRLCK		0x0400		/* write or exclusive lock */

/* file descriptor flags for fcntl() */
#define FSSH_FD_CLOEXEC		1			/* close on exec */

/* file access modes for open() */
#define FSSH_O_RDONLY		0x0000		/* read only */
#define FSSH_O_WRONLY		0x0001		/* write only */
#define FSSH_O_RDWR			0x0002		/* read and write */
#define FSSH_O_ACCMODE   	0x0003		/* mask to get the access modes above */
#define FSSH_O_RWMASK		FSSH_O_ACCMODE

/* flags for open() */
#define	FSSH_O_EXCL			0x0100		/* exclusive creat */
#define FSSH_O_CREAT		0x0200		/* create and open file */
#define FSSH_O_TRUNC		0x0400		/* open with truncation */
#define FSSH_O_NOCTTY		0x1000		/* currently unsupported */
#define	FSSH_O_NOTRAVERSE	0x2000		/* do not traverse leaf link */

/* flags for open() and fcntl() */
#define FSSH_O_CLOEXEC		0x00000040	/* close on exec */
#define	FSSH_O_NONBLOCK		0x00000080	/* non blocking io */
#define FSSH_O_APPEND		0x00000800	/* to end of file */
#define FSSH_O_TEXT			0x00004000	/* CR-LF translation */
#define FSSH_O_BINARY		0x00008000	/* no translation */
#define FSSH_O_SYNC			0x00010000	/* write synchronized I/O file integrity */
#define FSSH_O_RSYNC		0x00020000	/* read synchronized I/O file integrity */
#define FSSH_O_DSYNC		0x00040000	/* write synchronized I/O data integrity */
#define FSSH_O_NOFOLLOW		0x00080000	/* fail on symlinks */
#define FSSH_O_NOCACHE		0x00100000	/* do not use the file system cache if */
										/* possible */
#define FSSH_O_DIRECT		FSSH_O_NOCACHE
#define FSSH_O_DIRECTORY	0x00200000	/* fail if not a directory */

// TODO: currently not implemented additions:
	/* should we implement this? it's similar to O_NOTRAVERSE but will fail on symlinks */
#define FSSH_O_TEMPORARY	0x00400000	/* used to avoid writing temporary files to disk */

#define FSSH_S_IREAD		0x0100  /* owner may read */
#define FSSH_S_IWRITE		0x0080	/* owner may write */


#ifdef __cplusplus
extern "C" {
#endif

extern int	fssh_creat(const char *path, fssh_mode_t mode);
extern int	fssh_open(const char *pathname, int oflags, ...);
	/* the third argument is the permissions of the created file when O_CREAT
	   is passed in oflags */

extern int	fssh_fcntl(int fd, int op, ...);

#ifdef __cplusplus
}
#endif

#endif /* _FSSH_FCNTL_H */
