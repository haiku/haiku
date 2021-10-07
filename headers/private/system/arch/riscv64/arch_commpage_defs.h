/*
 * Copyright 2007, Travis Geiselbrecht. All rights reserved.
 * Copyright 2019, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_ARCH_RISCV64_COMMPAGE_DEFS_H
#define _SYSTEM_ARCH_RISCV64_COMMPAGE_DEFS_H

#ifndef _SYSTEM_COMMPAGE_DEFS_H
#       error Must not be included directly. Include <commpage_defs.h> instead!
#endif

#define COMMPAGE_ENTRY_RISCV64_SIGNAL_HANDLER (COMMPAGE_ENTRY_FIRST_ARCH_SPECIFIC + 0)
#define COMMPAGE_ENTRY_RISCV64_THREAD_EXIT (COMMPAGE_ENTRY_FIRST_ARCH_SPECIFIC + 1)

#endif  /* _SYSTEM_ARCH_RISCV64_COMMPAGE_DEFS_H */
