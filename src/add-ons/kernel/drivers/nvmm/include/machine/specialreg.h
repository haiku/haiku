/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * Copyright (c) 2008 The DragonFly Project.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)specialreg.h	7.1 (Berkeley) 5/9/91
 * $FreeBSD: src/sys/amd64/include/specialreg.h,v 1.39 2007/05/31 11:26:44 des Exp $
 */

#ifndef _CPU_SPECIALREG_H_
#define _CPU_SPECIALREG_H_

/*
 * Bits in CR0 special register
 */
#define	CR0_PE	0x00000001	/* Protected mode Enable */
#define	CR0_MP	0x00000002	/* "Math" Present (NPX or NPX emulator) */
#define	CR0_EM	0x00000004	/* EMulate non-NPX coproc. (trap ESC only) */
#define	CR0_TS	0x00000008	/* Task Switched (if MP, trap ESC and WAIT) */
#define	CR0_ET	0x00000010	/* Extension Type (387 (if set) vs 287) */
#define	CR0_NE	0x00000020	/* Numeric Error enable (EX16 vs IRQ13) */
#define	CR0_WP	0x00010000	/* Write Protect (honor page protect in all modes) */
#define	CR0_AM	0x00040000	/* Alignment Mask (set to enable AC flag) */
#define	CR0_NW	0x20000000	/* Not Write-through */
#define	CR0_CD	0x40000000	/* Cache Disable */
#define	CR0_PG	0x80000000	/* PaGing enable */

/*
 * Bits in CR4 special register
 */
#define	CR4_VME		0x00000001	/* Virtual 8086 mode extensions */
#define	CR4_PVI		0x00000002	/* Protected-mode virtual interrupts */
#define	CR4_TSD		0x00000004	/* Time stamp disable */
#define	CR4_DE		0x00000008	/* Debugging extensions */
#define	CR4_PSE		0x00000010	/* Page size extensions */
#define	CR4_PAE		0x00000020	/* Physical address extension */
#define	CR4_MCE		0x00000040	/* Machine check enable */
#define	CR4_PGE		0x00000080	/* Page global enable */
#define	CR4_PCE		0x00000100	/* Performance monitoring counter enable */
#define	CR4_OSFXSR	0x00000200	/* Fast FPU save/restore used by OS */
#define	CR4_OSXMMEXCPT	0x00000400	/* Enable SIMD/MMX2 to use except 16 */
#define	CR4_UMIP	0x00000800	/* User Mode Instruction Prevention */
#define	CR4_LA57	0x00001000	/* Enable 57-bit linear address */
#define	CR4_VMXE	0x00002000	/* Enable VMX - Intel specific */
#define	CR4_SMXE	0x00004000	/* Enable SMX - Intel specific */
#define	CR4_FSGSBASE	0x00010000	/* Enable *FSBASE and *GSBASE instructions */
#define	CR4_PCIDE	0x00020000	/* Enable Process Context IDentifiers */
#define	CR4_OSXSAVE	0x00040000	/* Enable XSave (for AVX Instructions) */
#define	CR4_SMEP	0x00100000	/* Supervisor-Mode Execution Prevent */
#define	CR4_SMAP	0x00200000	/* Supervisor-Mode Access Prevent */
#define	CR4_PKE		0x00400000	/* Protection Keys Enable for user pages */
#define	CR4_CET		0x00800000	/* Enable CET */
#define	CR4_PKS		0x01000000	/* Protection Keys Enable for kern pages */

/*
 * Extended Control Register XCR0
 */
#define	CPU_XFEATURE_X87	0x00000001	/* x87 FPU/MMX state */
#define	CPU_XFEATURE_SSE	0x00000002	/* SSE state */
#define	CPU_XFEATURE_YMM	0x00000004	/* AVX-256 (YMMn registers) */
#define	CPU_XFEATURE_AVX	CPU_XFEATURE_YMM

/*
 * CPUID "features" bits
 */

/* CPUID Fn0000_0001 %edx features */
#define	CPUID_FPU	0x00000001	/* processor has an FPU? */
#define	CPUID_VME	0x00000002	/* has virtual mode (%cr4's VME/PVI) */
#define	CPUID_DE	0x00000004	/* has debugging extension */
#define	CPUID_PSE	0x00000008	/* has 4MB page size extension */
#define	CPUID_TSC	0x00000010	/* has time stamp counter */
#define	CPUID_MSR	0x00000020	/* has model specific registers */
#define	CPUID_PAE	0x00000040	/* has physical address extension */
#define	CPUID_MCE	0x00000080	/* has machine check exception */
#define	CPUID_CX8	0x00000100	/* has CMPXCHG8B instruction */
#define	CPUID_APIC	0x00000200	/* has enabled APIC */
/* Bit 10 reserved	0x00000400 */
#define	CPUID_SEP	0x00000800	/* has SYSENTER/SYSEXIT extension */
#define	CPUID_MTRR	0x00001000	/* has memory type range register */
#define	CPUID_PGE	0x00002000	/* has page global extension */
#define	CPUID_MCA	0x00004000	/* has machine check architecture */
#define	CPUID_CMOV	0x00008000	/* has CMOVcc instruction */
#define	CPUID_PAT	0x00010000	/* Page Attribute Table */
#define	CPUID_PSE36	0x00020000	/* 36-bit PSE */
#define	CPUID_PSN	0x00040000	/* Processor Serial Number */
#define	CPUID_CLFSH	0x00080000	/* CLFLUSH instruction supported */
/* Bit 20 reserved	0x00100000 */
#define	CPUID_DS	0x00200000	/* Debug Store */
#define	CPUID_ACPI	0x00400000	/* ACPI performance modulation regs */
#define	CPUID_MMX	0x00800000	/* MMX supported */
#define	CPUID_FXSR	0x01000000	/* Fast FP/MMX Save/Restore */
#define	CPUID_SSE	0x02000000	/* Streaming SIMD Extensions */
#define	CPUID_SSE2	0x04000000	/* Streaming SIMD Extensions 2 */
#define	CPUID_SS	0x08000000	/* Self-Snoop */
#define	CPUID_HTT	0x10000000	/* Hyper-Threading Technology */
#define	CPUID_TM	0x20000000	/* Thermal Monitor (TCC) */
#define	CPUID_IA64	0x40000000	/* IA64 processor emulating x86 */
#define	CPUID_PBE	0x80000000	/* Pending Break Enable */

