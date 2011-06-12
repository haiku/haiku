/*
 *  Copyright (c) 2002-2011, Haiku Project. All rights reserved.
 *  Distributed under the terms of the Haiku license.
 *
 *  Author(s):
 *  Daniel Reinhold (danielre@users.sf.net)
 */


#include <OS.h>

#include <signal_defs.h>
#include <syscalls.h>


int
send_signal(thread_id thread, uint sig)
{
	return _kern_send_signal(thread, sig, NULL, SIGNAL_FLAG_SEND_TO_THREAD);
}
