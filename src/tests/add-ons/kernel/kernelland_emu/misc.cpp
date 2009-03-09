/*
 * Copyright 2002-2009, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Ingo Weinhold, bonefish@cs.tu-berlin.de.
 *		Axel Dörfler, axeld@pinc-software.de.
 */

#include <signal.h>
#include <stdio.h>
#include <string>

#include <KernelExport.h>


thread_id
spawn_kernel_thread(thread_func func, const char *name, int32 priority,
	void *data)
{
	return spawn_thread(func, name, priority, data);
}


extern "C" int
send_signal_etc(pid_t thread, uint signal, uint32 flags)
{
	return send_signal(thread, signal);
}