/* CPUID Fn0000_0001 %ecx features */
#define	CPUID2_SSE3	0x00000001	/* Streaming SIMD Extensions 3 */
#define	CPUID2_PCLMULQDQ 0x00000002	/* PCLMULQDQ instructions */
#define	CPUID2_DTES64	0x00000004	/* 64-bit Debug Trace */
#define	CPUID2_MON	0x00000008	/* MONITOR/MWAIT instructions */
#define	CPUID2_DS_CPL	0x00000010	/* CPL Qualified Debug Store */
#define	CPUID2_VMX	0x00000020	/* Virtual Machine eXtensions */
#define	CPUID2_SMX	0x00000040	/* Safer Mode eXtensions */
#define	CPUID2_EST	0x00000080	/* Enhanced SpeedStep Technology */
#define	CPUID2_TM2	0x00000100	/* Thermal Monitor 2 */
#define	CPUID2_SSSE3	0x00000200	/* Supplemental SSE3 */
#define	CPUID2_CNXTID	0x00000400	/* Context ID */
#define	CPUID2_SDBG	0x00000800	/* Silicon Debug */
#define	CPUID2_FMA	0x00001000	/* Fused Multiply Add */
#define	CPUID2_CX16	0x00002000	/* CMPXCHG16B instruction */
#define	CPUID2_XTPR	0x00004000	/* Task Priority Messages disabled? */
#define	CPUID2_PDCM	0x00008000	/* Perf/Debug Capability MSR */
/* Bit 16 reserved	0x00010000 */
#define	CPUID2_PCID	0x00020000	/* Process Context ID */
#define	CPUID2_DCA	0x00040000	/* Direct Cache Access */
#define	CPUID2_SSE41	0x00080000	/* Streaming SIMD Extensions 4.1 */
#define	CPUID2_SSE42	0x00100000	/* Streaming SIMD Extensions 4.2 */
#define	CPUID2_X2APIC	0x00200000	/* xAPIC Extensions */
#define	CPUID2_MOVBE	0x00400000	/* MOVBE (move after byteswap) */
#define	CPUID2_POPCNT	0x00800000	/* POPCOUNT instruction available */
#define	CPUID2_TSCDLT	0x01000000	/* LAPIC TSC-Deadline Mode support */
#define	CPUID2_AESNI	0x02000000	/* AES Instruction Set */
#define	CPUID2_XSAVE	0x04000000	/* XSave supported by CPU */
#define	CPUID2_OSXSAVE	0x08000000	/* XSave and AVX supported by OS */
#define	CPUID2_AVX	0x10000000	/* AVX instruction set support */
#define	CPUID2_F16C	0x20000000	/* F16C (half-precision) FP support */
#define	CPUID2_RDRAND	0x40000000	/* RDRAND (hardware random number) */
#define	CPUID2_VMM	0x80000000	/* Hypervisor present */

/* CPUID Fn0000_0001 %eax info */
#define	CPUID_STEPPING		0x0000000f
#define	CPUID_MODEL		0x000000f0
#define	CPUID_FAMILY		0x00000f00
#define	CPUID_EXT_MODEL		0x000f0000
#define	CPUID_EXT_FAMILY	0x0ff00000

#define	CPUID_TO_MODEL(id) \
	((((id) & CPUID_MODEL) >> 4) | (((id) & CPUID_EXT_MODEL) >> 12))
#define	CPUID_TO_FAMILY(id) \
	((((id) & CPUID_FAMILY) >> 8) + (((id) & CPUID_EXT_FAMILY) >> 20))

/* CPUID Fn0000_0001 %ebx info */
#define	CPUID_BRAND_INDEX	0x000000ff
#define	CPUID_CLFUSH_SIZE	0x0000ff00
#define	CPUID_HTT_CORES		0x00ff0000
#define	CPUID_HTT_CORE_SHIFT	16
#define	CPUID_LOCAL_APIC_ID	0xff000000

/*
 * Intel Deterministic Cache Parameters
 * CPUID Fn0000_0004
 */
#define	FUNC_4_MAX_CORE_NO(eax)	((((eax) >> 26) & 0x3f))

/*
 * Intel/AMD MONITOR/MWAIT
 * CPUID Fn0000_0005
 */
/* %ecx */
#define	CPUID_MWAIT_EXT		0x00000001	/* MONITOR/MWAIT Extensions */
#define	CPUID_MWAIT_INTBRK	0x00000002	/* Interrupt as Break Event */
/* %edx: number of substates for specific C-state */
#define	CPUID_MWAIT_CX_SUBCNT(edx, cstate) \
	(((edx) >> ((cstate) * 4)) & 0xf)

/* MWAIT EAX to Cx and its substate */
#define	MWAIT_EAX_TO_CX(x)	((((x) >> 4) + 1) & 0xf)
#define	MWAIT_EAX_TO_CX_SUB(x)	((x) & 0xf)

/* MWAIT EAX hint and ECX extension */
#define	MWAIT_EAX_HINT(cx, sub) \
	(((((uint32_t)(cx) - 1) & 0xf) << 4) | ((sub) & 0xf))
#define	MWAIT_ECX_INTBRK	0x1

/*
 * Intel/AMD Digital Thermal Sensor and Power Management
 * CPUID Fn0000_0006
 */
/* %eax */
#define	CPUID_THERMAL_SENSOR	0x00000001	/* Digital thermal sensor */
#define	CPUID_THERMAL_TURBO	0x00000002	/* Intel Turbo boost */
#define	CPUID_THERMAL_ARAT	0x00000004	/* Always running APIC timer */
#define	CPUID_THERMAL_PLN	0x00000010	/* Power limit notification */
#define	CPUID_THERMAL_ECMD	0x00000020	/* Clock modulation extension */
#define	CPUID_THERMAL_PTM	0x00000040	/* Package thermal management */
#define	CPUID_THERMAL_HWP	0x00000080	/* Hardware P-states */
/* %ecx */
#define	CPUID_THERMAL2_SETBH	0x00000008	/* Energy performance bias */

/*
 * Intel/AMD Structured Extended Feature
 * CPUID Fn0000_0007
 * %ecx == 0: Subleaf 0
 *	%eax: The Maximum input value for supported subleaf.
 *	%ebx: Feature bits.
 *	%ecx: Feature bits.
 *	%edx: Feature bits.
 * %ecx == 1: Structure Extendede Feature Enumeration Sub-leaf
 *	%eax: See below.
 */
