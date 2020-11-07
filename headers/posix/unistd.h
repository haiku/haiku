/*
 * Copyright 2004-2015 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _UNISTD_H_
#define _UNISTD_H_


#include <null.h>
#include <stdint.h>
#include <sys/types.h>


/* access modes */
#define R_OK	4
#define W_OK	2
#define X_OK	1
#define F_OK	0

/* standard file descriptors */
#define STDIN_FILENO	0
#define STDOUT_FILENO	1
#define STDERR_FILENO	2

/* function arguments needed by lockf() */
#define F_ULOCK		0	/* unlock locked sections */
#define F_LOCK		1	/* lock a section for exclusive use */
#define F_TLOCK		2	/* test and lock a section for exclusive use */
#define F_TEST		3	/* test a section for locks by other processes */

/* POSIX version support */
#define _POSIX_VERSION			(199009L)	/* TODO: Update! */

#define _POSIX_CHOWN_RESTRICTED	1
#define _POSIX_JOB_CONTROL		1
#define _POSIX_NO_TRUNC			0
#define _POSIX_SAVED_IDS		1
#define _POSIX_VDISABLE			((unsigned char)-2)
	/* TODO: Check this! */
/* TODO: Update these to the current POSIX version! Ideally after actually
	supporting the features. */
#define _POSIX_BARRIERS						(200112L)
#define _POSIX_SEMAPHORES					(200112L)
#define _POSIX_THREADS						(200112L)
#define _POSIX_MAPPED_FILES					(200809L)
#define _POSIX_THREAD_PROCESS_SHARED		(200809L)
#define _POSIX_THREAD_ATTR_STACKADDR		(200809L)
#define _POSIX_THREAD_ATTR_STACKSIZE		(200809L)
#define _POSIX_THREAD_PRIORITY_SCHEDULING	(-1)	/* currently unsupported */
#define _POSIX_REALTIME_SIGNALS				(200809L)
#define _POSIX_MEMORY_PROTECTION			(200809L)
#define _POSIX_MONOTONIC_CLOCK				(200809L)
#define _POSIX_TIMERS						(200809L)
#define _POSIX_CPUTIME						(200809L)
#define _POSIX_THREAD_CPUTIME				(200809L)


/* pathconf() constants */
/* BeOS supported values, do not touch */
#define _PC_CHOWN_RESTRICTED	1
#define _PC_MAX_CANON			2
#define _PC_MAX_INPUT			3
#define _PC_NAME_MAX			4
#define _PC_NO_TRUNC			5
#define _PC_PATH_MAX			6
#define _PC_PIPE_BUF			7
#define _PC_VDISABLE			8
#define _PC_LINK_MAX			25
/* new values */
#define _PC_SYNC_IO				26
#define _PC_ASYNC_IO			27
#define _PC_PRIO_IO				28
#define _PC_SOCK_MAXBUF			29
#define _PC_FILESIZEBITS		30
#define _PC_REC_INCR_XFER_SIZE	31
#define _PC_REC_MAX_XFER_SIZE	32
#define _PC_REC_MIN_XFER_SIZE	33
#define _PC_REC_XFER_ALIGN		34
#define _PC_ALLOC_SIZE_MIN		35
#define _PC_SYMLINK_MAX			36
#define _PC_2_SYMLINKS			37
#define _PC_XATTR_EXISTS		38
#define _PC_XATTR_ENABLED		39

