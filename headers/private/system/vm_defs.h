/*
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _SYSTEM_VM_DEFS_H
#define _SYSTEM_VM_DEFS_H

#include <OS.h>


// additional protection flags
// Note: the VM probably won't support all combinations - it will try
// its best, but create_area() will fail if it has to.
// Of course, the exact behaviour will be documented somewhere...

#define B_KERNEL_EXECUTE_AREA	0x40
#define B_KERNEL_STACK_AREA		0x80

#define B_USER_PROTECTION \
	(B_READ_AREA | B_WRITE_AREA | B_EXECUTE_AREA | B_STACK_AREA)
#define B_KERNEL_PROTECTION \
	(B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_KERNEL_EXECUTE_AREA \
	| B_KERNEL_STACK_AREA)

// TODO: These aren't really a protection flags, but since the "protection"
//	field is the only flag field, we currently use it for this.
//	A cleaner approach would be appreciated - maybe just an official generic
//	flags region in the protection field.
#define B_OVERCOMMITTING_AREA	0x1000
#define B_SHARED_AREA			0x2000
/* 0x4000 was B_KERNEL_AREA until hrev52545 */

#define B_USER_AREA_FLAGS		(B_USER_PROTECTION | B_OVERCOMMITTING_AREA)
#define B_KERNEL_AREA_FLAGS \
	(B_KERNEL_PROTECTION | B_USER_CLONEABLE_AREA | B_SHARED_AREA)

// mapping argument for several internal VM functions
enum {
	REGION_NO_PRIVATE_MAP = 0,
	REGION_PRIVATE_MAP
};

enum {
	// TODO: these are here only temporarily - it's a private
	// addition to the BeOS create_area() lock flags
	B_ALREADY_WIRED	= 7,
};

#define MEMORY_TYPE_SHIFT		28


#endif	/* _SYSTEM_VM_DEFS_H */
