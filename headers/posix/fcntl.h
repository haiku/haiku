#ifndef _FCNTL_H
#define _FCNTL_H
/* 
** Distributed under the terms of the OpenBeOS License.
*/

#include <sys/types.h>
#include <sys/stat.h>

/* commands that can be passed to fcntl */
#define	F_DUPFD			0x0001
#define	F_GETFD			0x0002
#define	F_SETFD			0x0004
#define	F_GETFL			0x0008
#define	F_SETFL			0x0010
#define F_GETLK         0x0020
#define F_RDLCK         0x0040
#define F_SETLK         0x0080
#define F_SETLKW        0x0100
#define F_UNLCK         0x0200
#define F_WRLCK         0x0400

#define FD_CLOEXEC		1	/* Close on exec.  */

/* flags for open() */
#define O_RDONLY		0	/* read only */
#define O_WRONLY		1	/* write only */
#define O_RDWR			2	/* read and write */
#define O_RWMASK		3	/* Mask to get open mode */

#define O_CLOEXEC		0x0040	/* close fd on exec */
#define	O_NONBLOCK		0x0080	/* non blocking io */
#define	O_EXCL			0x0100	/* exclusive creat */
#define O_CREAT			0x0200	/* create and open file */
#define O_TRUNC			0x0400	/* open with truncation */
#define O_APPEND		0x0800	/* to end of file */
#define O_NOCTTY    	0x1000  /* currently unsupported */
#define	O_NOTRAVERSE	0x2000	/* do not traverse leaf link */
#define O_ACCMODE   	0x0003  /* currently unsupported */
#define O_TEXT			0x4000	/* CR-LF translation	*/
#define O_BINARY		0x8000	/* no translation	*/

// currently not implemented additions:
#define O_NOFOLLOW		0x00010000
	/* should we implement this? it's similar to O_NOTRAVERSE but will fail on symlinks */
#define O_NOCACHE		0x00020000	/* doesn't use the file system cache if possible */
#define O_DIRECT		O_NOCACHE
#define O_SHLOCK		0x00040000
#define O_EXLOCK		0x00080000
#define O_MOUNT			0x00100000	/* for file systems */
#define O_FSYNC			0x10000000


/* advisory file locking */

struct flock {
	short l_type;
	short l_whence;
	off_t l_start;
	off_t l_len;
	pid_t l_pid;
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
