/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_X86_64_VM_H
#define _KERNEL_ARCH_X86_64_VM_H

/* This many pages will be read/written on I/O if possible */

#define NUM_IO_PAGES	4
	/* 16 kB */

#define PAGE_SHIFT 12

#endif	/* _KERNEL_ARCH_X86_64_VM_H */
