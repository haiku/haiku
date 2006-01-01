/*
** Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/
#ifndef _KERNEL_ARCH_PPC_CPU_H
#define _KERNEL_ARCH_PPC_CPU_H


#include <arch/ppc/arch_thread_types.h>
#include <kernel.h>


#define PAGE_SIZE 4096

struct iframe {
	uint32 srr0;
	uint32 srr1;
	uint32 dar;
	uint32 dsisr;
	uint32 lr;
	uint32 cr;
	uint32 xer;
	uint32 ctr;
	uint32 r31;
	uint32 r30;
	uint32 r29;
	uint32 r28;
	uint32 r27;
	uint32 r26;
	uint32 r25;
	uint32 r24;
	uint32 r23;
	uint32 r22;
	uint32 r21;
	uint32 r20;
	uint32 r19;
	uint32 r18;
	uint32 r17;
	uint32 r16;
	uint32 r15;
	uint32 r14;
	uint32 r13;
	uint32 r12;
	uint32 r11;
	uint32 r10;
	uint32 r9;
	uint32 r8;
	uint32 r7;
	uint32 r6;
	uint32 r5;
	uint32 r4;
	uint32 r3;
	uint32 r2;
	uint32 r1;
	uint32 r0;
};

enum machine_state {
	MSR_EXCEPTIONS_ENABLED			= 1L << 15,		// EE
	MSR_PRIVILEGE_LEVEL				= 1L << 14,		// PR
	MSR_FP_AVAILABLE				= 1L << 13,		// FP
	MSR_MACHINE_CHECK_ENABLED		= 1L << 12,		// ME
	MSR_EXCEPTION_PREFIX			= 1L << 6,		// IP
	MSR_INST_ADDRESS_TRANSLATION	= 1L << 5,		// IR
	MSR_DATA_ADDRESS_TRANSLATION	= 1L << 4,		// DR
};

struct block_address_translation;

#ifdef __cplusplus
extern "C" {
#endif

extern uint32 get_sdr1(void);
extern void set_sdr1(uint32 value);
extern uint32 get_sr(void *virtualAddress);
extern void set_sr(void *virtualAddress, uint32 value);
extern uint32 get_msr(void);
extern uint32 set_msr(uint32 value);

extern void set_ibat0(struct block_address_translation *bat);
extern void set_ibat1(struct block_address_translation *bat);
extern void set_ibat2(struct block_address_translation *bat);
extern void set_ibat3(struct block_address_translation *bat);
extern void set_dbat0(struct block_address_translation *bat);
extern void set_dbat1(struct block_address_translation *bat);
extern void set_dbat2(struct block_address_translation *bat);
extern void set_dbat3(struct block_address_translation *bat);

extern void get_ibat0(struct block_address_translation *bat);
extern void get_ibat1(struct block_address_translation *bat);
extern void get_ibat2(struct block_address_translation *bat);
extern void get_ibat3(struct block_address_translation *bat);
extern void get_dbat0(struct block_address_translation *bat);
extern void get_dbat1(struct block_address_translation *bat);
extern void get_dbat2(struct block_address_translation *bat);
extern void get_dbat3(struct block_address_translation *bat);

extern void reset_ibats(void);
extern void reset_dbats(void);

//extern void sethid0(unsigned int val);
//extern unsigned int getl2cr(void);
//extern void setl2cr(unsigned int val);
extern long long get_time_base(void);

extern void ppc_context_switch(void **_oldStackPointer, void *newStackPointer);

#ifdef __cplusplus
}
#endif

#define eieio()	asm volatile("eieio")
#define isync() asm volatile("isync")
#define tlbsync() asm volatile("tlbsync")
#define ppc_sync() asm volatile("sync")
#define tlbia() asm volatile("tlbia")
#define tlbie(addr) asm volatile("tlbie %0" :: "r" (addr))

#endif	/* _KERNEL_ARCH_PPC_CPU_H */