/* %ecx = 0, %ebx */
#define	CPUID_STDEXT_FSGSBASE	0x00000001 /* {RD,WR}{FS,GS}BASE */
#define	CPUID_STDEXT_TSC_ADJUST	0x00000002 /* IA32_TSC_ADJUST MSR support */
#define	CPUID_STDEXT_SGX	0x00000004 /* Software Guard Extensions */
#define	CPUID_STDEXT_BMI1	0x00000008 /* Advanced bit manipulation ext. 1st grp */
#define	CPUID_STDEXT_HLE	0x00000010 /* Hardware Lock Elision */
#define	CPUID_STDEXT_AVX2	0x00000020 /* Advanced Vector Extensions 2 */
#define	CPUID_STDEXT_FDP_EXC	0x00000040 /* x87FPU Data ptr updated only on x87exp */
#define	CPUID_STDEXT_SMEP	0x00000080 /* Supervisor-Mode Execution Prevention */
#define	CPUID_STDEXT_BMI2	0x00000100 /* Advanced bit manipulation ext. 2nd grp */
#define	CPUID_STDEXT_ERMS	0x00000200 /* Enhanced REP MOVSB/STOSB */
#define	CPUID_STDEXT_INVPCID	0x00000400 /* INVPCID instruction */
#define	CPUID_STDEXT_RTM	0x00000800 /* Restricted Transactional Memory */
#define	CPUID_STDEXT_PQM	0x00001000 /* Platform Quality of Service Monitoring */
#define	CPUID_STDEXT_NFPUSG	0x00002000 /* Deprecate FPU CS and FPU DS values */
#define	CPUID_STDEXT_MPX	0x00004000 /* Memory Protection Extensions */
#define	CPUID_STDEXT_PQE	0x00008000 /* Platform Quality of Service Enforcement */
#define	CPUID_STDEXT_AVX512F	0x00010000 /* AVX-512 Foundation */
#define	CPUID_STDEXT_AVX512DQ	0x00020000 /* AVX-512 Double/Quadword */
#define	CPUID_STDEXT_RDSEED	0x00040000 /* RDSEED instruction */
#define	CPUID_STDEXT_ADX	0x00080000 /* ADCX/ADOX instructions */
#define	CPUID_STDEXT_SMAP	0x00100000 /* Supervisor-Mode Access Prevention */
#define	CPUID_STDEXT_AVX512IFMA	0x00200000 /* AVX-512 Integer Fused Multiply Add */
/* Bit 22: reserved; was PCOMMIT */
#define	CPUID_STDEXT_CLFLUSHOPT	0x00800000 /* Cache Line FLUSH OPTimized */
#define	CPUID_STDEXT_CLWB	0x01000000 /* Cache Line Write Back */
#define	CPUID_STDEXT_PROCTRACE	0x02000000 /* Processor Trace */
#define	CPUID_STDEXT_AVX512PF	0x04000000 /* AVX-512 PreFetch */
#define	CPUID_STDEXT_AVX512ER	0x08000000 /* AVX-512 Exponential and Reciprocal */
#define	CPUID_STDEXT_AVX512CD	0x10000000 /* AVX-512 Conflict Detection */
#define	CPUID_STDEXT_SHA	0x20000000 /* SHA Extensions */
#define	CPUID_STDEXT_AVX512BW	0x40000000 /* AVX-512 Byte and Word */
#define	CPUID_STDEXT_AVX512VL	0x80000000 /* AVX-512 Vector Length */

/* %ecx = 0, %ecx */
#define	CPUID_STDEXT2_PREFETCHWT1	0x00000001 /* PREFETCHWT1 instruction */
#define	CPUID_STDEXT2_AVX512VBMI	0x00000002 /* AVX-512 Vector Byte Manipulation */
#define	CPUID_STDEXT2_UMIP		0x00000004 /* User-Mode Instruction prevention */
#define	CPUID_STDEXT2_PKU		0x00000008 /* Protection Keys for User-mode pages */
#define	CPUID_STDEXT2_OSPKE		0x00000010 /* PKU enabled by OS */
#define	CPUID_STDEXT2_WAITPKG		0x00000020 /* Timed pause and user-level monitor/wait */
#define	CPUID_STDEXT2_AVX512VBMI2	0x00000040 /* AVX-512 Vector Byte Manipulation 2 */
#define	CPUID_STDEXT2_CET_SS		0x00000080 /* CET Shadow Stack */
#define	CPUID_STDEXT2_GFNI		0x00000100 /* Galois Field instructions */
#define	CPUID_STDEXT2_VAES		0x00000200 /* Vector AES instruction set */
#define	CPUID_STDEXT2_VPCLMULQDQ	0x00000400 /* CLMUL instruction set */
#define	CPUID_STDEXT2_AVX512VNNI	0x00000800 /* Vector Neural Network instructions */
#define	CPUID_STDEXT2_AVX512BITALG	0x00001000 /* BITALG instructions */
#define	CPUID_STDEXT2_TME		0x00002000 /* Total Memory Encryption */
#define	CPUID_STDEXT2_AVX512VPOPCNTDQ	0x00004000 /* Vector Population Count Double/Quadword */
/* Bit 15: reserved */
#define	CPUID_STDEXT2_LA57		0x00010000 /* 57-bit linear addr & 5-level paging */
/* Bits 21-17: MAWAU value for BNDLDX/BNDSTX */
#define	CPUID_STDEXT2_RDPID		0x00400000 /* RDPID and IA32_TSC_AUX */
#define	CPUID_STDEXT2_KL		0x00800000 /* Key Locker */
#define	CPUID_STDEXT2_BUS_LOCK_DETECT	0x01000000 /* Bus-Lock Detection */
#define	CPUID_STDEXT2_CLDEMOTE		0x02000000 /* Cache line demote */
/* Bit 26: reserved */
#define	CPUID_STDEXT2_MOVDIRI		0x08000000 /* MOVDIRI instruction */
#define	CPUID_STDEXT2_MOVDIR64B		0x10000000 /* MOVDIR64B instruction */
#define	CPUID_STDEXT2_ENQCMD		0x20000000 /* Enqueue Stores */
#define	CPUID_STDEXT2_SGXLC		0x40000000 /* SGX Launch Configuration */
#define	CPUID_STDEXT2_PKS		0x80000000 /* Protection Keys for kern-mode pages */

/* %ecx = 0, %edx */
#define	CPUID_STDEXT3_AVX5124VNNIW	0x00000004 /* AVX512 4-reg Neural Network instructions */
#define	CPUID_STDEXT3_AVX5124FMAPS	0x00000008 /* AVX512 4-reg Multiply Accumulation Single precision */
#define	CPUID_STDEXT3_FSRM		0x00000010 /* Fast Short REP MOVE */
#define	CPUID_STDEXT3_UINTR		0x00000020 /* User Interrupts */
#define	CPUID_STDEXT3_AVX512VP2INTERSECT 0x00000100 /* AVX512 VP2INTERSECT */
#define	CPUID_STDEXT3_MCUOPT		0x00000200 /* IA32_MCU_OPT_CTRL */
#define	CPUID_STDEXT3_MD_CLEAR		0x00000400 /* VERW clears CPU buffers */
#define	CPUID_STDEXT3_TSXFA		0x00002000 /* MSR_TSX_FORCE_ABORT bit 0 */
#define	CPUID_STDEXT3_SERIALIZE		0x00004000 /* SERIALIZE instruction */
#define	CPUID_STDEXT3_HYBRID		0x00008000 /* Hybrid part */
#define	CPUID_STDEXT3_TSXLDTRK		0x00010000 /* TSX suspend load addr tracking */
#define	CPUID_STDEXT3_PCONFIG		0x00040000 /* Platform configuration */
#define	CPUID_STDEXT3_CET_IBT		0x00100000 /* CET Indirect Branch Tracking */
#define	CPUID_STDEXT3_IBPB		0x04000000 /* IBRS / IBPB Speculation Control */
#define	CPUID_STDEXT3_STIBP		0x08000000 /* STIBP Speculation Control */
#define	CPUID_STDEXT3_L1D_FLUSH		0x10000000 /* IA32_FLUSH_CMD MSR */
#define	CPUID_STDEXT3_ARCH_CAP		0x20000000 /* IA32_ARCH_CAPABILITIES */
#define	CPUID_STDEXT3_CORE_CAP		0x40000000 /* IA32_CORE_CAPABILITIES */
#define	CPUID_STDEXT3_SSBD		0x80000000 /* Speculative Store Bypass Disable */

