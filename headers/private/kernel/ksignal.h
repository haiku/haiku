/* 
** Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/
#ifndef _KERNEL_SIGNAL_H
#define _KERNEL_SIGNAL_H


#include <signal.h>

#define BLOCKABLE_SIGS	(~((1L << (SIGKILL - 1)) | (1L << (SIGSTOP - 1))))


extern int handle_signals(struct thread *t, int state);

extern int _user_send_signal(pid_t tid, uint sig);
extern int _user_sigprocmask(int how, const sigset_t *set, sigset_t *oldSet);
extern int _user_sigaction(int sig, const struct sigaction *action, struct sigaction *oldAction);
extern bigtime_t _user_set_alarm(bigtime_t time, uint32 mode);

#endif	/* _KERNEL_SIGNAL_H */
