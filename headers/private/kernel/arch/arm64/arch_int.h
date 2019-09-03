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
	__asm__ __volatile__("msr daifclr, #2");
}


static inline int
arch_int_disable_interrupts_inline(void)
{
	uint32 flags;

	__asm__ __volatile__(
			"mrs %0, daif\n\t"
			"msr daifset, #2\n\t"
			: "=&r"(flags));

	return flags;
}


static inline void
arch_int_restore_interrupts_inline(int oldState)
{
	WRITE_SPECIALREG(daif, oldState);
}


static inline bool
arch_int_are_interrupts_enabled_inline(void)
{
	uint32 flags;

	__asm__ __volatile__(
			"mrs %0, daif\n\t"
			: "=&r"(flags));

	return (flags & PSR_I) == 0;
}

// map the functions to the inline versions
#define arch_int_enable_interrupts()	arch_int_enable_interrupts_inline()
#define arch_int_disable_interrupts()	arch_int_disable_interrupts_inline()
#define arch_int_restore_interrupts(status)	\
	arch_int_restore_interrupts_inline(status)
#define arch_int_are_interrupts_enabled()	\
	arch_int_are_interrupts_enabled_inline()

#endif /* _KERNEL_ARCH_ARM64_ARCH_INT_H_ */
