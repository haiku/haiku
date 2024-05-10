/*
 * Copyright (c) 2018-2021 Maxime Villard, m00nbsd.net
 * All rights reserved.
 *
 * This code is part of the NVMM hypervisor.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _NVMM_X86_H_
#define _NVMM_X86_H_

#if defined(__NetBSD__)
#include <x86/specialreg.h>
#elif defined(__DragonFly__)
#include <machine/specialreg.h>
#endif

/* -------------------------------------------------------------------------- */

#ifndef ASM_NVMM

struct nvmm_x86_exit_memory {
	int prot;
	gpaddr_t gpa;
	uint8_t inst_len;
	uint8_t inst_bytes[15];
};

struct nvmm_x86_exit_io {
	bool in;
	uint16_t port;
	int8_t seg;
	uint8_t address_size;
	uint8_t operand_size;
	bool rep;
	bool str;
	uint64_t npc;
};

struct nvmm_x86_exit_rdmsr {
	uint32_t msr;
	uint64_t npc;
};

struct nvmm_x86_exit_wrmsr {
	uint32_t msr;
	uint64_t val;
	uint64_t npc;
};

struct nvmm_x86_exit_insn {
	uint64_t npc;
};

struct nvmm_x86_exit_invalid {
	uint64_t hwcode;
};

/* Generic. */
#define NVMM_VCPU_EXIT_NONE		0x0000000000000000ULL
#define NVMM_VCPU_EXIT_INVALID		0xFFFFFFFFFFFFFFFFULL
/* x86: operations. */
#define NVMM_VCPU_EXIT_MEMORY		0x0000000000000001ULL
#define NVMM_VCPU_EXIT_IO		0x0000000000000002ULL
/* x86: changes in VCPU state. */
#define NVMM_VCPU_EXIT_SHUTDOWN		0x0000000000001000ULL
#define NVMM_VCPU_EXIT_INT_READY	0x0000000000001001ULL
#define NVMM_VCPU_EXIT_NMI_READY	0x0000000000001002ULL
#define NVMM_VCPU_EXIT_HALTED		0x0000000000001003ULL
#define NVMM_VCPU_EXIT_TPR_CHANGED	0x0000000000001004ULL
/* x86: instructions. */
#define NVMM_VCPU_EXIT_RDMSR		0x0000000000002000ULL
#define NVMM_VCPU_EXIT_WRMSR		0x0000000000002001ULL
#define NVMM_VCPU_EXIT_MONITOR		0x0000000000002002ULL
#define NVMM_VCPU_EXIT_MWAIT		0x0000000000002003ULL
#define NVMM_VCPU_EXIT_CPUID		0x0000000000002004ULL

struct nvmm_x86_exit {
	uint64_t reason;
	union {
		struct nvmm_x86_exit_memory mem;
		struct nvmm_x86_exit_io io;
		struct nvmm_x86_exit_rdmsr rdmsr;
		struct nvmm_x86_exit_wrmsr wrmsr;
		struct nvmm_x86_exit_insn insn;
		struct nvmm_x86_exit_invalid inv;
	} u;
	struct {
		uint64_t rflags;
		uint64_t cr8;
		uint64_t int_shadow:1;
		uint64_t int_window_exiting:1;
		uint64_t nmi_window_exiting:1;
		uint64_t evt_pending:1;
		uint64_t rsvd:60;
	} exitstate;
};
#define nvmm_vcpu_exit		nvmm_x86_exit

#define NVMM_VCPU_EVENT_EXCP	0
#define NVMM_VCPU_EVENT_INTR	1

struct nvmm_x86_event {
	u_int type;
	uint8_t vector;
	union {
		struct {
			uint64_t error;
		} excp;
	} u;
};
#define nvmm_vcpu_event		nvmm_x86_event

struct nvmm_cap_md {
	uint64_t mach_conf_support;

	uint64_t vcpu_conf_support;
#define NVMM_CAP_ARCH_VCPU_CONF_CPUID	__BIT(0)
#define NVMM_CAP_ARCH_VCPU_CONF_TPR	__BIT(1)

	uint64_t xcr0_mask;
	uint32_t mxcsr_mask;
	uint32_t conf_cpuid_maxops;
	uint64_t rsvd[6];
};

#endif /* ASM_NVMM */

/* -------------------------------------------------------------------------- */

/*
 * State indexes. We use X64 as naming convention, not to confuse with X86
 * which originally implied 32bit.
 */

/* Segments. */
#define NVMM_X64_SEG_ES			0
#define NVMM_X64_SEG_CS			1
#define NVMM_X64_SEG_SS			2
#define NVMM_X64_SEG_DS			3
#define NVMM_X64_SEG_FS			4
#define NVMM_X64_SEG_GS			5
#define NVMM_X64_SEG_GDT		6
#define NVMM_X64_SEG_IDT		7
#define NVMM_X64_SEG_LDT		8
#define NVMM_X64_SEG_TR			9
#define NVMM_X64_NSEG			10

