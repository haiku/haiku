/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <kernel.h>
#include <syscalls.h>


status_t
shutdown(bool reboot)
{
	// ToDo: shutdown all system services!

	sync();

	return arch_cpu_shutdown(reboot);
}


//	#pragma mark -


status_t
_user_shutdown(bool reboot)
{
	if (geteuid() != 0)
		return B_NOT_ALLOWED;
	return shutdown(reboot);
}