/* sysconf() constants */
/* BeOS supported values, do not touch */
#define _SC_ARG_MAX				15
#define _SC_CHILD_MAX			16
#define _SC_CLK_TCK				17
#define _SC_JOB_CONTROL			18
#define _SC_NGROUPS_MAX			19
#define _SC_OPEN_MAX			20
#define _SC_SAVED_IDS			21
#define _SC_STREAM_MAX			22
#define _SC_TZNAME_MAX			23
#define _SC_VERSION				24
/* new values */
#define _SC_GETGR_R_SIZE_MAX	25
#define _SC_GETPW_R_SIZE_MAX	26
#define _SC_PAGE_SIZE			27
#define _SC_PAGESIZE			_SC_PAGE_SIZE
#define _SC_SEM_NSEMS_MAX		28
#define _SC_SEM_VALUE_MAX		29
#define _SC_SEMAPHORES			30
#define _SC_THREADS				31
/* TODO: check */
#define _SC_IOV_MAX						32
#define _SC_UIO_MAXIOV					_SC_IOV_MAX
#define _SC_NPROCESSORS_CONF			34
#define _SC_NPROCESSORS_ONLN			35
#define _SC_ATEXIT_MAX					37
#define _SC_PASS_MAX					39
#define _SC_PHYS_PAGES					40
#define _SC_AVPHYS_PAGES				41
#define _SC_PIPE						42
#define _SC_SELECT						43
#define _SC_POLL						44
#define _SC_MAPPED_FILES				45
#define _SC_THREAD_PROCESS_SHARED		46
#define _SC_THREAD_STACK_MIN			47
#define _SC_THREAD_ATTR_STACKADDR		48
#define _SC_THREAD_ATTR_STACKSIZE		49
#define _SC_THREAD_PRIORITY_SCHEDULING	50
#define _SC_REALTIME_SIGNALS			51
#define	_SC_MEMORY_PROTECTION			52
#define _SC_SIGQUEUE_MAX				53
#define _SC_RTSIG_MAX					54
#define _SC_MONOTONIC_CLOCK				55
#define _SC_DELAYTIMER_MAX				56
#define _SC_TIMER_MAX					57
#define	_SC_TIMERS						58
#define	_SC_CPUTIME						59
#define	_SC_THREAD_CPUTIME				60
#define _SC_HOST_NAME_MAX				61
#define _SC_REGEXP						62
#define _SC_SYMLOOP_MAX					63
#define _SC_SHELL						64
#define _SC_TTY_NAME_MAX				65


/* confstr() constants */
#define _CS_PATH				1

/* lseek() constants */
#ifndef SEEK_SET
#	define SEEK_SET 0
#endif
#ifndef SEEK_CUR
#	define SEEK_CUR 1
#endif
#ifndef SEEK_END
#	define SEEK_END 2
#endif
#ifndef SEEK_DATA
#	define SEEK_DATA 3
#endif
#ifndef SEEK_HOLE
#	define SEEK_HOLE 4
#endif