/* General Purpose Registers. */
#define NVMM_X64_GPR_RAX		0
#define NVMM_X64_GPR_RCX		1
#define NVMM_X64_GPR_RDX		2
#define NVMM_X64_GPR_RBX		3
#define NVMM_X64_GPR_RSP		4
#define NVMM_X64_GPR_RBP		5
#define NVMM_X64_GPR_RSI		6
#define NVMM_X64_GPR_RDI		7
#define NVMM_X64_GPR_R8			8
#define NVMM_X64_GPR_R9			9
#define NVMM_X64_GPR_R10		10
#define NVMM_X64_GPR_R11		11
#define NVMM_X64_GPR_R12		12
#define NVMM_X64_GPR_R13		13
#define NVMM_X64_GPR_R14		14
#define NVMM_X64_GPR_R15		15
#define NVMM_X64_GPR_RIP		16
#define NVMM_X64_GPR_RFLAGS		17
#define NVMM_X64_NGPR			18

/* Control Registers. */
#define NVMM_X64_CR_CR0			0
#define NVMM_X64_CR_CR2			1
#define NVMM_X64_CR_CR3			2
#define NVMM_X64_CR_CR4			3
#define NVMM_X64_CR_CR8			4
#define NVMM_X64_CR_XCR0		5
#define NVMM_X64_NCR			6

/* Debug Registers. */
#define NVMM_X64_DR_DR0			0
#define NVMM_X64_DR_DR1			1
#define NVMM_X64_DR_DR2			2
#define NVMM_X64_DR_DR3			3
#define NVMM_X64_DR_DR6			4
#define NVMM_X64_DR_DR7			5
#define NVMM_X64_NDR			6

/* MSRs. */
#define NVMM_X64_MSR_EFER		0
#define NVMM_X64_MSR_STAR		1
#define NVMM_X64_MSR_LSTAR		2
#define NVMM_X64_MSR_CSTAR		3
#define NVMM_X64_MSR_SFMASK		4
#define NVMM_X64_MSR_KERNELGSBASE	5
#define NVMM_X64_MSR_SYSENTER_CS	6
#define NVMM_X64_MSR_SYSENTER_ESP	7
#define NVMM_X64_MSR_SYSENTER_EIP	8
#define NVMM_X64_MSR_PAT		9
#define NVMM_X64_MSR_TSC		10
#define NVMM_X64_NMSR			11

#ifndef ASM_NVMM

#include <sys/types.h>
#include <sys/bitops.h>
#if defined(__DragonFly__)
#ifdef __x86_64__
#undef  __BIT
#define __BIT(__n)		__BIT64(__n)
#undef  __BITS
#define __BITS(__m, __n)	__BITS64(__m, __n)
#endif /* __x86_64__ */
#endif

/* Segment state. */
struct nvmm_x64_state_seg {
	uint16_t selector;
	struct {		/* hidden */
		uint16_t type:4;
		uint16_t s:1;
		uint16_t dpl:2;
		uint16_t p:1;
		uint16_t avl:1;
		uint16_t l:1;
		uint16_t def:1;
		uint16_t g:1;
		uint16_t rsvd:4;
	} attrib;
	uint32_t limit;		/* hidden */
	uint64_t base;		/* hidden */
};

/* Interrupt state. */
struct nvmm_x64_state_intr {
	uint64_t int_shadow:1;
	uint64_t int_window_exiting:1;
	uint64_t nmi_window_exiting:1;
	uint64_t evt_pending:1;
	uint64_t rsvd:60;
};

/* FPU state structures. */
union nvmm_x64_state_fpu_addr {
	uint64_t fa_64;
	struct {
		uint32_t fa_off;
		uint16_t fa_seg;
		uint16_t fa_opcode;
	} fa_32;
};
CTASSERT(sizeof(union nvmm_x64_state_fpu_addr) == 8);
struct nvmm_x64_state_fpu_mmreg {
	uint64_t mm_significand;
	uint16_t mm_exp_sign;
	uint8_t  mm_rsvd[6];
};
CTASSERT(sizeof(struct nvmm_x64_state_fpu_mmreg) == 16);
struct nvmm_x64_state_fpu_xmmreg {
	uint8_t xmm_bytes[16];
};
CTASSERT(sizeof(struct nvmm_x64_state_fpu_xmmreg) == 16);

/* FPU state (x87 + SSE). */
struct nvmm_x64_state_fpu {
	uint16_t fx_cw;		/* Control Word */
	uint16_t fx_sw;		/* Status Word */
	uint8_t fx_tw;		/* Tag Word */
	uint8_t fx_zero;
	uint16_t fx_opcode;
	union nvmm_x64_state_fpu_addr fx_ip;	/* Instruction Pointer */
	union nvmm_x64_state_fpu_addr fx_dp;	/* Data pointer */
	uint32_t fx_mxcsr;
	uint32_t fx_mxcsr_mask;
	struct nvmm_x64_state_fpu_mmreg fx_87_ac[8];	/* x87 registers */
	struct nvmm_x64_state_fpu_xmmreg fx_xmm[16];	/* XMM registers */
	uint8_t fx_rsvd[96];
} __aligned(16);
CTASSERT(sizeof(struct nvmm_x64_state_fpu) == 512);

/* Flags. */
#define NVMM_X64_STATE_SEGS	0x01
#define NVMM_X64_STATE_GPRS	0x02
#define NVMM_X64_STATE_CRS	0x04
#define NVMM_X64_STATE_DRS	0x08
#define NVMM_X64_STATE_MSRS	0x10
#define NVMM_X64_STATE_INTR	0x20
#define NVMM_X64_STATE_FPU	0x40
#define NVMM_X64_STATE_ALL	\
	(NVMM_X64_STATE_SEGS | NVMM_X64_STATE_GPRS | NVMM_X64_STATE_CRS | \
	 NVMM_X64_STATE_DRS | NVMM_X64_STATE_MSRS | NVMM_X64_STATE_INTR | \
	 NVMM_X64_STATE_FPU)

