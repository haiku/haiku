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
#define wfi() __asm__ __volatile__("wfi" : : : "memory")

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


#define DEFINE_ARM_GET_REG(name, cp, opc1, crn, crm, opc2) \
	static inline uint32 \
	arm_get_##name(void) \
	{ \
		uint32 res; \
		asm volatile ("mrc " #cp ", " #opc1 ", %0, " #crn ", " #crm ", " #opc2 : "=r" (res)); \
		return res; \
	}


#define DEFINE_ARM_SET_REG(name, cp, opc1, crn, crm, opc2) \
	static inline void \
	arm_set_##name(uint32 val) \
	{ \
		asm volatile ("mcr " #cp ", " #opc1 ", %0, " #crn ", " #crm ", " #opc2 :: "r" (val)); \
	}


/* CP15 c1, System Control Register */
DEFINE_ARM_GET_REG(sctlr, p15, 0, c1, c0, 0)
DEFINE_ARM_SET_REG(sctlr, p15, 0, c1, c0, 0)

/* CP15 c2, Translation table support registers */
DEFINE_ARM_GET_REG(ttbr0, p15, 0, c2, c0, 0)
DEFINE_ARM_SET_REG(ttbr0, p15, 0, c2, c0, 0)
DEFINE_ARM_GET_REG(ttbr1, p15, 0, c2, c0, 1)
DEFINE_ARM_SET_REG(ttbr1, p15, 0, c2, c0, 1)
DEFINE_ARM_GET_REG(ttbcr, p15, 0, c2, c0, 2)
DEFINE_ARM_SET_REG(ttbcr, p15, 0, c2, c0, 2)

/* CP15 c5 and c6, Memory system fault registers */
DEFINE_ARM_GET_REG(dfsr, p15, 0, c5, c0, 0)
DEFINE_ARM_GET_REG(ifsr, p15, 0, c5, c0, 1)
DEFINE_ARM_GET_REG(dfar, p15, 0, c6, c0, 0)
DEFINE_ARM_GET_REG(ifar, p15, 0, c6, c0, 2)

/* CP15 c13, Process, context and thread ID registers */
DEFINE_ARM_GET_REG(tpidruro, p15, 0, c13, c0, 3)
DEFINE_ARM_SET_REG(tpidruro, p15, 0, c13, c0, 3)
DEFINE_ARM_GET_REG(tpidrprw, p15, 0, c13, c0, 4)
DEFINE_ARM_SET_REG(tpidrprw, p15, 0, c13, c0, 4)


#undef DEFINE_ARM_GET_REG
#undef DEFINE_ARM_SET_REG


static inline addr_t
arm_get_fp(void)
{
	uint32 res;
	asm volatile ("mov %0, fp": "=r" (res));
	return res;
}


void arch_cpu_invalidate_TLB_page(addr_t page);

static inline void
arch_cpu_pause(void)
{
	// TODO: ARM Priority pause call
}


static inline void
arch_cpu_idle(void)
{
	wfi();
}


#ifdef __cplusplus
};
#endif

#endif	// !_ASSEMBLER

#endif	/* _KERNEL_ARCH_ARM_CPU_H */