/*
 * Intel x2APIC Features / Processor topology
 * CPUID Fn0000_000B
 */
#define	FUNC_B_THREAD_LEVEL	0

#define	FUNC_B_INVALID_TYPE	0
#define	FUNC_B_THREAD_TYPE	1
#define	FUNC_B_CORE_TYPE	2

#define	FUNC_B_TYPE(ecx)			(((ecx) >> 8) & 0xff)
#define	FUNC_B_BITS_SHIFT_NEXT_LEVEL(eax)	((eax) & 0x1f)
#define	FUNC_B_LEVEL_MAX_SIBLINGS(ebx)		((ebx) & 0xffff)

/*
 * Intel/AMD CPUID Processor Extended State Enumeration
 * CPUID Fn0000_000D
 * %ecx == 0: supported features info:
 *	%eax: Valid bits of lower 32bits of XCR0
 *	%ebx: Maximum save area size for features enabled in XCR0
 *	%ecx: Maximum save area size for all cpu features
 *	%edx: Valid bits of upper 32bits of XCR0
 * %ecx == 1:
 *	%eax: Bit 0 => xsaveopt instruction available (sandy bridge onwards)
 *	%ebx: Save area size for features enabled by XCR0 | IA32_XSS
 *	%ecx: Valid bits of lower 32bits of IA32_XSS
 *	%edx: Valid bits of upper 32bits of IA32_XSS
 * %ecx >= 2: Save area details for XCR0 bit n
 *	%eax: size of save area for this feature
 *	%ebx: offset of save area for this feature
 *	%ecx, %edx: reserved
 *	All of %eax, %ebx, %ecx and %edx are zero for unsupported features.
 */
/* %ecx = 1, %eax */
#define	CPUID_PES1_XSAVEOPT	0x00000001	/* xsaveopt instruction */
#define	CPUID_PES1_XSAVEC	0x00000002	/* xsavec & compacted XRSTOR */
#define	CPUID_PES1_XGETBV	0x00000004	/* xgetbv with ECX = 1 */
#define	CPUID_PES1_XSAVES	0x00000008	/* xsaves/xrstors, IA32_XSS */

/*
 * Extended Features
 * CPUID Fn8000_0001
 */
/* %edx */
#define	CPUID_SYSCALL	0x00000800	/* (Intel/AMD) SYSCALL/SYSRET */
#define	CPUID_MPC	0x00080000	/* (AMD) Multiprocessing Capable */
#define	CPUID_XD	0x00100000	/* (Intel) Execute Disable */
#define	CPUID_NOX	CPUID_XD	/* (AMD) No Execute Page Protection */
#define	CPUID_MMXX	0x00400000	/* (AMD) MMX Extensions */
#define	CPUID_FFXSR	0x02000000	/* (AMD) FXSAVE/FXSTOR Extensions */
#define	CPUID_PAGE1GB	0x04000000	/* (Intel) 1GB Large Page Support */
#define	CPUID_RDTSCP	0x08000000	/* (Intel) Read TSC Pair Instruction */
#define	CPUID_EM64T	0x20000000	/* (Intel) EM64T long mode */
#define	CPUID_3DNOW2	0x40000000	/* (AMD) 3DNow! Instruction Extension */
#define	CPUID_3DNOW	0x80000000	/* (AMD) 3DNow! Instructions */
	/* compatibility defines */
#define	AMDID_SYSCALL	CPUID_SYSCALL
#define	AMDID_MP	CPUID_MPC
#define	AMDID_NX	CPUID_NOX
#define	AMDID_EXT_MMX	CPUID_MMXX
#define	AMDID_FFXSR	CPUID_FFXSR
#define	AMDID_PAGE1GB	CPUID_PAGE1GB
#define	AMDID_RDTSCP	CPUID_RDTSCP
#define	AMDID_LM	CPUID_EM64T
#define	AMDID_EXT_3DNOW	CPUID_3DNOW2
#define	AMDID_3DNOW	CPUID_3DNOW
/* %ecx */
#define	CPUID_LAHF	0x00000001	/* (Intel/AMD) LAHF/SAHF in 64-bit mode */
#define	CPUID_CMPLEGACY	0x00000002	/* (AMD) Core multi-processing legacy mode */
#define	CPUID_SVM	0x00000004	/* (AMD) Secure Virtual Machine */
#define	CPUID_EAPIC	0x00000008	/* (AMD) Extended APIC space */
#define	CPUID_ALTMOVCR0	0x00000010	/* (AMD) LOCK MOV CR0 means MOV CR8 */
#define	CPUID_ABM	0x00000020	/* (AMD) LZCNT instruction */
#define	CPUID_SSE4A	0x00000040	/* (AMD) SSE4A instruction set */
#define	CPUID_MISALIGNSSE 0x00000080	/* (AMD) Misaligned SSE mode */
#define	CPUID_PREFETCHW	0x00000100	/* (Intel/AMD) PREFETCHW */
#define	CPUID_3DNOWPF	CPUID_PREFETCHW	/* 3DNow Prefetch */
#define	CPUID_OSVW	0x00000200	/* (AMD) OS visible workaround */
#define	CPUID_IBS	0x00000400	/* (AMD) Instruction Based Sampling */
#define	CPUID_XOP	0x00000800	/* (AMD) XOP instruction set */
#define	CPUID_SKINIT	0x00001000	/* (AMD) SKINIT and STGI */
#define	CPUID_WDT	0x00002000	/* (AMD) Watchdog timer support */
#define	CPUID_LWP	0x00008000	/* (AMD) Light Weight Profiling */
#define	CPUID_FMA4	0x00010000	/* (AMD) FMA4 instructions */
#define	CPUID_TCE	0x00020000	/* (AMD) Translation cache Extension */
#define	CPUID_NODEID	0x00080000	/* (AMD) NodeID MSR available*/
#define	CPUID_TBM	0x00200000	/* (AMD) TBM instructions */
#define	CPUID_TOPOEXT	0x00400000	/* (AMD) CPUID Topology Extension */
#define	CPUID_PCEC	0x00800000	/* (AMD) Processor Perf Counter Extension */
#define	CPUID_PCENB	0x01000000	/* (AMD) NB Perf Counter Extension */
#define	CPUID_SPM	0x02000000	/* (AMD) Stream Perf Mon */
#define	CPUID_DBE	0x04000000	/* (AMD) Data access breakpoint extension */
#define	CPUID_PTSC	0x08000000	/* (AMD) Performance time-stamp counter */
#define	CPUID_L2IPERFC	0x10000000	/* (AMD) L2I performance counter extension */
#define	CPUID_MWAITX	0x20000000	/* (AMD) MWAITX/MONITORX support */
#define	CPUID_ADDRMASKEXT 0x40000000	/* (AMD) Breakpoint Addressing Mask Extension */
	/* compatibility defines */