struct nvmm_x64_state {
	struct nvmm_x64_state_seg segs[NVMM_X64_NSEG];
	uint64_t gprs[NVMM_X64_NGPR];
	uint64_t crs[NVMM_X64_NCR];
	uint64_t drs[NVMM_X64_NDR];
	uint64_t msrs[NVMM_X64_NMSR];
	struct nvmm_x64_state_intr intr;
	struct nvmm_x64_state_fpu fpu;
};
#define nvmm_vcpu_state		nvmm_x64_state

/* -------------------------------------------------------------------------- */

#define NVMM_VCPU_CONF_CPUID	NVMM_VCPU_CONF_MD_BEGIN
#define NVMM_VCPU_CONF_TPR	(NVMM_VCPU_CONF_MD_BEGIN + 1)

struct nvmm_vcpu_conf_cpuid {
	/* The options. */
	uint32_t mask:1;
	uint32_t exit:1;
	uint32_t rsvd:30;

	/* The leaf. */
	uint32_t leaf;

	/* The params. */
	union {
		struct {
			struct {
				uint32_t eax;
				uint32_t ebx;
				uint32_t ecx;
				uint32_t edx;
			} set;
			struct {
				uint32_t eax;
				uint32_t ebx;
				uint32_t ecx;
				uint32_t edx;
			} del;
		} mask;
	} u;
};

struct nvmm_vcpu_conf_tpr {
	uint32_t exit_changed:1;
	uint32_t rsvd:31;
};

/* -------------------------------------------------------------------------- */

/*
 * CPUID defines.
 */

/* Fn0000_0001:EBX */
#define CPUID_0_01_EBX_BRAND_INDEX	__BITS(7,0)
#define CPUID_0_01_EBX_CLFLUSH_SIZE	__BITS(15,8)
#define CPUID_0_01_EBX_HTT_CORES	__BITS(23,16)
#define CPUID_0_01_EBX_LOCAL_APIC_ID	__BITS(31,24)
/* Fn0000_0001:ECX */
#define CPUID_0_01_ECX_SSE3		__BIT(0)
#define CPUID_0_01_ECX_PCLMULQDQ	__BIT(1)
#define CPUID_0_01_ECX_DTES64		__BIT(2)
#define CPUID_0_01_ECX_MONITOR		__BIT(3)
#define CPUID_0_01_ECX_DS_CPL		__BIT(4)
#define CPUID_0_01_ECX_VMX		__BIT(5)
#define CPUID_0_01_ECX_SMX		__BIT(6)
#define CPUID_0_01_ECX_EIST		__BIT(7)
#define CPUID_0_01_ECX_TM2		__BIT(8)
#define CPUID_0_01_ECX_SSSE3		__BIT(9)
#define CPUID_0_01_ECX_CNXTID		__BIT(10)
#define CPUID_0_01_ECX_SDBG		__BIT(11)
#define CPUID_0_01_ECX_FMA		__BIT(12)
#define CPUID_0_01_ECX_CX16		__BIT(13)
#define CPUID_0_01_ECX_XTPR		__BIT(14)
#define CPUID_0_01_ECX_PDCM		__BIT(15)
#define CPUID_0_01_ECX_PCID		__BIT(17)
#define CPUID_0_01_ECX_DCA		__BIT(18)
#define CPUID_0_01_ECX_SSE41		__BIT(19)
#define CPUID_0_01_ECX_SSE42		__BIT(20)
#define CPUID_0_01_ECX_X2APIC		__BIT(21)
#define CPUID_0_01_ECX_MOVBE		__BIT(22)
#define CPUID_0_01_ECX_POPCNT		__BIT(23)
#define CPUID_0_01_ECX_TSC_DEADLINE	__BIT(24)
#define CPUID_0_01_ECX_AESNI		__BIT(25)
#define CPUID_0_01_ECX_XSAVE		__BIT(26)
#define CPUID_0_01_ECX_OSXSAVE		__BIT(27)
#define CPUID_0_01_ECX_AVX		__BIT(28)
#define CPUID_0_01_ECX_F16C		__BIT(29)
#define CPUID_0_01_ECX_RDRAND		__BIT(30)
#define CPUID_0_01_ECX_RAZ		__BIT(31)
/* Fn0000_0001:EDX */
#define CPUID_0_01_EDX_FPU		__BIT(0)
#define CPUID_0_01_EDX_VME		__BIT(1)
#define CPUID_0_01_EDX_DE		__BIT(2)
#define CPUID_0_01_EDX_PSE		__BIT(3)
#define CPUID_0_01_EDX_TSC		__BIT(4)
#define CPUID_0_01_EDX_MSR		__BIT(5)
#define CPUID_0_01_EDX_PAE		__BIT(6)
#define CPUID_0_01_EDX_MCE		__BIT(7)
#define CPUID_0_01_EDX_CX8		__BIT(8)
#define CPUID_0_01_EDX_APIC		__BIT(9)
#define CPUID_0_01_EDX_SEP		__BIT(11)
#define CPUID_0_01_EDX_MTRR		__BIT(12)
#define CPUID_0_01_EDX_PGE		__BIT(13)
#define CPUID_0_01_EDX_MCA		__BIT(14)
#define CPUID_0_01_EDX_CMOV		__BIT(15)
#define CPUID_0_01_EDX_PAT		__BIT(16)
#define CPUID_0_01_EDX_PSE36		__BIT(17)
#define CPUID_0_01_EDX_PSN		__BIT(18)
#define CPUID_0_01_EDX_CLFSH		__BIT(19)
#define CPUID_0_01_EDX_DS		__BIT(21)
#define CPUID_0_01_EDX_ACPI		__BIT(22)
#define CPUID_0_01_EDX_MMX		__BIT(23)
#define CPUID_0_01_EDX_FXSR		__BIT(24)
#define CPUID_0_01_EDX_SSE		__BIT(25)
#define CPUID_0_01_EDX_SSE2		__BIT(26)
#define CPUID_0_01_EDX_SS		__BIT(27)
#define CPUID_0_01_EDX_HTT		__BIT(28)
#define CPUID_0_01_EDX_TM		__BIT(29)
#define CPUID_0_01_EDX_PBE		__BIT(31)

