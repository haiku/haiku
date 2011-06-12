/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _LIBROOT_SIGNAL_PRIVATE_H
#define _LIBROOT_SIGNAL_PRIVATE_H


#include <signal.h>
#include <sys/cdefs.h>

#include <signal_defs.h>


#define MAX_SIGNAL_NUMBER_BEOS		29


typedef __haiku_int32 sigset_t_beos;

struct sigaction_beos {
	__sighandler_t	sa_handler;
	sigset_t_beos	sa_mask;
	int				sa_flags;
	void*			sa_userdata;
};


static inline sigset_t_beos
to_beos_sigset(sigset_t set)
{
	// restrict to BeOS signals
	sigset_t_beos beosSet = (sigset_t_beos)(set
		& SIGNAL_RANGE_TO_MASK(1, MAX_SIGNAL_NUMBER_BEOS));

	// if SIGBUS is set, set SIGSEGV, since they have the same number in BeOS
	if ((set & SIGNAL_TO_MASK(SIGBUS)) != 0)
		beosSet |= SIGNAL_TO_MASK(SIGSEGV);

	return beosSet;
}


static inline sigset_t
from_beos_sigset(sigset_t_beos beosSet)
{
	sigset_t set = beosSet;

	// if SIGSEGV is set, set SIGBUS, since they have the same number in BeOS
	if ((set & SIGNAL_TO_MASK(SIGSEGV)) != 0)
		set |= SIGNAL_TO_MASK(SIGBUS);

	return set;
}


__BEGIN_DECLS


__sighandler_t __signal_beos(int signal, __sighandler_t signalHandler);
__sighandler_t __signal(int signal, __sighandler_t signalHandler);

int __sigaction_beos(int signal, const struct sigaction_beos* beosAction,
	struct sigaction_beos* beosOldAction);
int __sigaction(int signal, const struct sigaction* action,
	struct sigaction* oldAction);

__sighandler_t __sigset_beos(int signal, __sighandler_t signalHandler);
__sighandler_t __sigset(int signal, __sighandler_t signalHandler);

int __sigignore_beos(int signal);
int __sigignore(int signal);

int __sighold_beos(int signal);
int __sighold(int signal);

int __sigrelse_beos(int signal);
int __sigrelse(int signal);

int __sigpause_beos(int signal);
int __sigpause(int signal);

int __siginterrupt_beos(int signal, int flag);
int __siginterrupt(int signal, int flag);

int __pthread_sigmask_beos(int how, const sigset_t_beos* beosSet,
	sigset_t_beos* beosOldSet);
int __sigprocmask_beos(int how, const sigset_t_beos* beosSet,
	sigset_t_beos* beosOldSet);

int __pthread_sigmask(int how, const sigset_t* set, sigset_t* oldSet);
int __sigprocmask(int how, const sigset_t* set, sigset_t* oldSet);

int __sigpending_beos(sigset_t_beos* beosSet);
int __sigpending(sigset_t* set);

int __sigsuspend_beos(const sigset_t_beos* beosMask);
int __sigsuspend(const sigset_t* mask);

int __sigwait_beos(const sigset_t_beos* beosSet, int* _signal);
int __sigwait(const sigset_t* set, int* _signal);

int __sigemptyset_beos(sigset_t_beos* set);
int __sigfillset_beos(sigset_t_beos* set);
int __sigismember_beos(const sigset_t_beos* set, int signal);
int __sigaddset_beos(sigset_t_beos* set, int signal);
int __sigdelset_beos(sigset_t_beos* set, int signal);

int __sigemptyset(sigset_t* set);
int __sigfillset(sigset_t* set);
int __sigismember(const sigset_t* set, int signal);
int __sigaddset(sigset_t* set, int signal);
int __sigdelset(sigset_t* set, int signal);


__END_DECLS


#endif	// _LIBROOT_SIGNAL_PRIVATE_H