#define	AMDID2_LAHF	CPUID_LAHF
#define	AMDID2_CMP	CPUID_CMPLEGACY
#define	AMDID2_SVM	CPUID_SVM
#define	AMDID2_EXT_APIC	CPUID_EAPIC
#define	AMDID2_CR8	CPUID_ALTMOVCR0
#define	AMDID2_ABM	CPUID_ABM
#define	AMDID2_SSE4A	CPUID_SSE4A
#define	AMDID2_MAS	CPUID_MISALIGNSSE
#define	AMDID2_PREFETCH	CPUID_PREFETCHW
#define	AMDID2_OSVW	CPUID_OSVW
#define	AMDID2_IBS	CPUID_IBS
#define	AMDID2_SSE5	CPUID_XOP
#define	AMDID2_SKINIT	CPUID_SKINIT
#define	AMDID2_WDT	CPUID_WDT
#define	AMDID2_TOPOEXT	CPUID_TOPOEXT

/*
 * Advanced Power Management
 * CPUID Fn8000_0007 %edx
 * Only ITSC is for both Intel and AMD; others are for AMD only.
 */
#define	CPUID_APM_TS		0x00000001	/* Temperature Sensor */
#define	CPUID_APM_FID		0x00000002	/* Frequency ID control */
#define	CPUID_APM_VID		0x00000004	/* Voltage ID control */
#define	CPUID_APM_TTP		0x00000008	/* THERMTRIP (PCI F3xE4 register) */
#define	CPUID_APM_HTC		0x00000010	/* Hardware Thermal Control (TM) */
#define	CPUID_APM_STC		0x00000020	/* Software Thermal Control */
#define	CPUID_APM_100		0x00000040	/* 100MHz multiplier control */
#define	CPUID_APM_HWP		0x00000080	/* Hardware P-State control */
#define	CPUID_APM_ITSC		0x00000100	/* (Intel/AMD) Invariant TSC */
#define	CPUID_APM_CPB		0x00000200	/* Core Performance Boost */
#define	CPUID_APM_EFF		0x00000400	/* Effective Frequency (read-only) */
#define	CPUID_APM_PROCFI	0x00000800	/* Processor Feedback */
#define	CPUID_APM_PROCPR	0x00001000	/* Processor Power Reporting */
#define	CPUID_APM_CONNSTBY	0x00002000	/* Connected Standby */
#define	CPUID_APM_RAPL		0x00004000	/* Running Average Power Limit */
	/* compatibility defines */
#define	AMDPM_TS		CPUID_APM_TS
#define	AMDPM_FID		CPUID_APM_FID
#define	AMDPM_VID		CPUID_APM_VID
#define	AMDPM_TTP		CPUID_APM_TTP
#define	AMDPM_TM		CPUID_APM_HTC
#define	AMDPM_STC		CPUID_APM_STC
#define	AMDPM_100MHZ_STEPS	CPUID_APM_100
#define	AMDPM_HW_PSTATE		CPUID_APM_HWP
#define	AMDPM_TSC_INVARIANT	CPUID_APM_ITSC
#define	AMDPM_CPB		CPUID_APM_CPB

/*
 * AMD Processor Capacity Parameters and Extended Features
 * CPUID Fn8000_0008
 * %eax: Long Mode Size Identifiers
 * %ebx: Extended Feature Identifiers
 * %ecx: Size Identifiers
 * %edx: RDPRU Register Identifier Range
 */
/* %ebx */
#define	CPUID_CAPEX_CLZERO	0x00000001	/* CLZERO instruction */
#define	CPUID_CAPEX_IRPERF	0x00000002	/* InstRetCntMsr */
#define	CPUID_CAPEX_XSAVEERPTR	0x00000004	/* RstrFpErrPtrs by XRSTOR */
#define	CPUID_CAPEX_INVLPGB	0x00000008	/* INVLPGB and TLBSYNC instructions */
#define	CPUID_CAPEX_RDPRU	0x00000010	/* RDPRU instruction */
#define	CPUID_CAPEX_MCOMMIT	0x00000100	/* MCOMMIT instruction */
#define	CPUID_CAPEX_WBNOINVD	0x00000200	/* WBNOINVD instruction */
#define	CPUID_CAPEX_IBPB	0x00001000	/* Speculation Control IBPB */
#define	CPUID_CAPEX_INT_WBINVD	0x00002000	/* Interruptable WB[NO]INVD */
#define	CPUID_CAPEX_IBRS	0x00004000	/* Speculation Control IBRS */
#define	CPUID_CAPEX_STIBP	0x00008000	/* Speculation Control STIBP */
#define	CPUID_CAPEX_IBRS_ALWAYSON  0x00010000	/* IBRS always on mode */
#define	CPUID_CAPEX_STIBP_ALWAYSON 0x00020000	/* STIBP always on mode */
#define	CPUID_CAPEX_PREFER_IBRS	0x00040000	/* IBRS preferred */
#define	CPUID_CAPEX_SSBD	0x01000000	/* Speculation Control SSBD */
#define	CPUID_CAPEX_VIRT_SSBD	0x02000000	/* Virt Spec Control SSBD */
#define	CPUID_CAPEX_SSB_NO	0x04000000	/* SSBD not required */
/* %ecx info */
#define	AMDID_CMP_CORES		0x000000ff
#define	AMDID_COREID_SIZE	0x0000f000
#define	AMDID_COREID_SIZE_SHIFT	12

/*
 * AMD SVM Revision and Feature Identification
 * CPUID Fn8000_000A
 */
/* %eax - SVM revision */
#define	CPUID_AMD_SVM_REV		0x000000ff /* (bits 7-0) SVM revision number */
/* %edx - SVM features */
#define	CPUID_AMD_SVM_NP		0x00000001 /* Nested Paging */
#define	CPUID_AMD_SVM_LbrVirt		0x00000002 /* LBR virtualization */
#define	CPUID_AMD_SVM_SVML		0x00000004 /* SVM Lock */
#define	CPUID_AMD_SVM_NRIPS		0x00000008 /* NRIP Save on #VMEXIT */
#define	CPUID_AMD_SVM_TSCRateCtrl	0x00000010 /* MSR-based TSC rate control */
#define	CPUID_AMD_SVM_VMCBCleanBits	0x00000020 /* VMCB Clean Bits support */
#define	CPUID_AMD_SVM_FlushByASID	0x00000040 /* Flush by ASID */
#define	CPUID_AMD_SVM_DecodeAssist	0x00000080 /* Decode Assists support */
#define	CPUID_AMD_SVM_PauseFilter	0x00000400 /* PAUSE intercept filter */
#define	CPUID_AMD_SVM_PFThreshold	0x00001000 /* PAUSE filter threshold */
#define	CPUID_AMD_SVM_AVIC		0x00002000 /* Advanced Virtual Interrupt Controller */
#define	CPUID_AMD_SVM_V_VMSAVE_VMLOAD	0x00008000 /* VMSAVE/VMLOAD virtualization */
#define	CPUID_AMD_SVM_vGIF		0x00010000 /* Global Interrupt Flag virtualization */
#define	CPUID_AMD_SVM_GMET		0x00020000 /* Guest Mode Execution Trap */
#define	CPUID_AMD_SVM_SPEC_CTRL		0x00100000 /* SPEC_CTRL virtualization */
#define	CPUID_AMD_SVM_TLBICTL		0x01000000 /* TLB Intercept Control */