/* Fn0000_0004:EAX (Intel Deterministic Cache Parameter Leaf) */
#define CPUID_0_04_EAX_CACHETYPE	__BITS(4, 0)
#define CPUID_0_04_EAX_CACHETYPE_NULL		0
#define CPUID_0_04_EAX_CACHETYPE_DATA		1
#define CPUID_0_04_EAX_CACHETYPE_INSN		2
#define CPUID_0_04_EAX_CACHETYPE_UNIFIED	3
#define CPUID_0_04_EAX_CACHELEVEL	__BITS(7, 5)
#define CPUID_0_04_EAX_SELFINITCL	__BIT(8)
#define CPUID_0_04_EAX_FULLASSOC	__BIT(9)
#define CPUID_0_04_EAX_SHARING		__BITS(25, 14)
#define CPUID_0_04_EAX_CORE_P_PKG	__BITS(31, 26)

/* [ECX=0] Fn0000_0007:EBX (Structured Extended Features) */
#define CPUID_0_07_EBX_FSGSBASE		__BIT(0)
#define CPUID_0_07_EBX_TSC_ADJUST	__BIT(1)
#define CPUID_0_07_EBX_SGX		__BIT(2)
#define CPUID_0_07_EBX_BMI1		__BIT(3)
#define CPUID_0_07_EBX_HLE		__BIT(4)
#define CPUID_0_07_EBX_AVX2		__BIT(5)
#define CPUID_0_07_EBX_FDPEXONLY	__BIT(6)
#define CPUID_0_07_EBX_SMEP		__BIT(7)
#define CPUID_0_07_EBX_BMI2		__BIT(8)
#define CPUID_0_07_EBX_ERMS		__BIT(9)
#define CPUID_0_07_EBX_INVPCID		__BIT(10)
#define CPUID_0_07_EBX_RTM		__BIT(11)
#define CPUID_0_07_EBX_QM		__BIT(12)
#define CPUID_0_07_EBX_FPUCSDS		__BIT(13)
#define CPUID_0_07_EBX_MPX		__BIT(14)
#define CPUID_0_07_EBX_PQE		__BIT(15)
#define CPUID_0_07_EBX_AVX512F		__BIT(16)
#define CPUID_0_07_EBX_AVX512DQ		__BIT(17)
#define CPUID_0_07_EBX_RDSEED		__BIT(18)
#define CPUID_0_07_EBX_ADX		__BIT(19)
#define CPUID_0_07_EBX_SMAP		__BIT(20)
#define CPUID_0_07_EBX_AVX512_IFMA	__BIT(21)
#define CPUID_0_07_EBX_CLFLUSHOPT	__BIT(23)
#define CPUID_0_07_EBX_CLWB		__BIT(24)
#define CPUID_0_07_EBX_PT		__BIT(25)
#define CPUID_0_07_EBX_AVX512PF		__BIT(26)
#define CPUID_0_07_EBX_AVX512ER		__BIT(27)
#define CPUID_0_07_EBX_AVX512CD		__BIT(28)
#define CPUID_0_07_EBX_SHA		__BIT(29)
#define CPUID_0_07_EBX_AVX512BW		__BIT(30)
#define CPUID_0_07_EBX_AVX512VL		__BIT(31)
/* [ECX=0] Fn0000_0007:ECX (Structured Extended Features) */
#define CPUID_0_07_ECX_PREFETCHWT1	__BIT(0)
#define CPUID_0_07_ECX_AVX512_VBMI	__BIT(1)
#define CPUID_0_07_ECX_UMIP		__BIT(2)
#define CPUID_0_07_ECX_PKU		__BIT(3)
#define CPUID_0_07_ECX_OSPKE		__BIT(4)
#define CPUID_0_07_ECX_WAITPKG		__BIT(5)
#define CPUID_0_07_ECX_AVX512_VBMI2	__BIT(6)
#define CPUID_0_07_ECX_CET_SS		__BIT(7)
#define CPUID_0_07_ECX_GFNI		__BIT(8)
#define CPUID_0_07_ECX_VAES		__BIT(9)
#define CPUID_0_07_ECX_VPCLMULQDQ	__BIT(10)
#define CPUID_0_07_ECX_AVX512_VNNI	__BIT(11)
#define CPUID_0_07_ECX_AVX512_BITALG	__BIT(12)
#define CPUID_0_07_ECX_AVX512_VPOPCNTDQ __BIT(14)
#define CPUID_0_07_ECX_LA57		__BIT(16)
#define CPUID_0_07_ECX_MAWAU		__BITS(21, 17)
#define CPUID_0_07_ECX_RDPID		__BIT(22)
#define CPUID_0_07_ECX_KL		__BIT(23)
#define CPUID_0_07_ECX_CLDEMOTE		__BIT(25)
#define CPUID_0_07_ECX_MOVDIRI		__BIT(27)
#define CPUID_0_07_ECX_MOVDIR64B	__BIT(28)
#define CPUID_0_07_ECX_SGXLC		__BIT(30)
#define CPUID_0_07_ECX_PKS		__BIT(31)
/* [ECX=0] Fn0000_0007:EDX (Structured Extended Features) */
#define CPUID_0_07_EDX_AVX512_4VNNIW	__BIT(2)
#define CPUID_0_07_EDX_AVX512_4FMAPS	__BIT(3)
#define CPUID_0_07_EDX_FSREP_MOV	__BIT(4)
#define CPUID_0_07_EDX_AVX512_VP2INTERSECT __BIT(8)
#define CPUID_0_07_EDX_SRBDS_CTRL	__BIT(9)
#define CPUID_0_07_EDX_MD_CLEAR		__BIT(10)
#define CPUID_0_07_EDX_TSX_FORCE_ABORT	__BIT(13)
#define CPUID_0_07_EDX_SERIALIZE	__BIT(14)
#define CPUID_0_07_EDX_HYBRID		__BIT(15)
#define CPUID_0_07_EDX_TSXLDTRK		__BIT(16)
#define CPUID_0_07_EDX_CET_IBT		__BIT(20)
#define CPUID_0_07_EDX_IBRS		__BIT(26)
#define CPUID_0_07_EDX_STIBP		__BIT(27)
#define CPUID_0_07_EDX_L1D_FLUSH	__BIT(28)
#define CPUID_0_07_EDX_ARCH_CAP		__BIT(29)
#define CPUID_0_07_EDX_CORE_CAP		__BIT(30)
#define CPUID_0_07_EDX_SSBD		__BIT(31)

