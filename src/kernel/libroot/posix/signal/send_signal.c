#include <syscalls.h>
#include <signal.h>

/*
 *  Copyright (c) 2002, OpenBeOS Project. All rights reserved.
 *  Distributed under the terms of the OpenBeOS license.
 *
 *  send_signal.c:
 *  implements the signal function send_signal()
 *  this is merely a wrapper for a syscall
 *
 *
 *  Author(s):
 *  Daniel Reinhold (danielre@users.sf.net)
 *
 */


int
send_signal(pid_t tid, uint sig)
{
	return sys_send_signal(tid, sig);
}

