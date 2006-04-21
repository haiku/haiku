#ifndef _SIGNAL_H_
#define _SIGNAL_H_
/* 
** Distributed under the terms of the OpenBeOS License.
*/


#include <sys/types.h>


typedef int	sig_atomic_t;
typedef long sigset_t;

typedef void (*sig_func_t)(int);
typedef void (*__signal_func_ptr)(int);  /* deprecated, for compatibility with BeOS only */


typedef union sigval {
	int	sival_int;
	void	*sival_ptr;
} sigval_t;


// ToDo: actually make use of this structure!
typedef struct {
	int		si_signo;	/* signal number */
	int		si_code;	/* signal code */
	int		si_errno;	/* if non zero, an error number associated with this signal */
	pid_t	si_pid;		/* sending process ID */
	uid_t	si_uid;		/* real user ID of sending process */
	void	*si_addr;	/* address of faulting instruction */
	int		si_status;	/* exit value or signal */
	long	si_band;	/* band event for SIGPOLL */
	union sigval  	si_value;	/* signal value */
} siginfo_t;


/*
 * macros defining the standard signal handling behavior
 */
#define SIG_DFL	((sig_func_t) 0)   /* the signal was treated in the "default" manner */
#define SIG_IGN	((sig_func_t) 1)   /* the signal was ignored */
#define SIG_ERR	((sig_func_t)-1)   /* an error occurred during signal processing */
#define SIG_HOLD ((sig_func_t) 3)  /* the signal was hold */


/*
 * structure used by sigaction()
 *
 * Note: the 'sa_userdata' field is a non-Posix extension.
 * See the SPECIAL NOTES below for an explanation of this.
 * 
 */
struct sigaction {
	sig_func_t	sa_handler;
	sigset_t	sa_mask;
	int			sa_flags;
	void		*sa_userdata;  /* will be passed to the signal handler */
};


/*
 * values for sa_flags
 */
#define SA_NOCLDSTOP  0x01
#define SA_ONESHOT    0x02
#define SA_NOMASK     0x04
#define SA_NODEFER    SA_NOMASK
#define SA_RESTART    0x08
#define SA_STACK	0x10
#define SA_ONSTACK      SA_STACK
#define SA_RESETHAND	0x20
#define SA_SIGINFO	0x40
#define SA_NOCLDWAIT	0x80

/*
 * values for ss_flags
 */
#define SS_ONSTACK	0x1
#define SS_DISABLE	0x2

#define MINSIGSTKSZ	4096
#define SIGSTKSZ	16384

/*
 * for signals using an alternate stack
 */
typedef struct stack_t {
	void	*ss_sp;
	size_t	ss_size;
	int	ss_flags;
} stack_t;

typedef struct sigstack {
	int 	ss_onstack;
	void	*ss_sp;
} sigstack;

/*
 * for the 'how' arg of sigprocmask()
 */
#define SIG_BLOCK    1
#define SIG_UNBLOCK  2
#define SIG_SETMASK  3


/*
 * The list of all defined signals:
 *
 * The numbering of signals for OpenBeOS attempts to maintain 
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

#define SIGBUS     SIGSEGV /* for old style code */


/*
 * Signal numbers 30-32 are currently free but may be used in future
 * releases.  Use them at your own peril (if you do use them, at least
 * be smart and use them backwards from signal 32).
 */
#define MAX_SIGNO     32       /* the most signals that a single thread can reference */
#define __signal_max  29       /* the largest signal number that is actually defined */
#define NSIG (__signal_max+1)  /* the number of defined signals */


/* the global table of text strings containing descriptions for each signal */
extern const char * const sys_siglist[NSIG];


