/*
 * Copyright 2014, Paweł Dziepak, pdziepak@quarnos.org.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol <revol@free.fr>
 */
#ifndef _KERNEL_ARCH_RISCV64_ATOMIC_H
#define _KERNEL_ARCH_RISCV64_ATOMIC_H


static inline void
memory_read_barrier_inline(void)
{
	asm volatile ("nop;" : : : "memory");
#warning RISCV64: check memory_read_barrier_inline (FNOP ?)
}


static inline void
memory_write_barrier_inline(void)
{
	asm volatile ("nop;" : : : "memory");
#warning RISCV64: check memory_write_barrier_inline (FNOP ?)
}


static inline void
memory_full_barrier_inline(void)
{
	asm volatile ("nop;" : : : "memory");
#warning RISCV64: check memory_full_barrier_inline (FNOP ?)
}


#define memory_read_barrier memory_read_barrier_inline
#define memory_write_barrier memory_write_barrier_inline
#define memory_full_barrier memory_full_barrier_inline


#endif	// _KERNEL_ARCH_RISCV64_ATOMIC_H
