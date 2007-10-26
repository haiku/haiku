/*
** Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/
#ifndef _KERNEL_ARCH_M68K_CPU_H
#define _KERNEL_ARCH_M68K_CPU_H


#include <arch/m68k/arch_thread_types.h>
#include <kernel.h>


#define PAGE_SIZE 4096

#warning M68K: check for missing regs/movem
struct iframe {
	/* XXX: order depends on movem */
	uint32 d0;
	uint32 d1;
	uint32 d2;
	uint32 d3;
	uint32 d4;
	uint32 d5;
	uint32 d6;
	uint32 d7;
	uint32 a0;
	uint32 a1;
	uint32 a2;
	uint32 a3;
	uint32 a4;
	uint32 a5;
	uint32 a6;
	uint32 a7;
	/* 030 ex frame: */
	uint16 sr; /* contains ccr */
	uint32 pc;
	uint16 vector; /* [12:15] frame type */
	/* other stuff depending on frame type... do we really need that ? */
	union {
		struct {
			uint32 inst;
		} format2 _PACKED;
		struct {
			uint32 inst;
			uint16 intregs[4];
		} format9 _PACKED;
		struct {
			uint16 intregs[1];
			uint16 ssw;
			uint16 instpipe_c;
			uint16 instpipe_b;
			uint32 faultaddr;
			uint16 intregs2[2];
			uint32 dataout;
			uint16 intregs3[2];
		} formata _PACKED;
		struct {
			uint16 intregs[1];
			uint16 ssw;
			uint16 instpipe_c;
			uint16 instpipe_b;
			uint32 faultaddr;
			uint16 intregs2[2];
			uint32 dataout;
			uint16 intregs3[4];
			uint32 stbaddr;
			uint16 intregs4[2];
			uint32 datain;
			uint16 intregs5[3];
			uint16 intinfo;
			uint16 intregs6[18];
		} formatb _PACKED;
	};
/*	uint32 vector;
	uint32 srr0;
	uint32 srr1;
	uint32 dar;
	uint32 dsisr;
	uint32 lr;
	uint32 cr;
	uint32 xer;
	uint32 ctr;
	uint32 fpscr;
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
	double f31;
	double f30;
	double f29;
	double f28;
	double f27;
	double f26;
	double f25;
	double f24;
	double f23;
	double f22;
	double f21;
	double f20;
	double f19;
	double f18;
	double f17;
	double f16;
	double f15;
	double f14;
	double f13;
	double f12;
	double f11;
	double f10;
	double f9;
	double f8;
	double f7;
	double f6;
	double f5;
	double f4;
	double f3;
	double f2;
	double f1;
	double f0;*/
} _PACKED;

enum machine_state {
	MSR_EXCEPTIONS_ENABLED			= 1L << 15,		// EE
	MSR_PRIVILEGE_LEVEL				= 1L << 14,		// PR
	MSR_FP_AVAILABLE				= 1L << 13,		// FP
	MSR_MACHINE_CHECK_ENABLED		= 1L << 12,		// ME
	MSR_EXCEPTION_PREFIX			= 1L << 6,		// IP
	MSR_INST_ADDRESS_TRANSLATION	= 1L << 5,		// IR
	MSR_DATA_ADDRESS_TRANSLATION	= 1L << 4,		// DR
};

//struct block_address_translation;

typedef struct arch_cpu_info {
	int null;
} arch_cpu_info;


#ifdef __cplusplus
extern "C" {
#endif

#if 0
//PPC stuff
extern uint32 get_sdr1(void);
extern void set_sdr1(uint32 value);
extern uint32 get_sr(void *virtualAddress);
extern void set_sr(void *virtualAddress, uint32 value);
extern uint32 get_msr(void);
extern uint32 set_msr(uint32 value);
extern uint32 get_pvr(void);

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
#endif

//extern void sethid0(unsigned int val);
//extern unsigned int getl2cr(void);
//extern void setl2cr(unsigned int val);
extern long long get_time_base(void);

void __m68k_setup_system_time(vint32 *cvFactor);
	// defined in libroot: os/arch/system_time.c
int64 __m68k_get_time_base(void);
	// defined in libroot: os/arch/system_time_asm.S

extern void m68k_context_switch(void **_oldStackPointer, void *newStackPointer);

extern bool m68k_set_fault_handler(addr_t *handlerLocation, addr_t handler)
	__attribute__((noinline));

#ifdef __cplusplus
}
#endif

/* flushes insn pipeline */
#define m68k_nop() asm volatile("nop")
/* no FC bit needed */
#define pflush(addr) asm volatile("pflush #0,#0,(%0)" :: "a" (addr))
#define pflusha() asm volatile("pflusha")

//#define

#if 0
#define eieio()	asm volatile("eieio")
#define isync() asm volatile("isync")
#define tlbsync() asm volatile("tlbsync")
#define ppc_sync() asm volatile("sync")
#define tlbia() asm volatile("tlbia")
#define tlbie(addr) asm volatile("tlbie %0" :: "r" (addr))


// PowerPC processor version (the upper 16 bits of the PVR).
enum m68k_processor_version {
	MPC601		= 0x0001,
	MPC603		= 0x0003,
	MPC604		= 0x0004,
	MPC602		= 0x0005,
	MPC603e		= 0x0006,
	MPC603ev	= 0x0007,
	MPC750		= 0x0008,
	MPC604ev	= 0x0009,
	MPC7400		= 0x000c,
	MPC620		= 0x0014,
	IBM403		= 0x0020,
	IBM401A1	= 0x0021,
	IBM401B2	= 0x0022,
	IBM401C2	= 0x0023,
	IBM401D2	= 0x0024,
	IBM401E2	= 0x0025,
	IBM401F2	= 0x0026,
	IBM401G2	= 0x0027,
	IBMPOWER3	= 0x0041,
	MPC860		= 0x0050,
	MPC8240		= 0x0081,
	IBM405GP	= 0x4011,
	IBM405L		= 0x4161,
	IBM750FX	= 0x7000,
	MPC7450		= 0x8000,
	MPC7455		= 0x8001,
	MPC7457		= 0x8002,
	MPC7447A	= 0x8003,
	MPC7448		= 0x8004,
	MPC7410		= 0x800c,
	MPC8245		= 0x8081,
};
#endif


/*
	Use of (some) special purpose registers.

	SRP[63-32]: current struct thread*
	SRP[31-0] :
	CAAR      : can we use it ??

	PPC:
	SPRG0: per CPU physical address pointer to an ppc_cpu_exception_context
	       structure
	SPRG1: scratch
	SPRG2: current struct thread*
	SPRG3: TLS base pointer (only for userland threads)
*/

#endif	/* _KERNEL_ARCH_PPC_CPU_H */
