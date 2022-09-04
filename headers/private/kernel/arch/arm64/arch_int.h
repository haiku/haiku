/*
 * Copyright 2018, Jaroslaw Pelczar <jarek@jpelczar.com>
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_ARM64_ARCH_INT_H_
#define _KERNEL_ARCH_ARM64_ARCH_INT_H_


#include <SupportDefs.h>
#include <kernel/arch/arm64/arm_registers.h>


#define NUM_IO_VECTORS			1024

static inline void
arch_int_enable_interrupts_inline(void)
{
	asm volatile("msr daifclr, #0xf" : : : "memory");
}


static inline int
arch_int_disable_interrupts_inline(void)
{
	uint32 flags;

	asm volatile("mrs %0, daif\n" "msr daifset, #0xf" : "=r"(flags) : : "memory");

	return flags;
}


static inline void
arch_int_restore_interrupts_inline(int oldState)
{
	asm volatile("msr daif, %0" : : "r"(oldState) : "memory");
}


static inline bool
arch_int_are_interrupts_enabled_inline(void)
{
	return (READ_SPECIALREG(DAIF) & PSR_I) == 0;
}


// map the functions to the inline versions
#define arch_int_enable_interrupts()	arch_int_enable_interrupts_inline()
#define arch_int_disable_interrupts()	arch_int_disable_interrupts_inline()
#define arch_int_restore_interrupts(status)	\
	arch_int_restore_interrupts_inline(status)
#define arch_int_are_interrupts_enabled()	\
	arch_int_are_interrupts_enabled_inline()


#endif /* _KERNEL_ARCH_ARM64_ARCH_INT_H_ */
