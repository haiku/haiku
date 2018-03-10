/*
 * Copyright 2018, Jaroslaw Pelczar <jarek@jpelczar.com>
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_ARM64_ARCH_ATOMIC_H_
#define _KERNEL_ARCH_ARM64_ARCH_ATOMIC_H_


static inline void memory_read_barrier_inline(void)
{
	__asm__ __volatile__("dmb ishld");
}


static inline void memory_write_barrier_inline(void)
{
	__asm__ __volatile__("dsb ishst");
}


static inline void memory_full_barrier_inline(void)
{
	__asm__ __volatile__("dsb sy");
}


#define memory_read_barrier memory_read_barrier_inline
#define memory_write_barrier memory_write_barrier_inline
#define memory_full_barrier memory_full_barrier_inline


#endif /* _KERNEL_ARCH_ARM64_ARCH_ATOMIC_H_ */