#ifdef __cplusplus
extern "C" {
#endif

/* file functions */
extern int		access(const char *path, int accessMode);
extern int		faccessat(int fd, const char *path, int accessMode, int flag);

extern int		chdir(const char *path);
extern int		fchdir(int fd);
extern char		*getcwd(char *buffer, size_t size);

extern int		pipe(int fildes[2]);
extern int		dup(int fd);
extern int		dup2(int fd1, int fd2);
extern int		close(int fd);
extern int		link(const char *toPath, const char *path);
extern int		linkat(int toFD, const char *toPath, int pathFD,
					const char *path, int flag);
extern int		unlink(const char *name);
extern int		unlinkat(int fd, const char *path, int flag);
extern int		rmdir(const char *path);

extern ssize_t  readlink(const char *path, char *buffer, size_t bufferSize);
extern ssize_t	readlinkat(int fd, const char *path, char *buffer,
					size_t bufferSize);
extern int      symlink(const char *toPath, const char *symlinkPath);
extern int		symlinkat(const char *toPath, int fd, const char *symlinkPath);

extern int      ftruncate(int fd, off_t newSize);
extern int      truncate(const char *path, off_t newSize);
extern int		ioctl(int fd, unsigned long op, ...);

extern ssize_t	read(int fd, void *buffer, size_t count);
extern ssize_t  read_pos(int fd, off_t pos, void *buffer, size_t count);
extern ssize_t	pread(int fd, void *buffer, size_t count, off_t pos);
extern ssize_t	write(int fd, const void *buffer, size_t count);
extern ssize_t  write_pos(int fd, off_t pos, const void *buffer,size_t count);
extern ssize_t	pwrite(int fd, const void *buffer, size_t count, off_t pos);
extern off_t	lseek(int fd, off_t offset, int whence);

extern void		sync(void);
extern int		fsync(int fd);

extern int		chown(const char *path, uid_t owner, gid_t group);
extern int		fchown(int fd, uid_t owner, gid_t group);
extern int		lchown(const char *path, uid_t owner, gid_t group);
extern int		fchownat(int fd, const char *path, uid_t owner, gid_t group,
					int flag);

extern int		getpagesize(void);
extern int      getdtablesize(void);
extern long		sysconf(int name);
extern long		fpathconf(int fd, int name);
extern long		pathconf(const char *path, int name);
extern size_t	confstr(int name, char *buf, size_t len);
extern int		lockf(int fd, int function, off_t size);

/* process functions */
extern pid_t	fork(void);
extern pid_t	vfork(void);
extern int		execve(const char *path, char * const argv[],
					char *const environment[]);
extern int		execl(const char *path, const char *arg, ...);
extern int		execv(const char *path, char *const argv[]);
extern int		execlp(const char *file, const char *arg, ...);
extern int		execle(const char *path, const char *arg , ... /*, char **envp */);
extern int		execvp(const char *file, char *const argv[]);
extern int		execvpe(const char *file, char *const argv[],
					char *const environment[]);

extern void		_exit(int status) __attribute__ ((noreturn));

extern pid_t	tcgetpgrp(int fd);
extern int		tcsetpgrp(int fd, pid_t pgrpid);

extern int		brk(void *addr);
extern void		*sbrk(intptr_t increment);

extern unsigned	int	alarm(unsigned int seconds);
extern useconds_t	ualarm(useconds_t microSeconds, useconds_t interval);
extern unsigned int	sleep(unsigned int seconds);
extern int			usleep(unsigned int microSeconds);
extern int 			pause(void);

/* process */
extern pid_t	getpid(void);
extern pid_t	getpgrp(void);
extern pid_t	getppid(void);
extern pid_t	getsid(pid_t pid);
extern pid_t	getpgid(pid_t pid);

extern pid_t	setsid(void);
extern int		setpgid(pid_t pid, pid_t pgid);
extern pid_t	setpgrp(void);

extern int		chroot(const char *path);

extern int		nice(int incr);

/* access permissions */
extern gid_t	getegid(void);
extern uid_t	geteuid(void);
extern gid_t	getgid(void);
extern uid_t	getuid(void);

extern int		setgid(gid_t gid);
extern int		setuid(uid_t uid);
extern int		setegid(gid_t gid);
extern int		seteuid(uid_t uid);
extern int		setregid(gid_t rgid, gid_t egid);
extern int		setreuid(uid_t ruid, uid_t euid);

extern int		getgrouplist(const char* user, gid_t baseGroup,
					gid_t* groupList, int* groupCount);
extern int		getgroups(int groupCount, gid_t groupList[]);
extern int		initgroups(const char* user, gid_t baseGroup);
extern int		setgroups(int groupCount, const gid_t* groupList);

extern char		*getlogin(void);
extern int		getlogin_r(char *name, size_t nameSize);

/* host name */
extern int		sethostname(const char *hostName, size_t nameSize);
extern int		gethostname(char *hostName, size_t nameSize);

/* tty */
extern int		isatty(int fd);
extern char		*ttyname(int fd);
extern int		ttyname_r(int fd, char *buffer, size_t bufferSize);

/* misc */
extern char 	*crypt(const char *key, const char *salt);
extern void 	encrypt(char block[64], int edflag);
extern int		getopt(int argc, char *const *argv, const char *shortOpts);
extern void 	swab(const void *src, void *dest, ssize_t nbytes);

/* getopt() related external variables */
extern char *optarg;
extern int optind, opterr, optopt;

#ifdef __cplusplus
}
#endif

#endif  /* _UNISTD_H_ */
