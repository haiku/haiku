/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef _KERNEL_SIGNAL_H
#define _KERNEL_SIGNAL_H

#include <signal.h>

#define BLOCKABLE_SIGS	(~((1L << (SIGKILL - 1)) | (1L << (SIGSTOP - 1))))

extern int handle_signals(struct thread *t, int state);


extern int _user_send_signal(pid_t tid, uint sig);
extern int _user_sigaction(int sig, const struct sigaction *act, struct sigaction *oact);
extern bigtime_t _user_set_alarm(bigtime_t time, uint32 mode);

#endif	/* _KERNEL_SIGNAL_H */