/* Fn0000_000B:EAX (Extended Topology Enumeration) */
#define CPUID_0_0B_EAX_SHIFTNUM		__BITS(4, 0)
/* Fn0000_000B:ECX (Extended Topology Enumeration) */
#define CPUID_0_0B_ECX_LVLNUM		__BITS(7, 0)
#define CPUID_0_0B_ECX_LVLTYPE		__BITS(15, 8)
#define CPUID_0_0B_ECX_LVLTYPE_INVAL		0
#define CPUID_0_0B_ECX_LVLTYPE_SMT		1
#define CPUID_0_0B_ECX_LVLTYPE_CORE		2

/* [ECX=1] Fn0000_000D:EAX (Processor Extended State Enumeration) */
#define CPUID_0_0D_ECX1_EAX_XSAVEOPT	__BIT(0)
#define CPUID_0_0D_ECX1_EAX_XSAVEC	__BIT(1)
#define CPUID_0_0D_ECX1_EAX_XGETBV	__BIT(2)
#define CPUID_0_0D_ECX1_EAX_XSAVES	__BIT(3)

/* Fn8000_0001:ECX */
#define CPUID_8_01_ECX_LAHF		__BIT(0)
#define CPUID_8_01_ECX_CMPLEGACY	__BIT(1)
#define CPUID_8_01_ECX_SVM		__BIT(2)
#define CPUID_8_01_ECX_EAPIC		__BIT(3)
#define CPUID_8_01_ECX_ALTMOVCR8	__BIT(4)
#define CPUID_8_01_ECX_ABM		__BIT(5)
#define CPUID_8_01_ECX_SSE4A		__BIT(6)
#define CPUID_8_01_ECX_MISALIGNSSE	__BIT(7)
#define CPUID_8_01_ECX_3DNOWPF		__BIT(8)
#define CPUID_8_01_ECX_OSVW		__BIT(9)
#define CPUID_8_01_ECX_IBS		__BIT(10)
#define CPUID_8_01_ECX_XOP		__BIT(11)
#define CPUID_8_01_ECX_SKINIT		__BIT(12)
#define CPUID_8_01_ECX_WDT		__BIT(13)
#define CPUID_8_01_ECX_LWP		__BIT(15)
#define CPUID_8_01_ECX_FMA4		__BIT(16)
#define CPUID_8_01_ECX_TCE		__BIT(17)
#define CPUID_8_01_ECX_NODEID		__BIT(19)
#define CPUID_8_01_ECX_TBM		__BIT(21)
#define CPUID_8_01_ECX_TOPOEXT		__BIT(22)
#define CPUID_8_01_ECX_PCEC		__BIT(23)
#define CPUID_8_01_ECX_PCENB		__BIT(24)
#define CPUID_8_01_ECX_DBE		__BIT(26)
#define CPUID_8_01_ECX_PERFTSC		__BIT(27)
#define CPUID_8_01_ECX_PERFEXTLLC	__BIT(28)
#define CPUID_8_01_ECX_MWAITX		__BIT(29)
/* Fn8000_0001:EDX */
#define CPUID_8_01_EDX_FPU		__BIT(0)
#define CPUID_8_01_EDX_VME		__BIT(1)
#define CPUID_8_01_EDX_DE		__BIT(2)
#define CPUID_8_01_EDX_PSE		__BIT(3)
#define CPUID_8_01_EDX_TSC		__BIT(4)
#define CPUID_8_01_EDX_MSR		__BIT(5)
#define CPUID_8_01_EDX_PAE		__BIT(6)
#define CPUID_8_01_EDX_MCE		__BIT(7)
#define CPUID_8_01_EDX_CX8		__BIT(8)
#define CPUID_8_01_EDX_APIC		__BIT(9)
#define CPUID_8_01_EDX_SYSCALL		__BIT(11)
#define CPUID_8_01_EDX_MTRR		__BIT(12)
#define CPUID_8_01_EDX_PGE		__BIT(13)
#define CPUID_8_01_EDX_MCA		__BIT(14)
#define CPUID_8_01_EDX_CMOV		__BIT(15)
#define CPUID_8_01_EDX_PAT		__BIT(16)
#define CPUID_8_01_EDX_PSE36		__BIT(17)
#define CPUID_8_01_EDX_XD		__BIT(20)
#define CPUID_8_01_EDX_MMXEXT		__BIT(22)
#define CPUID_8_01_EDX_MMX		__BIT(23)
#define CPUID_8_01_EDX_FXSR		__BIT(24)
#define CPUID_8_01_EDX_FFXSR		__BIT(25)
#define CPUID_8_01_EDX_PAGE1GB		__BIT(26)
#define CPUID_8_01_EDX_RDTSCP		__BIT(27)
#define CPUID_8_01_EDX_LM		__BIT(29)
#define CPUID_8_01_EDX_3DNOWEXT		__BIT(30)
#define CPUID_8_01_EDX_3DNOW		__BIT(31)

