/*
 * Copyright 2014, Paweł Dziepak, pdziepak@quarnos.org.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_X86_32_ATOMIC_H
#define _KERNEL_ARCH_X86_32_ATOMIC_H


static inline void
memory_read_barrier_inline(void)
{
	asm volatile("lock; addl $0, (%%esp)" : : : "memory");
}


static inline void
memory_write_barrier_inline(void)
{
	asm volatile("lock; addl $0, (%%esp)" : : : "memory");
}


static inline void
memory_full_barrier_inline(void)
{
	asm volatile("lock; addl $0, (%%esp)" : : : "memory");
}


#define memory_read_barrier		memory_read_barrier_inline
#define memory_write_barrier	memory_write_barrier_inline
#define memory_full_barrier		memory_full_barrier_inline


#if __GNUC__ < 4


static inline void
atomic_set_inline(int32* value, int32 newValue)
{
	memory_write_barrier();
	*(volatile int32*)value = newValue;
}


static inline int32
atomic_get_and_set_inline(int32* value, int32 newValue)
{
	asm volatile("xchgl %0, (%1)"
		: "+r" (newValue)
		: "r" (value)
		: "memory");
	return newValue;
}


static inline int32
atomic_test_and_set_inline(int32* value, int32 newValue, int32 testAgainst)
{
	asm volatile("lock; cmpxchgl %2, (%3)"
		: "=a" (newValue)
		: "0" (testAgainst), "r" (newValue), "r" (value)
		: "memory");
	return newValue;
}


static inline int32
atomic_add_inline(int32* value, int32 newValue)
{
	asm volatile("lock; xaddl %0, (%1)"
		: "+r" (newValue)
		: "r" (value)
		: "memory");
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
#define atomic_test_and_set		atomic_test_and_set_inline
#define atomic_add				atomic_add_inline
#define atomic_get				atomic_get_inline


#endif	// dark ages


#endif	// _KERNEL_ARCH_X86_32_ATOMIC_H

