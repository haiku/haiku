/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_ARCH_x86_64_COMMPAGE_DEFS_H
#define _SYSTEM_ARCH_x86_64_COMMPAGE_DEFS_H

#ifndef _SYSTEM_COMMPAGE_DEFS_H
#	error Must not be included directly. Include <commpage_defs.h> instead!
#endif

#define COMMPAGE_ENTRY_X86_MEMCPY	(COMMPAGE_ENTRY_FIRST_ARCH_SPECIFIC + 0)
#define COMMPAGE_ENTRY_X86_MEMSET	(COMMPAGE_ENTRY_FIRST_ARCH_SPECIFIC + 1)
#define COMMPAGE_ENTRY_X86_SIGNAL_HANDLER \
									(COMMPAGE_ENTRY_FIRST_ARCH_SPECIFIC + 2)

#define ARCH_USER_COMMPAGE_ADDR		(0xffffffffffff0000)

#endif	/* _SYSTEM_ARCH_x86_64_COMMPAGE_DEFS_H */