/*
 * CPUID manufacturers identifiers
 */
#define	AMD_VENDOR_ID		"AuthenticAMD"
#define	CENTAUR_VENDOR_ID	"CentaurHauls"
#define	INTEL_VENDOR_ID		"GenuineIntel"

/*
 * Model-Specific Registers
 */
#define	MSR_P5_MC_ADDR		0x000
#define	MSR_P5_MC_TYPE		0x001
#define	MSR_TSC			0x010
#define	MSR_P5_CESR		0x011
#define	MSR_P5_CTR0		0x012
#define	MSR_P5_CTR1		0x013
#define	MSR_IA32_PLATFORM_ID	0x017

#define	MSR_APICBASE		0x01b
#define		APICBASE_RESERVED	0x000006ff
#define		APICBASE_BSP		0x00000100 /* bootstrap processor */
#define		APICBASE_X2APIC		0x00000400 /* x2APIC mode */
#define		APICBASE_ENABLED	0x00000800 /* software enable */
#define		APICBASE_ADDRESS	0xfffff000 /* physical address */

#define	MSR_EBL_CR_POWERON	0x02a
#define	MSR_TEST_CTL		0x033

#define	MSR_SPEC_CTRL		0x048	/* IBRS Spectre mitigation */
#define		SPEC_CTRL_IBRS		0x00000001
#define		SPEC_CTRL_STIBP		0x00000002
#define		SPEC_CTRL_SSBD		0x00000004
#define		SPEC_CTRL_DUMMY1	0x00010000 /* ficticious */
#define		SPEC_CTRL_DUMMY2	0x00020000 /* ficticious */
#define		SPEC_CTRL_DUMMY3	0x00040000 /* ficticious */
#define		SPEC_CTRL_DUMMY4	0x00080000 /* ficticious */
#define		SPEC_CTRL_DUMMY5	0x00100000 /* ficticious */
#define		SPEC_CTRL_DUMMY6	0x00200000 /* ficticious */

#define	MSR_PRED_CMD		0x049	/* IBPB Spectre mitigation */

#define	MSR_BIOS_UPDT_TRIG	0x079
#define	MSR_BBL_CR_D0		0x088
#define	MSR_BBL_CR_D1		0x089
#define	MSR_BBL_CR_D2		0x08a
#define	MSR_BIOS_SIGN		0x08b
#define	MSR_PERFCTR0		0x0c1
#define	MSR_PERFCTR1		0x0c2
#define	MSR_IA32_EXT_CONFIG	0x0ee	/* Undocumented. Core Solo/Duo only */
#define	MSR_MTRRcap		0x0fe

#define	MSR_IA32_ARCH_CAPABILITIES 0x10a
#define		IA32_ARCH_CAP_RDCL_NO	0x00000001
#define		IA32_ARCH_CAP_IBRS_ALL	0x00000002
#define		IA32_ARCH_CAP_RSBA	0x00000004
#define		IA32_ARCH_CAP_SKIP_L1DFL_VMENTRY 0x00000008
#define		IA32_ARCH_CAP_SSB_NO	0x00000010
#define		IA32_ARCH_CAP_MDS_NO	0x00000020
#define		IA32_ARCH_CAP_IF_PSCHANGE_MC_NO 0x00000040
#define		IA32_ARCH_CAP_TSX_CTRL	0x00000080
#define		IA32_ARCH_CAP_TAA_NO	0x00000100

#define	MSR_IA32_FLUSH_CMD	0x10b
#define		IA32_FLUSH_CMD_L1D	0x01

#define	MSR_BBL_CR_ADDR		0x116
#define	MSR_BBL_CR_DECC		0x118
#define	MSR_BBL_CR_CTL		0x119
#define	MSR_BBL_CR_TRIG		0x11a
#define	MSR_BBL_CR_BUSY		0x11b
#define	MSR_BBL_CR_CTL3		0x11e
#define	MSR_SYSENTER_CS		0x174
#define	MSR_SYSENTER_ESP	0x175
#define	MSR_SYSENTER_EIP	0x176
#define	MSR_MCG_CAP		0x179
#define	MSR_MCG_STATUS		0x17a
#define	MSR_MCG_CTL		0x17b
#define	MSR_EVNTSEL0		0x186
#define	MSR_EVNTSEL1		0x187
#define	MSR_THERM_CONTROL	0x19a
#define	MSR_THERM_INTERRUPT	0x19b
#define	MSR_THERM_STATUS	0x19c

#define	MSR_IA32_MISC_ENABLE	0x1a0	/* Enable Misc. Processor Features */
#define		IA32_MISC_FAST_STR_EN	(1ULL <<  0)
#define		IA32_MISC_ATCC_EN	(1ULL <<  3)
#define		IA32_MISC_PERFMON_EN	(1ULL <<  7)
#define		IA32_MISC_BTS_UNAVAIL	(1ULL << 11)
#define		IA32_MISC_PEBS_UNAVAIL	(1ULL << 12)
#define		IA32_MISC_EISST_EN	(1ULL << 16)
#define		IA32_MISC_MWAIT_EN	(1ULL << 18)
#define		IA32_MISC_LIMIT_CPUID	(1ULL << 22)
#define		IA32_MISC_XTPR_DIS	(1ULL << 23)
#define		IA32_MISC_XD_DIS	(1ULL << 34)

#define	MSR_IA32_TEMPERATURE_TARGET 0x1a2
#define	MSR_PKG_THERM_STATUS	0x1b1
#define	MSR_PKG_THERM_INTR	0x1b2
#define	MSR_DEBUGCTLMSR		0x1d9
#define	MSR_LASTBRANCHFROMIP	0x1db
#define	MSR_LASTBRANCHTOIP	0x1dc
#define	MSR_LASTINTFROMIP	0x1dd
#define	MSR_LASTINTTOIP		0x1de
#define	MSR_ROB_CR_BKUPTMPDR6	0x1e0
#define	MSR_MTRRVarBase		0x200
#define	MSR_MTRR64kBase		0x250
#define	MSR_MTRR16kBase		0x258
#define	MSR_MTRR4kBase		0x268
#define	MSR_PAT			0x277
#define	MSR_MTRRdefType		0x2ff
#define	MSR_MC0_CTL		0x400
#define	MSR_MC0_STATUS		0x401
#define	MSR_MC0_ADDR		0x402
#define	MSR_MC0_MISC		0x403
#define	MSR_MC1_CTL		0x404
#define	MSR_MC1_STATUS		0x405
#define	MSR_MC1_ADDR		0x406
#define	MSR_MC1_MISC		0x407
#define	MSR_MC2_CTL		0x408
#define	MSR_MC2_STATUS		0x409
#define	MSR_MC2_ADDR		0x40a
#define	MSR_MC2_MISC		0x40b
#define	MSR_MC3_CTL		0x40c
#define	MSR_MC3_STATUS		0x40d
#define	MSR_MC3_ADDR		0x40e
#define	MSR_MC3_MISC		0x40f
#define	MSR_MC4_CTL		0x410
#define	MSR_MC4_STATUS		0x411
#define	MSR_MC4_ADDR		0x412
#define	MSR_MC4_MISC		0x413
#define	MSR_RAPL_POWER_UNIT	0x606
#define	MSR_PKG_ENERGY_STATUS	0x611
#define	MSR_DRAM_ENERGY_STATUS	0x619
#define	MSR_PP0_ENERGY_STATUS	0x639
#define	MSR_PP1_ENERGY_STATUS	0x641
#define	MSR_PLATFORM_ENERGY_COUNTER 0x64d /* Skylake and later */
#define	MSR_PPERF		0x64e /* Productive Performance Count */
#define	MSR_PERF_LIMIT_REASONS	0x64f /* Indicator of Frequency Clipping */
#define	MSR_TSC_DEADLINE	0x6e0 /* LAPIC TSC Deadline Mode Target count */

