/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_ARCH_X86_DEFS_H
#define _SYSTEM_ARCH_X86_DEFS_H


#define SPINLOCK_PAUSE()	asm volatile("pause;")


#endif	/* _SYSTEM_ARCH_X86_DEFS_H */
