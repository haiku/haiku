/*
 * Copyright 2014, Paweł Dziepak, pdziepak@quarnos.org.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_X86_64_ATOMIC_H
#define _KERNEL_ARCH_X86_64_ATOMIC_H


static inline void
memory_read_barrier_inline(void)
{
	asm volatile("lfence" : : : "memory");
}


static inline void
memory_write_barrier_inline(void)
{
	asm volatile("sfence" : : : "memory");
}


static inline void
memory_full_barrier_inline(void)
{
	asm volatile("mfence" : : : "memory");
}


#define memory_read_barrier		memory_read_barrier_inline
#define memory_write_barrier	memory_write_barrier_inline
#define memory_full_barrier		memory_full_barrier_inline

#endif	// _KERNEL_ARCH_X86_64_ATOMIC_H

