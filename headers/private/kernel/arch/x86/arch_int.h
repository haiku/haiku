/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_x86_INT_H
#define _KERNEL_ARCH_x86_INT_H


#define ARCH_INTERRUPT_BASE	0x20
#define NUM_IO_VECTORS		(256 - ARCH_INTERRUPT_BASE)


static inline void
arch_int_enable_interrupts_inline(void)
{
	asm volatile("sti");
}


static inline int
arch_int_disable_interrupts_inline(void)
{
	int flags;

	asm volatile("pushfl;\n"
		"popl %0;\n"
		"cli" : "=g" (flags));
	return flags & 0x200 ? 1 : 0;
}


static inline void
arch_int_restore_interrupts_inline(int oldstate)
{
	int flags = oldstate ? 0x200 : 0;

	asm volatile("pushfl;\n"
		"popl	%0;\n"
		"andl	$0xfffffdff,%0;\n"
		"orl	%1,%0;\n"
		"pushl	%0;\n"
		"popfl\n"
		: "=&r"(flags) : "r"(flags));
}


static inline bool
arch_int_are_interrupts_enabled_inline(void)
{
	int flags;

	asm volatile("pushfl;\n"
		"popl %0;\n" : "=g" (flags));
	return flags & 0x200 ? 1 : 0;
}


// map the functions to the inline versions
#define arch_int_enable_interrupts()	arch_int_enable_interrupts_inline()
#define arch_int_disable_interrupts()	arch_int_disable_interrupts_inline()
#define arch_int_restore_interrupts(status)	\
	arch_int_restore_interrupts_inline(status)
#define arch_int_are_interrupts_enabled()	\
	arch_int_are_interrupts_enabled_inline()


#endif /* _KERNEL_ARCH_x86_INT_H */
