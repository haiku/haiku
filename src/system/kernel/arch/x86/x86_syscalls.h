/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_X86_SYSCALLS_H
#define _KERNEL_ARCH_X86_SYSCALLS_H


#include <SupportDefs.h>


void	x86_initialize_syscall();


#ifdef __x86_64__


static inline void
x86_set_syscall_stack(addr_t stackTop)
{
	// Nothing to do here, the thread's stack pointer is always accessible
	// via the GS segment.
}


#else


extern void (*gX86SetSyscallStack)(addr_t stackTop);


static inline void
x86_set_syscall_stack(addr_t stackTop)
{
	if (gX86SetSyscallStack != NULL)
		gX86SetSyscallStack(stackTop);
}


#endif	// __x86_64__

#endif	// _KERNEL_ARCH_X86_SYSCALLS_H
