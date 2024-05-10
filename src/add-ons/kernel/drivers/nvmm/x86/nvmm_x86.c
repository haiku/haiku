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

#include "../nvmm.h"
#include "../nvmm_internal.h"
#include "nvmm_x86.h"

/*
 * Code shared between x86-SVM and x86-VMX.
 */

const struct nvmm_x64_state nvmm_x86_reset_state = {
	.segs = {
		[NVMM_X64_SEG_ES] = {
			.selector = 0x0000,
			.base = 0x00000000,
			.limit = 0xFFFF,
			.attrib = {
				.type = 3,
				.s = 1,
				.p = 1,
			}
		},
		[NVMM_X64_SEG_CS] = {
			.selector = 0xF000,
			.base = 0xFFFF0000,
			.limit = 0xFFFF,
			.attrib = {
				.type = 3,
				.s = 1,
				.p = 1,
			}
		},
		[NVMM_X64_SEG_SS] = {
			.selector = 0x0000,
			.base = 0x00000000,
			.limit = 0xFFFF,
			.attrib = {
				.type = 3,
				.s = 1,
				.p = 1,
			}
		},
		[NVMM_X64_SEG_DS] = {
			.selector = 0x0000,
			.base = 0x00000000,
			.limit = 0xFFFF,
			.attrib = {
				.type = 3,
				.s = 1,
				.p = 1,
			}
		},
		[NVMM_X64_SEG_FS] = {
			.selector = 0x0000,
			.base = 0x00000000,
			.limit = 0xFFFF,
			.attrib = {
				.type = 3,
				.s = 1,
				.p = 1,
			}
		},
		[NVMM_X64_SEG_GS] = {
			.selector = 0x0000,
			.base = 0x00000000,
			.limit = 0xFFFF,
			.attrib = {
				.type = 3,
				.s = 1,
				.p = 1,
			}
		},
		[NVMM_X64_SEG_GDT] = {
			.selector = 0x0000,
			.base = 0x00000000,
			.limit = 0xFFFF,
			.attrib = {
				.type = 2,
				.s = 1,
				.p = 1,
			}
		},
		[NVMM_X64_SEG_IDT] = {
			.selector = 0x0000,
			.base = 0x00000000,
			.limit = 0xFFFF,
			.attrib = {
				.type = 2,
				.s = 1,
				.p = 1,
			}
		},
		[NVMM_X64_SEG_LDT] = {
			.selector = 0x0000,
			.base = 0x00000000,
			.limit = 0xFFFF,
			.attrib = {
				.type = 2,
				.s = 0,
				.p = 1,
			}
		},
		[NVMM_X64_SEG_TR] = {
			.selector = 0x0000,
			.base = 0x00000000,
			.limit = 0xFFFF,
			.attrib = {
				.type = 3,
				.s = 0,
				.p = 1,
			}
		},
	},

	.gprs = {
		[NVMM_X64_GPR_RAX] = 0x00000000,
		[NVMM_X64_GPR_RCX] = 0x00000000,
		[NVMM_X64_GPR_RDX] = 0x00000600,
		[NVMM_X64_GPR_RBX] = 0x00000000,
		[NVMM_X64_GPR_RSP] = 0x00000000,
		[NVMM_X64_GPR_RBP] = 0x00000000,
		[NVMM_X64_GPR_RSI] = 0x00000000,
		[NVMM_X64_GPR_RDI] = 0x00000000,
		[NVMM_X64_GPR_R8] = 0x00000000,
		[NVMM_X64_GPR_R9] = 0x00000000,
		[NVMM_X64_GPR_R10] = 0x00000000,
		[NVMM_X64_GPR_R11] = 0x00000000,
		[NVMM_X64_GPR_R12] = 0x00000000,
		[NVMM_X64_GPR_R13] = 0x00000000,
		[NVMM_X64_GPR_R14] = 0x00000000,
		[NVMM_X64_GPR_R15] = 0x00000000,
		[NVMM_X64_GPR_RIP] = 0x0000FFF0,
		[NVMM_X64_GPR_RFLAGS] = 0x00000002,
	},

	.crs = {
		[NVMM_X64_CR_CR0] = 0x60000010,
		[NVMM_X64_CR_CR2] = 0x00000000,
		[NVMM_X64_CR_CR3] = 0x00000000,
		[NVMM_X64_CR_CR4] = 0x00000000,
		[NVMM_X64_CR_CR8] = 0x00000000,
		[NVMM_X64_CR_XCR0] = 0x00000001,
	},

	.drs = {
		[NVMM_X64_DR_DR0] = 0x00000000,
		[NVMM_X64_DR_DR1] = 0x00000000,
		[NVMM_X64_DR_DR2] = 0x00000000,
		[NVMM_X64_DR_DR3] = 0x00000000,
		[NVMM_X64_DR_DR6] = 0xFFFF0FF0,
		[NVMM_X64_DR_DR7] = 0x00000400,
	},

	.msrs = {
		[NVMM_X64_MSR_EFER] = 0x00000000,
		[NVMM_X64_MSR_STAR] = 0x00000000,
		[NVMM_X64_MSR_LSTAR] = 0x00000000,
		[NVMM_X64_MSR_CSTAR] = 0x00000000,
		[NVMM_X64_MSR_SFMASK] = 0x00000000,
		[NVMM_X64_MSR_KERNELGSBASE] = 0x00000000,
		[NVMM_X64_MSR_SYSENTER_CS] = 0x00000000,
		[NVMM_X64_MSR_SYSENTER_ESP] = 0x00000000,
		[NVMM_X64_MSR_SYSENTER_EIP] = 0x00000000,
		[NVMM_X64_MSR_PAT] = 0x0007040600070406,
		[NVMM_X64_MSR_TSC] = 0,
	},

	.intr = {
		.int_shadow = 0,
		.int_window_exiting = 0,
		.nmi_window_exiting = 0,
		.evt_pending = 0,
	},

	.fpu = {
		.fx_cw = 0x0040,
		.fx_sw = 0x0000,
		.fx_tw = 0x55,
		.fx_zero = 0x55,
		.fx_mxcsr = 0x1F80,
	}
};

