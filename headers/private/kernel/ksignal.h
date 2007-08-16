/*
 * Copyright 2003-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_SIGNAL_H
#define _KERNEL_SIGNAL_H


#include <KernelExport.h>
#include <signal.h>


#define KILL_SIGNALS	((1L << (SIGKILL - 1)) | (1L << (SIGKILLTHR - 1)))


#ifdef __cplusplus
extern "C" {
#endif

extern bool handle_signals(struct thread *thread);
extern bool is_kill_signal_pending(void);
extern int has_signals_pending(void *_thread);

extern int sigaction_etc(thread_id threadID, int signal,
	const struct sigaction *newAction, struct sigaction *oldAction);

extern status_t _user_send_signal(pid_t tid, uint sig);
extern status_t _user_sigprocmask(int how, const sigset_t *set,
	sigset_t *oldSet);
extern status_t _user_sigaction(int sig, const struct sigaction *newAction,
	struct sigaction *oldAction);
extern bigtime_t _user_set_alarm(bigtime_t time, uint32 mode);
extern status_t _user_sigsuspend(const sigset_t *mask);
extern status_t _user_sigpending(sigset_t *set);
extern status_t _user_set_signal_stack(const stack_t *newUserStack,
	stack_t *oldUserStack);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_SIGNAL_H */
