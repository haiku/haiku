/*
 * Copyright 2007, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_COMMPAGE_DEFS_H
#define _SYSTEM_COMMPAGE_DEFS_H

/*! Some systemwide commpage constants, used in the kernel and libroot */

/* be careful what you put here, this file is included from assembly */
#define COMMPAGE_ENTRY_MAGIC				0
#define COMMPAGE_ENTRY_VERSION				1
#define COMMPAGE_ENTRY_REAL_TIME_DATA		2
#define COMMPAGE_ENTRY_FIRST_ARCH_SPECIFIC	3

#define COMMPAGE_SIZE (0x8000)
#define COMMPAGE_TABLE_ENTRIES 64

#define COMMPAGE_SIGNATURE 'COMM'
#define COMMPAGE_VERSION 1

#include <arch_commpage_defs.h>

#endif	/* _SYSTEM_COMMPAGE_DEFS_H */
