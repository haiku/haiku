/*
 * Copyright 2014, Paweł Dziepak, pdziepak@quarnos.org.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV <kallisti5@unixzen.com>
 */
#ifndef _KERNEL_ARCH_SPARC_ATOMIC_H
#define _KERNEL_ARCH_SPARC_ATOMIC_H


static inline void
memory_read_barrier_inline(void)
{
	// TODO
}


static inline void
memory_write_barrier_inline(void)
{
	// TODO
}


static inline void
memory_full_barrier_inline(void)
{
	// TODO
}


#define memory_read_barrier		memory_read_barrier_inline
#define memory_write_barrier	memory_write_barrier_inline
#define memory_full_barrier		memory_full_barrier_inline


static inline void
atomic_set_inline(int32* value, int32 newValue)
{
	memory_write_barrier();
	*(volatile int32*)value = newValue;
}


static inline int32
atomic_get_and_set_inline(int32* value, int32 newValue)
{
	// BIG TODO: PowerPC Atomic get and set
//	asm volatile("xchgl %0, (%1)"
//		: "+r" (newValue)
//		: "r" (value)
//		: "memory");
	return newValue;
}


static inline int32
atomic_test_and_set_inline(int32* value, int32 newValue, int32 testAgainst)
{
	// BIG TODO: PowerPC Atomic test and set inline
//	asm volatile("lock; cmpxchgl %2, (%3)"
//		: "=a" (newValue)
//		: "0" (testAgainst), "r" (newValue), "r" (value)
//		: "memory");
	return newValue;
}


static inline int32
atomic_add_inline(int32* value, int32 newValue)
{
	// BIG TODO: PowerPC Atomic add inline
//	asm volatile("lock; xaddl %0, (%1)"
//		: "+r" (newValue)
//		: "r" (value)
//		: "memory");
	return newValue;
}


static inline int32
atomic_get_inline(int32* value)
{
	int32 newValue = *(volatile int32*)value;
	memory_read_barrier();
	return newValue;
}


#define atomic_set				atomic_set_inline
#define atomic_get_and_set		atomic_get_and_set_inline
#ifndef atomic_test_and_set
#	define atomic_test_and_set	atomic_test_and_set_inline
#endif
#ifndef atomic_add
#	define atomic_add			atomic_add_inline
#endif
#define atomic_get				atomic_get_inline


#endif	// _KERNEL_ARCH_PPC_ATOMIC_H

