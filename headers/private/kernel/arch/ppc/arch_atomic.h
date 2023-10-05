/*
 * Copyright 2014, Paweł Dziepak, pdziepak@quarnos.org.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV <kallisti5@unixzen.com>
 */
#ifndef _KERNEL_ARCH_PPC_ATOMIC_H
#define _KERNEL_ARCH_PPC_ATOMIC_H


static inline void
memory_read_barrier_inline(void)
{
	#ifdef __powerpc64__
	asm volatile("lwsync" : : : "memory");
	#else
	asm volatile("sync" : : : "memory");
	#endif
}


static inline void
memory_write_barrier_inline(void)
{
	#ifdef __powerpc64__
	asm volatile("lwsync" : : : "memory");
	#else
	asm volatile("eieio" : : : "memory");
	#endif
}


static inline void
memory_full_barrier_inline(void)
{
	asm volatile("sync" : : : "memory");
}


#define memory_read_barrier		memory_read_barrier_inline
#define memory_write_barrier	memory_write_barrier_inline
#define memory_full_barrier		memory_full_barrier_inline


#endif	// _KERNEL_ARCH_PPC_ATOMIC_H
