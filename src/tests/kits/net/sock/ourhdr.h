/* -*- c-basic-offset: 8; -*- */
/* Our own header, to be included *after* all standard system headers */

#ifndef	__ourhdr_h
#define	__ourhdr_h

#include	<sys/types.h>	/* required for some of our prototypes */
#include	<stdio.h>		/* for convenience */
#include	<stdlib.h>		/* for convenience */
#include	<string.h>		/* for convenience */
#include	<unistd.h>		/* for convenience */

#ifdef	notdef	/* delete for systems that don't define this (SunOS 4.x) */
typedef	int	ssize_t;
#endif

#ifdef	notdef	/* delete if <stdlib.h> doesn't define these for getopt() */
extern char	*optarg;
extern int	optind, opterr, optopt;
#endif

#ifdef	notdef	/* delete if send() not supported (DEC OSF/1) */
#define	send(a,b,c,d)	sendto((a), (b), (c), (d), (struct sockaddr *) NULL, 0)
#endif

#define	MAXLINE	4096			/* max line length */

#define	FILE_MODE	(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
					/* default file access permissions for new files */
#define	DIR_MODE	(FILE_MODE | S_IXUSR | S_IXGRP | S_IXOTH)
					/* default permissions for new directories */

typedef	void	Sigfunc(int);	/* for signal handlers */

					/* 4.3BSD Reno <signal.h> doesn't define SIG_ERR */
#if	defined(SIG_IGN) && !defined(SIG_ERR)
#define	SIG_ERR	((Sigfunc *)-1)
#endif

#define	min(a,b)	((a) < (b) ? (a) : (b))
#define	max(a,b)	((a) > (b) ? (a) : (b))

					/* prototypes for our own functions */
char   *path_alloc(int *);			/* {Prog pathalloc} */
int		 open_max(void);			/* {Prog openmax} */
void	clr_fl(int, int);			/* {Prog setfl} */
void	set_fl(int, int);			/* {Prog setfl} */
void	pr_exit(int);				/* {Prog prexit} */
void	pr_mask(const char *);		/* {Prog prmask} */
Sigfunc	*signal_intr(int, Sigfunc *);/* {Prog signal_intr_function} */

int	tty_cbreak(int);			/* {Prog raw} */
int	tty_raw(int);				/* {Prog raw} */
int	tty_reset(int);			/* {Prog raw} */
void	tty_atexit(void);			/* {Prog raw} */
#ifdef	ECHO	/* only if <termios.h> has been included */
struct termios	*tty_termios(void);	/* {Prog raw} */
#endif

void	sleep_us(unsigned int);	/* {Ex sleepus} */
ssize_t	readn(int, void *, size_t);/* {Prog readn} */
ssize_t	writen(int, const void *, size_t);/* {Prog writen} */
int	daemon_init(void);			/* {Prog daemoninit} */

int	s_pipe(int *);				/* {Progs svr4_spipe bsd_spipe} */
int	recv_fd(int, ssize_t (*func)(int, const void *, size_t));
									/* {Progs recvfd_svr4 recvfd_43bsd} */
int	send_fd(int, int);			/* {Progs sendfd_svr4 sendfd_43bsd} */
int	send_err(int, int, const char *);/* {Prog senderr} */
int	serv_listen(const char *);	/* {Progs servlisten_svr4 servlisten_44bsd} */
int	serv_accept(int, uid_t *);	/* {Progs servaccept_svr4 servaccept_44bsd} */
int	cli_conn(const char *);	/* {Progs cliconn_svr4 cliconn_44bsd} */
int	buf_args(char *, int (*func)(int, char **));
									/* {Prog bufargs} */

int	ptym_open(char *);			/* {Progs ptyopen_svr4 ptyopen_44bsd} */
int	ptys_open(int, char *);	/* {Progs ptyopen_svr4 ptyopen_44bsd} */
#ifdef	TIOCGWINSZ
pid_t	pty_fork(int *, char *, const struct termios *,
		  const struct winsize *);	/* {Prog ptyfork} */
#endif
	
int	lock_reg(int, int, int, off_t, int, off_t);
									/* {Prog lockreg} */
#define	read_lock(fd, offset, whence, len) \
			lock_reg(fd, F_SETLK, F_RDLCK, offset, whence, len)
#define	readw_lock(fd, offset, whence, len) \
			lock_reg(fd, F_SETLKW, F_RDLCK, offset, whence, len)
#define	write_lock(fd, offset, whence, len) \
			lock_reg(fd, F_SETLK, F_WRLCK, offset, whence, len)
#define	writew_lock(fd, offset, whence, len) \
			lock_reg(fd, F_SETLKW, F_WRLCK, offset, whence, len)
#define	un_lock(fd, offset, whence, len) \
			lock_reg(fd, F_SETLK, F_UNLCK, offset, whence, len)

pid_t	lock_test(int, int, off_t, int, off_t);
									/* {Prog locktest} */

#define	is_readlock(fd, offset, whence, len) \
			lock_test(fd, F_RDLCK, offset, whence, len)
#define	is_writelock(fd, offset, whence, len) \
			lock_test(fd, F_WRLCK, offset, whence, len)

void	err_dump(const char *, ...);	/* {App misc_source} */
void	err_msg(const char *, ...);
void	err_quit(const char *, ...);
void	err_ret(const char *, ...);
void	err_sys(const char *, ...);

void	log_msg(const char *, ...);		/* {App misc_source} */
void	log_open(const char *, int, int);
void	log_quit(const char *, ...);
void	log_ret(const char *, ...);
void	log_sys(const char *, ...);

void	TELL_WAIT(void);		/* parent/child from {Sec race_conditions} */
void	TELL_PARENT(pid_t);
void	TELL_CHILD(pid_t);
void	WAIT_PARENT(void);
void	WAIT_CHILD(void);

#endif	/* __ourhdr_h */
