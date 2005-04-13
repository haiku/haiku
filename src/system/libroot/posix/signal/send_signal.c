/*
 *  Copyright (c) 2002-2004, Haiku Project. All rights reserved.
 *  Distributed under the terms of the Haiku license.
 *
 *  Author(s):
 *  Daniel Reinhold (danielre@users.sf.net)
 */


#include <syscalls.h>
#include <signal.h>


int
send_signal(pid_t thread, uint sig)
{
	return _kern_send_signal(thread, sig);
}

