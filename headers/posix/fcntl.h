/* 
** Distributed under the terms of the Haiku License.
*/
#ifndef _FCNTL_H
#define _FCNTL_H


#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


/* commands that can be passed to fcntl() */
#define	F_DUPFD			0x0001		/* duplicate fd */
#define	F_GETFD			0x0002		/* get fd flags */
#define	F_SETFD			0x0004		/* set fd flags */
#define	F_GETFL			0x0008		/* get file status flags and access mode */
#define	F_SETFL			0x0010		/* set file status flags */
#define F_GETLK         0x0020		/* get locking information */
#define F_RDLCK         0x0040		/* read or shared lock */
#define F_SETLK         0x0080		/* set locking information */
#define F_SETLKW        0x0100		/* as above, but waits if blocked */
#define F_UNLCK         0x0200		/* unlock */
#define F_WRLCK         0x0400		/* write or exclusive lock */

/* file descriptor flags for fcntl() */
#define FD_CLOEXEC		1			/* close on exec */

/* file access modes for open() */
#define O_RDONLY		0x0000		/* read only */
#define O_WRONLY		0x0001		/* write only */
#define O_RDWR			0x0002		/* read and write */
#define O_ACCMODE   	0x0003		/* mask to get the access modes above */
#define O_RWMASK		O_ACCMODE

/* flags for open() */
#define	O_EXCL			0x0100		/* exclusive creat */
#define O_CREAT			0x0200		/* create and open file */
#define O_TRUNC			0x0400		/* open with truncation */
#define O_NOCTTY		0x1000		/* currently unsupported */
#define	O_NOTRAVERSE	0x2000		/* do not traverse leaf link */

/* flags for open() and fcntl() */
#define O_CLOEXEC		0x00000040	/* close on exec */
#define	O_NONBLOCK		0x00000080	/* non blocking io */
#define O_APPEND		0x00000800	/* to end of file */
#define O_TEXT			0x00004000	/* CR-LF translation */
#define O_BINARY		0x00008000	/* no translation */
#define O_SYNC			0x00010000	/* write synchronized I/O file integrity */
#define O_RSYNC			0x00020000	/* read synchronized I/O file integrity */
#define O_DSYNC			0x00040000	/* write synchronized I/O data integrity */

// ToDo: currently not implemented additions:
#define O_NOFOLLOW		0x00080000
	/* should we implement this? it's similar to O_NOTRAVERSE but will fail on symlinks */
#define O_NOCACHE		0x00100000	/* doesn't use the file system cache if possible */
#define O_DIRECT		O_NOCACHE
#define O_MOUNT			0x00200000	/* for file systems */
#define O_TEMPORARY		0x00400000	/* used to avoid writing temporary files to disk */
#define O_SHLOCK		0x01000000	/* obtain shared lock */
#define O_EXLOCK		0x02000000	/* obtain exclusive lock */


/* advisory file locking */

struct flock {
	short	l_type;
	short	l_whence;
	off_t	l_start;
	off_t	l_len;
	pid_t	l_pid;
};

#define	LOCK_SH		0x01	/* shared file lock */
#define	LOCK_EX		0x02	/* exclusive file lock */
#define	LOCK_NB		0x04	/* don't block when locking */
#define	LOCK_UN		0x08	/* unlock file */

#define S_IREAD		0x0100  /* owner may read */
#define S_IWRITE	0x0080	/* owner may write */


#ifdef __cplusplus
extern "C" {
#endif

extern int	creat(const char *path, mode_t mode);
extern int	open(const char *pathname, int oflags, ...);
	/* the third argument is the permissions of the created file when O_CREAT
	   is passed in oflags */

extern int	fcntl(int fd, int op, ...);

#ifdef __cplusplus
}
#endif

#endif /* _FCNTL_H */