const struct nvmm_x86_cpuid_mask nvmm_cpuid_00000001 = {
	.eax = ~0,
	.ebx = ~0,
	.ecx =
	    CPUID_0_01_ECX_SSE3 |
	    CPUID_0_01_ECX_PCLMULQDQ |
	    /* CPUID_0_01_ECX_DTES64 excluded */
	    /* CPUID_0_01_ECX_MONITOR excluded */
	    /* CPUID_0_01_ECX_DS_CPL excluded */
	    /* CPUID_0_01_ECX_VMX excluded */
	    /* CPUID_0_01_ECX_SMX excluded */
	    /* CPUID_0_01_ECX_EIST excluded */
	    /* CPUID_0_01_ECX_TM2 excluded */
	    CPUID_0_01_ECX_SSSE3 |
	    /* CPUID_0_01_ECX_CNXTID excluded */
	    /* CPUID_0_01_ECX_SDBG excluded */
	    CPUID_0_01_ECX_FMA |
	    CPUID_0_01_ECX_CX16 |
	    /* CPUID_0_01_ECX_XTPR excluded */
	    /* CPUID_0_01_ECX_PDCM excluded */
	    /* CPUID_0_01_ECX_PCID excluded, but re-included in VMX */
	    /* CPUID_0_01_ECX_DCA excluded */
	    CPUID_0_01_ECX_SSE41 |
	    CPUID_0_01_ECX_SSE42 |
	    /* CPUID_0_01_ECX_X2APIC excluded */
	    CPUID_0_01_ECX_MOVBE |
	    CPUID_0_01_ECX_POPCNT |
	    /* CPUID_0_01_ECX_TSC_DEADLINE excluded */
	    CPUID_0_01_ECX_AESNI |
	    CPUID_0_01_ECX_XSAVE |
	    CPUID_0_01_ECX_OSXSAVE |
	    /* CPUID_0_01_ECX_AVX excluded */
	    CPUID_0_01_ECX_F16C |
	    CPUID_0_01_ECX_RDRAND,
	    /* CPUID_0_01_ECX_RAZ excluded */
	.edx =
	    CPUID_0_01_EDX_FPU |
	    CPUID_0_01_EDX_VME |
	    CPUID_0_01_EDX_DE |
	    CPUID_0_01_EDX_PSE |
	    CPUID_0_01_EDX_TSC |
	    CPUID_0_01_EDX_MSR |
	    CPUID_0_01_EDX_PAE |
	    /* CPUID_0_01_EDX_MCE excluded */
	    CPUID_0_01_EDX_CX8 |
	    CPUID_0_01_EDX_APIC |
	    CPUID_0_01_EDX_SEP |
	    /* CPUID_0_01_EDX_MTRR excluded */
	    CPUID_0_01_EDX_PGE |
	    /* CPUID_0_01_EDX_MCA excluded */
	    CPUID_0_01_EDX_CMOV |
	    CPUID_0_01_EDX_PAT |
	    CPUID_0_01_EDX_PSE36 |
	    /* CPUID_0_01_EDX_PSN excluded */
	    CPUID_0_01_EDX_CLFSH |
	    /* CPUID_0_01_EDX_DS excluded */
	    /* CPUID_0_01_EDX_ACPI excluded */
	    CPUID_0_01_EDX_MMX |
	    CPUID_0_01_EDX_FXSR |
	    CPUID_0_01_EDX_SSE |
	    CPUID_0_01_EDX_SSE2 |
	    CPUID_0_01_EDX_SS |
	    CPUID_0_01_EDX_HTT |
	    /* CPUID_0_01_EDX_TM excluded */
	    CPUID_0_01_EDX_PBE
};

