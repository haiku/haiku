/*
 * Copyright 2018, Jérôme Duval, jerome.duval@gmail.com.
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_X86_SYSCALLS_H
#define _KERNEL_ARCH_X86_SYSCALLS_H


#include <SupportDefs.h>


void	x86_initialize_syscall();
#if defined(__x86_64__) && defined(_COMPAT_MODE)
void	x86_compat_initialize_syscall();
#endif


extern void (*gX86SetSyscallStack)(addr_t stackTop);


static inline void
x86_set_syscall_stack(addr_t stackTop)
{
#if !defined(__x86_64__) || defined(_COMPAT_MODE)
	// TODO on x86_64, only necessary for 32-bit threads
	if (gX86SetSyscallStack != NULL)
		gX86SetSyscallStack(stackTop);
#endif
}


#endif	// _KERNEL_ARCH_X86_SYSCALLS_H
