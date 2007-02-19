/*
 * Copyright 2007, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __OS_KERNEL_ARCH_x86_COMMPAGE_H
#define __OS_KERNEL_ARCH_x86_COMMPAGE_H

/* some systemwide commpage constants, used in the kernel and libroot */

/* be careful what you put here, this file is included from assembly */
#define COMMPAGE_ENTRY_MAGIC   0
#define COMMPAGE_ENTRY_VERSION 1
#define COMMPAGE_ENTRY_SYSCALL 2

#define USER_COMMPAGE_ADDR (0xffff0000)
#define COMMPAGE_SIZE (0x8000)
#define TABLE_ENTRIES 64

#define COMMPAGE_SIGNATURE 'COMM'
#define COMMPAGE_VERSION 1

#endif