const struct nvmm_x86_cpuid_mask nvmm_cpuid_00000007 = {
	.eax = ~0,
	.ebx =
	    CPUID_0_07_EBX_FSGSBASE |
	    /* CPUID_0_07_EBX_TSC_ADJUST excluded */
	    /* CPUID_0_07_EBX_SGX excluded */
	    CPUID_0_07_EBX_BMI1 |
	    /* CPUID_0_07_EBX_HLE excluded */
	    /* CPUID_0_07_EBX_AVX2 excluded */
	    CPUID_0_07_EBX_FDPEXONLY |
	    CPUID_0_07_EBX_SMEP |
	    CPUID_0_07_EBX_BMI2 |
	    CPUID_0_07_EBX_ERMS |
	    /* CPUID_0_07_EBX_INVPCID excluded, but re-included in VMX */
	    /* CPUID_0_07_EBX_RTM excluded */
	    /* CPUID_0_07_EBX_QM excluded */
	    CPUID_0_07_EBX_FPUCSDS |
	    /* CPUID_0_07_EBX_MPX excluded */
	    /* CPUID_0_07_EBX_PQE excluded */
	    /* CPUID_0_07_EBX_AVX512F excluded */
	    /* CPUID_0_07_EBX_AVX512DQ excluded */
	    CPUID_0_07_EBX_RDSEED |
	    CPUID_0_07_EBX_ADX |
	    CPUID_0_07_EBX_SMAP |
	    /* CPUID_0_07_EBX_AVX512_IFMA excluded */
	    CPUID_0_07_EBX_CLFLUSHOPT |
	    CPUID_0_07_EBX_CLWB,
	    /* CPUID_0_07_EBX_PT excluded */
	    /* CPUID_0_07_EBX_AVX512PF excluded */
	    /* CPUID_0_07_EBX_AVX512ER excluded */
	    /* CPUID_0_07_EBX_AVX512CD excluded */
	    /* CPUID_0_07_EBX_SHA excluded */
	    /* CPUID_0_07_EBX_AVX512BW excluded */
	    /* CPUID_0_07_EBX_AVX512VL excluded */
	.ecx =
	    CPUID_0_07_ECX_PREFETCHWT1 |
	    /* CPUID_0_07_ECX_AVX512_VBMI excluded */
	    CPUID_0_07_ECX_UMIP |
	    /* CPUID_0_07_ECX_PKU excluded */
	    /* CPUID_0_07_ECX_OSPKE excluded */
	    /* CPUID_0_07_ECX_WAITPKG excluded */
	    /* CPUID_0_07_ECX_AVX512_VBMI2 excluded */
	    /* CPUID_0_07_ECX_CET_SS excluded */
	    CPUID_0_07_ECX_GFNI |
	    CPUID_0_07_ECX_VAES |
	    CPUID_0_07_ECX_VPCLMULQDQ |
	    /* CPUID_0_07_ECX_AVX512_VNNI excluded */
	    /* CPUID_0_07_ECX_AVX512_BITALG excluded */
	    /* CPUID_0_07_ECX_AVX512_VPOPCNTDQ excluded */
	    /* CPUID_0_07_ECX_LA57 excluded */
	    /* CPUID_0_07_ECX_MAWAU excluded */
	    /* CPUID_0_07_ECX_RDPID excluded */
	    CPUID_0_07_ECX_CLDEMOTE |
	    CPUID_0_07_ECX_MOVDIRI |
	    CPUID_0_07_ECX_MOVDIR64B,
	    /* CPUID_0_07_ECX_SGXLC excluded */
	    /* CPUID_0_07_ECX_PKS excluded */
	.edx =
	    /* CPUID_0_07_EDX_AVX512_4VNNIW excluded */
	    /* CPUID_0_07_EDX_AVX512_4FMAPS excluded */
	    CPUID_0_07_EDX_FSREP_MOV |
	    /* CPUID_0_07_EDX_AVX512_VP2INTERSECT excluded */
	    /* CPUID_0_07_EDX_SRBDS_CTRL excluded */
	    CPUID_0_07_EDX_MD_CLEAR |
	    /* CPUID_0_07_EDX_TSX_FORCE_ABORT excluded */
	    CPUID_0_07_EDX_SERIALIZE |
	    /* CPUID_0_07_EDX_HYBRID excluded */
	    /* CPUID_0_07_EDX_TSXLDTRK excluded */
	    /* CPUID_0_07_EDX_CET_IBT excluded */
	    /* CPUID_0_07_EDX_IBRS excluded */
	    /* CPUID_0_07_EDX_STIBP excluded */
	    /* CPUID_0_07_EDX_L1D_FLUSH excluded */
	    CPUID_0_07_EDX_ARCH_CAP
	    /* CPUID_0_07_EDX_CORE_CAP excluded */
	    /* CPUID_0_07_EDX_SSBD excluded */
};

