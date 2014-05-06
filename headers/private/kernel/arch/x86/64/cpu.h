/*
 * Copyright 2014, Paweł Dziepak, pdziepak@quarnos.org.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_X86_64_CPU_H
#define _KERNEL_ARCH_X86_64_CPU_H


#include <arch_thread_types.h>


static inline void
x86_context_switch(arch_thread* oldState, arch_thread* newState)
{
	asm volatile(
		"pushq	%%rbp;"
		"movq	$1f, %c[rip](%0);"
		"movq	%%rsp, %c[rsp](%0);"
		"movq	%c[rsp](%1), %%rsp;"
		"jmp	*%c[rip](%1);"
		"1:"
		"popq	%%rbp;"
		:
		: "a" (oldState), "d" (newState),
			[rsp] "i" (offsetof(arch_thread, current_stack)),
			[rip] "i" (offsetof(arch_thread, instruction_pointer))
		: "rbx", "rcx", "rdi", "rsi", "r8", "r9", "r10", "r11", "r12", "r13",
			"r14", "r15", "memory");
}


#endif	// _KERNEL_ARCH_X86_64_CPU_H