/* Hardware P-states interface */
#define	MSR_PM_ENABLE		0x770 /* Enable/disable HWP */
#define	MSR_HWP_CAPABILITIES	0x771 /* HWP Performance Range Enumeration */
#define	MSR_HWP_REQUEST_PKG	0x772 /* Control hints to all logical proc */
#define	MSR_HWP_INTERRUPT	0x773 /* Control HWP Native Interrupts */
#define	MSR_HWP_REQUEST		0x774 /* Control hints to a logical proc */
#define	MSR_HWP_STATUS		0x777

/*
 * PAT modes.
 */
#define	PAT_UNCACHEABLE		0x00
#define	PAT_WRITE_COMBINING	0x01
#define	PAT_WRITE_THROUGH	0x04
#define	PAT_WRITE_PROTECTED	0x05
#define	PAT_WRITE_BACK		0x06
#define	PAT_UNCACHED		0x07
#define	PAT_VALUE(i, m)		((long)(m) << (8 * (i)))
#define	PAT_MASK(i)		PAT_VALUE(i, 0xff)

/*
 * Constants related to MTRRs
 */
#define	MTRR_UNCACHEABLE	0x00
#define	MTRR_WRITE_COMBINING	0x01
#define	MTRR_WRITE_THROUGH	0x04
#define	MTRR_WRITE_PROTECTED	0x05
#define	MTRR_WRITE_BACK		0x06
#define	MTRR_N64K		8	/* numbers of fixed-size entries */
#define	MTRR_N16K		16
#define	MTRR_N4K		64
#define	MTRR_CAP_WC		0x0000000000000400UL
#define	MTRR_CAP_FIXED		0x0000000000000100UL
#define	MTRR_CAP_VCNT		0x00000000000000ffUL
#define	MTRR_DEF_ENABLE		0x0000000000000800UL
#define	MTRR_DEF_FIXED_ENABLE	0x0000000000000400UL
#define	MTRR_DEF_TYPE		0x00000000000000ffUL
#define	MTRR_PHYSBASE_PHYSBASE	0x000ffffffffff000UL
#define	MTRR_PHYSBASE_TYPE	0x00000000000000ffUL
#define	MTRR_PHYSMASK_PHYSMASK	0x000ffffffffff000UL
#define	MTRR_PHYSMASK_VALID	0x0000000000000800UL

/* Performance Control Register (5x86 only). */
#define	PCR0			0x20
#define	PCR0_RSTK		0x01	/* Enables return stack */
#define	PCR0_BTB		0x02	/* Enables branch target buffer */
#define	PCR0_LOOP		0x04	/* Enables loop */
#define	PCR0_AIS		0x08	/* Enables all instrcutions stalled to serialize pipe. */
#define	PCR0_MLR		0x10	/* Enables reordering of misaligned loads */
#define	PCR0_BTBRT		0x40	/* Enables BTB test register. */
#define	PCR0_LSSER		0x80	/* Disable reorder */

/* Device Identification Registers */
#define	DIR0			0xfe
#define	DIR1			0xff

/*
 * Machine Check register constants.
 */
#define	MCG_CAP_COUNT		0x000000ff
#define	MCG_CAP_CTL_P		0x00000100
#define	MCG_CAP_EXT_P		0x00000200
#define	MCG_CAP_TES_P		0x00000800
#define	MCG_CAP_EXT_CNT		0x00ff0000
#define	MCG_STATUS_RIPV		0x00000001
#define	MCG_STATUS_EIPV		0x00000002
#define	MCG_STATUS_MCIP		0x00000004
#define	MCG_CTL_ENABLE		0xffffffffffffffffUL
#define	MCG_CTL_DISABLE		0x0000000000000000UL
#define	MSR_MC_CTL(x)		(MSR_MC0_CTL + (x) * 4)
#define	MSR_MC_STATUS(x)	(MSR_MC0_STATUS + (x) * 4)
#define	MSR_MC_ADDR(x)		(MSR_MC0_ADDR + (x) * 4)
#define	MSR_MC_MISC(x)		(MSR_MC0_MISC + (x) * 4)
#define	MC_STATUS_MCA_ERROR	0x000000000000ffffUL
#define	MC_STATUS_MODEL_ERROR	0x00000000ffff0000UL
#define	MC_STATUS_OTHER_INFO	0x01ffffff00000000UL
#define	MC_STATUS_PCC		0x0200000000000000UL
#define	MC_STATUS_ADDRV		0x0400000000000000UL
#define	MC_STATUS_MISCV		0x0800000000000000UL
#define	MC_STATUS_EN		0x1000000000000000UL
#define	MC_STATUS_UC		0x2000000000000000UL
#define	MC_STATUS_OVER		0x4000000000000000UL
#define	MC_STATUS_VAL		0x8000000000000000UL

/*
 * The following four 3-byte registers control the non-cacheable regions.
 * These registers must be written as three separate bytes.
 *
 * NCRx+0: A31-A24 of starting address
 * NCRx+1: A23-A16 of starting address
 * NCRx+2: A15-A12 of starting address | NCR_SIZE_xx.
 *
 * The non-cacheable region's starting address must be aligned to the
 * size indicated by the NCR_SIZE_xx field.
 */
#define	NCR1	0xc4
#define	NCR2	0xc7
#define	NCR3	0xca
#define	NCR4	0xcd

#define	NCR_SIZE_0K	0
#define	NCR_SIZE_4K	1
#define	NCR_SIZE_8K	2
#define	NCR_SIZE_16K	3
#define	NCR_SIZE_32K	4
#define	NCR_SIZE_64K	5
#define	NCR_SIZE_128K	6
#define	NCR_SIZE_256K	7
#define	NCR_SIZE_512K	8
#define	NCR_SIZE_1M	9
#define	NCR_SIZE_2M	10
#define	NCR_SIZE_4M	11
#define	NCR_SIZE_8M	12
#define	NCR_SIZE_16M	13
#define	NCR_SIZE_32M	14
#define	NCR_SIZE_4G	15

/*
 * The address region registers are used to specify the location and
 * size for the eight address regions.
 *
 * ARRx + 0: A31-A24 of start address
 * ARRx + 1: A23-A16 of start address
 * ARRx + 2: A15-A12 of start address | ARR_SIZE_xx
 */
#define	ARR0	0xc4
#define	ARR1	0xc7
#define	ARR2	0xca
#define	ARR3	0xcd
#define	ARR4	0xd0
#define	ARR5	0xd3
#define	ARR6	0xd6
#define	ARR7	0xd9