const struct nvmm_x86_cpuid_mask nvmm_cpuid_80000001 = {
	.eax = ~0,
	.ebx = ~0,
	.ecx =
	    CPUID_8_01_ECX_LAHF |
	    CPUID_8_01_ECX_CMPLEGACY |
	    /* CPUID_8_01_ECX_SVM excluded */
	    /* CPUID_8_01_ECX_EAPIC excluded */
	    CPUID_8_01_ECX_ALTMOVCR8 |
	    CPUID_8_01_ECX_ABM |
	    CPUID_8_01_ECX_SSE4A |
	    CPUID_8_01_ECX_MISALIGNSSE |
	    CPUID_8_01_ECX_3DNOWPF |
	    /* CPUID_8_01_ECX_OSVW excluded */
	    /* CPUID_8_01_ECX_IBS excluded */
	    CPUID_8_01_ECX_XOP |
	    /* CPUID_8_01_ECX_SKINIT excluded */
	    /* CPUID_8_01_ECX_WDT excluded */
	    /* CPUID_8_01_ECX_LWP excluded */
	    CPUID_8_01_ECX_FMA4 |
	    CPUID_8_01_ECX_TCE |
	    /* CPUID_8_01_ECX_NODEID excluded */
	    CPUID_8_01_ECX_TBM |
	    CPUID_8_01_ECX_TOPOEXT,
	    /* CPUID_8_01_ECX_PCEC excluded */
	    /* CPUID_8_01_ECX_PCENB excluded */
	    /* CPUID_8_01_ECX_DBE excluded */
	    /* CPUID_8_01_ECX_PERFTSC excluded */
	    /* CPUID_8_01_ECX_PERFEXTLLC excluded */
	    /* CPUID_8_01_ECX_MWAITX excluded */
	.edx =
	    CPUID_8_01_EDX_FPU |
	    CPUID_8_01_EDX_VME |
	    CPUID_8_01_EDX_DE |
	    CPUID_8_01_EDX_PSE |
	    CPUID_8_01_EDX_TSC |
	    CPUID_8_01_EDX_MSR |
	    CPUID_8_01_EDX_PAE |
	    /* CPUID_8_01_EDX_MCE excluded */
	    CPUID_8_01_EDX_CX8 |
	    CPUID_8_01_EDX_APIC |
	    CPUID_8_01_EDX_SYSCALL |
	    /* CPUID_8_01_EDX_MTRR excluded */
	    CPUID_8_01_EDX_PGE |
	    /* CPUID_8_01_EDX_MCA excluded */
	    CPUID_8_01_EDX_CMOV |
	    CPUID_8_01_EDX_PAT |
	    CPUID_8_01_EDX_PSE36 |
	    CPUID_8_01_EDX_XD |
	    CPUID_8_01_EDX_MMXEXT |
	    CPUID_8_01_EDX_MMX |
	    CPUID_8_01_EDX_FXSR |
	    CPUID_8_01_EDX_FFXSR |
	    CPUID_8_01_EDX_PAGE1GB |
	    /* CPUID_8_01_EDX_RDTSCP excluded */
	    CPUID_8_01_EDX_LM |
	    CPUID_8_01_EDX_3DNOWEXT |
	    CPUID_8_01_EDX_3DNOW
};

