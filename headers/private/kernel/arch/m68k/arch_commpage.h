/*
 * Copyright 2007, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_M68K_COMMPAGE_H
#define _KERNEL_ARCH_M68K_COMMPAGE_H

#ifndef _KERNEL_COMMPAGE_H
#	error Must not be included directly. Include <commpage.h> instead!
#endif

#define COMMPAGE_ENTRY_M68K_SYSCALL	(COMMPAGE_ENTRY_FIRST_ARCH_SPECIFIC + 0)
#define COMMPAGE_ENTRY_M68K_MEMCPY	(COMMPAGE_ENTRY_FIRST_ARCH_SPECIFIC + 1)

#define ARCH_USER_COMMPAGE_ADDR (0xffff0000)

#endif	/* _KERNEL_ARCH_M68K_COMMPAGE_H */
