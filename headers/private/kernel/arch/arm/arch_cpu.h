/*
** Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/
#ifndef _KERNEL_ARCH_M68K_CPU_H
#define _KERNEL_ARCH_M68K_CPU_H

#ifndef _ASSEMBLER

#include <arch/arm/arch_thread_types.h>
#include <kernel.h>

#endif	// !_ASSEMBLER



#ifndef _ASSEMBLER

/* raw exception frames */



#warning ARM: check for missing regs/movem
struct iframe {
	uint32 r0;
	uint32 r1;
	uint32 r2;
	uint32 r3;
	uint32 r4;
	uint32 r5;
	uint32 r6;
	uint32 r7;
	uint32 r8;
	uint32 r9;
	uint32 r10;
	uint32 r11;
	uint32 r12;
	uint32 r13;
	uint32 lr;
	uint32 pc;
	uint32 cpsr;
} _PACKED;


typedef struct arch_cpu_info {
	int null;
} arch_cpu_info;


#ifdef __cplusplus
extern "C" {
#endif


//extern void sethid0(unsigned int val);
//extern unsigned int getl2cr(void);
//extern void setl2cr(unsigned int val);
extern long long get_time_base(void);

//void __m68k_setup_system_time(vint32 *cvFactor);
	// defined in libroot: os/arch/system_time.c
//int64 __m68k_get_time_base(void);
	// defined in libroot: os/arch/system_time_asm.S

extern void arm_context_switch(void **_oldStackPointer, void *newStackPointer);

extern bool arm_set_fault_handler(addr_t *handlerLocation, addr_t handler)
	__attribute__((noinline));

//extern bool m68k_is_hw_register_readable(addr_t address);
//extern bool m68k_is_hw_register_writable(addr_t address, uint16 value);
	// defined in kernel: arch/m68k/cpu_asm.S

#ifdef __cplusplus
}
#endif

struct arm_cpu_ops {
	void (*flush_insn_pipeline)(void);
	void (*flush_atc_all)(void);
	void (*flush_atc_user)(void);
	void (*flush_atc_addr)(addr_t addr);
	void (*flush_dcache)(addr_t address, size_t len);
	void (*flush_icache)(addr_t address, size_t len);
	void (*idle)(void);
};
#warning ARM:check  cpu_ops 
extern struct arm_cpu_ops cpu_ops;

//#define


extern int arch_cpu_type;
extern int arch_fpu_type;
extern int arch_mmu_type;
extern int arch_platform;
extern int arch_machine;

#endif	// !_ASSEMBLER

#endif	/* _KERNEL_ARCH_ARM_CPU_H */
