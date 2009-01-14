/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "launch_speedup.h"

#include <OS.h>
#include <syscalls.h>
#include <generic_syscall_defs.h>

#include <stdio.h>
#include <string.h>


int
main(int argc, char **argv)
{
	uint32 version = 0;
	status_t status = _kern_generic_syscall(LAUNCH_SPEEDUP_SYSCALLS, B_SYSCALL_INFO,
		&version, sizeof(version));
	if (status != B_OK) {
		// the launch speedup module is not available
		fprintf(stderr, "\"launch_speedup\" module not available.\n");
		return 1;
	}

	_kern_generic_syscall(LAUNCH_SPEEDUP_SYSCALLS, LAUNCH_SPEEDUP_STOP_SESSION,
		(void *)"system boot", strlen("system boot"));
	return 0;
}