#define	ARR_SIZE_0K	0
#define	ARR_SIZE_4K	1
#define	ARR_SIZE_8K	2
#define	ARR_SIZE_16K	3
#define	ARR_SIZE_32K	4
#define	ARR_SIZE_64K	5
#define	ARR_SIZE_128K	6
#define	ARR_SIZE_256K	7
#define	ARR_SIZE_512K	8
#define	ARR_SIZE_1M	9
#define	ARR_SIZE_2M	10
#define	ARR_SIZE_4M	11
#define	ARR_SIZE_8M	12
#define	ARR_SIZE_16M	13
#define	ARR_SIZE_32M	14
#define	ARR_SIZE_4G	15

/*
 * The region control registers specify the attributes associated with
 * the ARRx addres regions.
 */
#define	RCR0	0xdc
#define	RCR1	0xdd
#define	RCR2	0xde
#define	RCR3	0xdf
#define	RCR4	0xe0
#define	RCR5	0xe1
#define	RCR6	0xe2
#define	RCR7	0xe3

#define	RCR_RCD	0x01	/* Disables caching for ARRx (x = 0-6). */
#define	RCR_RCE	0x01	/* Enables caching for ARR7. */
#define	RCR_WWO	0x02	/* Weak write ordering. */
#define	RCR_WL	0x04	/* Weak locking. */
#define	RCR_WG	0x08	/* Write gathering. */
#define	RCR_WT	0x10	/* Write-through. */
#define	RCR_NLB	0x20	/* LBA# pin is not asserted. */

/* AMD Write Allocate Top-Of-Memory and Control Register */
#define	AMD_WT_ALLOC_TME	0x40000	/* top-of-memory enable */
#define	AMD_WT_ALLOC_PRE	0x20000	/* programmable range enable */
#define	AMD_WT_ALLOC_FRE	0x10000	/* fixed (A0000-FFFFF) range enable */

/*
 * x86_64 MSR's
 */
#define	MSR_EFER	0xc0000080	/* Extended Feature Enable Register */
#define		EFER_SCE	0x00000001	/* System Call Extensions (R/W) */
#define		EFER_LME	0x00000100	/* Long Mode Enable (R/W) */
#define		EFER_LMA	0x00000400	/* Long Mode Active (R) */
#define		EFER_NXE	0x00000800	/* PTE No-Execute Enable (R/W) */
#define		EFER_SVME	0x00001000	/* SVM Enable (R/W) */
#define		EFER_LMSLE	0x00002000	/* Long Mode Segment Limit Enable */
#define		EFER_FFXSR	0x00004000	/* Fast FXSAVE/FXRSTOR Enable */
#define		EFER_TCE	0x00008000	/* Translation Cache Extension */
#define		EFER_MCOMMIT	0x00020000	/* MCOMMIT Enable */
#define		EFER_INTWB	0x00040000	/* Intr WBINVD/WBNOINVD Enable */

#define	MSR_STAR	0xc0000081	/* legacy mode SYSCALL target/cs/ss */
#define	MSR_LSTAR	0xc0000082	/* long mode SYSCALL target rip */
#define	MSR_CSTAR	0xc0000083	/* compat mode SYSCALL target rip */
#define	MSR_SF_MASK	0xc0000084	/* syscall flags mask */
#define	MSR_FSBASE	0xc0000100	/* base address of the %fs "segment" */
#define	MSR_GSBASE	0xc0000101	/* base address of the %gs "segment" */
#define	MSR_KGSBASE	0xc0000102	/* base address of the kernel %gs */
#define	MSR_TSC_AUX	0xc0000103	/* TSC_AUX register (for rdtscp) */
#define	MSR_PERFEVSEL0	0xc0010000
#define	MSR_PERFEVSEL1	0xc0010001
#define	MSR_PERFEVSEL2	0xc0010002
#define	MSR_PERFEVSEL3	0xc0010003
#define	MSR_K7_PERFCTR0	0xc0010004
#define	MSR_K7_PERFCTR1	0xc0010005
#define	MSR_K7_PERFCTR2	0xc0010006
#define	MSR_K7_PERFCTR3	0xc0010007
#define	MSR_SYSCFG	0xc0010010
#define	MSR_IORRBASE0	0xc0010016
#define	MSR_IORRMASK0	0xc0010017
#define	MSR_IORRBASE1	0xc0010018
#define	MSR_IORRMASK1	0xc0010019
#define	MSR_TOP_MEM	0xc001001a	/* boundary for ram below 4G */
#define	MSR_TOP_MEM2	0xc001001d	/* boundary for ram above 4G */

#define	MSR_AMD_NB_CFG	0xc001001f	/* Northbridge Configuration */
#define		NB_CFG_INITAPICCPUIDLO	(1ULL << 54)

#define	MSR_AMD_PATCH_LEVEL	0x0000008b
#define	MSR_AMD_PATCH_LOADER	0xc0010020	/* update microcode */

#define	MSR_AMD_VM_CR	0xc0010114	/* SVM: feature control */
#define		VM_CR_DPD		0x00000001 /* Debug Port Disable */
#define		VM_CR_R_INIT		0x00000002 /* Intercept INIT signals */
#define		VM_CR_DIS_A20M		0x00000004 /* Disable A20 masking */
#define		VM_CR_LOCK		0x00000008 /* SVM Lock */
#define		VM_CR_SVMDIS		0x00000010 /* SVM Disable */

#define	MSR_AMD_VM_HSAVE_PA	0xc0010117	/* SVM: host save area address */
#define	MSR_AMD_LS_CFG	0xc0011020	/* Load-Store Configuration */
#define	MSR_AMD_IC_CFG	0xc0011021	/* Instruction Cache Configuration */
#define	MSR_AMD_DE_CFG	0xc0011029	/* Decode Configuration */

/* VIA ACE crypto featureset: for via_feature_rng */
#define	VIA_HAS_RNG		1	/* cpu has RNG */

/* VIA ACE crypto featureset: for via_feature_xcrypt */
#define	VIA_HAS_AES		1	/* cpu has AES */
#define	VIA_HAS_SHA		2	/* cpu has SHA1 & SHA256 */
#define	VIA_HAS_MM		4	/* cpu has RSA instructions */
#define	VIA_HAS_AESCTR		8	/* cpu has AES-CTR instructions */

/* Centaur Extended Feature flags */
#define	VIA_CPUID_HAS_RNG	0x000004
#define	VIA_CPUID_DO_RNG	0x000008
#define	VIA_CPUID_HAS_ACE	0x000040
#define	VIA_CPUID_DO_ACE	0x000080
#define	VIA_CPUID_HAS_ACE2	0x000100
#define	VIA_CPUID_DO_ACE2	0x000200
#define	VIA_CPUID_HAS_PHE	0x000400
#define	VIA_CPUID_DO_PHE	0x000800
#define	VIA_CPUID_HAS_PMM	0x001000
#define	VIA_CPUID_DO_PMM	0x002000

#endif /* !_CPU_SPECIALREG_H_ */
