/*
 * Copyright 2013-2022 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef BOOT_ARCH_CPU_H
#define BOOT_ARCH_CPU_H


#include <SupportDefs.h>
#include <boot/vfs.h>


#ifdef __cplusplus
extern "C" {
#endif

status_t boot_arch_cpu_init(void);
void arch_ucode_load(BootVolume& volume);

bigtime_t system_time();
void spin(bigtime_t microseconds);


static inline uint32
cpu_read_CPSR(void)
{
	uint32 res;
	asm volatile("MRS %0, CPSR": "=r" (res));
	return res;
}


static inline uint32
mmu_read_SCTLR(void)
{
	uint32 res;
	asm volatile("MRC p15, 0, %0, c1, c0, 0": "=r" (res));
	return res;
}


static inline uint32
mmu_read_TTBR0(void)
{
	uint32 res;
	asm volatile("MRC p15, 0, %0, c2, c0, 0": "=r" (res));
	return res;
}


static inline uint32
mmu_read_TTBR1(void)
{
	uint32 res;
	asm volatile("MRC p15, 0, %0, c2, c0, 1": "=r" (res));
	return res;
}


static inline uint32
mmu_read_TTBCR(void)
{
	uint32 res;
	asm volatile("MRC p15, 0, %0, c2, c0, 2": "=r" (res));
	return res;
}


static inline uint32
mmu_read_DACR(void)
{
	uint32 res;
	asm volatile("MRC p15, 0, %0, c3, c0, 0": "=r" (res));
	return res;
}


#ifdef __cplusplus
}
#endif


#endif /* BOOT_ARCH_CPU_H */
