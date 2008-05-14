/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_GENERIC_SYSCALLS_DEFS_H
#define _SYSTEM_GENERIC_SYSCALLS_DEFS_H


/* predefined functions */
#define B_RESERVED_SYSCALL_BASE		0x80000000
#define B_SYSCALL_INFO				(B_RESERVED_SYSCALL_BASE)
	// gets a minimum version uint32, and fills it with the current version on
	// return


#endif	/* _SYSTEM_GENERIC_SYSCALLS_DEFS_H */