/* Fn8000_0007:EDX (Advanced Power Management) */
#define CPUID_8_07_EDX_TS		__BIT(0)
#define CPUID_8_07_EDX_FID		__BIT(1)
#define CPUID_8_07_EDX_VID		__BIT(2)
#define CPUID_8_07_EDX_TTP		__BIT(3)
#define CPUID_8_07_EDX_TM		__BIT(4)
#define CPUID_8_07_EDX_100MHzSteps	__BIT(6)
#define CPUID_8_07_EDX_HwPstate		__BIT(7)
#define CPUID_8_07_EDX_TscInvariant	__BIT(8)
#define CPUID_8_07_EDX_CPB		__BIT(9)
#define CPUID_8_07_EDX_EffFreqRO	__BIT(10)
#define CPUID_8_07_EDX_ProcFeedbackIntf	__BIT(11)
#define CPUID_8_07_EDX_ProcPowerReport	__BIT(12)

/* Fn8000_0008:EBX */
#define CPUID_8_08_EBX_CLZERO		__BIT(0)
#define CPUID_8_08_EBX_InstRetCntMsr	__BIT(1)
#define CPUID_8_08_EBX_RstrFpErrPtrs	__BIT(2)
#define CPUID_8_08_EBX_INVLPGB		__BIT(3)
#define CPUID_8_08_EBX_RDPRU		__BIT(4)
#define CPUID_8_08_EBX_MCOMMIT		__BIT(8)
#define CPUID_8_08_EBX_WBNOINVD		__BIT(9)
#define CPUID_8_08_EBX_IBPB		__BIT(12)
#define CPUID_8_08_EBX_INT_WBINVD	__BIT(13)
#define CPUID_8_08_EBX_IBRS		__BIT(14)
#define CPUID_8_08_EBX_STIBP		__BIT(15)
#define CPUID_8_08_EBX_IBRS_ALWAYSON	__BIT(16)
#define CPUID_8_08_EBX_STIBP_ALWAYSON	__BIT(17)
#define CPUID_8_08_EBX_PREFER_IBRS	__BIT(18)
#define CPUID_8_08_EBX_EferLmsleUnsupp	__BIT(20)
#define CPUID_8_08_EBX_INVLPGBnestedPg	__BIT(21)
#define CPUID_8_08_EBX_SSBD		__BIT(24)
#define CPUID_8_08_EBX_VIRT_SSBD	__BIT(25)
#define CPUID_8_08_EBX_SSB_NO		__BIT(26)
/* Fn8000_0008:ECX */
#define CPUID_8_08_ECX_NC		__BITS(7,0)
#define CPUID_8_08_ECX_ApicIdSize	__BITS(15,12)
#define CPUID_8_08_ECX_PerfTscSize	__BITS(17,16)

/* Fn8000_000A:EAX (SVM features) */
#define CPUID_8_0A_EAX_SvmRev		__BITS(7,0)
/* Fn8000_000A:EDX (SVM features) */
#define CPUID_8_0A_EDX_NP		__BIT(0)
#define CPUID_8_0A_EDX_LbrVirt		__BIT(1)
#define CPUID_8_0A_EDX_SVML		__BIT(2)
#define CPUID_8_0A_EDX_NRIPS		__BIT(3)
#define CPUID_8_0A_EDX_TscRateMsr	__BIT(4)
#define CPUID_8_0A_EDX_VmcbClean	__BIT(5)
#define CPUID_8_0A_EDX_FlushByASID	__BIT(6)
#define CPUID_8_0A_EDX_DecodeAssists	__BIT(7)
#define CPUID_8_0A_EDX_PauseFilter	__BIT(10)
#define CPUID_8_0A_EDX_PFThreshold	__BIT(12)
#define CPUID_8_0A_EDX_AVIC		__BIT(13)
#define CPUID_8_0A_EDX_VMSAVEvirt	__BIT(15)
#define CPUID_8_0A_EDX_VGIF		__BIT(16)
#define CPUID_8_0A_EDX_GMET		__BIT(17)
#define CPUID_8_0A_EDX_SSSCheck		__BIT(19)
#define CPUID_8_0A_EDX_SpecCtrl		__BIT(20)
#define CPUID_8_0A_EDX_TlbiCtl		__BIT(24)

