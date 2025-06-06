/*
 * Copyright 2018, Jérôme Duval, jerome.duval@gmail.com.
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_ARCH_x86_CPU_H
#define _KERNEL_ARCH_x86_CPU_H


#ifndef _ASSEMBLER

#include <module.h>

#include <arch_thread_types.h>

#include <arch/x86/arch_altcodepatch.h>
#include <arch/x86/arch_cpuasm.h>
#include <arch/x86/descriptors.h>

#ifdef __x86_64__
#	include <arch/x86/64/cpu.h>
#endif

#endif	// !_ASSEMBLER


#define CPU_MAX_CACHE_LEVEL	8

#define CACHE_LINE_SIZE		64


// MSR registers (possibly Intel specific)
#define IA32_MSR_TSC					0x10
#define IA32_MSR_PLATFORM_ID			0x17
#define IA32_MSR_APIC_BASE				0x1b
#define IA32_MSR_SPEC_CTRL				0x48
#define IA32_MSR_PRED_CMD				0x49
#define IA32_MSR_UCODE_WRITE			0x79	// IA32_BIOS_UPDT_TRIG
#define IA32_MSR_UCODE_REV				0x8b	// IA32_BIOS_SIGN_ID
#define IA32_MSR_PLATFORM_INFO			0xce
#define IA32_MSR_MPERF					0xe7
#define IA32_MSR_APERF					0xe8
#define IA32_MSR_MTRR_CAPABILITIES		0xfe
#define IA32_MSR_ARCH_CAPABILITIES		0x10a
#define IA32_MSR_FLUSH_CMD				0x10b
#define IA32_MSR_SYSENTER_CS			0x174
#define IA32_MSR_SYSENTER_ESP			0x175
#define IA32_MSR_SYSENTER_EIP			0x176
#define IA32_MSR_PERF_STATUS			0x198
#define IA32_MSR_PERF_CTL				0x199
#define IA32_MSR_TURBO_RATIO_LIMIT		0x1ad
#define IA32_MSR_ENERGY_PERF_BIAS		0x1b0
#define IA32_MSR_MTRR_DEFAULT_TYPE		0x2ff
#define IA32_MSR_MTRR_PHYSICAL_BASE_0	0x200
#define IA32_MSR_MTRR_PHYSICAL_MASK_0	0x201
#define IA32_MSR_PAT					0x277

// MSR SPEC CTRL bits
#define IA32_MSR_SPEC_CTRL_IBRS			(1 << 0)
#define IA32_MSR_SPEC_CTRL_STIBP		(1 << 1)
#define IA32_MSR_SPEC_CTRL_SSBD			(1 << 2)

// MSR PRED CMD bits
#define IA32_MSR_PRED_CMD_IBPB			(1 << 0)

// MSR APIC BASE bits
#define IA32_MSR_APIC_BASE_BSP			0x00000100
#define IA32_MSR_APIC_BASE_X2APIC		0x00000400
#define IA32_MSR_APIC_BASE_ENABLED		0x00000800
#define IA32_MSR_APIC_BASE_ADDRESS		0xfffff000

// MSR EFER bits
// reference
#define IA32_MSR_EFER_SYSCALL			(1 << 0)
#define IA32_MSR_EFER_NX				(1 << 11)

// MSR ARCH CAPABILITIES bits
#define IA32_MSR_ARCH_CAP_RDCL_NO			(1 << 0)
#define IA32_MSR_ARCH_CAP_IBRS_ALL			(1 << 1)
#define IA32_MSR_ARCH_CAP_RSBA				(1 << 2)
#define IA32_MSR_ARCH_CAP_SKIP_L1D_VMENTRY	(1 << 3)
#define IA32_MSR_ARCH_CAP_SSB_NO			(1 << 4)

// MSR FLUSH CMD bits
#define IA32_MSR_L1D_FLUSH			(1 << 1)

// X2APIC MSRs.
#define IA32_MSR_APIC_ID					0x00000802
#define IA32_MSR_APIC_VERSION				0x00000803
#define IA32_MSR_APIC_TASK_PRIORITY			0x00000808
#define IA32_MSR_APIC_PROCESSOR_PRIORITY	0x0000080a
#define IA32_MSR_APIC_EOI					0x0000080b
#define IA32_MSR_APIC_LOGICAL_DEST			0x0000080d
#define IA32_MSR_APIC_SPURIOUS_INTR_VECTOR	0x0000080f
#define IA32_MSR_APIC_ERROR_STATUS			0x00000828
#define IA32_MSR_APIC_INTR_COMMAND			0x00000830
#define IA32_MSR_APIC_LVT_TIMER				0x00000832
#define IA32_MSR_APIC_LVT_THERMAL_SENSOR	0x00000833
#define IA32_MSR_APIC_LVT_PERFMON_COUNTERS	0x00000834
#define IA32_MSR_APIC_LVT_LINT0				0x00000835
#define IA32_MSR_APIC_LVT_LINT1				0x00000836
#define IA32_MSR_APIC_LVT_ERROR				0x00000837
#define IA32_MSR_APIC_INITIAL_TIMER_COUNT	0x00000838
#define IA32_MSR_APIC_CURRENT_TIMER_COUNT	0x00000839
#define IA32_MSR_APIC_TIMER_DIVIDE_CONFIG	0x0000083e
#define IA32_MSR_APIC_SELF_IPI				0x0000083f
#define IA32_MSR_XSS						0x00000da0

// x86_64 MSRs.
#define IA32_MSR_EFER					0xc0000080
#define IA32_MSR_STAR					0xc0000081
#define IA32_MSR_LSTAR					0xc0000082
#define IA32_MSR_CSTAR					0xc0000083
#define IA32_MSR_FMASK					0xc0000084
#define IA32_MSR_FS_BASE				0xc0000100
#define IA32_MSR_GS_BASE				0xc0000101
#define IA32_MSR_KERNEL_GS_BASE			0xc0000102
#define IA32_MSR_TSC_AUX				0xc0000103

// AMD MSR registers
#define MSR_F10H_HWCR						0xc0010015
#define 	HWCR_TSCFREQSEL					(1 << 24)
#define MSR_K8_UCODE_UPDATE				0xc0010020
#define K8_MSR_IPM						0xc0010055
#define MSR_F10H_PSTATEDEF(x)				(0xc0010064 + (x))
#define 	PSTATEDEF_EN					(1ULL << 63)
#define MSR_F10H_DE_CFG					0xc0011029
#define 	DE_CFG_SERIALIZE_LFENCE			(1 << 1)

#define MSR_AMD_CPPC_CAP1				0xc00102b0
#define		AMD_CPPC_LOWEST_PERF(x)		((x) & 0xff)
#define		AMD_CPPC_LOWNONLIN_PERF(x)	((x >> 8) & 0xff)
#define		AMD_CPPC_NOMINAL_PERF(x)	((x >> 16) & 0xff)
#define		AMD_CPPC_HIGHEST_PERF(x)	((x >> 24) & 0xff)
#define MSR_AMD_CPPC_ENABLE				0xc00102b1
#define MSR_AMD_CPPC_REQ				0xc00102b3
#define		AMD_CPPC_MAX_PERF(x)		((x) & 0xff)
#define		AMD_CPPC_MIN_PERF(x)		(((x) & 0xff) << 8)
#define		AMD_CPPC_DES_PERF(x)		(((x) & 0xff) << 16)
#define		AMD_CPPC_EPP_PERF(x)		(((x) & 0xff) << 24)

#define		AMD_CPPC_EPP_PERFORMANCE			0x00
#define		AMD_CPPC_EPP_BALANCE_PERFORMANCE	0x80
#define		AMD_CPPC_EPP_BALANCE_POWERSAVE		0xbf
#define		AMD_CPPC_EPP_POWERSAVE				0xff
#define MSR_AMD_CPPC_STATUS				0xc00102b4


// Hardware P-States MSR registers §14.4.1
// reference https://software.intel.com/content/dam/develop/public/us/en/documents/253669-sdm-vol-3b.pdf
#define IA32_MSR_PM_ENABLE				0x00000770
#define IA32_MSR_HWP_CAPABILITIES		0x00000771
#define IA32_MSR_HWP_REQUEST_PKG		0x00000772
#define IA32_MSR_HWP_INTERRUPT			0x00000773
#define IA32_MSR_HWP_REQUEST			0x00000774
#define IA32_MSR_HWP_STATUS				0x00000777

// IA32_MSR_HWP_CAPABILITIES bits §14.4.3
#define	IA32_HWP_CAPS_HIGHEST_PERFORMANCE(x)	(((x) >> 0) & 0xff)
#define	IA32_HWP_CAPS_GUARANTEED_PERFORMANCE(x)	(((x) >> 8) & 0xff)
#define	IA32_HWP_CAPS_EFFICIENT_PERFORMANCE(x)	(((x) >> 16) & 0xff)
#define	IA32_HWP_CAPS_LOWEST_PERFORMANCE(x)		(((x) >> 24) & 0xff)

// IA32_MSR_HWP_REQUEST bits §14.4.4.1
#define	IA32_HWP_REQUEST_MINIMUM_PERFORMANCE			(0xffULL << 0)
#define	IA32_HWP_REQUEST_MAXIMUM_PERFORMANCE			(0xffULL << 8)
#define	IA32_HWP_REQUEST_DESIRED_PERFORMANCE			(0xffULL << 16)
#define	IA32_HWP_REQUEST_ENERGY_PERFORMANCE_PREFERENCE	(0xffULL << 24)
#define	IA32_HWP_REQUEST_ACTIVITY_WINDOW				(0x3ffULL << 32)
#define	IA32_HWP_REQUEST_PACKAGE_CONTROL				(1ULL << 42)
#define	IA32_HWP_REQUEST_ACTIVITY_WINDOW_VALID			(1ULL << 59)
#define	IA32_HWP_REQUEST_EPP_VALID 						(1ULL << 60)
#define	IA32_HWP_REQUEST_DESIRED_VALID					(1ULL << 61)
#define	IA32_HWP_REQUEST_MAXIMUM_VALID					(1ULL << 62)
#define	IA32_HWP_REQUEST_MINIMUM_VALID					(1ULL << 63)

// IA32_MSR_PAT bits
#define IA32_MSR_PAT_ENTRY_MASK							0x7ULL
#define IA32_MSR_PAT_ENTRY_SHIFT(x)						(x * 8)
#define IA32_MSR_PAT_TYPE_UNCACHEABLE					0x0ULL
#define IA32_MSR_PAT_TYPE_WRITE_COMBINING				0x1ULL
#define IA32_MSR_PAT_TYPE_WRITE_THROUGH					0x4ULL
#define IA32_MSR_PAT_TYPE_WRITE_PROTECTED				0x5ULL
#define IA32_MSR_PAT_TYPE_WRITE_BACK					0x6ULL
#define IA32_MSR_PAT_TYPE_UNCACHED						0x7ULL

// cpuid leaves
#define IA32_CPUID_LEAF_MWAIT				0x5
#define IA32_CPUID_LEAF_XSTATE				0xd
#define IA32_CPUID_LEAF_TSC					0x15
#define IA32_CPUID_LEAF_FREQUENCY			0x16

// x86 features from cpuid eax 1, edx register
// reference http://www.intel.com/Assets/en_US/PDF/appnote/241618.pdf (Table 5-5)
#define IA32_FEATURE_FPU	(1 << 0) // x87 fpu
#define IA32_FEATURE_VME	(1 << 1) // virtual 8086
#define IA32_FEATURE_DE		(1 << 2) // debugging extensions
#define IA32_FEATURE_PSE	(1 << 3) // page size extensions
#define IA32_FEATURE_TSC	(1 << 4) // rdtsc instruction
#define IA32_FEATURE_MSR	(1 << 5) // rdmsr/wrmsr instruction
#define IA32_FEATURE_PAE	(1 << 6) // extended 3 level page table addressing
#define IA32_FEATURE_MCE	(1 << 7) // machine check exception
#define IA32_FEATURE_CX8	(1 << 8) // cmpxchg8b instruction
#define IA32_FEATURE_APIC	(1 << 9) // local apic on chip
//							(1 << 10) // Reserved
#define IA32_FEATURE_SEP	(1 << 11) // SYSENTER/SYSEXIT
#define IA32_FEATURE_MTRR	(1 << 12) // MTRR
#define IA32_FEATURE_PGE	(1 << 13) // paging global bit
#define IA32_FEATURE_MCA	(1 << 14) // machine check architecture
#define IA32_FEATURE_CMOV	(1 << 15) // cmov instruction
#define IA32_FEATURE_PAT	(1 << 16) // page attribute table
#define IA32_FEATURE_PSE36	(1 << 17) // page size extensions with 4MB pages
#define IA32_FEATURE_PSN	(1 << 18) // processor serial number
#define IA32_FEATURE_CLFSH	(1 << 19) // cflush instruction
//							(1 << 20) // Reserved
#define IA32_FEATURE_DS		(1 << 21) // debug store
#define IA32_FEATURE_ACPI	(1 << 22) // thermal monitor and clock ctrl
#define IA32_FEATURE_MMX	(1 << 23) // mmx instructions
#define IA32_FEATURE_FXSR	(1 << 24) // FXSAVE/FXRSTOR instruction
#define IA32_FEATURE_SSE	(1 << 25) // SSE
#define IA32_FEATURE_SSE2	(1 << 26) // SSE2
#define IA32_FEATURE_SS		(1 << 27) // self snoop
#define IA32_FEATURE_HTT	(1 << 28) // hyperthreading
#define IA32_FEATURE_TM		(1 << 29) // thermal monitor
#define IA32_FEATURE_IA64	(1 << 30) // IA64 processor emulating x86
#define IA32_FEATURE_PBE	(1 << 31) // pending break enable

// x86 features from cpuid eax 1, ecx register
// reference http://www.intel.com/Assets/en_US/PDF/appnote/241618.pdf (Table 5-4)
#define IA32_FEATURE_EXT_SSE3		(1 << 0) // SSE3
#define IA32_FEATURE_EXT_PCLMULQDQ	(1 << 1) // PCLMULQDQ Instruction
#define IA32_FEATURE_EXT_DTES64		(1 << 2) // 64-Bit Debug Store
#define IA32_FEATURE_EXT_MONITOR	(1 << 3) // MONITOR/MWAIT
#define IA32_FEATURE_EXT_DSCPL		(1 << 4) // CPL qualified debug store
#define IA32_FEATURE_EXT_VMX		(1 << 5) // Virtual Machine Extensions
#define IA32_FEATURE_EXT_SMX		(1 << 6) // Safer Mode Extensions
#define IA32_FEATURE_EXT_EST		(1 << 7) // Enhanced SpeedStep
#define IA32_FEATURE_EXT_TM2		(1 << 8) // Thermal Monitor 2
#define IA32_FEATURE_EXT_SSSE3		(1 << 9) // Supplemental SSE-3
#define IA32_FEATURE_EXT_CNXTID		(1 << 10) // L1 Context ID
//									(1 << 11) // Reserved
#define IA32_FEATURE_EXT_FMA		(1 << 12) // Fused Multiply Add
#define IA32_FEATURE_EXT_CX16		(1 << 13) // CMPXCHG16B
#define IA32_FEATURE_EXT_XTPR		(1 << 14) // xTPR Update Control
#define IA32_FEATURE_EXT_PDCM		(1 << 15) // Perfmon and Debug Capability
//									(1 << 16) // Reserved
#define IA32_FEATURE_EXT_PCID		(1 << 17) // Process Context Identifiers
#define IA32_FEATURE_EXT_DCA		(1 << 18) // Direct Cache Access
#define IA32_FEATURE_EXT_SSE4_1		(1 << 19) // SSE4.1
#define IA32_FEATURE_EXT_SSE4_2		(1 << 20) // SSE4.2
#define IA32_FEATURE_EXT_X2APIC		(1 << 21) // Extended xAPIC Support
#define IA32_FEATURE_EXT_MOVBE 		(1 << 22) // MOVBE Instruction
#define IA32_FEATURE_EXT_POPCNT		(1 << 23) // POPCNT Instruction
#define IA32_FEATURE_EXT_TSCDEADLINE (1 << 24) // Time Stamp Counter Deadline
#define IA32_FEATURE_EXT_AES		(1 << 25) // AES Instruction Extensions
#define IA32_FEATURE_EXT_XSAVE		(1 << 26) // XSAVE/XSTOR States
#define IA32_FEATURE_EXT_OSXSAVE	(1 << 27) // OS-Enabled XSAVE
#define IA32_FEATURE_EXT_AVX		(1 << 28) // Advanced Vector Extensions
#define IA32_FEATURE_EXT_F16C		(1 << 29) // 16-bit FP conversion
#define IA32_FEATURE_EXT_RDRND		(1 << 30) // RDRAND instruction
#define IA32_FEATURE_EXT_HYPERVISOR	(1 << 31) // Running on a hypervisor

// x86 features from cpuid eax 0x80000001, ecx register (AMD)
#define IA32_FEATURE_AMD_EXT_CMPLEGACY	(1 << 1) // Core MP legacy mode
#define IA32_FEATURE_AMD_EXT_TOPOLOGY	(1 << 22) // Topology extensions

// x86 features from cpuid eax 0x80000001, edx register (AMD)
// only care about the ones that are unique to this register
#define IA32_FEATURE_AMD_EXT_SYSCALL	(1 << 11) // SYSCALL/SYSRET
#define IA32_FEATURE_AMD_EXT_NX			(1 << 20) // no execute bit
#define IA32_FEATURE_AMD_EXT_MMXEXT		(1 << 22) // mmx extensions
#define IA32_FEATURE_AMD_EXT_FFXSR		(1 << 25) // fast FXSAVE/FXRSTOR
#define IA32_FEATURE_AMD_EXT_PDPE1GB	(1 << 26) // Gibibyte pages
#define IA32_FEATURE_AMD_EXT_RDTSCP		(1 << 27) // rdtscp instruction
#define IA32_FEATURE_AMD_EXT_LONG		(1 << 29) // long mode
#define IA32_FEATURE_AMD_EXT_3DNOWEXT	(1 << 30) // 3DNow! extensions
#define IA32_FEATURE_AMD_EXT_3DNOW		(1 << 31) // 3DNow!

// some of the features from cpuid eax 0x80000001, edx register (AMD) are also
// available on Intel processors
#define IA32_FEATURES_INTEL_EXT			(IA32_FEATURE_AMD_EXT_SYSCALL		\
											| IA32_FEATURE_AMD_EXT_NX		\
											| IA32_FEATURE_AMD_EXT_PDPE1GB	\
											| IA32_FEATURE_AMD_EXT_RDTSCP	\
											| IA32_FEATURE_AMD_EXT_LONG)

// x86 defined features from cpuid eax 6, eax register
// reference https://software.intel.com/content/dam/develop/public/us/en/documents/253666-sdm-vol-2a.pdf (Table 3-8)
#define IA32_FEATURE_DTS	(1 << 0) // Digital Thermal Sensor
#define IA32_FEATURE_ITB	(1 << 1) // Intel Turbo Boost Technology
#define IA32_FEATURE_ARAT	(1 << 2) // Always running APIC Timer
#define IA32_FEATURE_PLN	(1 << 4) // Power Limit Notification
#define IA32_FEATURE_ECMD	(1 << 5) // Extended Clock Modulation Duty
#define IA32_FEATURE_PTM	(1 << 6) // Package Thermal Management
#define IA32_FEATURE_HWP	(1 << 7) // Hardware P-states
#define IA32_FEATURE_HWP_NOTIFY	(1 << 8) // HWP Notification
#define IA32_FEATURE_HWP_ACTWIN	(1 << 9) // HWP Activity Window
#define IA32_FEATURE_HWP_EPP	(1 << 10) // HWP Energy Performance Preference
#define IA32_FEATURE_HWP_PLR	(1 << 11) // HWP Package Level Request
#define IA32_FEATURE_HDC	(1 << 13) // Hardware Duty Cycling
#define IA32_FEATURE_TBMT3	(1 << 14) // Turbo Boost Max Technology 3.0
#define IA32_FEATURE_HWP_CAP	(1 << 15) // HWP Capabilities
#define IA32_FEATURE_HWP_PECI	(1 << 16) // HWP PECI override
#define IA32_FEATURE_HWP_FLEX	(1 << 17) // Flexible HWP
#define IA32_FEATURE_HWP_FAST	(1 << 18) // Fast access for HWP_REQUEST MSR
#define IA32_FEATURE_HW_FEEDBACK	(1 << 19) // HW_FEEDBACK*, PACKAGE_THERM*
#define IA32_FEATURE_HWP_IGNIDL	(1 << 20) // Ignore Idle Logical Processor HWP

// x86 defined features from cpuid eax 6, ecx register
// reference http://www.intel.com/Assets/en_US/PDF/appnote/241618.pdf (Table 5-11)
#define IA32_FEATURE_APERFMPERF	(1 << 0) // IA32_APERF, IA32_MPERF
#define IA32_FEATURE_EPB	(1 << 3) // IA32_ENERGY_PERF_BIAS

// x86 features from cpuid eax 7, ebx register
// reference http://www.intel.com/Assets/en_US/PDF/appnote/241618.pdf (Table 3-8)
#define IA32_FEATURE_TSC_ADJUST	(1 << 1) // IA32_TSC_ADJUST MSR supported
#define IA32_FEATURE_SGX		(1 << 2) // Software Guard Extensions
#define IA32_FEATURE_BMI1		(1 << 3) // Bit Manipulation Instruction Set 1
#define IA32_FEATURE_HLE		(1 << 4) // Hardware Lock Elision
#define IA32_FEATURE_AVX2		(1 << 5) // Advanced Vector Extensions 2
#define IA32_FEATURE_SMEP		(1 << 7) // Supervisor-Mode Execution Prevention
#define IA32_FEATURE_BMI2		(1 << 8) // Bit Manipulation Instruction Set 2
#define IA32_FEATURE_ERMS		(1 << 9) // Enhanced REP MOVSB/STOSB
#define IA32_FEATURE_INVPCID	(1 << 10) // INVPCID instruction
#define IA32_FEATURE_RTM		(1 << 11) // Transactional Synchronization Extensions
#define IA32_FEATURE_CQM		(1 << 12) // Platform Quality of Service Monitoring
#define IA32_FEATURE_MPX		(1 << 14) // Memory Protection Extensions
#define IA32_FEATURE_RDT_A		(1 << 15) // Resource Director Technology Allocation
#define IA32_FEATURE_AVX512F	(1 << 16) // AVX-512 Foundation
#define IA32_FEATURE_AVX512DQ	(1 << 17) // AVX-512 Doubleword and Quadword Instructions
#define IA32_FEATURE_RDSEED		(1 << 18) // RDSEED instruction
#define IA32_FEATURE_ADX		(1 << 19) // ADX (Multi-Precision Add-Carry Instruction Extensions)
#define IA32_FEATURE_SMAP		(1 << 20) // Supervisor Mode Access Prevention
#define IA32_FEATURE_AVX512IFMA	(1 << 21) // AVX-512 Integer Fused Multiply-Add Instructions
#define IA32_FEATURE_PCOMMIT	(1 << 22) // PCOMMIT instruction
#define IA32_FEATURE_CLFLUSHOPT	(1 << 23) // CLFLUSHOPT instruction
#define IA32_FEATURE_CLWB		(1 << 24) // CLWB instruction
#define IA32_FEATURE_INTEL_PT	(1 << 25) // Intel Processor Trace
#define IA32_FEATURE_AVX512PF	(1 << 26) // AVX-512 Prefetch Instructions
#define IA32_FEATURE_AVX512ER	(1 << 27) // AVX-512 Exponential and Reciprocal Instructions
#define IA32_FEATURE_AVX512CD	(1 << 28) // AVX-512 Conflict Detection Instructions
#define IA32_FEATURE_SHA_NI		(1 << 29) // SHA extensions
#define IA32_FEATURE_AVX512BW	(1 << 30) // AVX-512 Byte and Word Instructions
#define IA32_FEATURE_AVX512VI	(1 << 31) // AVX-512 Vector Length Extensions

// x86 features from cpuid eax 7, ecx register
// reference http://www.intel.com/Assets/en_US/PDF/appnote/241618.pdf (Table 3-8)
// https://en.wikipedia.org/wiki/CPUID#EAX=7,_ECX=0:_Extended_Features
#define IA32_FEATURE_AVX512VMBI		(1 << 1) // AVX-512 Vector Bit Manipulation Instructions
#define IA32_FEATURE_UMIP			(1 << 2) // User-mode Instruction Prevention
#define IA32_FEATURE_PKU			(1 << 3) // Memory Protection Keys for User-mode pages
#define IA32_FEATURE_OSPKE			(1 << 4) // PKU enabled by OS
#define IA32_FEATURE_AVX512VMBI2	(1 << 6) // AVX-512 Vector Bit Manipulation Instructions 2
#define IA32_FEATURE_GFNI			(1 << 8) // Galois Field instructions
#define IA32_FEATURE_VAES			(1 << 9) // AES instruction set (VEX-256/EVEX)
#define IA32_FEATURE_VPCLMULQDQ		(1 << 10) // CLMUL instruction set (VEX-256/EVEX)
#define IA32_FEATURE_AVX512_VNNI	(1 << 11) // AVX-512 Vector Neural Network Instructions
#define IA32_FEATURE_AVX512_BITALG	(1 << 12) // AVX-512 BITALG instructions
#define IA32_FEATURE_AVX512_VPOPCNTDQ (1 << 14) // AVX-512 Vector Population Count D/Q
#define IA32_FEATURE_LA57			(1 << 16) // 5-level page tables
#define IA32_FEATURE_RDPID			(1 << 22) // RDPID Instruction
#define IA32_FEATURE_SGX_LC			(1 << 30) // SGX Launch Configuration

// x86 features from cpuid eax 7, edx register
// https://en.wikipedia.org/wiki/CPUID#EAX=7,_ECX=0:_Extended_Features
#define IA32_FEATURE_AVX512_4VNNIW	(1 << 2) // AVX-512 4-register Neural Network Instructions
#define IA32_FEATURE_AVX512_4FMAPS	(1 << 3) // AVX-512 4-register Multiply Accumulation Single precision
#define IA32_FEATURE_HYBRID_CPU		(1 << 15)	// CPUs are of several types
#define IA32_FEATURE_IBRS			(1 << 26)	// IBRS / IBPB Speculation Control
#define IA32_FEATURE_STIBP			(1 << 27)	// STIBP Speculation Control
#define IA32_FEATURE_L1D_FLUSH		(1 << 28)	// L1D_FLUSH supported
#define IA32_FEATURE_ARCH_CAPABILITIES	(1 << 29)	// IA32_ARCH_CAPABILITIES MSR
#define IA32_FEATURE_SSBD			(1 << 31)	// Speculative Store Bypass Disable

// x86 features from cpuid eax 0xd, ecx 1, eax register
// reference http://www.intel.com/Assets/en_US/PDF/appnote/241618.pdf (Table 3-8)
#define IA32_FEATURE_XSAVEOPT		(1 << 0) // XSAVEOPT Instruction
#define IA32_FEATURE_XSAVEC			(1 << 1) // XSAVEC and compacted XRSTOR
#define IA32_FEATURE_XGETBV1		(1 << 2) // XGETBV with ECX=1 Instruction
#define IA32_FEATURE_XSAVES			(1 << 3) // XSAVES and XRSTORS Instruction

// x86 defined features from cpuid eax 0x80000007, edx register
#define IA32_FEATURE_AMD_HW_PSTATE		(1 << 7)
#define IA32_FEATURE_INVARIANT_TSC		(1 << 8)
#define IA32_FEATURE_CPB				(1 << 9)
#define IA32_FEATURE_PROC_FEEDBACK		(1 << 11)

// x86 defined features from cpuid eax 0x80000008, ebx register
#define IA32_FEATURE_CLZERO			(1 << 0)	// CLZERO instruction
#define IA32_FEATURE_IBPB			(1 << 12)	// IBPB Support only (no IBRS)
#define IA32_FEATURE_AMD_SSBD		(1 << 24)	// Speculative Store Bypass Disable
#define IA32_FEATURE_VIRT_SSBD		(1 << 25)	// Virtualized Speculative Store Bypass Disable
#define IA32_FEATURE_AMD_SSB_NO		(1 << 26)	// Speculative Store Bypass is fixed in hardware
#define IA32_FEATURE_CPPC			(1 << 27)	// Collaborative Processor Performance Control


// Memory type ranges
#define IA32_MTR_UNCACHED				0
#define IA32_MTR_WRITE_COMBINING		1
#define IA32_MTR_WRITE_THROUGH			4
#define IA32_MTR_WRITE_PROTECTED		5
#define IA32_MTR_WRITE_BACK				6

// EFLAGS register
#define X86_EFLAGS_CARRY						0x00000001
#define X86_EFLAGS_RESERVED1					0x00000002
#define X86_EFLAGS_PARITY						0x00000004
#define X86_EFLAGS_AUXILIARY_CARRY				0x00000010
#define X86_EFLAGS_ZERO							0x00000040
#define X86_EFLAGS_SIGN							0x00000080
#define X86_EFLAGS_TRAP							0x00000100
#define X86_EFLAGS_INTERRUPT					0x00000200
#define X86_EFLAGS_DIRECTION					0x00000400
#define X86_EFLAGS_OVERFLOW						0x00000800
#define X86_EFLAGS_IO_PRIVILEG_LEVEL			0x00003000
#define X86_EFLAGS_IO_PRIVILEG_LEVEL_SHIFT		12
#define X86_EFLAGS_NESTED_TASK					0x00004000
#define X86_EFLAGS_RESUME						0x00010000
#define X86_EFLAGS_V86_MODE						0x00020000
#define X86_EFLAGS_ALIGNMENT_CHECK				0x00040000	// also SMAP status
#define X86_EFLAGS_VIRTUAL_INTERRUPT			0x00080000
#define X86_EFLAGS_VIRTUAL_INTERRUPT_PENDING	0x00100000
#define X86_EFLAGS_ID							0x00200000

#define X86_EFLAGS_USER_FLAGS	(X86_EFLAGS_CARRY | X86_EFLAGS_PARITY \
	| X86_EFLAGS_AUXILIARY_CARRY | X86_EFLAGS_ZERO | X86_EFLAGS_SIGN \
	| X86_EFLAGS_DIRECTION | X86_EFLAGS_OVERFLOW)

#define CR0_CACHE_DISABLE		(1UL << 30)
#define CR0_NOT_WRITE_THROUGH	(1UL << 29)
#define CR0_FPU_EMULATION		(1UL << 2)
#define CR0_MONITOR_FPU			(1UL << 1)

// Control Register CR4 flags §2.5
// https://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-vol-3a-part-1-manual.pdf
#define IA32_CR4_VME			(1UL << 0)
#define IA32_CR4_PVI			(1UL << 1)
#define IA32_CR4_TSD			(1UL << 2)
#define IA32_CR4_DE				(1UL << 3)
#define IA32_CR4_PSE			(1UL << 4)
#define IA32_CR4_PAE			(1UL << 5)
#define IA32_CR4_MCE			(1UL << 6)
#define IA32_CR4_GLOBAL_PAGES	(1UL << 7)
#define IA32_CR4_PCE			(1UL << 8)
#define CR4_OS_FXSR				(1UL << 9)
#define CR4_OS_XMM_EXCEPTION	(1UL << 10)
#define IA32_CR4_UMIP			(1UL << 11)
#define IA32_CR4_LA57			(1UL << 12)
#define IA32_CR4_VMXE			(1UL << 13)
#define IA32_CR4_SMXE			(1UL << 14)
#define IA32_CR4_FSGSBASE		(1UL << 16)
#define IA32_CR4_PCIDE			(1UL << 17)
#define IA32_CR4_OSXSAVE		(1UL << 18)
#define IA32_CR4_SMEP			(1UL << 20)
#define IA32_CR4_SMAP			(1UL << 21)
#define IA32_CR4_PKE			(1UL << 22)

// Extended Control Register XCR0 flags §13.3
// https://software.intel.com/content/dam/develop/public/us/en/documents/253665-sdm-vol-1.pdf
#define IA32_XCR0_X87			(1UL << 0)
#define IA32_XCR0_SSE			(1UL << 1)
#define IA32_XCR0_AVX			(1UL << 2)
#define IA32_XCR0_BNDREG		(1UL << 3)
#define IA32_XCR0_BNDCSR		(1UL << 4)
#define IA32_XCR0_OPMASK		(1UL << 5)
#define IA32_XCR0_ZMM_HI256		(1UL << 6)
#define IA32_XCR0_HI16_ZMM		(1UL << 7)
#define IA32_XCR0_PT			(1UL << 8)
#define IA32_XCR0_PKRU			(1UL << 9)

// page fault error codes (http://wiki.osdev.org/Page_Fault)
#define PGFAULT_P						0x01	// Protection violation
#define PGFAULT_W						0x02	// Write
#define PGFAULT_U						0x04	// Usermode
#define PGFAULT_RSVD					0x08	// Reserved bits
#define PGFAULT_I						0x10	// Instruction fetch

// iframe types
#define IFRAME_TYPE_SYSCALL				0x1
#define IFRAME_TYPE_OTHER				0x2
#define IFRAME_TYPE_MASK				0xf


#ifndef _ASSEMBLER


struct X86PagingStructures;


typedef struct x86_mtrr_info {
	uint64	base;
	uint64	size;
	uint8	type;
} x86_mtrr_info;

typedef struct x86_cpu_module_info {
	module_info	info;
	uint32		(*count_mtrrs)(void);
	void		(*init_mtrrs)(void);

	void		(*set_mtrr)(uint32 index, uint64 base, uint64 length,
					uint8 type);
	status_t	(*get_mtrr)(uint32 index, uint64* _base, uint64* _length,
					uint8* _type);
	void		(*set_mtrrs)(uint8 defaultType, const x86_mtrr_info* infos,
					uint32 count);
} x86_cpu_module_info;

// features
enum x86_feature_type {
	FEATURE_COMMON = 0,     // cpuid eax=1, edx register
	FEATURE_EXT,            // cpuid eax=1, ecx register
	FEATURE_EXT_AMD_ECX,	// cpuid eax=0x80000001, ecx register (AMD)
	FEATURE_EXT_AMD,        // cpuid eax=0x80000001, edx register (AMD)
	FEATURE_6_EAX,          // cpuid eax=6, eax registers
	FEATURE_6_ECX,          // cpuid eax=6, ecx registers
	FEATURE_7_EBX,          // cpuid eax=7, ebx registers
	FEATURE_7_ECX,          // cpuid eax=7, ecx registers
	FEATURE_7_EDX,          // cpuid eax=7, edx registers
	FEATURE_EXT_7_EDX,		// cpuid eax=0x80000007, edx register
	FEATURE_EXT_8_EBX,		// cpuid eax=0x80000008, ebx register
	FEATURE_D_1_EAX,		// cpuid eax=0xd, ecx=1, eax register

	FEATURE_NUM
};

enum x86_vendors {
	VENDOR_INTEL = 0,
	VENDOR_AMD,
	VENDOR_CYRIX,
	VENDOR_UMC,
	VENDOR_NEXGEN,
	VENDOR_CENTAUR,
	VENDOR_RISE,
	VENDOR_TRANSMETA,
	VENDOR_NSC,
	VENDOR_HYGON,

	VENDOR_NUM,
	VENDOR_UNKNOWN,
};


typedef struct arch_cpu_info {
	// saved cpu info
	enum x86_vendors	vendor;
	uint32				feature[FEATURE_NUM];
	char				model_name[49];
	const char*			vendor_name;
	int					type;
	int					family;
	int					extended_family;
	int					stepping;
	int					model;
	int					extended_model;
	uint32				patch_level;
	uint8				hybrid_type;

	uint32				logical_apic_id;

	uint64				mperf_prev;
	uint64				aperf_prev;
	bigtime_t			perf_timestamp;
	uint64				frequency;

	struct X86PagingStructures* active_paging_structures;

	size_t				dr6;	// temporary storage for debug registers (cf.
	size_t				dr7;	// x86_exit_user_debug_at_kernel_entry())

	// local TSS for this cpu
	struct tss			tss;
#ifndef __x86_64__
	struct tss			double_fault_tss;
	void*				kernel_tls;
#endif
} arch_cpu_info;


// Reference Intel SDM Volume 3 9.11 "Microcode Update Facilities"
// https://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-vol-3a-part-1-manual.pdf
// 9.11.1 Table 9-7. Microcode Update Field Definitions
struct intel_microcode_header {
	uint32 header_version;
	uint32 update_revision;
	uint32 date;
	uint32 processor_signature;
	uint32 checksum;
	uint32 loader_revision;
	uint32 processor_flags;
	uint32 data_size;
	uint32 total_size;
	uint32 reserved[3];
};


struct intel_microcode_extended_signature_header {
	uint32 extended_signature_count;
	uint32 extended_checksum;
	uint32 reserved[3];
};


struct intel_microcode_extended_signature {
	uint32 processor_signature;
	uint32 processor_flags;
	uint32 checksum;
};


// AMD Microcode structures

struct amd_container_header {
	uint32 magic;
};


struct amd_section_header {
	uint32 type;
	uint32 size;
};


struct amd_equiv_cpu_entry {
	uint32 installed_cpu;
	uint32 fixed_errata_mask;
	uint32 fixed_errata_compare;
	uint16 equiv_cpu;
	uint16 res;
};


struct amd_microcode_header {
	uint32 data_code;
	uint32 patch_id;
	uint16 mc_patch_data_id;
	uint8 mc_patch_data_len;
	uint8 init_flag;
	uint32 mc_patch_data_checksum;
	uint32 nb_dev_id;
	uint32 sb_dev_id;
	uint16 processor_rev_id;
	uint8 nb_rev_id;
	uint8 sb_rev_id;
	uint8 bios_api_rev;
	uint8 reserved1[3];
	uint32 match_reg[8];
};


extern void (*gCpuIdleFunc)(void);


#ifdef __cplusplus
extern "C" {
#endif

struct arch_thread;

#ifdef __x86_64__
void __x86_setup_system_time(uint64 conversionFactor,
	uint64 conversionFactorNsecs);
#else
void __x86_setup_system_time(uint32 conversionFactor,
	uint32 conversionFactorNsecs, bool conversionFactorNsecsShift);
#endif

status_t __x86_patch_errata_percpu(int cpu);

void x86_userspace_thread_exit(void);
void x86_end_userspace_thread_exit(void);

addr_t x86_get_stack_frame();
uint32 x86_count_mtrrs(void);
void x86_set_mtrr(uint32 index, uint64 base, uint64 length, uint8 type);
status_t x86_get_mtrr(uint32 index, uint64* _base, uint64* _length,
	uint8* _type);
void x86_set_mtrrs(uint8 defaultType, const x86_mtrr_info* infos,
	uint32 count);
void x86_init_fpu();
bool x86_check_feature(uint32 feature, enum x86_feature_type type);
bool x86_use_pat();
void* x86_get_double_fault_stack(int32 cpu, size_t* _size);
int32 x86_double_fault_get_cpu(void);

void x86_invalid_exception(iframe* frame);
void x86_fatal_exception(iframe* frame);
void x86_unexpected_exception(iframe* frame);
void x86_hardware_interrupt(iframe* frame);
void x86_page_fault_exception(iframe* iframe);

#ifndef __x86_64__

void x86_swap_pgdir(addr_t newPageDir);

void x86_context_switch(struct arch_thread* oldState,
	struct arch_thread* newState);

void x86_fnsave(void* fpuState);
void x86_frstor(const void* fpuState);

void x86_fxsave(void* fpuState);
void x86_fxrstor(const void* fpuState);

void x86_noop_swap(void* oldFpuState, const void* newFpuState);
void x86_fnsave_swap(void* oldFpuState, const void* newFpuState);
void x86_fxsave_swap(void* oldFpuState, const void* newFpuState);

#endif


static inline void
arch_cpu_idle(void)
{
	gCpuIdleFunc();
}


static inline void
arch_cpu_pause(void)
{
	asm volatile("pause" : : : "memory");
}


#ifdef __cplusplus
}	// extern "C" {
#endif

#endif	// !_ASSEMBLER

#endif	/* _KERNEL_ARCH_x86_CPU_H */
