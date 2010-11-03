/*
 * Copyright 2002-2010 Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SIGNAL_H_
#define _SIGNAL_H_


#include <sys/types.h>


typedef int	sig_atomic_t;
typedef __haiku_int32 sigset_t;

typedef void (*sighandler_t)(int);
	/* GNU-like signal handler typedef */

typedef void (*__signal_func_ptr)(int);
	/* deprecated, for compatibility with BeOS only */


/* macros defining the standard signal handling behavior */
#define SIG_DFL		((sighandler_t)0)	/* "default" signal behaviour */
#define SIG_IGN		((sighandler_t)1)	/* ignore signal */
#define SIG_ERR		((sighandler_t)-1)	/* an error occurred during signal processing */
#define SIG_HOLD	((sighandler_t)3)	/* the signal was hold */

/* TODO: Support this structure, or more precisely the SA_SIGINFO flag. To do
 * this properly we need real-time signal support. Both are commented out for
 * the time being to not make "configure" scripts think we do support them. */
#if 0
typedef struct {
	int		si_signo;	/* signal number */
	int		si_code;	/* signal code */
	int		si_errno;	/* if non zero, an error number associated with this signal */
	pid_t	si_pid;		/* sending process ID */
	uid_t	si_uid;		/* real user ID of sending process */
	void	*si_addr;	/* address of faulting instruction */
	int		si_status;	/* exit value or signal */
	long	si_band;	/* band event for SIGPOLL */
} siginfo_t;
#endif	/* 0 */

/*
 * structure used by sigaction()
 *
 * Note: the 'sa_userdata' field is a non-POSIX extension.
 * See the documentation for more info on this.
 */
struct sigaction {
	sighandler_t sa_handler;
	sigset_t	sa_mask;
	int			sa_flags;
	void		*sa_userdata;  /* will be passed to the signal handler */
};

/* values for sa_flags */
#define SA_NOCLDSTOP	0x01
#define SA_NOCLDWAIT	0x02
#define SA_RESETHAND	0x04
#define SA_NODEFER		0x08
#define SA_RESTART		0x10
#define SA_ONSTACK		0x20
/* #define SA_SIGINFO		0x40 */
#define SA_NOMASK		SA_NODEFER
#define SA_STACK		SA_ONSTACK
#define SA_ONESHOT		SA_RESETHAND

/* values for ss_flags */
#define SS_ONSTACK		0x1
#define SS_DISABLE		0x2

#define MINSIGSTKSZ		4096
#define SIGSTKSZ		16384

/*
 * for signals using an alternate stack
 */
typedef struct stack_t {
	void	*ss_sp;
	size_t	ss_size;
	int		ss_flags;
} stack_t;

typedef struct sigstack {
	int 	ss_onstack;
	void	*ss_sp;
} sigstack;

/* for the 'how' arg of sigprocmask() */
#define SIG_BLOCK		1
#define SIG_UNBLOCK		2
#define SIG_SETMASK		3

/*
 * The list of all defined signals:
 *
 * The numbering of signals for Haiku attempts to maintain
 * some consistency with UN*X conventions so that things
 * like "kill -9" do what you expect.
 */
#define	SIGHUP		1	/* hangup -- tty is gone! */
#define SIGINT		2	/* interrupt */
#define SIGQUIT		3	/* `quit' special character typed in tty  */
#define SIGILL		4	/* illegal instruction */
#define SIGCHLD		5	/* child process exited */
#define SIGABRT		6	/* abort() called, dont' catch */
#define SIGPIPE		7	/* write to a pipe w/no readers */
#define SIGFPE		8	/* floating point exception */
#define SIGKILL		9	/* kill a team (not catchable) */
#define SIGSTOP		10	/* suspend a thread (not catchable) */
#define SIGSEGV		11	/* segmentation violation (read: invalid pointer) */
#define SIGCONT		12	/* continue execution if suspended */
#define SIGTSTP		13	/* `stop' special character typed in tty */
#define SIGALRM		14	/* an alarm has gone off (see alarm()) */
#define SIGTERM		15	/* termination requested */
#define SIGTTIN		16	/* read of tty from bg process */
#define SIGTTOU		17	/* write to tty from bg process */
#define SIGUSR1		18	/* app defined signal 1 */
#define SIGUSR2		19	/* app defined signal 2 */
#define SIGWINCH	20	/* tty window size changed */
#define SIGKILLTHR	21	/* be specific: kill just the thread, not team */
#define SIGTRAP		22	/* Trace/breakpoint trap */
#define SIGPOLL		23	/* Pollable event */
#define SIGPROF		24	/* Profiling timer expired */
#define SIGSYS		25	/* Bad system call */
#define SIGURG		26	/* High bandwidth data is available at socket */
#define SIGVTALRM	27	/* Virtual timer expired */
#define SIGXCPU		28	/* CPU time limit exceeded */
#define SIGXFSZ		29	/* File size limit exceeded */