/* -------------------------------------------------------------------------- */

/*
 * Register defines. We mainly rely on the already-existing OS definitions.
 */

#if defined(__DragonFly__)

#define XCR0_X87		CPU_XFEATURE_X87	/* 0x00000001 */
#define XCR0_SSE		CPU_XFEATURE_SSE	/* 0x00000002 */

#define MSR_MISC_ENABLE		MSR_IA32_MISC_ENABLE	/* 0x1a0 */
#define MSR_CR_PAT		MSR_PAT			/* 0x277 */
#define MSR_SFMASK		MSR_SF_MASK		/* 0xc0000084 */
#define MSR_KERNELGSBASE	MSR_KGSBASE		/* 0xc0000102 */
#define MSR_NB_CFG		MSR_AMD_NB_CFG		/* 0xc001001f */
#define MSR_IC_CFG		MSR_AMD_IC_CFG		/* 0xc0011021 */
#define MSR_DE_CFG		MSR_AMD_DE_CFG		/* 0xc0011029 */
#define MSR_UCODE_AMD_PATCHLEVEL MSR_AMD_PATCH_LEVEL	/* 0x0000008b */

/* MSR_IA32_ARCH_CAPABILITIES (0x10a) */
#define 	IA32_ARCH_RDCL_NO	IA32_ARCH_CAP_RDCL_NO
#define 	IA32_ARCH_IBRS_ALL	IA32_ARCH_CAP_IBRS_ALL
#define 	IA32_ARCH_RSBA		IA32_ARCH_CAP_RSBA
#define 	IA32_ARCH_SKIP_L1DFL_VMENTRY	IA32_ARCH_CAP_SKIP_L1DFL_VMENTRY
#define 	IA32_ARCH_SSB_NO	IA32_ARCH_CAP_SSB_NO
#define 	IA32_ARCH_MDS_NO	IA32_ARCH_CAP_MDS_NO
#define 	IA32_ARCH_IF_PSCHANGE_MC_NO	IA32_ARCH_CAP_IF_PSCHANGE_MC_NO
#define 	IA32_ARCH_TSX_CTRL	IA32_ARCH_CAP_TSX_CTRL
#define 	IA32_ARCH_TAA_NO	IA32_ARCH_CAP_TAA_NO

/* MSR_IA32_FLUSH_CMD (0x10b) */
#define 	IA32_FLUSH_CMD_L1D_FLUSH	IA32_FLUSH_CMD_L1D

#endif /* __DragonFly__ */

/* -------------------------------------------------------------------------- */

#ifdef _KERNEL
#define NVMM_X86_MACH_NCONF	0
#define NVMM_X86_VCPU_NCONF	2

struct nvmm_x86_cpuid_mask {
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
};

/* FPU area + XSAVE header. */
struct nvmm_x86_xsave {
	struct nvmm_x64_state_fpu fpu;
	uint64_t xstate_bv;
	uint64_t xcomp_bv;
	uint8_t rsvd0[8];
	uint8_t rsvd[40];
};
CTASSERT(sizeof(struct nvmm_x86_xsave) == 512 + 64);

extern const struct nvmm_x64_state nvmm_x86_reset_state;
extern const struct nvmm_x86_cpuid_mask nvmm_cpuid_00000001;
extern const struct nvmm_x86_cpuid_mask nvmm_cpuid_00000007;
extern const struct nvmm_x86_cpuid_mask nvmm_cpuid_80000001;
extern const struct nvmm_x86_cpuid_mask nvmm_cpuid_80000007;
extern const struct nvmm_x86_cpuid_mask nvmm_cpuid_80000008;

bool nvmm_x86_pat_validate(uint64_t);
uint32_t nvmm_x86_xsave_size(uint64_t);

/* -------------------------------------------------------------------------- */

/*
 * ASM defines. We mainly rely on the already-existing OS definitions.
 */

#if defined(__NetBSD__)
#include <x86/cpufunc.h>
#include <x86/fpu.h>
#elif defined(__DragonFly__)
#include <machine/cpufunc.h>
#include <machine/npx.h>
#endif

/* CPUID. */
typedef struct {
	uint32_t eax, ebx, ecx, edx;
} cpuid_desc_t;

#if defined(__NetBSD__)
#define x86_get_cpuid(l, d)	x86_cpuid(l, (uint32_t *)d)
#define x86_get_cpuid2(l, c, d)	x86_cpuid2(l, c, (uint32_t *)d)
#elif defined(__DragonFly__)
#define x86_get_cpuid(l, d)	do_cpuid(l, (uint32_t *)d)
#define x86_get_cpuid2(l, c, d)	cpuid_count(l, c, (uint32_t *)d)
#endif

