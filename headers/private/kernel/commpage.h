/*
 * Copyright 2007, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_COMMPAGE_H
#define _KERNEL_COMMPAGE_H

/*! Some systemwide commpage constants, used in the kernel and libroot */

#ifndef _ASSEMBLER
#	include <SupportDefs.h>
#endif


/* be careful what you put here, this file is included from assembly */
#define COMMPAGE_ENTRY_MAGIC				0
#define COMMPAGE_ENTRY_VERSION				1
#define COMMPAGE_ENTRY_REAL_TIME_DATA		2
#define COMMPAGE_ENTRY_FIRST_ARCH_SPECIFIC	3

#define COMMPAGE_SIZE (0x8000)
#define COMMPAGE_TABLE_ENTRIES 64

#define COMMPAGE_SIGNATURE 'COMM'
#define COMMPAGE_VERSION 1

#define USER_COMMPAGE_ADDR	ARCH_USER_COMMPAGE_ADDR
	// set by the architecture specific implementation

#ifndef _ASSEMBLER

#define USER_COMMPAGE_TABLE	((void**)(USER_COMMPAGE_ADDR))

#ifdef __cplusplus
extern "C" {
#endif

status_t	commpage_init(void);
void*		allocate_commpage_entry(int entry, size_t size);
void*		fill_commpage_entry(int entry, const void* copyFrom, size_t size);

status_t	arch_commpage_init(void);
	// implemented in the architecture specific part

#ifdef __cplusplus
}	// extern "C"
#endif

#endif	// ! _ASSEMBLER

#include <arch_commpage.h>

#endif	/* _KERNEL_COMMPAGE_H */
