/*
 * Copyright 2003-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_SIGNAL_H
#define _KERNEL_SIGNAL_H


#include <KernelExport.h>
#include <signal.h>


namespace BKernel {
	struct Thread;
}

using BKernel::Thread;


#define KILL_SIGNALS	((1L << (SIGKILL - 1)) | (1L << (SIGKILLTHR - 1)))

#define SIGNAL_TO_MASK(signal)	(1LL << (signal - 1))

// additional send_signal_etc() flag
#define SIGNAL_FLAG_TEAMS_LOCKED			(0x10000)
	// interrupts are disabled and team lock is held
#define SIGNAL_FLAG_DONT_RESTART_SYSCALL	(0x20000)


#ifdef __cplusplus
extern "C" {
#endif

extern bool handle_signals(Thread *thread);
extern bool is_kill_signal_pending(void);
extern int has_signals_pending(void *_thread);
extern bool is_signal_blocked(int signal);

extern void update_current_thread_signals_flag();

extern int sigaction_etc(thread_id threadID, int signal,
	const struct sigaction *newAction, struct sigaction *oldAction);

extern status_t _user_send_signal(pid_t tid, uint sig);
extern status_t _user_sigprocmask(int how, const sigset_t *set,
	sigset_t *oldSet);
extern status_t _user_sigaction(int sig, const struct sigaction *newAction,
	struct sigaction *oldAction);
extern bigtime_t _user_set_alarm(bigtime_t time, uint32 mode);
extern status_t _user_sigwait(const sigset_t *set, int *_signal);
extern status_t _user_sigsuspend(const sigset_t *mask);
extern status_t _user_sigpending(sigset_t *set);
extern status_t _user_set_signal_stack(const stack_t *newUserStack,
	stack_t *oldUserStack);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_SIGNAL_H */
