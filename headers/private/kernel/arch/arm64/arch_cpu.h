/*
 * Copyright 2018, Jaroslaw Pelczar <jarek@jpelczar.com>
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_ARM64_ARCH_CPU_H_
#define _KERNEL_ARCH_ARM64_ARCH_CPU_H_


#define CPU_MAX_CACHE_LEVEL 	8
#define CACHE_LINE_SIZE 		64

#define set_ac()
#define clear_ac()

#include <kernel/arch/arm64/arm_registers.h>

#ifndef _ASSEMBLER

#include <arch/arm64/arch_thread_types.h>
#include <kernel.h>

#define arm64_sev()  		__asm__ __volatile__("sev" : : : "memory")
#define arm64_wfe()  		__asm__ __volatile__("wfe" : : : "memory")
#define arm64_dsb()  		__asm__ __volatile__("dsb" : : : "memory")
#define arm64_dmb()  		__asm__ __volatile__("dmb" : : : "memory")
#define arm64_isb()  		__asm__ __volatile__("isb" : : : "memory")
#define arm64_nop()  		__asm__ __volatile__("nop" : : : "memory")
#define arm64_yield() 	__asm__ __volatile__("yield" : : : "memory")

/* Extract CPU affinity levels 0-3 */
#define	CPU_AFF0(mpidr)	(u_int)(((mpidr) >> 0) & 0xff)
#define	CPU_AFF1(mpidr)	(u_int)(((mpidr) >> 8) & 0xff)
#define	CPU_AFF2(mpidr)	(u_int)(((mpidr) >> 16) & 0xff)
#define	CPU_AFF3(mpidr)	(u_int)(((mpidr) >> 32) & 0xff)
#define	CPU_AFF0_MASK	0xffUL
#define	CPU_AFF1_MASK	0xff00UL
#define	CPU_AFF2_MASK	0xff0000UL
#define	CPU_AFF3_MASK	0xff00000000UL
#define	CPU_AFF_MASK	(CPU_AFF0_MASK | CPU_AFF1_MASK | \
    CPU_AFF2_MASK| CPU_AFF3_MASK)	/* Mask affinity fields in MPIDR_EL1 */


#define	CPU_IMPL_ARM		0x41
#define	CPU_IMPL_BROADCOM	0x42
#define	CPU_IMPL_CAVIUM		0x43
#define	CPU_IMPL_DEC		0x44
#define	CPU_IMPL_INFINEON	0x49
#define	CPU_IMPL_FREESCALE	0x4D
#define	CPU_IMPL_NVIDIA		0x4E
#define	CPU_IMPL_APM		0x50
#define	CPU_IMPL_QUALCOMM	0x51
#define	CPU_IMPL_MARVELL	0x56
#define	CPU_IMPL_INTEL		0x69

#define	CPU_PART_THUNDER	0x0A1
#define	CPU_PART_FOUNDATION	0xD00
#define	CPU_PART_CORTEX_A35	0xD04
#define	CPU_PART_CORTEX_A53	0xD03
#define	CPU_PART_CORTEX_A55	0xD05
#define	CPU_PART_CORTEX_A57	0xD07
#define	CPU_PART_CORTEX_A72	0xD08
#define	CPU_PART_CORTEX_A73	0xD09
#define	CPU_PART_CORTEX_A75	0xD0A

#define	CPU_REV_THUNDER_1_0	0x00
#define	CPU_REV_THUNDER_1_1	0x01

#define	CPU_IMPL(midr)	(((midr) >> 24) & 0xff)
#define	CPU_PART(midr)	(((midr) >> 4) & 0xfff)
#define	CPU_VAR(midr)	(((midr) >> 20) & 0xf)
#define	CPU_REV(midr)	(((midr) >> 0) & 0xf)

#define	CPU_IMPL_TO_MIDR(val)	(((val) & 0xff) << 24)
#define	CPU_PART_TO_MIDR(val)	(((val) & 0xfff) << 4)
#define	CPU_VAR_TO_MIDR(val)	(((val) & 0xf) << 20)
#define	CPU_REV_TO_MIDR(val)	(((val) & 0xf) << 0)

#define	CPU_IMPL_MASK	(0xff << 24)
#define	CPU_PART_MASK	(0xfff << 4)
#define	CPU_VAR_MASK	(0xf << 20)
#define	CPU_REV_MASK	(0xf << 0)

#define	CPU_ID_RAW(impl, part, var, rev)		\
    (CPU_IMPL_TO_MIDR((impl)) |				\
    CPU_PART_TO_MIDR((part)) | CPU_VAR_TO_MIDR((var)) |	\
    CPU_REV_TO_MIDR((rev)))

#define	CPU_MATCH(mask, impl, part, var, rev)		\
    (((mask) & PCPU_GET(midr)) ==			\
    ((mask) & CPU_ID_RAW((impl), (part), (var), (rev))))

#define	CPU_MATCH_RAW(mask, devid)			\
    (((mask) & PCPU_GET(midr)) == ((mask) & (devid)))

static inline uint64 arm64_get_cyclecount(void)
{
	return READ_SPECIALREG(cntvct_el0);
}

#define	ADDRESS_TRANSLATE_FUNC(stage)					\
static inline uint64									\
arm64_address_translate_ ##stage (uint64 addr)		\
{														\
	uint64 ret;											\
														\
	__asm __volatile(									\
	    "at " __ARMREG_STRING(stage) ", %1 \n"					\
	    "mrs %0, par_el1" : "=r"(ret) : "r"(addr));		\
														\
	return (ret);										\
}

ADDRESS_TRANSLATE_FUNC(s1e0r)
ADDRESS_TRANSLATE_FUNC(s1e0w)
ADDRESS_TRANSLATE_FUNC(s1e1r)
ADDRESS_TRANSLATE_FUNC(s1e1w)

/* raw exception frames */
struct iframe {
	uint64			sp;
	uint64			lr;
	uint64			elr;
	uint32			spsr;
	uint32			esr;
	uint64			x[30];
};

#ifdef __cplusplus
namespace BKernel {
	struct Thread;
}  // namespace BKernel

typedef struct arch_cpu_info {
	uint32						mpidr;
	BKernel::Thread*			last_vfp_user;
} arch_cpu_info;
#endif

#ifdef __cplusplus
extern "C" {
#endif

static inline void arch_cpu_pause(void)
{
	arm64_yield();
}

static inline void arch_cpu_idle(void)
{
	arm64_yield();
}

#ifdef __cplusplus
}
#endif

#endif


#endif /* _KERNEL_ARCH_ARM64_ARCH_CPU_H_ */