#ifdef __cplusplus
extern "C" {
#endif

sig_func_t signal(int sig, sig_func_t signal_handler);
int     raise(int sig);
int     kill(pid_t pid, int sig);
int     send_signal(pid_t tid, uint sig);

int     sigaction(int sig, const struct sigaction *act, struct sigaction *oact);
int     sigprocmask(int how, const sigset_t *set, sigset_t *oset);
int     sigpending(sigset_t *set);
int     sigsuspend(const sigset_t *mask);
int 	sigwait(const sigset_t *set, int *sig);

int     sigemptyset(sigset_t *set);
int     sigfillset(sigset_t *set);
int 	sigaddset(sigset_t *set, int signo);
int 	sigdelset(sigset_t *set, int signo);
int 	sigismember(const sigset_t *set, int signo);

const char *strsignal(int sig);

void        set_signal_stack(void *ptr, size_t size);
int         sigaltstack(const stack_t *ss, stack_t *oss);         /* XXXdbg */

/* 
 * pthread extension : equivalent of sigprocmask() 
 */
extern int pthread_sigmask(int how, const sigset_t *set, sigset_t *oset); 

extern inline int
sigismember(const sigset_t *set, int sig)
{
	sigset_t mask = (((sigset_t) 1) << (( sig ) - 1)) ;	
	return   (*set & mask) ? 1 : 0 ;	
}

extern inline int
sigaddset(sigset_t *set, int sig)	
{
	sigset_t mask = (((sigset_t) 1) << (( sig ) - 1)) ;	
	return   ((*set |= mask), 0) ;	
}

extern inline int
sigdelset(sigset_t *set, int sig)	
{
	sigset_t mask = (((sigset_t) 1) << (( sig ) - 1)) ;	
	return   ((*set &= ~mask), 0) ;	
}

#ifdef __cplusplus
}
#endif

/*
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
 *    my_signal_handler(int sig, char *somedata, vregs regs)
 *    {
 *    . . .
 *    }
 *
 *    struct sigaction sa;
 *    char data_buffer[32];
 *
 *    sa.sa_handler = (sig_func_t) my_signal_handler;
 *    sigemptyset(&sa.sa_mask);
 *    sigaddset(&sa.sa_mask, SIGINT);
 *    sa.sa_userdata = data_buffer;
 *
 *    // install the handler
 *    sigaction(SIGINT, &sa, NULL);
 *
 * The two additional arguments available to the signal handler are extensions
 * to the Posix standard. This feature was introduced by the BeOS and retained
 * by OpenBeOS. However, to remain compatible with Posix and ANSI C, the type
 * of the sa_handler field is defined as 'sig_func_t'. This requires the handler
 * to be cast when assigned to the sa_handler field, as in the example above.
 *
 * NOTE: C++ member functions can not be signal handlers!
 * This is because they expect a "this" pointer as the first argument.
 *
 *
 * The 3 arguments that OpenBeOS provides to signal handlers are as follows:
 *
 *  - The first argument is the (usual) signal number.
 *
 *  - The second argument is whatever value is put in the sa_userdata field
 *    of the sigaction struct.
 *
 *  - The third argument is a pointer to a vregs struct (defined below).
 *    The vregs struct contains the contents of the volatile registers at
 *    the time the signal was delivered to your thread. You can change the fields
 *    of the structure. After your signal handler completes, the OS uses this struct
 *    to reload the registers for your thread (privileged registers are not loaded
 *    of course). The vregs struct is of course terribly machine dependent.
 *    If you use it, you should expect to have to re-work your code when new
 *    processors come out. Nonetheless, the ability to change the registers does
 *    open some interesting programming possibilities.
 */

/*
 * the vregs struct:
 *
 * signal handlers get this as the last argument
 */

typedef struct vregs vregs;

#if __POWERPC__
struct vregs
{
	ulong pc,                                         /* program counter */
	      r0,                                         /* scratch */
	      r1,                                         /* stack ptr */
	      r2,                                         /* TOC */
	      r3,r4,r5,r6,r7,r8,r9,r10,                   /* volatile regs */
	      r11,r12;                                    /* scratch regs */

   double f0,                                         /* fp scratch */
	      f1,f2,f3,f4,f5,f6,f7,f8,f9,f10,f11,f12,f13; /* fp volatile regs */

	ulong filler1,                                    /* place holder */
	      fpscr,                                      /* fp condition codes */
	      ctr, xer, cr, msr, lr;                      /* misc. status */
};
#endif /* __POWERPC__ */ 

#if __INTEL__