#define SIGBUS		SIGSEGV /* for old style code */

/*
 * Signal numbers 30-32 are currently free but may be used in future
 * releases.  Use them at your own peril (if you do use them, at least
 * be smart and use them backwards from signal 32).
 */
#define MAX_SIGNO		32	/* the most signals that a single thread can reference */
#define __signal_max	29	/* the largest signal number that is actually defined */
#define NSIG			(__signal_max+1)
	/* the number of defined signals */


/* the global table of text strings containing descriptions for each signal */
extern const char * const sys_siglist[NSIG];


#ifdef __cplusplus
extern "C" {
#endif

sighandler_t signal(int sig, sighandler_t signalHandler);
sighandler_t sigset(int sig, sighandler_t signalHandler);
int     raise(int sig);
int     kill(pid_t pid, int sig);
int     send_signal(pid_t tid, unsigned int sig);
int		killpg(pid_t processGroupID, int sig);

int     sigaction(int sig, const struct sigaction *act, struct sigaction *oact);
int		siginterrupt(int sig, int flag);
int     sigprocmask(int how, const sigset_t *set, sigset_t *oset);
int     sigpending(sigset_t *set);
int     sigsuspend(const sigset_t *mask);
int 	sigwait(const sigset_t *set, int *sig);

int     sigemptyset(sigset_t *set);
int     sigfillset(sigset_t *set);
int 	sigaddset(sigset_t *set, int signo);
int 	sigdelset(sigset_t *set, int signo);
int 	sigismember(const sigset_t *set, int signo);
int		sigignore(int signo);
int		sighold(int signo);
int		sigrelse(int signo);
int		sigpause(int signo);

void	set_signal_stack(void *ptr, size_t size);
int		sigaltstack(const stack_t *ss, stack_t *oss);

/* pthread extension : equivalent of sigprocmask()  */
int		pthread_sigmask(int how, const sigset_t *set, sigset_t *oset);

#ifdef __cplusplus
}
#endif

/* TODO: move this into the documentation!
 * ==================================================
 * !!! SPECIAL NOTES CONCERNING NON-POSIX EXTENSIONS:
 * ==================================================
 *
 * The standard Posix interface for signal handlers is not as useful
 * as it could be. The handler can define only one single argument
 * (the signal number). For example:
 *    void
 *    my_signal_handler(int sig)
 *    {
 *    . . .
 *    }
 *
 *    // install the handler
 *    signal(SIGINT, &my_signal_handler);
 *
 * The sigaction() function allows finer grained control of the signal
 * handling. It also allows an opportunity, via the 'sigaction' struct, to
 * enable additional data to be passed to the handler. For example:
 *    void
 *    my_signal_handler(int sig, char *userData, vregs regs)
 *    {
 *    . . .
 *    }
 *
 *    struct sigaction sa;
 *    char data_buffer[32];
 *
 *    sa.sa_handler = (sighandler_t)my_signal_handler;
 *    sigemptyset(&sa.sa_mask);
 *    sa.sa_userdata = userData;
 *
 *    // install the handler
 *    sigaction(SIGINT, &sa, NULL);
 *
 * The two additional arguments available to the signal handler are extensions
 * to the Posix standard. This feature was introduced by the BeOS and retained
 * by Haiku. However, to remain compatible with Posix and ANSI C, the type
 * of the sa_handler field is defined as 'sighandler_t'. This requires the handler
 * to be cast when assigned to the sa_handler field, as in the example above.
 *
 * The 3 arguments that Haiku provides to signal handlers are as follows:
 * 1) The first argument is the (usual) signal number.
 *
 * 2) The second argument is whatever value is put in the sa_userdata field
 *    of the sigaction struct.
 *
 * 3) The third argument is a pointer to a vregs struct (defined below).
 *    The vregs struct contains the contents of the volatile registers at
 *    the time the signal was delivered to your thread. You can change the fields
 *    of the structure. After your signal handler completes, the OS uses this struct
 *    to reload the registers for your thread (privileged registers are not loaded
 *    of course). The vregs struct is of course terribly machine dependent.
 */

/*
 * the vregs struct:
 *
 * signal handlers get this as the last argument
 */

typedef struct vregs vregs;

/* include architecture specific definitions */
#include __HAIKU_ARCH_HEADER(signal.h)


#endif /* _SIGNAL_H_ */
