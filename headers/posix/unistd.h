/* 
** Distributed under the terms of the Haiku License.
*/
#ifndef _UNISTD_H_
#define _UNISTD_H_


#include <null.h>
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

/* POSIX version support */
#define _POSIX_VERSION			(199009L)

/* pathconf() constants */
#define _PC_CHOWN_RESTRICTED	1
#define _PC_MAX_CANON			2
#define _PC_MAX_INPUT			3
#define _PC_NAME_MAX			4
#define _PC_NO_TRUNC			5
#define _PC_PATH_MAX			6
#define _PC_PIPE_BUF			7
#define _PC_VDISABLE			8
#define _PC_LINK_MAX			25
#define _POSIX_CHOWN_RESTRICTED	9
#define _POSIX_JOB_CONTROL		10
#define _POSIX_NO_TRUNC			11
#define _POSIX_SAVED_IDS		12
#define _POSIX_VDISABLE			((cc_t) - 2)

/* sysconf() constants */
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


#ifdef __cplusplus
extern "C" {
#endif

/* file functions */
extern int		access(const char *path, int accessMode);

extern int		chdir(const char *path);
extern int		fchdir(int fd);
extern char		*getcwd(char *buffer, size_t size);

extern int		pipe(int fildes[2]);
extern int		dup(int fd);
extern int		dup2(int fd1, int fd2);
extern int		close(int fd);
extern int		link(const char *name, const char *new_name);
extern int		unlink(const char *name);
extern int		rmdir(const char *path);

extern ssize_t  readlink(const char *path, char *buffer, size_t bufferSize);
extern int      symlink(const char *from, const char *to);

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

extern int		sync(void);
extern int		fsync(int fd);

extern int		chown(const char *path, uid_t owner, gid_t group);
extern int		fchown(int fd, uid_t owner, gid_t group);
extern int		lchown(const char *path, uid_t owner, gid_t group);

extern int		mknod(const char *name, mode_t mode, dev_t dev);

/* mount flags */
#define B_MOUNT_READ_ONLY		1
#define B_MOUNT_VIRTUAL_DEVICE	2

extern int		mount(const char *filesystem, const char *where, const char *device,
					ulong flags, void *parms, int len);
extern int		unmount(const char *path);

extern int      getdtablesize(void);
extern long		sysconf(int name);
extern long		fpathconf(int fd, int name);
extern long		pathconf(const char *path, int name);

/* process functions */
extern pid_t	fork(void);
extern int		execve(const char *path, char * const argv[], char * const envp[]);
extern int		execl(const char *path, const char *arg, ...);
extern int		execv(const char *path, char *const *argv);
extern int		execlp(const char *file, const char *arg, ...);
extern int		execle(const char *path, const char *arg , ... /*, char **envp */);
extern int		exect(const char *path, char *const *argv);
extern int		execvp(const char *file, char *const *argv);

extern void		_exit(int status);

extern int		system(const char *string);
extern pid_t	tcgetpgrp(int fd);
extern int		tcsetpgrp(int fd, pid_t pgrpid);
extern void		*sbrk(long incr);

extern uint		alarm(unsigned int seconds);
extern uint		ualarm(unsigned int microSeconds, unsigned int interval);
extern uint 	sleep(unsigned int seconds);
extern int		usleep(unsigned int microSeconds);
extern clock_t	clock(void);
extern int 		pause(void);

/* process */
extern pid_t	getpid(void);
extern pid_t	getpgrp(void);
extern pid_t	getppid(void);
extern pid_t	getsid(pid_t pid);
extern pid_t	getpgid(pid_t pid);

extern pid_t	setsid(void);
extern int		setpgid(pid_t pid, pid_t pgid);

/* access permissions */				
extern gid_t	getegid(void);
extern uid_t	geteuid(void);
extern gid_t	getgid(void);
extern int		getgroups(int groupSize, gid_t groupList[]);
extern uid_t	getuid(void);
extern char		*cuserid(char *s);

extern int		setgid(gid_t gid);
extern int		setuid(uid_t uid);

extern char		*getlogin(void);
extern int		getlogin_r(char *name, size_t nameSize);

/* host name */
extern int		sethostname(const char *hostName, size_t nameSize);
extern int		gethostname(char *hostName, size_t nameSize);

/* tty */
extern int		isatty(int fd);
extern char		*ttyname(int fd);
extern int		ttyname_r(int fd, char *buffer, size_t bufferSize);
extern char		*ctermid(char *s);

/* misc */
extern char 	*crypt(const char *key, const char *salt);
extern int		getopt(int argc, char *const *argv, const char *shortOpts);

/* getopt() related external variables */
extern char *optarg;
extern int optind, opterr, optopt;

// ToDo: should be moved to stdlib.h
extern char **environ;

#ifdef __cplusplus
}
#endif

#endif  /* _UNISTD_H_ */
