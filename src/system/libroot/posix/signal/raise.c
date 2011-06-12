/*
 *  Copyright (c) 2002-2011, Haiku Project. All rights reserved.
 *  Distributed under the terms of the Haiku license.
 *
 *  Author(s):
 *  Daniel Reinhold (danielre@users.sf.net)
 */


#include <signal.h>

#include <errno.h>

#include <OS.h>

#include <syscall_utils.h>


int
raise(int sig)
{
	RETURN_AND_SET_ERRNO(send_signal(find_thread(NULL), sig));
}

