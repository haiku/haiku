/*
 * Copyright 2003-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_SIGNAL_H
#define _KERNEL_SIGNAL_H


#include <KernelExport.h>
#include <signal.h>


#define KILL_SIGNALS	((1L << (SIGKILL - 1)) | (1L << (SIGKILLTHR - 1)))
#define BLOCKABLE_SIGS	(~(KILL_SIGNALS | (1L << (SIGSTOP - 1))))


#ifdef __cplusplus
extern "C" {
#endif

extern int handle_signals(struct thread *t, cpu_status *state);
extern bool is_kill_signal_pending(void);
extern int has_signals_pending(void *_thread);

extern int sigaction_etc(thread_id threadID, int signal,
			const struct sigaction *act, struct sigaction *oact);

extern int _user_send_signal(pid_t tid, uint sig);
extern int _user_sigprocmask(int how, const sigset_t *set, sigset_t *oldSet);
extern int _user_sigaction(int sig, const struct sigaction *action, struct sigaction *oldAction);
extern bigtime_t _user_set_alarm(bigtime_t time, uint32 mode);
extern int _user_sigsuspend(const sigset_t *mask);
extern int _user_sigpending(sigset_t *set);

#ifdef __cplusplus
}	// extern "C"
#endif

#endif	/* _KERNEL_SIGNAL_H */