/* Control registers. */
#if defined(__NetBSD__)
#define x86_get_cr0()		rcr0()
#define x86_get_cr2()		rcr2()
#define x86_get_cr3()		rcr3()
#define x86_get_cr4()		rcr4()
#define x86_set_cr0(v)		lcr0(v)
#define x86_set_cr2(v)		lcr2(v)
#define x86_set_cr4(v)		lcr4(v)
#elif defined(__DragonFly__)
#define x86_get_cr0()		rcr0()
#define x86_get_cr2()		rcr2()
#define x86_get_cr3()		rcr3()
#define x86_get_cr4()		rcr4()
#define x86_set_cr0(v)		load_cr0(v)
#define x86_set_cr2(v)		load_cr2(v)
#define x86_set_cr4(v)		load_cr4(v)
#endif

/* Debug registers. */
#if defined(__NetBSD__)
#include <x86/dbregs.h>
static inline void
x86_curthread_save_dbregs(uint64_t *drs __unused)
{
	x86_dbregs_save(curlwp);
}
static inline void
x86_curthread_restore_dbregs(uint64_t *drs __unused)
{
	x86_dbregs_restore(curlwp);
}
#define x86_get_dr0()		rdr0()
#define x86_get_dr1()		rdr1()
#define x86_get_dr2()		rdr2()
#define x86_get_dr3()		rdr3()
#define x86_get_dr6()		rdr6()
#define x86_get_dr7()		rdr7()
#define x86_set_dr0(v)		ldr0(v)
#define x86_set_dr1(v)		ldr1(v)
#define x86_set_dr2(v)		ldr2(v)
#define x86_set_dr3(v)		ldr3(v)
#define x86_set_dr6(v)		ldr6(v)
#define x86_set_dr7(v)		ldr7(v)
#elif defined(__DragonFly__)
#include <sys/proc.h> /* struct lwp */
static inline void
x86_curthread_save_dbregs(uint64_t *drs)
{
	struct pcb *pcb = curthread->td_lwp->lwp_thread->td_pcb;

	if (__predict_true(!(pcb->pcb_flags & PCB_DBREGS)))
		return;

	drs[NVMM_X64_DR_DR0] = rdr0();
	drs[NVMM_X64_DR_DR1] = rdr1();
	drs[NVMM_X64_DR_DR2] = rdr2();
	drs[NVMM_X64_DR_DR3] = rdr3();
	drs[NVMM_X64_DR_DR6] = rdr6();
	drs[NVMM_X64_DR_DR7] = rdr7();
}
static inline void
x86_curthread_restore_dbregs(uint64_t *drs)
{
	struct pcb *pcb = curthread->td_lwp->lwp_thread->td_pcb;

	if (__predict_true(!(pcb->pcb_flags & PCB_DBREGS)))
		return;

	load_dr0(drs[NVMM_X64_DR_DR0]);
	load_dr1(drs[NVMM_X64_DR_DR1]);
	load_dr2(drs[NVMM_X64_DR_DR2]);
	load_dr3(drs[NVMM_X64_DR_DR3]);
	load_dr6(drs[NVMM_X64_DR_DR6]);
	load_dr7(drs[NVMM_X64_DR_DR7]);
}
#define x86_get_dr0()		rdr0()
#define x86_get_dr1()		rdr1()
#define x86_get_dr2()		rdr2()
#define x86_get_dr3()		rdr3()
#define x86_get_dr6()		rdr6()
#define x86_get_dr7()		rdr7()
#define x86_set_dr0(v)		load_dr0(v)
#define x86_set_dr1(v)		load_dr1(v)
#define x86_set_dr2(v)		load_dr2(v)
#define x86_set_dr3(v)		load_dr3(v)
#define x86_set_dr6(v)		load_dr6(v)
#define x86_set_dr7(v)		load_dr7(v)
#endif

/* FPU. */
#if defined(__NetBSD__)
#define x86_curthread_save_fpu()	fpu_kern_enter()
#define x86_curthread_restore_fpu()	fpu_kern_leave()
#define x86_save_fpu(a, m)		fpu_area_save(a, m, true)
#define x86_restore_fpu(a, m)		fpu_area_restore(a, m, true)
#elif defined(__DragonFly__)
#define x86_curthread_save_fpu()	/* TODO */
#define x86_curthread_restore_fpu()	/* TODO */
#define x86_save_fpu(a, m)				\
	({						\
		fpusave((union savefpu *)(a), m);	\
		load_cr0(rcr0() | CR0_TS);		\
	})
#define x86_restore_fpu(a, m)				\
	({						\
		__asm volatile("clts" ::: "memory");	\
		fpurstor((union savefpu *)(a), m);	\
	})
#endif

/* XCRs. */
static inline uint64_t
x86_get_xcr(uint32_t xcr)
{
	uint32_t low, high;

	__asm volatile (
		"xgetbv"
		: "=a" (low), "=d" (high)
		: "c" (xcr)
	);

	return (low | ((uint64_t)high << 32));
}

static inline void
x86_set_xcr(uint32_t xcr, uint64_t val)
{
	uint32_t low, high;

	low = val;
	high = val >> 32;
	__asm volatile (
		"xsetbv"
		:
		: "a" (low), "d" (high), "c" (xcr)
		: "memory"
	);
}

#if defined(__DragonFly__)
#define x86_xsave_features	npx_xcr0_mask
#define x86_fpu_mxcsr_mask	npx_mxcsr_mask
#endif

#endif /* _KERNEL */

#endif /* ASM_NVMM */

#endif /* _NVMM_X86_H_ */