typedef struct packed_fp_stack {
	unsigned char	st0[10];
	unsigned char	st1[10];
	unsigned char	st2[10];
	unsigned char	st3[10];
	unsigned char	st4[10];
	unsigned char	st5[10];
	unsigned char	st6[10];
	unsigned char	st7[10];
} packed_fp_stack;

typedef struct packed_mmx_regs {
	unsigned char	mm0[10];
	unsigned char	mm1[10];
	unsigned char	mm2[10];
	unsigned char	mm3[10];
	unsigned char	mm4[10];
	unsigned char	mm5[10];
	unsigned char	mm6[10];
	unsigned char	mm7[10];
} packed_mmx_regs;

typedef struct old_extended_regs {
	unsigned short	fp_control;
	unsigned short	_reserved1;
	unsigned short	fp_status;
	unsigned short	_reserved2;
	unsigned short	fp_tag;
	unsigned short	_reserved3;
	unsigned long	fp_eip;
	unsigned short	fp_cs;
	unsigned short	fp_opcode;
	unsigned long	fp_datap;
	unsigned short	fp_ds;
	unsigned short	_reserved4;
	union {
		packed_fp_stack	fp;
		packed_mmx_regs	mmx;
	} fp_mmx;
} old_extended_regs;

typedef struct fp_stack {
	unsigned char	st0[10];
	unsigned char	_reserved_42_47[6];
	unsigned char	st1[10];
	unsigned char	_reserved_58_63[6];
	unsigned char	st2[10];
	unsigned char	_reserved_74_79[6];
	unsigned char	st3[10];
	unsigned char	_reserved_90_95[6];
	unsigned char	st4[10];
	unsigned char	_reserved_106_111[6];
	unsigned char	st5[10];
	unsigned char	_reserved_122_127[6];
	unsigned char	st6[10];
	unsigned char	_reserved_138_143[6];
	unsigned char	st7[10];
	unsigned char	_reserved_154_159[6];
} fp_stack;

typedef struct mmx_regs {
	unsigned char	mm0[10];
	unsigned char	_reserved_42_47[6];
	unsigned char	mm1[10];
	unsigned char	_reserved_58_63[6];
	unsigned char	mm2[10];
	unsigned char	_reserved_74_79[6];
	unsigned char	mm3[10];
	unsigned char	_reserved_90_95[6];
	unsigned char	mm4[10];
	unsigned char	_reserved_106_111[6];
	unsigned char	mm5[10];
	unsigned char	_reserved_122_127[6];
	unsigned char	mm6[10];
	unsigned char	_reserved_138_143[6];
	unsigned char	mm7[10];
	unsigned char	_reserved_154_159[6];
} mmx_regs;
	
typedef struct xmmx_regs {
	unsigned char	xmm0[16];
	unsigned char	xmm1[16];
	unsigned char	xmm2[16];
	unsigned char	xmm3[16];
	unsigned char	xmm4[16];
	unsigned char	xmm5[16];
	unsigned char	xmm6[16];
	unsigned char	xmm7[16];
} xmmx_regs;

typedef struct new_extended_regs {
	unsigned short	fp_control;
	unsigned short	fp_status;
	unsigned short	fp_tag;  
	unsigned short	fp_opcode;
	unsigned long	fp_eip;
	unsigned short	fp_cs;
	unsigned short	res_14_15;
	unsigned long	fp_datap;
	unsigned short	fp_ds;
	unsigned short	_reserved_22_23;
	unsigned long	mxcsr;
	unsigned long	_reserved_28_31;
	union {
		fp_stack fp;
		mmx_regs mmx;
	} fp_mmx;
	xmmx_regs xmmx;
	unsigned char	_reserved_288_511[224];
} new_extended_regs;

typedef struct extended_regs {
	union {
		old_extended_regs	old_format;
		new_extended_regs	new_format;
	} state;
	unsigned long	format;  
} extended_regs;

struct vregs {
	unsigned long			eip;
	unsigned long			eflags;
	unsigned long			eax;
	unsigned long			ecx;
	unsigned long			edx;
	unsigned long			esp;
	unsigned long			ebp;
	unsigned long			_reserved_1;
	extended_regs	xregs;
	unsigned long			_reserved_2[3];
};
 
#endif /* __INTEL__ */

#endif /* _SIGNAL_H_ */
