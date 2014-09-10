/*
 * Copyright 2014, Paweł Dziepak, pdziepak@quarnos.org.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_X86_64_CPU_H
#define _KERNEL_ARCH_X86_64_CPU_H


#include <arch_thread_types.h>


static inline uint64_t
x86_read_msr(uint32_t msr)
{
	uint64_t high, low;
	asm volatile("rdmsr" : "=a" (low), "=d" (high) : "c" (msr));
	return (high << 32) | low;
}


static inline void
x86_write_msr(uint32_t msr, uint64_t value)
{
	asm volatile("wrmsr" : : "a" (value) , "d" (value >> 32), "c" (msr));
}


static inline void
x86_context_switch(arch_thread* oldState, arch_thread* newState)
{
	uint16_t fpuControl;
	asm volatile("fnstcw %0" : "=m" (fpuControl));
	uint32_t sseControl;
	asm volatile("stmxcsr %0" : "=m" (sseControl));
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
			"r14", "r15", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5",
			"xmm6", "xmm7", "xmm8", "xmm9", "xmm10", "xmm11", "xmm12", "xmm13",
			"xmm14", "xmm15", "memory");
	asm volatile("ldmxcsr %0" : : "m" (sseControl));
	asm volatile("fldcw %0" : : "m" (fpuControl));
}


static inline void
x86_swap_pgdir(uintptr_t root)
{
	asm volatile("movq	%0, %%cr3" : : "r" (root));
}


#endif	// _KERNEL_ARCH_X86_64_CPU_H
