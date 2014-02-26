/*
 * Copyright 2014, Paweł Dziepak, pdziepak@quarnos.org.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV <kallisti5@unixzen.com>
 */
#ifndef _KERNEL_ARCH_ARM_ATOMIC_H
#define _KERNEL_ARCH_ARM_ATOMIC_H


#if __ARM_ARCH__ <= 5
#define isb() __asm__ __volatile__("" : : : "memory")
#define dsb() __asm__ __volatile__("mcr p15, 0, %0, c7, c10, 4" \
	: : "r" (0) : "memory")
#define dmb() __asm__ __volatile__("" : : : "memory")
#elif __ARM_ARCH__ == 6
#define isb() __asm__ __volatile__("mcr p15, 0, %0, c7, c5, 4" \
	: : "r" (0) : "memory")
#define dsb() __asm__ __volatile__("mcr p15, 0, %0, c7, c10, 4" \
	: : "r" (0) : "memory")
#define dmb() __asm__ __volatile__("mcr p15, 0, %0, c7, c10, 5" \
	: : "r" (0) : "memory")
#else /* ARMv7+ */
#define isb() __asm__ __volatile__("isb" : : : "memory")
#define dsb() __asm__ __volatile__("dsb" : : : "memory")
#define dmb() __asm__ __volatile__("dmb" : : : "memory")
#endif


static inline void
memory_read_barrier_inline(void)
{
	dmb();
}


static inline void
memory_write_barrier_inline(void)
{
	dmb();
}


static inline void
memory_full_barrier_inline(void)
{
	dmb();
}


#define memory_read_barrier memory_read_barrier_inline
#define memory_write_barrier memory_write_barrier_inline
#define memory_full_barrier memory_full_barrier_inline


#endif	// _KERNEL_ARCH_ARM_ATOMIC_H