const struct nvmm_x86_cpuid_mask nvmm_cpuid_80000007 = {
	.eax = 0,
	.ebx = 0,
	.ecx = 0,
	.edx =
	    /* CPUID_8_07_EDX_TS excluded */
	    /* CPUID_8_07_EDX_FID excluded */
	    /* CPUID_8_07_EDX_VID excluded */
	    /* CPUID_8_07_EDX_TTP excluded */
	    /* CPUID_8_07_EDX_TM excluded */
	    /* CPUID_8_07_EDX_100MHzSteps excluded */
	    /* CPUID_8_07_EDX_HwPstate excluded */
	    CPUID_8_07_EDX_TscInvariant,
	    /* CPUID_8_07_EDX_CPB excluded */
	    /* CPUID_8_07_EDX_EffFreqRO excluded */
	    /* CPUID_8_07_EDX_ProcFeedbackIntf excluded */
	    /* CPUID_8_07_EDX_ProcPowerReport excluded */
};

const struct nvmm_x86_cpuid_mask nvmm_cpuid_80000008 = {
	.eax = ~0,
	.ebx =
	    CPUID_8_08_EBX_CLZERO |
	    /* CPUID_8_08_EBX_InstRetCntMsr excluded */
	    CPUID_8_08_EBX_RstrFpErrPtrs |
	    /* CPUID_8_08_EBX_INVLPGB excluded */
	    /* CPUID_8_08_EBX_RDPRU excluded */
	    /* CPUID_8_08_EBX_MCOMMIT excluded */
	    CPUID_8_08_EBX_WBNOINVD,
	    /* CPUID_8_08_EBX_IBPB excluded */
	    /* CPUID_8_08_EBX_INT_WBINVD excluded */
	    /* CPUID_8_08_EBX_IBRS excluded */
	    /* CPUID_8_08_EBX_EferLmsleUnsupp excluded */
	    /* CPUID_8_08_EBX_INVLPGBnestedPg excluded */
	    /* CPUID_8_08_EBX_STIBP excluded */
	    /* CPUID_8_08_EBX_IBRS_ALWAYSON excluded */
	    /* CPUID_8_08_EBX_STIBP_ALWAYSON excluded */
	    /* CPUID_8_08_EBX_PREFER_IBRS excluded */
	    /* CPUID_8_08_EBX_SSBD excluded */
	    /* CPUID_8_08_EBX_VIRT_SSBD excluded */
	    /* CPUID_8_08_EBX_SSB_NO excluded */
	.ecx = 0,
	.edx = 0
};

bool
nvmm_x86_pat_validate(uint64_t val)
{
	uint8_t *pat = (uint8_t *)&val;
	size_t i;

	for (i = 0; i < 8; i++) {
		if (__predict_false(pat[i] & ~__BITS(2,0)))
			return false;
		if (__predict_false(pat[i] == 2 || pat[i] == 3))
			return false;
	}

	return true;
}

uint32_t
nvmm_x86_xsave_size(uint64_t xcr0)
{
	uint32_t size;

	if (xcr0 & XCR0_SSE) {
		size = 512; /* x87 + SSE */
	} else {
		size = 108; /* x87 */
	}
	size += 64; /* XSAVE header */

	return size;
}
