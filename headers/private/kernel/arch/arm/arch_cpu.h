/*
** Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef _KERNEL_ARCH_ARM_CPU_H
#define _KERNEL_ARCH_ARM_CPU_H


#define CPU_MAX_CACHE_LEVEL 8
#define CACHE_LINE_SIZE 64
	// TODO: Could be 32-bits sometimes?


#define isb() __asm__ __volatile__("isb" : : : "memory")
#define dsb() __asm__ __volatile__("dsb" : : : "memory")
#define dmb() __asm__ __volatile__("dmb" : : : "memory")

#define set_ac()
#define clear_ac()


#ifndef _ASSEMBLER

#include <arch/arm/arch_thread_types.h>
#include <kernel.h>

/**! Values for arch_cpu_info.arch */
enum {
	ARCH_ARM_PRE_ARM7,
	ARCH_ARM_v3,
	ARCH_ARM_v4,
	ARCH_ARM_v4T,
	ARCH_ARM_v5,
	ARCH_ARM_v5T,
	ARCH_ARM_v5TE,
	ARCH_ARM_v5TEJ,
	ARCH_ARM_v6,
	ARCH_ARM_v7
};

typedef struct arch_cpu_info {
	/* For a detailed interpretation of these values,
	   see "The System Control coprocessor",
	   "Main ID register" in your ARM ARM */
	int implementor;
	int part_number;
	int revision;
	int variant;
	int arch;
} arch_cpu_info;

#ifdef __cplusplus
extern "C" {
#endif

extern uint32 arm_get_dfsr(void);
extern uint32 arm_get_ifsr(void);
extern addr_t arm_get_dfar(void);
extern addr_t arm_get_ifar(void);

extern addr_t arm_get_fp(void);

extern int arm_get_sctlr(void);
extern int arm_set_sctlr(int val);

void arch_cpu_invalidate_TLB_page(addr_t page);

static inline void
arch_cpu_pause(void)
{
	// TODO: ARM Priority pause call
}


static inline void
arch_cpu_idle(void)
{
	uint32 Rd = 0;
	asm volatile("mcr p15, 0, %[c7format], c7, c0, 4"
		: : [c7format] "r" (Rd) );
}


#ifdef __cplusplus
};
#endif

#endif	// !_ASSEMBLER

#endif	/* _KERNEL_ARCH_ARM_CPU_H */
