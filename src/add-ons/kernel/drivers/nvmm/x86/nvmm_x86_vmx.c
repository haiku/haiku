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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mman.h>

#include "../nvmm.h"
#include "../nvmm_internal.h"
#include "nvmm_x86.h"

int vmx_vmlaunch(uint64_t *gprs);
int vmx_vmresume(uint64_t *gprs);
void vmx_resume_rip(void);

struct ept_desc {
	uint64_t eptp;
	uint64_t mbz;
} __packed;

struct vpid_desc {
	uint64_t vpid;
	uint64_t addr;
} __packed;

static inline void
vmx_vmxon(paddr_t *pa)
{
	__asm volatile (
		"vmxon		%[pa];"
		"jz		vmx_insn_failvalid;"
		"jc		vmx_insn_failinvalid;"
		:
		: [pa] "m" (*pa)
		: "memory", "cc"
	);
}

static inline void
vmx_vmxoff(void)
{
	__asm volatile (
		"vmxoff;"
		"jz		vmx_insn_failvalid;"
		"jc		vmx_insn_failinvalid;"
		:
		:
		: "memory", "cc"
	);
}

static inline void
vmx_invept(uint64_t op, struct ept_desc *desc)
{
	__asm volatile (
		"invept		%[desc],%[op];"
		"jz		vmx_insn_failvalid;"
		"jc		vmx_insn_failinvalid;"
		:
		: [desc] "m" (*desc), [op] "r" (op)
		: "memory", "cc"
	);
}

static inline void
vmx_invvpid(uint64_t op, struct vpid_desc *desc)
{
	__asm volatile (
		"invvpid	%[desc],%[op];"
		"jz		vmx_insn_failvalid;"
		"jc		vmx_insn_failinvalid;"
		:
		: [desc] "m" (*desc), [op] "r" (op)
		: "memory", "cc"
	);
}

static inline uint64_t
vmx_vmread(uint64_t field)
{
	uint64_t value;

	__asm volatile (
		"vmread		%[field],%[value];"
		"jz		vmx_insn_failvalid;"
		"jc		vmx_insn_failinvalid;"
		: [value] "=r" (value)
		: [field] "r" (field)
		: "cc"
	);

	return value;
}

static inline void
vmx_vmwrite(uint64_t field, uint64_t value)
{
	__asm volatile (
		"vmwrite	%[value],%[field];"
		"jz		vmx_insn_failvalid;"
		"jc		vmx_insn_failinvalid;"
		:
		: [field] "r" (field), [value] "r" (value)
		: "cc"
	);
}

static inline paddr_t __unused
vmx_vmptrst(void)
{
	paddr_t pa;

	__asm volatile (
		"vmptrst	%[pa];"
		:
		: [pa] "m" (*(paddr_t *)&pa)
		: "memory"
	);

	return pa;
}

static inline void
vmx_vmptrld(paddr_t *pa)
{
	__asm volatile (
		"vmptrld	%[pa];"
		"jz		vmx_insn_failvalid;"
		"jc		vmx_insn_failinvalid;"
		:
		: [pa] "m" (*pa)
		: "memory", "cc"
	);
}

static inline void
vmx_vmclear(paddr_t *pa)
{
	__asm volatile (
		"vmclear	%[pa];"
		"jz		vmx_insn_failvalid;"
		"jc		vmx_insn_failinvalid;"
		:
		: [pa] "m" (*pa)
		: "memory", "cc"
	);
}

static inline void
vmx_cli(void)
{
	__asm volatile ("cli" ::: "memory");
}

static inline void
vmx_sti(void)
{
	__asm volatile ("sti" ::: "memory");
}

#define MSR_IA32_FEATURE_CONTROL	0x003A
#define		IA32_FEATURE_CONTROL_LOCK	__BIT(0)
#define		IA32_FEATURE_CONTROL_IN_SMX	__BIT(1)
#define		IA32_FEATURE_CONTROL_OUT_SMX	__BIT(2)

#define MSR_IA32_VMX_BASIC		0x0480
#define		IA32_VMX_BASIC_IDENT		__BITS(30,0)
#define		IA32_VMX_BASIC_DATA_SIZE	__BITS(44,32)
#define		IA32_VMX_BASIC_MEM_WIDTH	__BIT(48)
#define		IA32_VMX_BASIC_DUAL		__BIT(49)
#define		IA32_VMX_BASIC_MEM_TYPE		__BITS(53,50)
#define			MEM_TYPE_UC		0
#define			MEM_TYPE_WB		6
#define		IA32_VMX_BASIC_IO_REPORT	__BIT(54)
#define		IA32_VMX_BASIC_TRUE_CTLS	__BIT(55)

#define MSR_IA32_VMX_PINBASED_CTLS		0x0481
#define MSR_IA32_VMX_PROCBASED_CTLS		0x0482
#define MSR_IA32_VMX_EXIT_CTLS			0x0483
#define MSR_IA32_VMX_ENTRY_CTLS			0x0484
#define MSR_IA32_VMX_PROCBASED_CTLS2		0x048B

#define MSR_IA32_VMX_TRUE_PINBASED_CTLS		0x048D
#define MSR_IA32_VMX_TRUE_PROCBASED_CTLS	0x048E
#define MSR_IA32_VMX_TRUE_EXIT_CTLS		0x048F
#define MSR_IA32_VMX_TRUE_ENTRY_CTLS		0x0490

#define MSR_IA32_VMX_CR0_FIXED0			0x0486
#define MSR_IA32_VMX_CR0_FIXED1			0x0487
#define MSR_IA32_VMX_CR4_FIXED0			0x0488
#define MSR_IA32_VMX_CR4_FIXED1			0x0489

#define MSR_IA32_VMX_EPT_VPID_CAP	0x048C
#define		IA32_VMX_EPT_VPID_XO			__BIT(0)
#define		IA32_VMX_EPT_VPID_WALKLENGTH_4		__BIT(6)
#define		IA32_VMX_EPT_VPID_UC			__BIT(8)
#define		IA32_VMX_EPT_VPID_WB			__BIT(14)
#define		IA32_VMX_EPT_VPID_2MB			__BIT(16)
#define		IA32_VMX_EPT_VPID_1GB			__BIT(17)
#define		IA32_VMX_EPT_VPID_INVEPT		__BIT(20)
#define		IA32_VMX_EPT_VPID_FLAGS_AD		__BIT(21)
#define		IA32_VMX_EPT_VPID_ADVANCED_VMEXIT_INFO	__BIT(22)
#define		IA32_VMX_EPT_VPID_SHSTK			__BIT(23)
#define		IA32_VMX_EPT_VPID_INVEPT_CONTEXT	__BIT(25)
#define		IA32_VMX_EPT_VPID_INVEPT_ALL		__BIT(26)
#define		IA32_VMX_EPT_VPID_INVVPID		__BIT(32)
#define		IA32_VMX_EPT_VPID_INVVPID_ADDR		__BIT(40)
#define		IA32_VMX_EPT_VPID_INVVPID_CONTEXT	__BIT(41)
#define		IA32_VMX_EPT_VPID_INVVPID_ALL		__BIT(42)
#define		IA32_VMX_EPT_VPID_INVVPID_CONTEXT_NOG	__BIT(43)

/* -------------------------------------------------------------------------- */

/* 16-bit control fields */
#define VMCS_VPID				0x00000000
#define VMCS_PIR_VECTOR				0x00000002
#define VMCS_EPTP_INDEX				0x00000004
/* 16-bit guest-state fields */
#define VMCS_GUEST_ES_SELECTOR			0x00000800
#define VMCS_GUEST_CS_SELECTOR			0x00000802
#define VMCS_GUEST_SS_SELECTOR			0x00000804
#define VMCS_GUEST_DS_SELECTOR			0x00000806
#define VMCS_GUEST_FS_SELECTOR			0x00000808
#define VMCS_GUEST_GS_SELECTOR			0x0000080A
#define VMCS_GUEST_LDTR_SELECTOR		0x0000080C
#define VMCS_GUEST_TR_SELECTOR			0x0000080E
#define VMCS_GUEST_INTR_STATUS			0x00000810
#define VMCS_PML_INDEX				0x00000812
/* 16-bit host-state fields */
#define VMCS_HOST_ES_SELECTOR			0x00000C00
#define VMCS_HOST_CS_SELECTOR			0x00000C02
#define VMCS_HOST_SS_SELECTOR			0x00000C04
#define VMCS_HOST_DS_SELECTOR			0x00000C06
#define VMCS_HOST_FS_SELECTOR			0x00000C08
#define VMCS_HOST_GS_SELECTOR			0x00000C0A
#define VMCS_HOST_TR_SELECTOR			0x00000C0C
/* 64-bit control fields */
#define VMCS_IO_BITMAP_A			0x00002000
#define VMCS_IO_BITMAP_B			0x00002002
#define VMCS_MSR_BITMAP				0x00002004
#define VMCS_EXIT_MSR_STORE_ADDRESS		0x00002006
#define VMCS_EXIT_MSR_LOAD_ADDRESS		0x00002008
#define VMCS_ENTRY_MSR_LOAD_ADDRESS		0x0000200A
#define VMCS_EXECUTIVE_VMCS			0x0000200C
#define VMCS_PML_ADDRESS			0x0000200E
#define VMCS_TSC_OFFSET				0x00002010
#define VMCS_VIRTUAL_APIC			0x00002012
#define VMCS_APIC_ACCESS			0x00002014
#define VMCS_PIR_DESC				0x00002016
#define VMCS_VM_CONTROL				0x00002018
#define VMCS_EPTP				0x0000201A
#define		EPTP_TYPE			__BITS(2,0)
#define			EPTP_TYPE_UC		0
#define			EPTP_TYPE_WB		6
#define		EPTP_WALKLEN			__BITS(5,3)
#define		EPTP_FLAGS_AD			__BIT(6)
#define		EPTP_SSS			__BIT(7)
#define		EPTP_PHYSADDR			__BITS(63,12)
#define VMCS_EOI_EXIT0				0x0000201C
#define VMCS_EOI_EXIT1				0x0000201E
#define VMCS_EOI_EXIT2				0x00002020
#define VMCS_EOI_EXIT3				0x00002022
#define VMCS_EPTP_LIST				0x00002024
#define VMCS_VMREAD_BITMAP			0x00002026
#define VMCS_VMWRITE_BITMAP			0x00002028
#define VMCS_VIRTUAL_EXCEPTION			0x0000202A
#define VMCS_XSS_EXIT_BITMAP			0x0000202C
#define VMCS_ENCLS_EXIT_BITMAP			0x0000202E
#define VMCS_SUBPAGE_PERM_TABLE_PTR		0x00002030
#define VMCS_TSC_MULTIPLIER			0x00002032
#define VMCS_ENCLV_EXIT_BITMAP			0x00002036
/* 64-bit read-only fields */
#define VMCS_GUEST_PHYSICAL_ADDRESS		0x00002400
/* 64-bit guest-state fields */
#define VMCS_LINK_POINTER			0x00002800
#define VMCS_GUEST_IA32_DEBUGCTL		0x00002802
#define VMCS_GUEST_IA32_PAT			0x00002804
#define VMCS_GUEST_IA32_EFER			0x00002806
#define VMCS_GUEST_IA32_PERF_GLOBAL_CTRL	0x00002808
#define VMCS_GUEST_PDPTE0			0x0000280A
#define VMCS_GUEST_PDPTE1			0x0000280C
#define VMCS_GUEST_PDPTE2			0x0000280E
#define VMCS_GUEST_PDPTE3			0x00002810
#define VMCS_GUEST_BNDCFGS			0x00002812
#define VMCS_GUEST_RTIT_CTL			0x00002814
#define VMCS_GUEST_PKRS				0x00002818
/* 64-bit host-state fields */
#define VMCS_HOST_IA32_PAT			0x00002C00
#define VMCS_HOST_IA32_EFER			0x00002C02
#define VMCS_HOST_IA32_PERF_GLOBAL_CTRL		0x00002C04
#define VMCS_HOST_IA32_PKRS			0x00002C06
/* 32-bit control fields */
#define VMCS_PINBASED_CTLS			0x00004000
#define		PIN_CTLS_INT_EXITING		__BIT(0)
#define		PIN_CTLS_NMI_EXITING		__BIT(3)
#define		PIN_CTLS_VIRTUAL_NMIS		__BIT(5)
#define		PIN_CTLS_ACTIVATE_PREEMPT_TIMER	__BIT(6)
#define		PIN_CTLS_PROCESS_POSTED_INTS	__BIT(7)
#define VMCS_PROCBASED_CTLS			0x00004002
#define		PROC_CTLS_INT_WINDOW_EXITING	__BIT(2)
#define		PROC_CTLS_USE_TSC_OFFSETTING	__BIT(3)
#define		PROC_CTLS_HLT_EXITING		__BIT(7)
#define		PROC_CTLS_INVLPG_EXITING	__BIT(9)
#define		PROC_CTLS_MWAIT_EXITING		__BIT(10)
#define		PROC_CTLS_RDPMC_EXITING		__BIT(11)
#define		PROC_CTLS_RDTSC_EXITING		__BIT(12)
#define		PROC_CTLS_RCR3_EXITING		__BIT(15)
#define		PROC_CTLS_LCR3_EXITING		__BIT(16)
#define		PROC_CTLS_RCR8_EXITING		__BIT(19)
#define		PROC_CTLS_LCR8_EXITING		__BIT(20)
#define		PROC_CTLS_USE_TPR_SHADOW	__BIT(21)
#define		PROC_CTLS_NMI_WINDOW_EXITING	__BIT(22)
#define		PROC_CTLS_DR_EXITING		__BIT(23)
#define		PROC_CTLS_UNCOND_IO_EXITING	__BIT(24)
#define		PROC_CTLS_USE_IO_BITMAPS	__BIT(25)
#define		PROC_CTLS_MONITOR_TRAP_FLAG	__BIT(27)
#define		PROC_CTLS_USE_MSR_BITMAPS	__BIT(28)
#define		PROC_CTLS_MONITOR_EXITING	__BIT(29)
#define		PROC_CTLS_PAUSE_EXITING		__BIT(30)
#define		PROC_CTLS_ACTIVATE_CTLS2	__BIT(31)
#define VMCS_EXCEPTION_BITMAP			0x00004004
#define VMCS_PF_ERROR_MASK			0x00004006
#define VMCS_PF_ERROR_MATCH			0x00004008
#define VMCS_CR3_TARGET_COUNT			0x0000400A
#define VMCS_EXIT_CTLS				0x0000400C
#define		EXIT_CTLS_SAVE_DEBUG_CONTROLS	__BIT(2)
#define		EXIT_CTLS_HOST_LONG_MODE	__BIT(9)
#define		EXIT_CTLS_LOAD_PERFGLOBALCTRL	__BIT(12)
#define		EXIT_CTLS_ACK_INTERRUPT		__BIT(15)
#define		EXIT_CTLS_SAVE_PAT		__BIT(18)
#define		EXIT_CTLS_LOAD_PAT		__BIT(19)
#define		EXIT_CTLS_SAVE_EFER		__BIT(20)
#define		EXIT_CTLS_LOAD_EFER		__BIT(21)
#define		EXIT_CTLS_SAVE_PREEMPT_TIMER	__BIT(22)
#define		EXIT_CTLS_CLEAR_BNDCFGS		__BIT(23)
#define		EXIT_CTLS_CONCEAL_PT		__BIT(24)
#define		EXIT_CTLS_CLEAR_RTIT_CTL	__BIT(25)
#define		EXIT_CTLS_LOAD_CET		__BIT(28)
#define		EXIT_CTLS_LOAD_PKRS		__BIT(29)
#define VMCS_EXIT_MSR_STORE_COUNT		0x0000400E
#define VMCS_EXIT_MSR_LOAD_COUNT		0x00004010
#define VMCS_ENTRY_CTLS				0x00004012
#define		ENTRY_CTLS_LOAD_DEBUG_CONTROLS	__BIT(2)
#define		ENTRY_CTLS_LONG_MODE		__BIT(9)
#define		ENTRY_CTLS_SMM			__BIT(10)
#define		ENTRY_CTLS_DISABLE_DUAL		__BIT(11)
#define		ENTRY_CTLS_LOAD_PERFGLOBALCTRL	__BIT(13)
#define		ENTRY_CTLS_LOAD_PAT		__BIT(14)
#define		ENTRY_CTLS_LOAD_EFER		__BIT(15)
#define		ENTRY_CTLS_LOAD_BNDCFGS		__BIT(16)
#define		ENTRY_CTLS_CONCEAL_PT		__BIT(17)
#define		ENTRY_CTLS_LOAD_RTIT_CTL	__BIT(18)
#define		ENTRY_CTLS_LOAD_CET		__BIT(20)
#define		ENTRY_CTLS_LOAD_PKRS		__BIT(22)
#define VMCS_ENTRY_MSR_LOAD_COUNT		0x00004014
#define VMCS_ENTRY_INTR_INFO			0x00004016
#define		INTR_INFO_VECTOR		__BITS(7,0)
#define		INTR_INFO_TYPE			__BITS(10,8)
#define			INTR_TYPE_EXT_INT	0
#define			INTR_TYPE_NMI		2
#define			INTR_TYPE_HW_EXC	3
#define			INTR_TYPE_SW_INT	4
#define			INTR_TYPE_PRIV_SW_EXC	5
#define			INTR_TYPE_SW_EXC	6
#define			INTR_TYPE_OTHER		7
#define		INTR_INFO_ERROR			__BIT(11)
#define		INTR_INFO_VALID			__BIT(31)
#define VMCS_ENTRY_EXCEPTION_ERROR		0x00004018
#define VMCS_ENTRY_INSTRUCTION_LENGTH		0x0000401A
#define VMCS_TPR_THRESHOLD			0x0000401C
#define VMCS_PROCBASED_CTLS2			0x0000401E
#define		PROC_CTLS2_VIRT_APIC_ACCESSES	__BIT(0)
#define		PROC_CTLS2_ENABLE_EPT		__BIT(1)
#define		PROC_CTLS2_DESC_TABLE_EXITING	__BIT(2)
#define		PROC_CTLS2_ENABLE_RDTSCP	__BIT(3)
#define		PROC_CTLS2_VIRT_X2APIC		__BIT(4)
#define		PROC_CTLS2_ENABLE_VPID		__BIT(5)
#define		PROC_CTLS2_WBINVD_EXITING	__BIT(6)
#define		PROC_CTLS2_UNRESTRICTED_GUEST	__BIT(7)
#define		PROC_CTLS2_APIC_REG_VIRT	__BIT(8)
#define		PROC_CTLS2_VIRT_INT_DELIVERY	__BIT(9)
#define		PROC_CTLS2_PAUSE_LOOP_EXITING	__BIT(10)
#define		PROC_CTLS2_RDRAND_EXITING	__BIT(11)
#define		PROC_CTLS2_INVPCID_ENABLE	__BIT(12)
#define		PROC_CTLS2_VMFUNC_ENABLE	__BIT(13)
#define		PROC_CTLS2_VMCS_SHADOWING	__BIT(14)
#define		PROC_CTLS2_ENCLS_EXITING	__BIT(15)
#define		PROC_CTLS2_RDSEED_EXITING	__BIT(16)
#define		PROC_CTLS2_PML_ENABLE		__BIT(17)
#define		PROC_CTLS2_EPT_VIOLATION	__BIT(18)
#define		PROC_CTLS2_CONCEAL_VMX_FROM_PT	__BIT(19)
#define		PROC_CTLS2_XSAVES_ENABLE	__BIT(20)
#define		PROC_CTLS2_MODE_BASED_EXEC_EPT	__BIT(22)
#define		PROC_CTLS2_SUBPAGE_PERMISSIONS	__BIT(23)
#define		PROC_CTLS2_PT_USES_GPA		__BIT(24)
#define		PROC_CTLS2_USE_TSC_SCALING	__BIT(25)
#define		PROC_CTLS2_WAIT_PAUSE_ENABLE	__BIT(26)
#define		PROC_CTLS2_ENCLV_EXITING	__BIT(28)
#define VMCS_PLE_GAP				0x00004020
#define VMCS_PLE_WINDOW				0x00004022
/* 32-bit read-only data fields */
#define VMCS_INSTRUCTION_ERROR			0x00004400
#define VMCS_EXIT_REASON			0x00004402
#define VMCS_EXIT_INTR_INFO			0x00004404
#define VMCS_EXIT_INTR_ERRCODE			0x00004406
#define VMCS_IDT_VECTORING_INFO			0x00004408
#define VMCS_IDT_VECTORING_ERROR		0x0000440A
#define VMCS_EXIT_INSTRUCTION_LENGTH		0x0000440C
#define VMCS_EXIT_INSTRUCTION_INFO		0x0000440E
/* 32-bit guest-state fields */
#define VMCS_GUEST_ES_LIMIT			0x00004800
#define VMCS_GUEST_CS_LIMIT			0x00004802
#define VMCS_GUEST_SS_LIMIT			0x00004804
#define VMCS_GUEST_DS_LIMIT			0x00004806
#define VMCS_GUEST_FS_LIMIT			0x00004808
#define VMCS_GUEST_GS_LIMIT			0x0000480A
#define VMCS_GUEST_LDTR_LIMIT			0x0000480C
#define VMCS_GUEST_TR_LIMIT			0x0000480E
#define VMCS_GUEST_GDTR_LIMIT			0x00004810
#define VMCS_GUEST_IDTR_LIMIT			0x00004812
#define VMCS_GUEST_ES_ACCESS_RIGHTS		0x00004814
#define VMCS_GUEST_CS_ACCESS_RIGHTS		0x00004816
#define VMCS_GUEST_SS_ACCESS_RIGHTS		0x00004818
#define VMCS_GUEST_DS_ACCESS_RIGHTS		0x0000481A
#define VMCS_GUEST_FS_ACCESS_RIGHTS		0x0000481C
#define VMCS_GUEST_GS_ACCESS_RIGHTS		0x0000481E
#define VMCS_GUEST_LDTR_ACCESS_RIGHTS		0x00004820
#define VMCS_GUEST_TR_ACCESS_RIGHTS		0x00004822
#define VMCS_GUEST_INTERRUPTIBILITY		0x00004824
#define		INT_STATE_STI			__BIT(0)
#define		INT_STATE_MOVSS			__BIT(1)
#define		INT_STATE_SMI			__BIT(2)
#define		INT_STATE_NMI			__BIT(3)
#define		INT_STATE_ENCLAVE		__BIT(4)
#define VMCS_GUEST_ACTIVITY			0x00004826
#define VMCS_GUEST_SMBASE			0x00004828
#define VMCS_GUEST_IA32_SYSENTER_CS		0x0000482A
#define VMCS_PREEMPTION_TIMER_VALUE		0x0000482E
/* 32-bit host state fields */
#define VMCS_HOST_IA32_SYSENTER_CS		0x00004C00
/* Natural-Width control fields */
#define VMCS_CR0_MASK				0x00006000
#define VMCS_CR4_MASK				0x00006002
#define VMCS_CR0_SHADOW				0x00006004
#define VMCS_CR4_SHADOW				0x00006006
#define VMCS_CR3_TARGET0			0x00006008
#define VMCS_CR3_TARGET1			0x0000600A
#define VMCS_CR3_TARGET2			0x0000600C
#define VMCS_CR3_TARGET3			0x0000600E
/* Natural-Width read-only fields */
#define VMCS_EXIT_QUALIFICATION			0x00006400
#define VMCS_IO_RCX				0x00006402
#define VMCS_IO_RSI				0x00006404
#define VMCS_IO_RDI				0x00006406
#define VMCS_IO_RIP				0x00006408
#define VMCS_GUEST_LINEAR_ADDRESS		0x0000640A
/* Natural-Width guest-state fields */
#define VMCS_GUEST_CR0				0x00006800
#define VMCS_GUEST_CR3				0x00006802
#define VMCS_GUEST_CR4				0x00006804
#define VMCS_GUEST_ES_BASE			0x00006806
#define VMCS_GUEST_CS_BASE			0x00006808
#define VMCS_GUEST_SS_BASE			0x0000680A
#define VMCS_GUEST_DS_BASE			0x0000680C
#define VMCS_GUEST_FS_BASE			0x0000680E
#define VMCS_GUEST_GS_BASE			0x00006810
#define VMCS_GUEST_LDTR_BASE			0x00006812
#define VMCS_GUEST_TR_BASE			0x00006814
#define VMCS_GUEST_GDTR_BASE			0x00006816
#define VMCS_GUEST_IDTR_BASE			0x00006818
#define VMCS_GUEST_DR7				0x0000681A
#define VMCS_GUEST_RSP				0x0000681C
#define VMCS_GUEST_RIP				0x0000681E
#define VMCS_GUEST_RFLAGS			0x00006820
#define VMCS_GUEST_PENDING_DBG_EXCEPTIONS	0x00006822
#define VMCS_GUEST_IA32_SYSENTER_ESP		0x00006824
#define VMCS_GUEST_IA32_SYSENTER_EIP		0x00006826
#define VMCS_GUEST_IA32_S_CET			0x00006828
#define VMCS_GUEST_SSP				0x0000682A
#define VMCS_GUEST_IA32_INTR_SSP_TABLE		0x0000682C
/* Natural-Width host-state fields */
#define VMCS_HOST_CR0				0x00006C00
#define VMCS_HOST_CR3				0x00006C02
#define VMCS_HOST_CR4				0x00006C04
#define VMCS_HOST_FS_BASE			0x00006C06
#define VMCS_HOST_GS_BASE			0x00006C08
#define VMCS_HOST_TR_BASE			0x00006C0A
#define VMCS_HOST_GDTR_BASE			0x00006C0C
#define VMCS_HOST_IDTR_BASE			0x00006C0E
#define VMCS_HOST_IA32_SYSENTER_ESP		0x00006C10
#define VMCS_HOST_IA32_SYSENTER_EIP		0x00006C12
#define VMCS_HOST_RSP				0x00006C14
#define VMCS_HOST_RIP				0x00006C16
#define VMCS_HOST_IA32_S_CET			0x00006C18
#define VMCS_HOST_SSP				0x00006C1A
#define VMCS_HOST_IA32_INTR_SSP_TABLE		0x00006C1C

/* VMX basic exit reasons. */
#define VMCS_EXITCODE_EXC_NMI			0
#define VMCS_EXITCODE_EXT_INT			1
#define VMCS_EXITCODE_SHUTDOWN			2
#define VMCS_EXITCODE_INIT			3
#define VMCS_EXITCODE_SIPI			4
#define VMCS_EXITCODE_SMI			5
#define VMCS_EXITCODE_OTHER_SMI			6
#define VMCS_EXITCODE_INT_WINDOW		7
#define VMCS_EXITCODE_NMI_WINDOW		8
#define VMCS_EXITCODE_TASK_SWITCH		9
#define VMCS_EXITCODE_CPUID			10
#define VMCS_EXITCODE_GETSEC			11
#define VMCS_EXITCODE_HLT			12
#define VMCS_EXITCODE_INVD			13
#define VMCS_EXITCODE_INVLPG			14
#define VMCS_EXITCODE_RDPMC			15
#define VMCS_EXITCODE_RDTSC			16
#define VMCS_EXITCODE_RSM			17
#define VMCS_EXITCODE_VMCALL			18
#define VMCS_EXITCODE_VMCLEAR			19
#define VMCS_EXITCODE_VMLAUNCH			20
#define VMCS_EXITCODE_VMPTRLD			21
#define VMCS_EXITCODE_VMPTRST			22
#define VMCS_EXITCODE_VMREAD			23
#define VMCS_EXITCODE_VMRESUME			24
#define VMCS_EXITCODE_VMWRITE			25
#define VMCS_EXITCODE_VMXOFF			26
#define VMCS_EXITCODE_VMXON			27
#define VMCS_EXITCODE_CR			28
#define VMCS_EXITCODE_DR			29
#define VMCS_EXITCODE_IO			30
#define VMCS_EXITCODE_RDMSR			31
#define VMCS_EXITCODE_WRMSR			32
#define VMCS_EXITCODE_FAIL_GUEST_INVALID	33
#define VMCS_EXITCODE_FAIL_MSR_INVALID		34
#define VMCS_EXITCODE_MWAIT			36
#define VMCS_EXITCODE_TRAP_FLAG			37
#define VMCS_EXITCODE_MONITOR			39
#define VMCS_EXITCODE_PAUSE			40
#define VMCS_EXITCODE_FAIL_MACHINE_CHECK	41
#define VMCS_EXITCODE_TPR_BELOW			43
#define VMCS_EXITCODE_APIC_ACCESS		44
#define VMCS_EXITCODE_VEOI			45
#define VMCS_EXITCODE_GDTR_IDTR			46
#define VMCS_EXITCODE_LDTR_TR			47
#define VMCS_EXITCODE_EPT_VIOLATION		48
#define VMCS_EXITCODE_EPT_MISCONFIG		49
#define VMCS_EXITCODE_INVEPT			50
#define VMCS_EXITCODE_RDTSCP			51
#define VMCS_EXITCODE_PREEMPT_TIMEOUT		52
#define VMCS_EXITCODE_INVVPID			53
#define VMCS_EXITCODE_WBINVD			54
#define VMCS_EXITCODE_XSETBV			55
#define VMCS_EXITCODE_APIC_WRITE		56
#define VMCS_EXITCODE_RDRAND			57
#define VMCS_EXITCODE_INVPCID			58
#define VMCS_EXITCODE_VMFUNC			59
#define VMCS_EXITCODE_ENCLS			60
#define VMCS_EXITCODE_RDSEED			61
#define VMCS_EXITCODE_PAGE_LOG_FULL		62
#define VMCS_EXITCODE_XSAVES			63
#define VMCS_EXITCODE_XRSTORS			64
#define VMCS_EXITCODE_SPP			66
#define VMCS_EXITCODE_UMWAIT			67
#define VMCS_EXITCODE_TPAUSE			68

/* -------------------------------------------------------------------------- */

static void vmx_vcpu_state_provide(struct nvmm_cpu *, uint64_t);
static void vmx_vcpu_state_commit(struct nvmm_cpu *);

/*
 * These host values are static, they do not change at runtime and are the same
 * on all CPUs. We save them here because they are not saved in the VMCS.
 */
static struct {
	uint64_t xcr0;
	uint64_t star;
	uint64_t lstar;
	uint64_t cstar;
	uint64_t sfmask;
} vmx_global_hstate __cacheline_aligned;

#define VMX_MSRLIST_STAR		0
#define VMX_MSRLIST_LSTAR		1
#define VMX_MSRLIST_CSTAR		2
#define VMX_MSRLIST_SFMASK		3
#define VMX_MSRLIST_KERNELGSBASE	4
#define VMX_MSRLIST_EXIT_NMSR		5
#define VMX_MSRLIST_L1DFLUSH		5

/* On entry, we may do +1 to include L1DFLUSH. */
static size_t vmx_msrlist_entry_nmsr __read_mostly = VMX_MSRLIST_EXIT_NMSR;

struct vmxon {
	uint32_t ident;
#define VMXON_IDENT_REVISION	__BITS(30,0)

	uint8_t data[PAGE_SIZE - 4];
} __packed;

CTASSERT(sizeof(struct vmxon) == PAGE_SIZE);

struct vmxoncpu {
	vaddr_t va;
	paddr_t pa;
};

static struct vmxoncpu vmxoncpu[OS_MAXCPUS];

struct vmcs {
	uint32_t ident;
#define VMCS_IDENT_REVISION	__BITS(30,0)
#define VMCS_IDENT_SHADOW	__BIT(31)

	uint32_t abort;
	uint8_t data[PAGE_SIZE - 8];
} __packed;

CTASSERT(sizeof(struct vmcs) == PAGE_SIZE);

struct msr_entry {
	uint32_t msr;
	uint32_t rsvd;
	uint64_t val;
} __packed;

#define VPID_MAX	0xFFFF

/* Make sure we never run out of VPIDs. */
CTASSERT(VPID_MAX-1 >= NVMM_MAX_MACHINES * NVMM_MAX_VCPUS);

static uint64_t vmx_tlb_flush_op __read_mostly;
static uint64_t vmx_ept_flush_op __read_mostly;
static uint64_t vmx_eptp_type __read_mostly;
static bool vmx_ept_has_ad __read_mostly;

static uint64_t vmx_pinbased_ctls __read_mostly;
static uint64_t vmx_procbased_ctls __read_mostly;
static uint64_t vmx_procbased_ctls2 __read_mostly;
static uint64_t vmx_entry_ctls __read_mostly;
static uint64_t vmx_exit_ctls __read_mostly;

static uint64_t vmx_cr0_fixed0 __read_mostly;
static uint64_t vmx_cr0_fixed1 __read_mostly;
static uint64_t vmx_cr4_fixed0 __read_mostly;
static uint64_t vmx_cr4_fixed1 __read_mostly;

#define VMX_PINBASED_CTLS_ONE	\
	(PIN_CTLS_INT_EXITING| \
	 PIN_CTLS_NMI_EXITING| \
	 PIN_CTLS_VIRTUAL_NMIS)

#define VMX_PINBASED_CTLS_ZERO	0

#define VMX_PROCBASED_CTLS_ONE	\
	(PROC_CTLS_USE_TSC_OFFSETTING| \
	 PROC_CTLS_HLT_EXITING| \
	 PROC_CTLS_MWAIT_EXITING | \
	 PROC_CTLS_RDPMC_EXITING | \
	 PROC_CTLS_RCR8_EXITING | \
	 PROC_CTLS_LCR8_EXITING | \
	 PROC_CTLS_UNCOND_IO_EXITING | /* no I/O bitmap */ \
	 PROC_CTLS_USE_MSR_BITMAPS | \
	 PROC_CTLS_MONITOR_EXITING | \
	 PROC_CTLS_ACTIVATE_CTLS2)

#define VMX_PROCBASED_CTLS_ZERO	\
	(PROC_CTLS_RCR3_EXITING| \
	 PROC_CTLS_LCR3_EXITING)

#define VMX_PROCBASED_CTLS2_ONE	\
	(PROC_CTLS2_ENABLE_EPT| \
	 PROC_CTLS2_ENABLE_VPID| \
	 PROC_CTLS2_UNRESTRICTED_GUEST)

#define VMX_PROCBASED_CTLS2_ZERO	0

#define VMX_ENTRY_CTLS_ONE	\
	(ENTRY_CTLS_LOAD_DEBUG_CONTROLS| \
	 ENTRY_CTLS_LOAD_EFER| \
	 ENTRY_CTLS_LOAD_PAT)

#define VMX_ENTRY_CTLS_ZERO	\
	(ENTRY_CTLS_SMM| \
	 ENTRY_CTLS_DISABLE_DUAL)

#define VMX_EXIT_CTLS_ONE	\
	(EXIT_CTLS_SAVE_DEBUG_CONTROLS| \
	 EXIT_CTLS_HOST_LONG_MODE| \
	 EXIT_CTLS_SAVE_PAT| \
	 EXIT_CTLS_LOAD_PAT| \
	 EXIT_CTLS_SAVE_EFER| \
	 EXIT_CTLS_LOAD_EFER)

#define VMX_EXIT_CTLS_ZERO	0

static uint8_t *vmx_asidmap __read_mostly;
static uint32_t vmx_maxasid __read_mostly;
static os_mtx_t vmx_asidlock __cacheline_aligned;

#define VMX_XCR0_MASK_DEFAULT	(XCR0_X87|XCR0_SSE)
static uint64_t vmx_xcr0_mask __read_mostly;

#define VMX_NCPUIDS	32

#define VMCS_NPAGES	1
#define VMCS_SIZE	(VMCS_NPAGES * PAGE_SIZE)

#define MSRBM_NPAGES	1
#define MSRBM_SIZE	(MSRBM_NPAGES * PAGE_SIZE)

#define CR0_STATIC_MASK \
	(CR0_ET | CR0_NW | CR0_CD)

#define CR4_VALID \
	(CR4_VME |			\
	 CR4_PVI |			\
	 CR4_TSD |			\
	 CR4_DE |			\
	 CR4_PSE |			\
	 CR4_PAE |			\
	 CR4_MCE |			\
	 CR4_PGE |			\
	 CR4_PCE |			\
	 CR4_OSFXSR |			\
	 CR4_OSXMMEXCPT |		\
	 CR4_UMIP |			\
	 /* CR4_LA57 excluded */	\
	 /* CR4_VMXE excluded */	\
	 /* CR4_SMXE excluded */	\
	 CR4_FSGSBASE |			\
	 CR4_PCIDE |			\
	 CR4_OSXSAVE |			\
	 CR4_SMEP |			\
	 CR4_SMAP			\
	 /* CR4_PKE excluded */		\
	 /* CR4_CET excluded */		\
	 /* CR4_PKS excluded */)
#define CR4_INVALID \
	(0xFFFFFFFFFFFFFFFFULL & ~CR4_VALID)

#define EFER_TLB_FLUSH \
	(EFER_NXE|EFER_LMA|EFER_LME)
#define CR0_TLB_FLUSH \
	(CR0_PG|CR0_WP|CR0_CD|CR0_NW)
#define CR4_TLB_FLUSH \
	(CR4_PSE|CR4_PAE|CR4_PGE|CR4_PCIDE|CR4_SMEP)

/* -------------------------------------------------------------------------- */

struct vmx_machdata {
	volatile uint64_t mach_htlb_gen;
};

static const size_t vmx_vcpu_conf_sizes[NVMM_X86_VCPU_NCONF] = {
	[NVMM_VCPU_CONF_MD(NVMM_VCPU_CONF_CPUID)] =
	    sizeof(struct nvmm_vcpu_conf_cpuid),
	[NVMM_VCPU_CONF_MD(NVMM_VCPU_CONF_TPR)] =
	    sizeof(struct nvmm_vcpu_conf_tpr)
};

struct vmx_cpudata {
	/* General. */
	uint64_t asid;
	bool gtlb_want_flush;
	bool gtsc_want_update;
	uint64_t vcpu_htlb_gen;
	os_cpuset_t *htlb_want_flush;

	/* VMCS. */
	struct vmcs *vmcs;
	paddr_t vmcs_pa;
	size_t vmcs_refcnt;
	os_cpu_t *vmcs_cpu;
	bool vmcs_launched;

	/* MSR bitmap. */
	uint8_t *msrbm;
	paddr_t msrbm_pa;

	/* Percpu host state, absent from VMCS. */
	struct {
		uint64_t kernelgsbase;
		uint64_t drs[NVMM_X64_NDR];
#ifdef __DragonFly__
		mcontext_t hmctx;  /* TODO: remove this like NetBSD */
#endif
	} hstate;

	/* Intr state. */
	bool int_window_exit;
	bool nmi_window_exit;
	bool evt_pending;

	/* Guest state. */
	struct msr_entry *gmsr;
	paddr_t gmsr_pa;
	uint64_t gmsr_misc_enable;
	uint64_t gcr2;
	uint64_t gcr8;
	uint64_t gxcr0;
	uint64_t gprs[NVMM_X64_NGPR];
	uint64_t drs[NVMM_X64_NDR];
	uint64_t gtsc_offset;
	uint64_t gtsc_match;
	struct nvmm_x86_xsave gxsave __aligned(64);

	/* VCPU configuration. */
	bool cpuidpresent[VMX_NCPUIDS];
	struct nvmm_vcpu_conf_cpuid cpuid[VMX_NCPUIDS];
	struct nvmm_vcpu_conf_tpr tpr;
};

static const struct {
	uint64_t selector;
	uint64_t attrib;
	uint64_t limit;
	uint64_t base;
} vmx_guest_segs[NVMM_X64_NSEG] = {
	[NVMM_X64_SEG_ES] = {
		VMCS_GUEST_ES_SELECTOR,
		VMCS_GUEST_ES_ACCESS_RIGHTS,
		VMCS_GUEST_ES_LIMIT,
		VMCS_GUEST_ES_BASE
	},
	[NVMM_X64_SEG_CS] = {
		VMCS_GUEST_CS_SELECTOR,
		VMCS_GUEST_CS_ACCESS_RIGHTS,
		VMCS_GUEST_CS_LIMIT,
		VMCS_GUEST_CS_BASE
	},
	[NVMM_X64_SEG_SS] = {
		VMCS_GUEST_SS_SELECTOR,
		VMCS_GUEST_SS_ACCESS_RIGHTS,
		VMCS_GUEST_SS_LIMIT,
		VMCS_GUEST_SS_BASE
	},
	[NVMM_X64_SEG_DS] = {
		VMCS_GUEST_DS_SELECTOR,
		VMCS_GUEST_DS_ACCESS_RIGHTS,
		VMCS_GUEST_DS_LIMIT,
		VMCS_GUEST_DS_BASE
	},
	[NVMM_X64_SEG_FS] = {
		VMCS_GUEST_FS_SELECTOR,
		VMCS_GUEST_FS_ACCESS_RIGHTS,
		VMCS_GUEST_FS_LIMIT,
		VMCS_GUEST_FS_BASE
	},
	[NVMM_X64_SEG_GS] = {
		VMCS_GUEST_GS_SELECTOR,
		VMCS_GUEST_GS_ACCESS_RIGHTS,
		VMCS_GUEST_GS_LIMIT,
		VMCS_GUEST_GS_BASE
	},
	[NVMM_X64_SEG_GDT] = {
		0, /* doesn't exist */
		0, /* doesn't exist */
		VMCS_GUEST_GDTR_LIMIT,
		VMCS_GUEST_GDTR_BASE
	},
	[NVMM_X64_SEG_IDT] = {
		0, /* doesn't exist */
		0, /* doesn't exist */
		VMCS_GUEST_IDTR_LIMIT,
		VMCS_GUEST_IDTR_BASE
	},
	[NVMM_X64_SEG_LDT] = {
		VMCS_GUEST_LDTR_SELECTOR,
		VMCS_GUEST_LDTR_ACCESS_RIGHTS,
		VMCS_GUEST_LDTR_LIMIT,
		VMCS_GUEST_LDTR_BASE
	},
	[NVMM_X64_SEG_TR] = {
		VMCS_GUEST_TR_SELECTOR,
		VMCS_GUEST_TR_ACCESS_RIGHTS,
		VMCS_GUEST_TR_LIMIT,
		VMCS_GUEST_TR_BASE
	}
};

/* -------------------------------------------------------------------------- */

static uint64_t
vmx_get_revision(void)
{
	uint64_t msr;

	msr = rdmsr(MSR_IA32_VMX_BASIC);
	msr &= IA32_VMX_BASIC_IDENT;

	return msr;
}

static
OS_IPI_FUNC(vmx_vmclear_ipi)
{
	paddr_t vmcs_pa = (paddr_t)arg;
	vmx_vmclear(&vmcs_pa);
}

static void
vmx_vmclear_remote(os_cpu_t *cpu, paddr_t vmcs_pa)
{
	int bound;

	OS_ASSERT(os_preempt_disabled());

	/*
	 * TODO: OSify curlwp_bind().
	 */

	bound = curlwp_bind();
	os_preempt_enable();

	os_ipi_unicast(cpu, vmx_vmclear_ipi, (void *)vmcs_pa);

	os_preempt_disable();
	curlwp_bindx(bound);
}

static void
vmx_vmcs_enter(struct nvmm_cpu *vcpu)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;
	os_cpu_t *vmcs_cpu;

	cpudata->vmcs_refcnt++;
	if (cpudata->vmcs_refcnt > 1) {
		OS_ASSERT(os_preempt_disabled());
		OS_ASSERT(vmx_vmptrst() == cpudata->vmcs_pa);
		return;
	}

	vmcs_cpu = cpudata->vmcs_cpu;
	cpudata->vmcs_cpu = (void *)0x00FFFFFFFFFFFFFF; /* clobber */

	os_preempt_disable();

	if (vmcs_cpu == NULL) {
		/* This VMCS is loaded for the first time. */
		vmx_vmclear(&cpudata->vmcs_pa);
		cpudata->vmcs_launched = false;
	} else if (vmcs_cpu != os_curcpu()) {
		/* This VMCS is active on a remote CPU. */
		vmx_vmclear_remote(vmcs_cpu, cpudata->vmcs_pa);
		cpudata->vmcs_launched = false;
	} else {
		/* This VMCS is active on curcpu, nothing to do. */
	}

	vmx_vmptrld(&cpudata->vmcs_pa);
}

static void
vmx_vmcs_leave(struct nvmm_cpu *vcpu)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;

	OS_ASSERT(os_preempt_disabled());
	OS_ASSERT(vmx_vmptrst() == cpudata->vmcs_pa);
	OS_ASSERT(cpudata->vmcs_refcnt > 0);
	cpudata->vmcs_refcnt--;

	if (cpudata->vmcs_refcnt > 0) {
		return;
	}

	cpudata->vmcs_cpu = os_curcpu();
	os_preempt_enable();
}

static void
vmx_vmcs_destroy(struct nvmm_cpu *vcpu)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;

	OS_ASSERT(os_preempt_disabled());
	OS_ASSERT(vmx_vmptrst() == cpudata->vmcs_pa);
	OS_ASSERT(cpudata->vmcs_refcnt == 1);
	cpudata->vmcs_refcnt--;

	vmx_vmclear(&cpudata->vmcs_pa);
	os_preempt_enable();
}

/* -------------------------------------------------------------------------- */

static void
vmx_event_waitexit_enable(struct nvmm_cpu *vcpu, bool nmi)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;
	uint64_t ctls1;

	ctls1 = vmx_vmread(VMCS_PROCBASED_CTLS);

	if (nmi) {
		// XXX INT_STATE_NMI?
		ctls1 |= PROC_CTLS_NMI_WINDOW_EXITING;
		cpudata->nmi_window_exit = true;
	} else {
		ctls1 |= PROC_CTLS_INT_WINDOW_EXITING;
		cpudata->int_window_exit = true;
	}

	vmx_vmwrite(VMCS_PROCBASED_CTLS, ctls1);
}

static void
vmx_event_waitexit_disable(struct nvmm_cpu *vcpu, bool nmi)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;
	uint64_t ctls1;

	ctls1 = vmx_vmread(VMCS_PROCBASED_CTLS);

	if (nmi) {
		ctls1 &= ~PROC_CTLS_NMI_WINDOW_EXITING;
		cpudata->nmi_window_exit = false;
	} else {
		ctls1 &= ~PROC_CTLS_INT_WINDOW_EXITING;
		cpudata->int_window_exit = false;
	}

	vmx_vmwrite(VMCS_PROCBASED_CTLS, ctls1);
}

static inline bool
vmx_excp_has_rf(uint8_t vector)
{
	switch (vector) {
	case 1:		/* #DB */
	case 4:		/* #OF */
	case 8:		/* #DF */
	case 18:	/* #MC */
		return false;
	default:
		return true;
	}
}

static inline int
vmx_excp_has_error(uint8_t vector)
{
	switch (vector) {
	case 8:		/* #DF */
	case 10:	/* #TS */
	case 11:	/* #NP */
	case 12:	/* #SS */
	case 13:	/* #GP */
	case 14:	/* #PF */
	case 17:	/* #AC */
	case 21:	/* #CP */
	case 30:	/* #SX */
		return 1;
	default:
		return 0;
	}
}

static int
vmx_vcpu_inject(struct nvmm_cpu *vcpu)
{
	struct nvmm_comm_page *comm = vcpu->comm;
	struct vmx_cpudata *cpudata = vcpu->cpudata;
	int type = 0, err = 0, ret = EINVAL;
	uint64_t rflags, info, error;
	u_int evtype;
	uint8_t vector;

	evtype = comm->event.type;
	vector = comm->event.vector;
	error = comm->event.u.excp.error;
	__insn_barrier();

	vmx_vmcs_enter(vcpu);

	switch (evtype) {
	case NVMM_VCPU_EVENT_EXCP:
		if (vector == 2 || vector >= 32)
			goto out;
		if (vector == 3 || vector == 0)
			goto out;
		if (vmx_excp_has_rf(vector)) {
			rflags = vmx_vmread(VMCS_GUEST_RFLAGS);
			vmx_vmwrite(VMCS_GUEST_RFLAGS, rflags | PSL_RF);
		}
		type = INTR_TYPE_HW_EXC;
		err = vmx_excp_has_error(vector);
		break;
	case NVMM_VCPU_EVENT_INTR:
		type = INTR_TYPE_EXT_INT;
		if (vector == 2) {
			type = INTR_TYPE_NMI;
			vmx_event_waitexit_enable(vcpu, true);
		}
		err = 0;
		break;
	default:
		goto out;
	}

	info =
	    __SHIFTIN((uint64_t)vector, INTR_INFO_VECTOR) |
	    __SHIFTIN((uint64_t)type, INTR_INFO_TYPE) |
	    __SHIFTIN((uint64_t)err, INTR_INFO_ERROR) |
	    __SHIFTIN((uint64_t)1, INTR_INFO_VALID);
	vmx_vmwrite(VMCS_ENTRY_INTR_INFO, info);
	vmx_vmwrite(VMCS_ENTRY_EXCEPTION_ERROR, error);

	cpudata->evt_pending = true;
	ret = 0;

out:
	vmx_vmcs_leave(vcpu);
	return ret;
}

static void
vmx_inject_ud(struct nvmm_cpu *vcpu)
{
	struct nvmm_comm_page *comm = vcpu->comm;
	int ret __diagused;

	comm->event.type = NVMM_VCPU_EVENT_EXCP;
	comm->event.vector = 6;
	comm->event.u.excp.error = 0;

	ret = vmx_vcpu_inject(vcpu);
	OS_ASSERT(ret == 0);
}

static void
vmx_inject_gp(struct nvmm_cpu *vcpu)
{
	struct nvmm_comm_page *comm = vcpu->comm;
	int ret __diagused;

	comm->event.type = NVMM_VCPU_EVENT_EXCP;
	comm->event.vector = 13;
	comm->event.u.excp.error = 0;

	ret = vmx_vcpu_inject(vcpu);
	OS_ASSERT(ret == 0);
}

static inline int
vmx_vcpu_event_commit(struct nvmm_cpu *vcpu)
{
	if (__predict_true(!vcpu->comm->event_commit)) {
		return 0;
	}
	vcpu->comm->event_commit = false;
	return vmx_vcpu_inject(vcpu);
}

static inline void
vmx_inkernel_advance(void)
{
	uint64_t rip, inslen, intstate, rflags;

	/*
	 * Maybe we should also apply single-stepping and debug exceptions.
	 * Matters for guest-ring3, because it can execute 'cpuid' under a
	 * debugger.
	 */

	inslen = vmx_vmread(VMCS_EXIT_INSTRUCTION_LENGTH);
	rip = vmx_vmread(VMCS_GUEST_RIP);
	vmx_vmwrite(VMCS_GUEST_RIP, rip + inslen);

	rflags = vmx_vmread(VMCS_GUEST_RFLAGS);
	vmx_vmwrite(VMCS_GUEST_RFLAGS, rflags & ~PSL_RF);

	intstate = vmx_vmread(VMCS_GUEST_INTERRUPTIBILITY);
	vmx_vmwrite(VMCS_GUEST_INTERRUPTIBILITY,
	    intstate & ~(INT_STATE_STI|INT_STATE_MOVSS));
}

static void
vmx_exit_invalid(struct nvmm_vcpu_exit *exit, uint64_t code)
{
	exit->u.inv.hwcode = code;
	exit->reason = NVMM_VCPU_EXIT_INVALID;
}

static void
vmx_exit_exc_nmi(struct nvmm_machine *mach, struct nvmm_cpu *vcpu,
    struct nvmm_vcpu_exit *exit)
{
	uint64_t qual;

	qual = vmx_vmread(VMCS_EXIT_INTR_INFO);

	if ((qual & INTR_INFO_VALID) == 0) {
		goto error;
	}
	if (__SHIFTOUT(qual, INTR_INFO_TYPE) != INTR_TYPE_NMI) {
		goto error;
	}

	exit->reason = NVMM_VCPU_EXIT_NONE;
	return;

error:
	vmx_exit_invalid(exit, VMCS_EXITCODE_EXC_NMI);
}

#define VMX_CPUID_MAX_BASIC		0x16
#define VMX_CPUID_MAX_HYPERVISOR	0x40000000
#define VMX_CPUID_MAX_EXTENDED		0x80000008
static uint32_t vmx_cpuid_max_basic __read_mostly;
static uint32_t vmx_cpuid_max_extended __read_mostly;

static void
vmx_inkernel_exec_cpuid(struct vmx_cpudata *cpudata, uint32_t eax, uint32_t ecx)
{
	cpuid_desc_t descs;

	x86_get_cpuid2(eax, ecx, &descs);
	cpudata->gprs[NVMM_X64_GPR_RAX] = descs.eax;
	cpudata->gprs[NVMM_X64_GPR_RBX] = descs.ebx;
	cpudata->gprs[NVMM_X64_GPR_RCX] = descs.ecx;
	cpudata->gprs[NVMM_X64_GPR_RDX] = descs.edx;
}

static void
vmx_inkernel_handle_cpuid(struct nvmm_machine *mach, struct nvmm_cpu *vcpu,
    uint32_t eax, uint32_t ecx)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;
	unsigned int ncpus;
	uint32_t clevel;
	uint64_t cr4;

	if (eax < 0x40000000) {
		if (__predict_false(eax > vmx_cpuid_max_basic)) {
			eax = vmx_cpuid_max_basic;
			vmx_inkernel_exec_cpuid(cpudata, eax, ecx);
		}
	} else if (eax < 0x80000000) {
		if (__predict_false(eax > VMX_CPUID_MAX_HYPERVISOR)) {
			eax = vmx_cpuid_max_basic;
			vmx_inkernel_exec_cpuid(cpudata, eax, ecx);
		}
	} else {
		if (__predict_false(eax > vmx_cpuid_max_extended)) {
			eax = vmx_cpuid_max_basic;
			vmx_inkernel_exec_cpuid(cpudata, eax, ecx);
		}
	}

	switch (eax) {
	case 0x00000000:
		cpudata->gprs[NVMM_X64_GPR_RAX] = vmx_cpuid_max_basic;
		break;
	case 0x00000001:
		cpudata->gprs[NVMM_X64_GPR_RAX] &= nvmm_cpuid_00000001.eax;

		cpudata->gprs[NVMM_X64_GPR_RBX] &= ~CPUID_0_01_EBX_LOCAL_APIC_ID;
		cpudata->gprs[NVMM_X64_GPR_RBX] |= __SHIFTIN(vcpu->cpuid,
		    CPUID_0_01_EBX_LOCAL_APIC_ID);

		ncpus = os_atomic_load_uint(&mach->ncpus);
		cpudata->gprs[NVMM_X64_GPR_RBX] &= ~CPUID_0_01_EBX_HTT_CORES;
		cpudata->gprs[NVMM_X64_GPR_RBX] |= __SHIFTIN(ncpus,
		    CPUID_0_01_EBX_HTT_CORES);

		cpudata->gprs[NVMM_X64_GPR_RCX] &= nvmm_cpuid_00000001.ecx;
		cpudata->gprs[NVMM_X64_GPR_RCX] |= CPUID_0_01_ECX_RAZ;
		if (vmx_procbased_ctls2 & PROC_CTLS2_INVPCID_ENABLE) {
			cpudata->gprs[NVMM_X64_GPR_RCX] |= CPUID_0_01_ECX_PCID;
		}

		cpudata->gprs[NVMM_X64_GPR_RDX] &= nvmm_cpuid_00000001.edx;

		/* CPUID_0_01_ECX_OSXSAVE depends on CR4. */
		cr4 = vmx_vmread(VMCS_GUEST_CR4);
		if (!(cr4 & CR4_OSXSAVE)) {
			cpudata->gprs[NVMM_X64_GPR_RCX] &= ~CPUID_0_01_ECX_OSXSAVE;
		}
		break;
	case 0x00000002:
		break;
	case 0x00000003:
		cpudata->gprs[NVMM_X64_GPR_RAX] = 0;
		cpudata->gprs[NVMM_X64_GPR_RBX] = 0;
		cpudata->gprs[NVMM_X64_GPR_RCX] = 0;
		cpudata->gprs[NVMM_X64_GPR_RDX] = 0;
		break;
	case 0x00000004: /* Deterministic Cache Parameters */
		ncpus = os_atomic_load_uint(&mach->ncpus);
		clevel = __SHIFTOUT(cpudata->gprs[NVMM_X64_GPR_RAX],
		    CPUID_0_04_EAX_CACHELEVEL);

		cpudata->gprs[NVMM_X64_GPR_RAX] &= ~CPUID_0_04_EAX_SHARING;
		if (clevel >= 3) {
			/* L3 and above: all CPUs. */
			cpudata->gprs[NVMM_X64_GPR_RAX] |=
			    __SHIFTIN(ncpus - 1, CPUID_0_04_EAX_SHARING);
		} else {
			/* L2 and below: one LP per CPU. */
			cpudata->gprs[NVMM_X64_GPR_RAX] |=
			    __SHIFTIN(0, CPUID_0_04_EAX_SHARING);
		}

		cpudata->gprs[NVMM_X64_GPR_RAX] &= ~CPUID_0_04_EAX_CORE_P_PKG;
		cpudata->gprs[NVMM_X64_GPR_RAX] |=
		    __SHIFTIN(ncpus - 1, CPUID_0_04_EAX_CORE_P_PKG);
		break;
	case 0x00000005: /* MONITOR/MWAIT */
	case 0x00000006: /* Thermal and Power Management */
		cpudata->gprs[NVMM_X64_GPR_RAX] = 0;
		cpudata->gprs[NVMM_X64_GPR_RBX] = 0;
		cpudata->gprs[NVMM_X64_GPR_RCX] = 0;
		cpudata->gprs[NVMM_X64_GPR_RDX] = 0;
		break;
	case 0x00000007: /* Structured Extended Feature Flags Enumeration */
		switch (ecx) {
		case 0:
			cpudata->gprs[NVMM_X64_GPR_RAX] = 0;
			cpudata->gprs[NVMM_X64_GPR_RBX] &= nvmm_cpuid_00000007.ebx;
			cpudata->gprs[NVMM_X64_GPR_RCX] &= nvmm_cpuid_00000007.ecx;
			cpudata->gprs[NVMM_X64_GPR_RDX] &= nvmm_cpuid_00000007.edx;
			if (vmx_procbased_ctls2 & PROC_CTLS2_INVPCID_ENABLE) {
				cpudata->gprs[NVMM_X64_GPR_RBX] |= CPUID_0_07_EBX_INVPCID;
			}
			break;
		default:
			cpudata->gprs[NVMM_X64_GPR_RAX] = 0;
			cpudata->gprs[NVMM_X64_GPR_RBX] = 0;
			cpudata->gprs[NVMM_X64_GPR_RCX] = 0;
			cpudata->gprs[NVMM_X64_GPR_RDX] = 0;
			break;
		}
		break;
	case 0x00000008: /* Empty */
	case 0x00000009: /* Direct Cache Access Information */
		cpudata->gprs[NVMM_X64_GPR_RAX] = 0;
		cpudata->gprs[NVMM_X64_GPR_RBX] = 0;
		cpudata->gprs[NVMM_X64_GPR_RCX] = 0;
		cpudata->gprs[NVMM_X64_GPR_RDX] = 0;
		break;
	case 0x0000000A: /* Architectural Performance Monitoring */
		cpudata->gprs[NVMM_X64_GPR_RAX] = 0;
		cpudata->gprs[NVMM_X64_GPR_RBX] = 0;
		cpudata->gprs[NVMM_X64_GPR_RCX] = 0;
		cpudata->gprs[NVMM_X64_GPR_RDX] = 0;
		break;
	case 0x0000000B: /* Extended Topology Enumeration */
		switch (ecx) {
		case 0: /* Threads */
			cpudata->gprs[NVMM_X64_GPR_RAX] = 0;
			cpudata->gprs[NVMM_X64_GPR_RBX] = 0;
			cpudata->gprs[NVMM_X64_GPR_RCX] =
			    __SHIFTIN(ecx, CPUID_0_0B_ECX_LVLNUM) |
			    __SHIFTIN(CPUID_0_0B_ECX_LVLTYPE_SMT, CPUID_0_0B_ECX_LVLTYPE);
			cpudata->gprs[NVMM_X64_GPR_RDX] = vcpu->cpuid;
			break;
		case 1: /* Cores */
			ncpus = os_atomic_load_uint(&mach->ncpus);
			cpudata->gprs[NVMM_X64_GPR_RAX] = ilog2(ncpus);
			cpudata->gprs[NVMM_X64_GPR_RBX] = ncpus;
			cpudata->gprs[NVMM_X64_GPR_RCX] =
			    __SHIFTIN(ecx, CPUID_0_0B_ECX_LVLNUM) |
			    __SHIFTIN(CPUID_0_0B_ECX_LVLTYPE_CORE, CPUID_0_0B_ECX_LVLTYPE);
			cpudata->gprs[NVMM_X64_GPR_RDX] = vcpu->cpuid;
			break;
		default:
			cpudata->gprs[NVMM_X64_GPR_RAX] = 0;
			cpudata->gprs[NVMM_X64_GPR_RBX] = 0;
			cpudata->gprs[NVMM_X64_GPR_RCX] = 0; /* LVLTYPE_INVAL */
			cpudata->gprs[NVMM_X64_GPR_RDX] = 0;
			break;
		}
		break;
	case 0x0000000C: /* Empty */
		cpudata->gprs[NVMM_X64_GPR_RAX] = 0;
		cpudata->gprs[NVMM_X64_GPR_RBX] = 0;
		cpudata->gprs[NVMM_X64_GPR_RCX] = 0;
		cpudata->gprs[NVMM_X64_GPR_RDX] = 0;
		break;
	case 0x0000000D: /* Processor Extended State Enumeration */
		if (vmx_xcr0_mask == 0) {
			break;
		}
		switch (ecx) {
		case 0:
			/* Supported XCR0 bits. */
			cpudata->gprs[NVMM_X64_GPR_RAX] = vmx_xcr0_mask & 0xFFFFFFFF;
			cpudata->gprs[NVMM_X64_GPR_RDX] = vmx_xcr0_mask >> 32;
			/* XSAVE size for currently enabled XCR0 features. */
			cpudata->gprs[NVMM_X64_GPR_RBX] = nvmm_x86_xsave_size(cpudata->gxcr0);
			/* XSAVE size for all supported XCR0 features. */
			cpudata->gprs[NVMM_X64_GPR_RCX] = nvmm_x86_xsave_size(vmx_xcr0_mask);
			break;
		case 1:
			cpudata->gprs[NVMM_X64_GPR_RAX] &=
			    (CPUID_0_0D_ECX1_EAX_XSAVEOPT |
			     CPUID_0_0D_ECX1_EAX_XSAVEC |
			     CPUID_0_0D_ECX1_EAX_XGETBV);
			cpudata->gprs[NVMM_X64_GPR_RBX] = 0;
			cpudata->gprs[NVMM_X64_GPR_RCX] = 0;
			cpudata->gprs[NVMM_X64_GPR_RDX] = 0;
			break;
		default:
			cpudata->gprs[NVMM_X64_GPR_RAX] = 0;
			cpudata->gprs[NVMM_X64_GPR_RBX] = 0;
			cpudata->gprs[NVMM_X64_GPR_RCX] = 0;
			cpudata->gprs[NVMM_X64_GPR_RDX] = 0;
			break;
		}
		break;
	case 0x0000000E: /* Empty */
	case 0x0000000F: /* Intel RDT Monitoring Enumeration */
	case 0x00000010: /* Intel RDT Allocation Enumeration */
		cpudata->gprs[NVMM_X64_GPR_RAX] = 0;
		cpudata->gprs[NVMM_X64_GPR_RBX] = 0;
		cpudata->gprs[NVMM_X64_GPR_RCX] = 0;
		cpudata->gprs[NVMM_X64_GPR_RDX] = 0;
		break;
	case 0x00000011: /* Empty */
	case 0x00000012: /* Intel SGX Capability Enumeration */
	case 0x00000013: /* Empty */
	case 0x00000014: /* Intel Processor Trace Enumeration */
		cpudata->gprs[NVMM_X64_GPR_RAX] = 0;
		cpudata->gprs[NVMM_X64_GPR_RBX] = 0;
		cpudata->gprs[NVMM_X64_GPR_RCX] = 0;
		cpudata->gprs[NVMM_X64_GPR_RDX] = 0;
		break;
	case 0x00000015: /* TSC and Nominal Core Crystal Clock Information */
	case 0x00000016: /* Processor Frequency Information */
		break;

	case 0x40000000: /* Hypervisor Information */
		cpudata->gprs[NVMM_X64_GPR_RAX] = VMX_CPUID_MAX_HYPERVISOR;
		cpudata->gprs[NVMM_X64_GPR_RBX] = 0;
		cpudata->gprs[NVMM_X64_GPR_RCX] = 0;
		cpudata->gprs[NVMM_X64_GPR_RDX] = 0;
		memcpy(&cpudata->gprs[NVMM_X64_GPR_RBX], "___ ", 4);
		memcpy(&cpudata->gprs[NVMM_X64_GPR_RCX], "NVMM", 4);
		memcpy(&cpudata->gprs[NVMM_X64_GPR_RDX], " ___", 4);
		break;

	case 0x80000000:
		cpudata->gprs[NVMM_X64_GPR_RAX] = vmx_cpuid_max_extended;
		break;
	case 0x80000001:
		cpudata->gprs[NVMM_X64_GPR_RAX] &= nvmm_cpuid_80000001.eax;
		cpudata->gprs[NVMM_X64_GPR_RBX] &= nvmm_cpuid_80000001.ebx;
		cpudata->gprs[NVMM_X64_GPR_RCX] &= nvmm_cpuid_80000001.ecx;
		cpudata->gprs[NVMM_X64_GPR_RDX] &= nvmm_cpuid_80000001.edx;
		break;
	case 0x80000002: /* Processor Brand String */
	case 0x80000003: /* Processor Brand String */
	case 0x80000004: /* Processor Brand String */
	case 0x80000005: /* Reserved Zero */
	case 0x80000006: /* Cache Information */
		break;
	case 0x80000007: /* TSC Information */
		cpudata->gprs[NVMM_X64_GPR_RAX] &= nvmm_cpuid_80000007.eax;
		cpudata->gprs[NVMM_X64_GPR_RBX] &= nvmm_cpuid_80000007.ebx;
		cpudata->gprs[NVMM_X64_GPR_RCX] &= nvmm_cpuid_80000007.ecx;
		cpudata->gprs[NVMM_X64_GPR_RDX] &= nvmm_cpuid_80000007.edx;
		break;
	case 0x80000008: /* Address Sizes */
		cpudata->gprs[NVMM_X64_GPR_RAX] &= nvmm_cpuid_80000008.eax;
		cpudata->gprs[NVMM_X64_GPR_RBX] &= nvmm_cpuid_80000008.ebx;
		cpudata->gprs[NVMM_X64_GPR_RCX] &= nvmm_cpuid_80000008.ecx;
		cpudata->gprs[NVMM_X64_GPR_RDX] &= nvmm_cpuid_80000008.edx;
		break;

	default:
		break;
	}
}

static void
vmx_exit_insn(struct nvmm_vcpu_exit *exit, uint64_t reason)
{
	uint64_t inslen, rip;

	inslen = vmx_vmread(VMCS_EXIT_INSTRUCTION_LENGTH);
	rip = vmx_vmread(VMCS_GUEST_RIP);
	exit->u.insn.npc = rip + inslen;
	exit->reason = reason;
}

static void
vmx_exit_cpuid(struct nvmm_machine *mach, struct nvmm_cpu *vcpu,
    struct nvmm_vcpu_exit *exit)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;
	struct nvmm_vcpu_conf_cpuid *cpuid;
	uint32_t eax, ecx;
	size_t i;

	eax = (cpudata->gprs[NVMM_X64_GPR_RAX] & 0xFFFFFFFF);
	ecx = (cpudata->gprs[NVMM_X64_GPR_RCX] & 0xFFFFFFFF);
	vmx_inkernel_exec_cpuid(cpudata, eax, ecx);
	vmx_inkernel_handle_cpuid(mach, vcpu, eax, ecx);

	for (i = 0; i < VMX_NCPUIDS; i++) {
		if (!cpudata->cpuidpresent[i]) {
			continue;
		}
		cpuid = &cpudata->cpuid[i];
		if (cpuid->leaf != eax) {
			continue;
		}

		if (cpuid->exit) {
			vmx_exit_insn(exit, NVMM_VCPU_EXIT_CPUID);
			return;
		}
		OS_ASSERT(cpuid->mask);

		/* del */
		cpudata->gprs[NVMM_X64_GPR_RAX] &= ~cpuid->u.mask.del.eax;
		cpudata->gprs[NVMM_X64_GPR_RBX] &= ~cpuid->u.mask.del.ebx;
		cpudata->gprs[NVMM_X64_GPR_RCX] &= ~cpuid->u.mask.del.ecx;
		cpudata->gprs[NVMM_X64_GPR_RDX] &= ~cpuid->u.mask.del.edx;

		/* set */
		cpudata->gprs[NVMM_X64_GPR_RAX] |= cpuid->u.mask.set.eax;
		cpudata->gprs[NVMM_X64_GPR_RBX] |= cpuid->u.mask.set.ebx;
		cpudata->gprs[NVMM_X64_GPR_RCX] |= cpuid->u.mask.set.ecx;
		cpudata->gprs[NVMM_X64_GPR_RDX] |= cpuid->u.mask.set.edx;

		break;
	}

	vmx_inkernel_advance();
	exit->reason = NVMM_VCPU_EXIT_NONE;
}

static void
vmx_exit_hlt(struct nvmm_machine *mach, struct nvmm_cpu *vcpu,
    struct nvmm_vcpu_exit *exit)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;
	uint64_t rflags;

	if (cpudata->int_window_exit) {
		rflags = vmx_vmread(VMCS_GUEST_RFLAGS);
		if (rflags & PSL_I) {
			vmx_event_waitexit_disable(vcpu, false);
		}
	}

	vmx_inkernel_advance();
	exit->reason = NVMM_VCPU_EXIT_HALTED;
}

#define VMX_QUAL_CR_NUM		__BITS(3,0)
#define VMX_QUAL_CR_TYPE	__BITS(5,4)
#define		CR_TYPE_WRITE	0
#define		CR_TYPE_READ	1
#define		CR_TYPE_CLTS	2
#define		CR_TYPE_LMSW	3
#define VMX_QUAL_CR_LMSW_OPMEM	__BIT(6)
#define VMX_QUAL_CR_GPR		__BITS(11,8)
#define VMX_QUAL_CR_LMSW_SRC	__BIT(31,16)

static inline int
vmx_check_cr(uint64_t crval, uint64_t fixed0, uint64_t fixed1)
{
	/* Bits set to 1 in fixed0 are fixed to 1. */
	if ((crval & fixed0) != fixed0) {
		return -1;
	}
	/* Bits set to 0 in fixed1 are fixed to 0. */
	if (crval & ~fixed1) {
		return -1;
	}
	return 0;
}

static int
vmx_inkernel_handle_cr0(struct nvmm_machine *mach, struct nvmm_cpu *vcpu,
    uint64_t qual)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;
	uint64_t type, gpr, oldcr0, realcr0, fakecr0;
	uint64_t efer, ctls1;

	type = __SHIFTOUT(qual, VMX_QUAL_CR_TYPE);
	if (type != CR_TYPE_WRITE) {
		return -1;
	}

	gpr = __SHIFTOUT(qual, VMX_QUAL_CR_GPR);
	OS_ASSERT(gpr < 16);

	if (gpr == NVMM_X64_GPR_RSP) {
		fakecr0 = vmx_vmread(VMCS_GUEST_RSP);
	} else {
		fakecr0 = cpudata->gprs[gpr];
	}

	/*
	 * fakecr0 is the value the guest believes is in %cr0. realcr0 is the
	 * actual value in %cr0.
	 *
	 * In fakecr0 we must force CR0_ET to 1.
	 *
	 * In realcr0 we must force CR0_NW and CR0_CD to 0, and CR0_ET and
	 * CR0_NE to 1.
	 */
	fakecr0 |= CR0_ET;
	realcr0 = (fakecr0 & ~CR0_STATIC_MASK) | CR0_ET | CR0_NE;

	if (vmx_check_cr(realcr0, vmx_cr0_fixed0, vmx_cr0_fixed1) == -1) {
		return -1;
	}

	/*
	 * XXX Handle 32bit PAE paging, need to set PDPTEs, fetched manually
	 * from CR3.
	 */

	if (realcr0 & CR0_PG) {
		ctls1 = vmx_vmread(VMCS_ENTRY_CTLS);
		efer = vmx_vmread(VMCS_GUEST_IA32_EFER);
		if (efer & EFER_LME) {
			ctls1 |= ENTRY_CTLS_LONG_MODE;
			efer |= EFER_LMA;
		} else {
			ctls1 &= ~ENTRY_CTLS_LONG_MODE;
			efer &= ~EFER_LMA;
		}
		vmx_vmwrite(VMCS_GUEST_IA32_EFER, efer);
		vmx_vmwrite(VMCS_ENTRY_CTLS, ctls1);
	}

	oldcr0 = (vmx_vmread(VMCS_CR0_SHADOW) & CR0_STATIC_MASK) |
	    (vmx_vmread(VMCS_GUEST_CR0) & ~CR0_STATIC_MASK);
	if ((oldcr0 ^ fakecr0) & CR0_TLB_FLUSH) {
		cpudata->gtlb_want_flush = true;
	}

	vmx_vmwrite(VMCS_CR0_SHADOW, fakecr0);
	vmx_vmwrite(VMCS_GUEST_CR0, realcr0);
	vmx_inkernel_advance();
	return 0;
}

static int
vmx_inkernel_handle_cr4(struct nvmm_machine *mach, struct nvmm_cpu *vcpu,
    uint64_t qual)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;
	uint64_t type, gpr, oldcr4, cr4;

	type = __SHIFTOUT(qual, VMX_QUAL_CR_TYPE);
	if (type != CR_TYPE_WRITE) {
		return -1;
	}

	gpr = __SHIFTOUT(qual, VMX_QUAL_CR_GPR);
	OS_ASSERT(gpr < 16);

	if (gpr == NVMM_X64_GPR_RSP) {
		gpr = vmx_vmread(VMCS_GUEST_RSP);
	} else {
		gpr = cpudata->gprs[gpr];
	}

	if (gpr & CR4_INVALID) {
		return -1;
	}
	cr4 = gpr | CR4_VMXE;
	if (vmx_check_cr(cr4, vmx_cr4_fixed0, vmx_cr4_fixed1) == -1) {
		return -1;
	}

	oldcr4 = vmx_vmread(VMCS_GUEST_CR4);
	if ((oldcr4 ^ gpr) & CR4_TLB_FLUSH) {
		cpudata->gtlb_want_flush = true;
	}

	vmx_vmwrite(VMCS_GUEST_CR4, cr4);
	vmx_inkernel_advance();
	return 0;
}

static int
vmx_inkernel_handle_cr8(struct nvmm_machine *mach, struct nvmm_cpu *vcpu,
    uint64_t qual, struct nvmm_vcpu_exit *exit)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;
	uint64_t type, gpr;
	bool write;

	type = __SHIFTOUT(qual, VMX_QUAL_CR_TYPE);
	if (type == CR_TYPE_WRITE) {
		write = true;
	} else if (type == CR_TYPE_READ) {
		write = false;
	} else {
		return -1;
	}

	gpr = __SHIFTOUT(qual, VMX_QUAL_CR_GPR);
	OS_ASSERT(gpr < 16);

	if (write) {
		if (gpr == NVMM_X64_GPR_RSP) {
			cpudata->gcr8 = vmx_vmread(VMCS_GUEST_RSP);
		} else {
			cpudata->gcr8 = cpudata->gprs[gpr];
		}
		if (cpudata->tpr.exit_changed) {
			exit->reason = NVMM_VCPU_EXIT_TPR_CHANGED;
		}
	} else {
		if (gpr == NVMM_X64_GPR_RSP) {
			vmx_vmwrite(VMCS_GUEST_RSP, cpudata->gcr8);
		} else {
			cpudata->gprs[gpr] = cpudata->gcr8;
		}
	}

	vmx_inkernel_advance();
	return 0;
}

static void
vmx_exit_cr(struct nvmm_machine *mach, struct nvmm_cpu *vcpu,
    struct nvmm_vcpu_exit *exit)
{
	uint64_t qual;
	int ret;

	exit->reason = NVMM_VCPU_EXIT_NONE;

	qual = vmx_vmread(VMCS_EXIT_QUALIFICATION);

	switch (__SHIFTOUT(qual, VMX_QUAL_CR_NUM)) {
	case 0:
		ret = vmx_inkernel_handle_cr0(mach, vcpu, qual);
		break;
	case 4:
		ret = vmx_inkernel_handle_cr4(mach, vcpu, qual);
		break;
	case 8:
		ret = vmx_inkernel_handle_cr8(mach, vcpu, qual, exit);
		break;
	default:
		ret = -1;
		break;
	}

	if (ret == -1) {
		vmx_inject_gp(vcpu);
	}
}

#define VMX_QUAL_IO_SIZE	__BITS(2,0)
#define		IO_SIZE_8	0
#define		IO_SIZE_16	1
#define		IO_SIZE_32	3
#define VMX_QUAL_IO_IN		__BIT(3)
#define VMX_QUAL_IO_STR		__BIT(4)
#define VMX_QUAL_IO_REP		__BIT(5)
#define VMX_QUAL_IO_DX		__BIT(6)
#define VMX_QUAL_IO_PORT	__BITS(31,16)

#define VMX_INFO_IO_ADRSIZE	__BITS(9,7)
#define		IO_ADRSIZE_16	0
#define		IO_ADRSIZE_32	1
#define		IO_ADRSIZE_64	2
#define VMX_INFO_IO_SEG		__BITS(17,15)

static void
vmx_exit_io(struct nvmm_machine *mach, struct nvmm_cpu *vcpu,
    struct nvmm_vcpu_exit *exit)
{
	uint64_t qual, info, inslen, rip;

	qual = vmx_vmread(VMCS_EXIT_QUALIFICATION);
	info = vmx_vmread(VMCS_EXIT_INSTRUCTION_INFO);

	exit->reason = NVMM_VCPU_EXIT_IO;

	exit->u.io.in = (qual & VMX_QUAL_IO_IN) != 0;
	exit->u.io.port = __SHIFTOUT(qual, VMX_QUAL_IO_PORT);

	OS_ASSERT(__SHIFTOUT(info, VMX_INFO_IO_SEG) < 6);
	exit->u.io.seg = __SHIFTOUT(info, VMX_INFO_IO_SEG);

	if (__SHIFTOUT(info, VMX_INFO_IO_ADRSIZE) == IO_ADRSIZE_64) {
		exit->u.io.address_size = 8;
	} else if (__SHIFTOUT(info, VMX_INFO_IO_ADRSIZE) == IO_ADRSIZE_32) {
		exit->u.io.address_size = 4;
	} else if (__SHIFTOUT(info, VMX_INFO_IO_ADRSIZE) == IO_ADRSIZE_16) {
		exit->u.io.address_size = 2;
	}

	if (__SHIFTOUT(qual, VMX_QUAL_IO_SIZE) == IO_SIZE_32) {
		exit->u.io.operand_size = 4;
	} else if (__SHIFTOUT(qual, VMX_QUAL_IO_SIZE) == IO_SIZE_16) {
		exit->u.io.operand_size = 2;
	} else if (__SHIFTOUT(qual, VMX_QUAL_IO_SIZE) == IO_SIZE_8) {
		exit->u.io.operand_size = 1;
	}

	exit->u.io.rep = (qual & VMX_QUAL_IO_REP) != 0;
	exit->u.io.str = (qual & VMX_QUAL_IO_STR) != 0;

	if (exit->u.io.in && exit->u.io.str) {
		exit->u.io.seg = NVMM_X64_SEG_ES;
	}

	inslen = vmx_vmread(VMCS_EXIT_INSTRUCTION_LENGTH);
	rip = vmx_vmread(VMCS_GUEST_RIP);
	exit->u.io.npc = rip + inslen;

	vmx_vcpu_state_provide(vcpu,
	    NVMM_X64_STATE_GPRS | NVMM_X64_STATE_SEGS |
	    NVMM_X64_STATE_CRS | NVMM_X64_STATE_MSRS);
}

static const uint64_t msr_ignore_list[] = {
	MSR_BIOS_SIGN,
	MSR_IA32_PLATFORM_ID
};

static bool
vmx_inkernel_handle_msr(struct nvmm_machine *mach, struct nvmm_cpu *vcpu,
    struct nvmm_vcpu_exit *exit)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;
	uint64_t val;
	size_t i;

	if (exit->reason == NVMM_VCPU_EXIT_RDMSR) {
		if (exit->u.rdmsr.msr == MSR_CR_PAT) {
			val = vmx_vmread(VMCS_GUEST_IA32_PAT);
			cpudata->gprs[NVMM_X64_GPR_RAX] = (val & 0xFFFFFFFF);
			cpudata->gprs[NVMM_X64_GPR_RDX] = (val >> 32);
			goto handled;
		}
		if (exit->u.rdmsr.msr == MSR_MISC_ENABLE) {
			val = cpudata->gmsr_misc_enable;
			cpudata->gprs[NVMM_X64_GPR_RAX] = (val & 0xFFFFFFFF);
			cpudata->gprs[NVMM_X64_GPR_RDX] = (val >> 32);
			goto handled;
		}
		if (exit->u.rdmsr.msr == MSR_IA32_ARCH_CAPABILITIES) {
			cpuid_desc_t descs;
			x86_get_cpuid(0x00000000, &descs);
			if (descs.eax < 7) {
				goto error;
			}
			x86_get_cpuid(0x00000007, &descs);
			if (!(descs.edx & CPUID_0_07_EDX_ARCH_CAP)) {
				goto error;
			}
			val = rdmsr(MSR_IA32_ARCH_CAPABILITIES);
			val &= (IA32_ARCH_RDCL_NO |
			    IA32_ARCH_SSB_NO |
			    IA32_ARCH_MDS_NO |
			    IA32_ARCH_TAA_NO);
			cpudata->gprs[NVMM_X64_GPR_RAX] = (val & 0xFFFFFFFF);
			cpudata->gprs[NVMM_X64_GPR_RDX] = (val >> 32);
			goto handled;
		}
		for (i = 0; i < __arraycount(msr_ignore_list); i++) {
			if (msr_ignore_list[i] != exit->u.rdmsr.msr)
				continue;
			val = 0;
			cpudata->gprs[NVMM_X64_GPR_RAX] = (val & 0xFFFFFFFF);
			cpudata->gprs[NVMM_X64_GPR_RDX] = (val >> 32);
			goto handled;
		}
	} else {
		if (exit->u.wrmsr.msr == MSR_TSC) {
			cpudata->gtsc_offset = exit->u.wrmsr.val - rdtsc();
			cpudata->gtsc_want_update = true;
			goto handled;
		}
		if (exit->u.wrmsr.msr == MSR_CR_PAT) {
			val = exit->u.wrmsr.val;
			if (__predict_false(!nvmm_x86_pat_validate(val))) {
				goto error;
			}
			vmx_vmwrite(VMCS_GUEST_IA32_PAT, val);
			goto handled;
		}
		if (exit->u.wrmsr.msr == MSR_MISC_ENABLE) {
			/* Don't care. */
			goto handled;
		}
		for (i = 0; i < __arraycount(msr_ignore_list); i++) {
			if (msr_ignore_list[i] != exit->u.wrmsr.msr)
				continue;
			goto handled;
		}
	}

	return false;

handled:
	vmx_inkernel_advance();
	return true;

error:
	vmx_inject_gp(vcpu);
	return true;
}

static void
vmx_exit_rdmsr(struct nvmm_machine *mach, struct nvmm_cpu *vcpu,
    struct nvmm_vcpu_exit *exit)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;
	uint64_t inslen, rip;

	exit->reason = NVMM_VCPU_EXIT_RDMSR;
	exit->u.rdmsr.msr = (cpudata->gprs[NVMM_X64_GPR_RCX] & 0xFFFFFFFF);

	if (vmx_inkernel_handle_msr(mach, vcpu, exit)) {
		exit->reason = NVMM_VCPU_EXIT_NONE;
		return;
	}

	inslen = vmx_vmread(VMCS_EXIT_INSTRUCTION_LENGTH);
	rip = vmx_vmread(VMCS_GUEST_RIP);
	exit->u.rdmsr.npc = rip + inslen;

	vmx_vcpu_state_provide(vcpu, NVMM_X64_STATE_GPRS);
}

static void
vmx_exit_wrmsr(struct nvmm_machine *mach, struct nvmm_cpu *vcpu,
    struct nvmm_vcpu_exit *exit)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;
	uint64_t rdx, rax, inslen, rip;

	rdx = cpudata->gprs[NVMM_X64_GPR_RDX];
	rax = cpudata->gprs[NVMM_X64_GPR_RAX];

	exit->reason = NVMM_VCPU_EXIT_WRMSR;
	exit->u.wrmsr.msr = (cpudata->gprs[NVMM_X64_GPR_RCX] & 0xFFFFFFFF);
	exit->u.wrmsr.val = (rdx << 32) | (rax & 0xFFFFFFFF);

	if (vmx_inkernel_handle_msr(mach, vcpu, exit)) {
		exit->reason = NVMM_VCPU_EXIT_NONE;
		return;
	}

	inslen = vmx_vmread(VMCS_EXIT_INSTRUCTION_LENGTH);
	rip = vmx_vmread(VMCS_GUEST_RIP);
	exit->u.wrmsr.npc = rip + inslen;

	vmx_vcpu_state_provide(vcpu, NVMM_X64_STATE_GPRS);
}

static void
vmx_exit_xsetbv(struct nvmm_machine *mach, struct nvmm_cpu *vcpu,
    struct nvmm_vcpu_exit *exit)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;
	uint64_t val;

	exit->reason = NVMM_VCPU_EXIT_NONE;

	val = (cpudata->gprs[NVMM_X64_GPR_RDX] << 32) |
	    (cpudata->gprs[NVMM_X64_GPR_RAX] & 0xFFFFFFFF);

	if (__predict_false(cpudata->gprs[NVMM_X64_GPR_RCX] != 0)) {
		goto error;
	} else if (__predict_false((val & ~vmx_xcr0_mask) != 0)) {
		goto error;
	} else if (__predict_false((val & XCR0_X87) == 0)) {
		goto error;
	}

	cpudata->gxcr0 = val;

	vmx_inkernel_advance();
	return;

error:
	vmx_inject_gp(vcpu);
}

#define VMX_EPT_VIOLATION_READ		__BIT(0)
#define VMX_EPT_VIOLATION_WRITE		__BIT(1)
#define VMX_EPT_VIOLATION_EXECUTE	__BIT(2)

static void
vmx_exit_epf(struct nvmm_machine *mach, struct nvmm_cpu *vcpu,
    struct nvmm_vcpu_exit *exit)
{
	uint64_t perm;
	gpaddr_t gpa;

	gpa = vmx_vmread(VMCS_GUEST_PHYSICAL_ADDRESS);

	exit->reason = NVMM_VCPU_EXIT_MEMORY;
	perm = vmx_vmread(VMCS_EXIT_QUALIFICATION);
	if (perm & VMX_EPT_VIOLATION_WRITE)
		exit->u.mem.prot = PROT_WRITE;
	else if (perm & VMX_EPT_VIOLATION_EXECUTE)
		exit->u.mem.prot = PROT_EXEC;
	else
		exit->u.mem.prot = PROT_READ;
	exit->u.mem.gpa = gpa;
	exit->u.mem.inst_len = 0;

	vmx_vcpu_state_provide(vcpu,
	    NVMM_X64_STATE_GPRS | NVMM_X64_STATE_SEGS |
	    NVMM_X64_STATE_CRS | NVMM_X64_STATE_MSRS);
}

/* -------------------------------------------------------------------------- */

static void
vmx_vcpu_guest_fpu_enter(struct nvmm_cpu *vcpu)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;

#if defined(__NetBSD__)
	x86_curthread_save_fpu();
#elif defined(__DragonFly__)
	/*
	 * NOTE: Host FPU state depends on whether the user program used the
	 *       FPU or not.  Need to use npxpush()/npxpop() to handle this.
	 */
	npxpush(&cpudata->hstate.hmctx);
#endif

	x86_restore_fpu(&cpudata->gxsave, vmx_xcr0_mask);
	if (vmx_xcr0_mask != 0) {
		x86_set_xcr(0, cpudata->gxcr0);
	}
}

static void
vmx_vcpu_guest_fpu_leave(struct nvmm_cpu *vcpu)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;

	if (vmx_xcr0_mask != 0) {
		x86_set_xcr(0, vmx_global_hstate.xcr0);
	}
	x86_save_fpu(&cpudata->gxsave, vmx_xcr0_mask);

#if defined(__NetBSD__)
	x86_curthread_restore_fpu();
#elif defined(__DragonFly__)
	npxpop(&cpudata->hstate.hmctx);
#endif
}

static void
vmx_vcpu_guest_dbregs_enter(struct nvmm_cpu *vcpu)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;

	x86_curthread_save_dbregs(cpudata->hstate.drs);

	x86_set_dr7(0);

	x86_set_dr0(cpudata->drs[NVMM_X64_DR_DR0]);
	x86_set_dr1(cpudata->drs[NVMM_X64_DR_DR1]);
	x86_set_dr2(cpudata->drs[NVMM_X64_DR_DR2]);
	x86_set_dr3(cpudata->drs[NVMM_X64_DR_DR3]);
	x86_set_dr6(cpudata->drs[NVMM_X64_DR_DR6]);
}

static void
vmx_vcpu_guest_dbregs_leave(struct nvmm_cpu *vcpu)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;

	cpudata->drs[NVMM_X64_DR_DR0] = x86_get_dr0();
	cpudata->drs[NVMM_X64_DR_DR1] = x86_get_dr1();
	cpudata->drs[NVMM_X64_DR_DR2] = x86_get_dr2();
	cpudata->drs[NVMM_X64_DR_DR3] = x86_get_dr3();
	cpudata->drs[NVMM_X64_DR_DR6] = x86_get_dr6();

	x86_curthread_restore_dbregs(cpudata->hstate.drs);
}

static void
vmx_vcpu_guest_misc_enter(struct nvmm_cpu *vcpu)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;

	/* This gets restored automatically by the CPU. */
	vmx_vmwrite(VMCS_HOST_IDTR_BASE, (uint64_t)os_curcpu_idt());
	vmx_vmwrite(VMCS_HOST_FS_BASE, rdmsr(MSR_FSBASE));
	vmx_vmwrite(VMCS_HOST_CR3, x86_get_cr3());
	vmx_vmwrite(VMCS_HOST_CR4, x86_get_cr4());

	/* Save the percpu host state. */
	cpudata->hstate.kernelgsbase = rdmsr(MSR_KERNELGSBASE);
}

static void
vmx_vcpu_guest_misc_leave(struct nvmm_cpu *vcpu)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;

	/* Restore the global host state. */
	wrmsr(MSR_STAR, vmx_global_hstate.star);
	wrmsr(MSR_LSTAR, vmx_global_hstate.lstar);
	wrmsr(MSR_CSTAR, vmx_global_hstate.cstar);
	wrmsr(MSR_SFMASK, vmx_global_hstate.sfmask);

	/* Restore the percpu host state. */
	wrmsr(MSR_KERNELGSBASE, cpudata->hstate.kernelgsbase);
}

/* -------------------------------------------------------------------------- */

#define VMX_INVVPID_ADDRESS		0
#define VMX_INVVPID_CONTEXT		1
#define VMX_INVVPID_ALL			2
#define VMX_INVVPID_CONTEXT_NOGLOBAL	3

#define VMX_INVEPT_CONTEXT		1
#define VMX_INVEPT_ALL			2

static inline void
vmx_gtlb_catchup(struct nvmm_cpu *vcpu, int hcpu)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;

	if (vcpu->hcpu_last != hcpu) {
		cpudata->gtlb_want_flush = true;
	}
}

static inline void
vmx_htlb_catchup(struct nvmm_cpu *vcpu, int hcpu)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;
	struct ept_desc ept_desc;

	if (__predict_true(!os_cpuset_isset(cpudata->htlb_want_flush, hcpu))) {
		return;
	}

	ept_desc.eptp = vmx_vmread(VMCS_EPTP);
	ept_desc.mbz = 0;
	vmx_invept(vmx_ept_flush_op, &ept_desc);
	os_cpuset_clear(cpudata->htlb_want_flush, hcpu);
}

static inline uint64_t
vmx_htlb_flush(struct nvmm_machine *mach, struct vmx_cpudata *cpudata)
{
	struct ept_desc ept_desc;
	uint64_t machgen;

#if defined(__NetBSD__)
	machgen = ((struct vmx_machdata *)mach->machdata)->mach_htlb_gen;
#elif defined(__DragonFly__)
	clear_xinvltlb();
	machgen = vmspace_pmap(mach->vm)->pm_invgen;
#endif
	if (__predict_true(machgen == cpudata->vcpu_htlb_gen)) {
		return machgen;
	}

	os_cpuset_setrunning(cpudata->htlb_want_flush);

	ept_desc.eptp = vmx_vmread(VMCS_EPTP);
	ept_desc.mbz = 0;
	vmx_invept(vmx_ept_flush_op, &ept_desc);

	return machgen;
}

static inline void
vmx_htlb_flush_ack(struct vmx_cpudata *cpudata, uint64_t machgen)
{
	cpudata->vcpu_htlb_gen = machgen;
	os_cpuset_clear(cpudata->htlb_want_flush, os_curcpu_number());
}

static inline void
vmx_exit_evt(struct vmx_cpudata *cpudata)
{
	uint64_t info, err, inslen;

	cpudata->evt_pending = false;

	info = vmx_vmread(VMCS_IDT_VECTORING_INFO);
	if (__predict_true((info & INTR_INFO_VALID) == 0)) {
		return;
	}
	err = vmx_vmread(VMCS_IDT_VECTORING_ERROR);

	vmx_vmwrite(VMCS_ENTRY_INTR_INFO, info);
	vmx_vmwrite(VMCS_ENTRY_EXCEPTION_ERROR, err);

	switch (__SHIFTOUT(info, INTR_INFO_TYPE)) {
	case INTR_TYPE_SW_INT:
	case INTR_TYPE_PRIV_SW_EXC:
	case INTR_TYPE_SW_EXC:
		inslen = vmx_vmread(VMCS_EXIT_INSTRUCTION_LENGTH);
		vmx_vmwrite(VMCS_ENTRY_INSTRUCTION_LENGTH, inslen);
	}

	cpudata->evt_pending = true;
}

static int
vmx_vcpu_run(struct nvmm_machine *mach, struct nvmm_cpu *vcpu,
    struct nvmm_vcpu_exit *exit)
{
	struct nvmm_comm_page *comm = vcpu->comm;
	struct vmx_cpudata *cpudata = vcpu->cpudata;
	struct vpid_desc vpid_desc;
	uint64_t exitcode;
	uint64_t intstate;
	uint64_t machgen;
	int hcpu, ret;
	int error = 0;
	bool launched;

	vmx_vmcs_enter(vcpu);

	vmx_vcpu_state_commit(vcpu);
	comm->state_cached = 0;

#ifndef __DragonFly__
	if (__predict_false(vmx_vcpu_event_commit(vcpu) != 0)) {
		vmx_vmcs_leave(vcpu);
		return EINVAL;
	}
#endif

	hcpu = os_curcpu_number();
	launched = cpudata->vmcs_launched;

	vmx_gtlb_catchup(vcpu, hcpu);
	vmx_htlb_catchup(vcpu, hcpu);

	if (vcpu->hcpu_last != hcpu) {
		vmx_vmwrite(VMCS_HOST_TR_SELECTOR, os_curcpu_tss_sel());
		vmx_vmwrite(VMCS_HOST_TR_BASE, (uint64_t)os_curcpu_tss());
		vmx_vmwrite(VMCS_HOST_GDTR_BASE, (uint64_t)os_curcpu_gdt());
		vmx_vmwrite(VMCS_HOST_GS_BASE, rdmsr(MSR_GSBASE));
		cpudata->gtsc_want_update = true;
		vcpu->hcpu_last = hcpu;

#ifdef __DragonFly__
		/*
		 * XXX: We aren't tracking overloaded CPUs (multiple vCPUs
		 *      scheduled on the same physical CPU) yet so there are
		 *      currently no calls to pmap_del_cpu().
		 */
		pmap_add_cpu(mach->vm, hcpu);
#endif
	}

	vmx_vcpu_guest_dbregs_enter(vcpu);
	vmx_vcpu_guest_misc_enter(vcpu);

	while (1) {
		if (cpudata->gtlb_want_flush) {
			vpid_desc.vpid = cpudata->asid;
			vpid_desc.addr = 0;
			vmx_invvpid(vmx_tlb_flush_op, &vpid_desc);
			cpudata->gtlb_want_flush = false;
		}

		if (__predict_false(cpudata->gtsc_want_update)) {
			vmx_vmwrite(VMCS_TSC_OFFSET, cpudata->gtsc_offset);
			cpudata->gtsc_want_update = false;
		}

		vmx_cli();
		vmx_vcpu_guest_fpu_enter(vcpu);
		machgen = vmx_htlb_flush(mach, cpudata);

#ifdef __DragonFly__
		/*
		 * Check for pending host events (e.g., interrupt, AST)
		 * to make the state safe to VM Entry.  This check must
		 * be done after the cli to avoid gd_reqflags pending
		 * races.
		 *
		 * Emulators may assume that event injection succeeds, but
		 * we have to return to process these events.  To deal with
		 * this, use ERESTART mechanics.
		 */
		if (__predict_false(mycpu->gd_reqflags & RQF_HVM_MASK)) {
			/* INVEPT executed, so ack hTLB flush. */
			vmx_htlb_flush_ack(cpudata, machgen);
			vmx_vcpu_guest_fpu_leave(vcpu);
			vmx_sti();
			exit->reason = NVMM_VCPU_EXIT_NONE;
			error = ERESTART;
			break;
		}

		/*
		 * Only commit event requests when we are absolutely
		 * sure that we can issue the vmlaunch/vmresume.
		 */
		if (__predict_false(vmx_vcpu_event_commit(vcpu) != 0)) {
			/* INVEPT executed, so ack hTLB flush. */
			vmx_htlb_flush_ack(cpudata, machgen);
			vmx_vcpu_guest_fpu_leave(vcpu);
			vmx_sti();
			exit->reason = NVMM_VCPU_EXIT_NONE;
			error = EINVAL;
			break;
		}
#endif

		x86_set_cr2(cpudata->gcr2);
		if (launched) {
			ret = vmx_vmresume(cpudata->gprs);
		} else {
			ret = vmx_vmlaunch(cpudata->gprs);
		}
		cpudata->gcr2 = x86_get_cr2();
		vmx_htlb_flush_ack(cpudata, machgen);
		vmx_vcpu_guest_fpu_leave(vcpu);
		vmx_sti();

		if (__predict_false(ret != 0)) {
			vmx_exit_invalid(exit, -1);
			break;
		}
		vmx_exit_evt(cpudata);

		launched = true;

		exitcode = vmx_vmread(VMCS_EXIT_REASON);
		exitcode &= __BITS(15,0);

		switch (exitcode) {
		case VMCS_EXITCODE_EXC_NMI:
			vmx_exit_exc_nmi(mach, vcpu, exit);
			break;
		case VMCS_EXITCODE_EXT_INT:
			exit->reason = NVMM_VCPU_EXIT_NONE;
			break;
		case VMCS_EXITCODE_CPUID:
			vmx_exit_cpuid(mach, vcpu, exit);
			break;
		case VMCS_EXITCODE_HLT:
			vmx_exit_hlt(mach, vcpu, exit);
			break;
		case VMCS_EXITCODE_CR:
			vmx_exit_cr(mach, vcpu, exit);
			break;
		case VMCS_EXITCODE_IO:
			vmx_exit_io(mach, vcpu, exit);
			break;
		case VMCS_EXITCODE_RDMSR:
			vmx_exit_rdmsr(mach, vcpu, exit);
			break;
		case VMCS_EXITCODE_WRMSR:
			vmx_exit_wrmsr(mach, vcpu, exit);
			break;
		case VMCS_EXITCODE_SHUTDOWN:
			exit->reason = NVMM_VCPU_EXIT_SHUTDOWN;
			break;
		case VMCS_EXITCODE_MONITOR:
			vmx_exit_insn(exit, NVMM_VCPU_EXIT_MONITOR);
			break;
		case VMCS_EXITCODE_MWAIT:
			vmx_exit_insn(exit, NVMM_VCPU_EXIT_MWAIT);
			break;
		case VMCS_EXITCODE_XSETBV:
			vmx_exit_xsetbv(mach, vcpu, exit);
			break;
		case VMCS_EXITCODE_RDPMC:
		case VMCS_EXITCODE_RDTSCP:
		case VMCS_EXITCODE_INVVPID:
		case VMCS_EXITCODE_INVEPT:
		case VMCS_EXITCODE_VMCALL:
		case VMCS_EXITCODE_VMCLEAR:
		case VMCS_EXITCODE_VMLAUNCH:
		case VMCS_EXITCODE_VMPTRLD:
		case VMCS_EXITCODE_VMPTRST:
		case VMCS_EXITCODE_VMREAD:
		case VMCS_EXITCODE_VMRESUME:
		case VMCS_EXITCODE_VMWRITE:
		case VMCS_EXITCODE_VMXOFF:
		case VMCS_EXITCODE_VMXON:
			vmx_inject_ud(vcpu);
			exit->reason = NVMM_VCPU_EXIT_NONE;
			break;
		case VMCS_EXITCODE_EPT_VIOLATION:
			vmx_exit_epf(mach, vcpu, exit);
			break;
		case VMCS_EXITCODE_INT_WINDOW:
			vmx_event_waitexit_disable(vcpu, false);
			exit->reason = NVMM_VCPU_EXIT_INT_READY;
			break;
		case VMCS_EXITCODE_NMI_WINDOW:
			vmx_event_waitexit_disable(vcpu, true);
			exit->reason = NVMM_VCPU_EXIT_NMI_READY;
			break;
		default:
			vmx_exit_invalid(exit, exitcode);
			break;
		}

		/* If no reason to return to userland, keep rolling. */
		if (os_return_needed()) {
			break;
		}
		if (exit->reason != NVMM_VCPU_EXIT_NONE) {
			break;
		}
	}

	cpudata->vmcs_launched = launched;

	vmx_vcpu_guest_misc_leave(vcpu);
	vmx_vcpu_guest_dbregs_leave(vcpu);

	exit->exitstate.rflags = vmx_vmread(VMCS_GUEST_RFLAGS);
	exit->exitstate.cr8 = cpudata->gcr8;
	intstate = vmx_vmread(VMCS_GUEST_INTERRUPTIBILITY);
	exit->exitstate.int_shadow =
	    (intstate & (INT_STATE_STI|INT_STATE_MOVSS)) != 0;
	exit->exitstate.int_window_exiting = cpudata->int_window_exit;
	exit->exitstate.nmi_window_exiting = cpudata->nmi_window_exit;
	exit->exitstate.evt_pending = cpudata->evt_pending;

	vmx_vmcs_leave(vcpu);

	return error;
}

/* -------------------------------------------------------------------------- */

static void
vmx_vcpu_msr_allow(uint8_t *bitmap, uint64_t msr, bool read, bool write)
{
	uint64_t byte;
	uint8_t bitoff;

	if (msr < 0x00002000) {
		/* Range 1 */
		byte = ((msr - 0x00000000) / 8) + 0;
	} else if (msr >= 0xC0000000 && msr < 0xC0002000) {
		/* Range 2 */
		byte = ((msr - 0xC0000000) / 8) + 1024;
	} else {
		panic("%s: wrong range", __func__);
	}

	bitoff = (msr & 0x7);

	if (read) {
		bitmap[byte] &= ~__BIT(bitoff);
	}
	if (write) {
		bitmap[2048 + byte] &= ~__BIT(bitoff);
	}
}

#define VMX_SEG_ATTRIB_TYPE		__BITS(3,0)
#define VMX_SEG_ATTRIB_S		__BIT(4)
#define VMX_SEG_ATTRIB_DPL		__BITS(6,5)
#define VMX_SEG_ATTRIB_P		__BIT(7)
#define VMX_SEG_ATTRIB_AVL		__BIT(12)
#define VMX_SEG_ATTRIB_L		__BIT(13)
#define VMX_SEG_ATTRIB_DEF		__BIT(14)
#define VMX_SEG_ATTRIB_G		__BIT(15)
#define VMX_SEG_ATTRIB_UNUSABLE		__BIT(16)

static void
vmx_vcpu_setstate_seg(const struct nvmm_x64_state_seg *segs, int idx)
{
	uint64_t attrib;

	attrib =
	    __SHIFTIN(segs[idx].attrib.type, VMX_SEG_ATTRIB_TYPE) |
	    __SHIFTIN(segs[idx].attrib.s, VMX_SEG_ATTRIB_S) |
	    __SHIFTIN(segs[idx].attrib.dpl, VMX_SEG_ATTRIB_DPL) |
	    __SHIFTIN(segs[idx].attrib.p, VMX_SEG_ATTRIB_P) |
	    __SHIFTIN(segs[idx].attrib.avl, VMX_SEG_ATTRIB_AVL) |
	    __SHIFTIN(segs[idx].attrib.l, VMX_SEG_ATTRIB_L) |
	    __SHIFTIN(segs[idx].attrib.def, VMX_SEG_ATTRIB_DEF) |
	    __SHIFTIN(segs[idx].attrib.g, VMX_SEG_ATTRIB_G) |
	    (!segs[idx].attrib.p ? VMX_SEG_ATTRIB_UNUSABLE : 0);

	if (idx != NVMM_X64_SEG_GDT && idx != NVMM_X64_SEG_IDT) {
		vmx_vmwrite(vmx_guest_segs[idx].selector, segs[idx].selector);
		vmx_vmwrite(vmx_guest_segs[idx].attrib, attrib);
	}
	vmx_vmwrite(vmx_guest_segs[idx].limit, segs[idx].limit);
	vmx_vmwrite(vmx_guest_segs[idx].base, segs[idx].base);
}

static void
vmx_vcpu_getstate_seg(struct nvmm_x64_state_seg *segs, int idx)
{
	uint64_t selector = 0, attrib = 0, base, limit;

	if (idx != NVMM_X64_SEG_GDT && idx != NVMM_X64_SEG_IDT) {
		selector = vmx_vmread(vmx_guest_segs[idx].selector);
		attrib = vmx_vmread(vmx_guest_segs[idx].attrib);
	}
	limit = vmx_vmread(vmx_guest_segs[idx].limit);
	base = vmx_vmread(vmx_guest_segs[idx].base);

	segs[idx].selector = selector;
	segs[idx].limit = limit;
	segs[idx].base = base;
	segs[idx].attrib.type = __SHIFTOUT(attrib, VMX_SEG_ATTRIB_TYPE);
	segs[idx].attrib.s = __SHIFTOUT(attrib, VMX_SEG_ATTRIB_S);
	segs[idx].attrib.dpl = __SHIFTOUT(attrib, VMX_SEG_ATTRIB_DPL);
	segs[idx].attrib.p = __SHIFTOUT(attrib, VMX_SEG_ATTRIB_P);
	segs[idx].attrib.avl = __SHIFTOUT(attrib, VMX_SEG_ATTRIB_AVL);
	segs[idx].attrib.l = __SHIFTOUT(attrib, VMX_SEG_ATTRIB_L);
	segs[idx].attrib.def = __SHIFTOUT(attrib, VMX_SEG_ATTRIB_DEF);
	segs[idx].attrib.g = __SHIFTOUT(attrib, VMX_SEG_ATTRIB_G);
	if (attrib & VMX_SEG_ATTRIB_UNUSABLE) {
		segs[idx].attrib.p = 0;
	}
}

static inline bool
vmx_state_gtlb_flush(const struct nvmm_x64_state *state, uint64_t flags)
{
	uint64_t cr0, cr3, cr4, efer;

	if (flags & NVMM_X64_STATE_CRS) {
		cr0 = vmx_vmread(VMCS_GUEST_CR0);
		if ((cr0 ^ state->crs[NVMM_X64_CR_CR0]) & CR0_TLB_FLUSH) {
			return true;
		}
		cr3 = vmx_vmread(VMCS_GUEST_CR3);
		if (cr3 != state->crs[NVMM_X64_CR_CR3]) {
			return true;
		}
		cr4 = vmx_vmread(VMCS_GUEST_CR4);
		if ((cr4 ^ state->crs[NVMM_X64_CR_CR4]) & CR4_TLB_FLUSH) {
			return true;
		}
	}

	if (flags & NVMM_X64_STATE_MSRS) {
		efer = vmx_vmread(VMCS_GUEST_IA32_EFER);
		if ((efer ^
		     state->msrs[NVMM_X64_MSR_EFER]) & EFER_TLB_FLUSH) {
			return true;
		}
	}

	return false;
}

static void
vmx_vcpu_setstate(struct nvmm_cpu *vcpu)
{
	struct nvmm_comm_page *comm = vcpu->comm;
	const struct nvmm_x64_state *state = &comm->state;
	struct vmx_cpudata *cpudata = vcpu->cpudata;
	struct msr_entry *gmsr = cpudata->gmsr;
	struct nvmm_x64_state_fpu *fpustate;
	uint64_t ctls1, intstate;
	uint64_t flags;

	flags = comm->state_wanted;

	vmx_vmcs_enter(vcpu);

	if (vmx_state_gtlb_flush(state, flags)) {
		cpudata->gtlb_want_flush = true;
	}

	if (flags & NVMM_X64_STATE_SEGS) {
		vmx_vcpu_setstate_seg(state->segs, NVMM_X64_SEG_CS);
		vmx_vcpu_setstate_seg(state->segs, NVMM_X64_SEG_DS);
		vmx_vcpu_setstate_seg(state->segs, NVMM_X64_SEG_ES);
		vmx_vcpu_setstate_seg(state->segs, NVMM_X64_SEG_FS);
		vmx_vcpu_setstate_seg(state->segs, NVMM_X64_SEG_GS);
		vmx_vcpu_setstate_seg(state->segs, NVMM_X64_SEG_SS);
		vmx_vcpu_setstate_seg(state->segs, NVMM_X64_SEG_GDT);
		vmx_vcpu_setstate_seg(state->segs, NVMM_X64_SEG_IDT);
		vmx_vcpu_setstate_seg(state->segs, NVMM_X64_SEG_LDT);
		vmx_vcpu_setstate_seg(state->segs, NVMM_X64_SEG_TR);
	}

	CTASSERT(sizeof(cpudata->gprs) == sizeof(state->gprs));
	if (flags & NVMM_X64_STATE_GPRS) {
		memcpy(cpudata->gprs, state->gprs, sizeof(state->gprs));

		vmx_vmwrite(VMCS_GUEST_RIP, state->gprs[NVMM_X64_GPR_RIP]);
		vmx_vmwrite(VMCS_GUEST_RSP, state->gprs[NVMM_X64_GPR_RSP]);
		vmx_vmwrite(VMCS_GUEST_RFLAGS, state->gprs[NVMM_X64_GPR_RFLAGS]);
	}

	if (flags & NVMM_X64_STATE_CRS) {
		/*
		 * CR0_ET must be 1 both in the shadow and the real register.
		 * CR0_NE must be 1 in the real register.
		 * CR0_NW and CR0_CD must be 0 in the real register.
		 */
		vmx_vmwrite(VMCS_CR0_SHADOW,
		    (state->crs[NVMM_X64_CR_CR0] & CR0_STATIC_MASK) |
		    CR0_ET);
		vmx_vmwrite(VMCS_GUEST_CR0,
		    (state->crs[NVMM_X64_CR_CR0] & ~CR0_STATIC_MASK) |
		    CR0_ET | CR0_NE);

		cpudata->gcr2 = state->crs[NVMM_X64_CR_CR2];

		/* XXX We are not handling PDPTE here. */
		vmx_vmwrite(VMCS_GUEST_CR3, state->crs[NVMM_X64_CR_CR3]);

		/* CR4_VMXE is mandatory. */
		vmx_vmwrite(VMCS_GUEST_CR4,
		    (state->crs[NVMM_X64_CR_CR4] & CR4_VALID) | CR4_VMXE);

		cpudata->gcr8 = state->crs[NVMM_X64_CR_CR8];

		if (vmx_xcr0_mask != 0) {
			/* Clear illegal XCR0 bits, set mandatory X87 bit. */
			cpudata->gxcr0 = state->crs[NVMM_X64_CR_XCR0];
			cpudata->gxcr0 &= vmx_xcr0_mask;
			cpudata->gxcr0 |= XCR0_X87;
		}
	}

	CTASSERT(sizeof(cpudata->drs) == sizeof(state->drs));
	if (flags & NVMM_X64_STATE_DRS) {
		memcpy(cpudata->drs, state->drs, sizeof(state->drs));

		cpudata->drs[NVMM_X64_DR_DR6] &= 0xFFFFFFFF;
		vmx_vmwrite(VMCS_GUEST_DR7, cpudata->drs[NVMM_X64_DR_DR7]);
	}

	if (flags & NVMM_X64_STATE_MSRS) {
		gmsr[VMX_MSRLIST_STAR].val =
		    state->msrs[NVMM_X64_MSR_STAR];
		gmsr[VMX_MSRLIST_LSTAR].val =
		    state->msrs[NVMM_X64_MSR_LSTAR];
		gmsr[VMX_MSRLIST_CSTAR].val =
		    state->msrs[NVMM_X64_MSR_CSTAR];
		gmsr[VMX_MSRLIST_SFMASK].val =
		    state->msrs[NVMM_X64_MSR_SFMASK];
		gmsr[VMX_MSRLIST_KERNELGSBASE].val =
		    state->msrs[NVMM_X64_MSR_KERNELGSBASE];

		vmx_vmwrite(VMCS_GUEST_IA32_EFER,
		    state->msrs[NVMM_X64_MSR_EFER]);
		vmx_vmwrite(VMCS_GUEST_IA32_PAT,
		    state->msrs[NVMM_X64_MSR_PAT]);
		vmx_vmwrite(VMCS_GUEST_IA32_SYSENTER_CS,
		    state->msrs[NVMM_X64_MSR_SYSENTER_CS]);
		vmx_vmwrite(VMCS_GUEST_IA32_SYSENTER_ESP,
		    state->msrs[NVMM_X64_MSR_SYSENTER_ESP]);
		vmx_vmwrite(VMCS_GUEST_IA32_SYSENTER_EIP,
		    state->msrs[NVMM_X64_MSR_SYSENTER_EIP]);

		/*
		 * The emulator might NOT want to set the TSC, because doing
		 * so would destroy TSC MP-synchronization across CPUs.  Try
		 * to figure out what the emulator meant to do.
		 *
		 * If writing the last TSC value we reported via getstate or
		 * a zero value, assume that the emulator does not want to
		 * write to the TSC.
		 */
		if (state->msrs[NVMM_X64_MSR_TSC] != cpudata->gtsc_match &&
		    state->msrs[NVMM_X64_MSR_TSC] != 0) {
			cpudata->gtsc_offset =
			    state->msrs[NVMM_X64_MSR_TSC] - rdtsc();
			cpudata->gtsc_want_update = true;
		}

		/* ENTRY_CTLS_LONG_MODE must match EFER_LMA. */
		ctls1 = vmx_vmread(VMCS_ENTRY_CTLS);
		if (state->msrs[NVMM_X64_MSR_EFER] & EFER_LMA) {
			ctls1 |= ENTRY_CTLS_LONG_MODE;
		} else {
			ctls1 &= ~ENTRY_CTLS_LONG_MODE;
		}
		vmx_vmwrite(VMCS_ENTRY_CTLS, ctls1);
	}

	if (flags & NVMM_X64_STATE_INTR) {
		intstate = vmx_vmread(VMCS_GUEST_INTERRUPTIBILITY);
		intstate &= ~(INT_STATE_STI|INT_STATE_MOVSS);
		if (state->intr.int_shadow) {
			intstate |= INT_STATE_MOVSS;
		}
		vmx_vmwrite(VMCS_GUEST_INTERRUPTIBILITY, intstate);

		if (state->intr.int_window_exiting) {
			vmx_event_waitexit_enable(vcpu, false);
		} else {
			vmx_event_waitexit_disable(vcpu, false);
		}

		if (state->intr.nmi_window_exiting) {
			vmx_event_waitexit_enable(vcpu, true);
		} else {
			vmx_event_waitexit_disable(vcpu, true);
		}
	}

	CTASSERT(sizeof(cpudata->gxsave.fpu) == sizeof(state->fpu));
	if (flags & NVMM_X64_STATE_FPU) {
		memcpy(&cpudata->gxsave.fpu, &state->fpu, sizeof(state->fpu));

		fpustate = (struct nvmm_x64_state_fpu *)&cpudata->gxsave.fpu;
		fpustate->fx_mxcsr_mask &= x86_fpu_mxcsr_mask;
		fpustate->fx_mxcsr &= fpustate->fx_mxcsr_mask;

		if (vmx_xcr0_mask != 0) {
			/* Reset XSTATE_BV, to force a reload. */
			cpudata->gxsave.xstate_bv = vmx_xcr0_mask;
		}
	}

	vmx_vmcs_leave(vcpu);

	comm->state_wanted = 0;
	comm->state_cached |= flags;
}

static void
vmx_vcpu_getstate(struct nvmm_cpu *vcpu)
{
	struct nvmm_comm_page *comm = vcpu->comm;
	struct nvmm_x64_state *state = &comm->state;
	struct vmx_cpudata *cpudata = vcpu->cpudata;
	struct msr_entry *gmsr = cpudata->gmsr;
	uint64_t intstate, flags;

	flags = comm->state_wanted;

	vmx_vmcs_enter(vcpu);

	if (flags & NVMM_X64_STATE_SEGS) {
		vmx_vcpu_getstate_seg(state->segs, NVMM_X64_SEG_CS);
		vmx_vcpu_getstate_seg(state->segs, NVMM_X64_SEG_DS);
		vmx_vcpu_getstate_seg(state->segs, NVMM_X64_SEG_ES);
		vmx_vcpu_getstate_seg(state->segs, NVMM_X64_SEG_FS);
		vmx_vcpu_getstate_seg(state->segs, NVMM_X64_SEG_GS);
		vmx_vcpu_getstate_seg(state->segs, NVMM_X64_SEG_SS);
		vmx_vcpu_getstate_seg(state->segs, NVMM_X64_SEG_GDT);
		vmx_vcpu_getstate_seg(state->segs, NVMM_X64_SEG_IDT);
		vmx_vcpu_getstate_seg(state->segs, NVMM_X64_SEG_LDT);
		vmx_vcpu_getstate_seg(state->segs, NVMM_X64_SEG_TR);
	}

	CTASSERT(sizeof(cpudata->gprs) == sizeof(state->gprs));
	if (flags & NVMM_X64_STATE_GPRS) {
		memcpy(state->gprs, cpudata->gprs, sizeof(state->gprs));

		state->gprs[NVMM_X64_GPR_RIP] = vmx_vmread(VMCS_GUEST_RIP);
		state->gprs[NVMM_X64_GPR_RSP] = vmx_vmread(VMCS_GUEST_RSP);
		state->gprs[NVMM_X64_GPR_RFLAGS] = vmx_vmread(VMCS_GUEST_RFLAGS);
	}

	if (flags & NVMM_X64_STATE_CRS) {
		state->crs[NVMM_X64_CR_CR0] =
		    (vmx_vmread(VMCS_CR0_SHADOW) & CR0_STATIC_MASK) |
		    (vmx_vmread(VMCS_GUEST_CR0) & ~CR0_STATIC_MASK);
		state->crs[NVMM_X64_CR_CR2] = cpudata->gcr2;
		state->crs[NVMM_X64_CR_CR3] = vmx_vmread(VMCS_GUEST_CR3);
		state->crs[NVMM_X64_CR_CR4] = vmx_vmread(VMCS_GUEST_CR4);
		state->crs[NVMM_X64_CR_CR8] = cpudata->gcr8;
		state->crs[NVMM_X64_CR_XCR0] = cpudata->gxcr0;

		/* Hide VMXE. */
		state->crs[NVMM_X64_CR_CR4] &= ~CR4_VMXE;
	}

	CTASSERT(sizeof(cpudata->drs) == sizeof(state->drs));
	if (flags & NVMM_X64_STATE_DRS) {
		memcpy(state->drs, cpudata->drs, sizeof(state->drs));

		state->drs[NVMM_X64_DR_DR7] = vmx_vmread(VMCS_GUEST_DR7);
	}

	if (flags & NVMM_X64_STATE_MSRS) {
		state->msrs[NVMM_X64_MSR_STAR] =
		    gmsr[VMX_MSRLIST_STAR].val;
		state->msrs[NVMM_X64_MSR_LSTAR] =
		    gmsr[VMX_MSRLIST_LSTAR].val;
		state->msrs[NVMM_X64_MSR_CSTAR] =
		    gmsr[VMX_MSRLIST_CSTAR].val;
		state->msrs[NVMM_X64_MSR_SFMASK] =
		    gmsr[VMX_MSRLIST_SFMASK].val;
		state->msrs[NVMM_X64_MSR_KERNELGSBASE] =
		    gmsr[VMX_MSRLIST_KERNELGSBASE].val;
		state->msrs[NVMM_X64_MSR_EFER] =
		    vmx_vmread(VMCS_GUEST_IA32_EFER);
		state->msrs[NVMM_X64_MSR_PAT] =
		    vmx_vmread(VMCS_GUEST_IA32_PAT);
		state->msrs[NVMM_X64_MSR_SYSENTER_CS] =
		    vmx_vmread(VMCS_GUEST_IA32_SYSENTER_CS);
		state->msrs[NVMM_X64_MSR_SYSENTER_ESP] =
		    vmx_vmread(VMCS_GUEST_IA32_SYSENTER_ESP);
		state->msrs[NVMM_X64_MSR_SYSENTER_EIP] =
		    vmx_vmread(VMCS_GUEST_IA32_SYSENTER_EIP);
		state->msrs[NVMM_X64_MSR_TSC] = rdtsc() + cpudata->gtsc_offset;

		/* Save reported TSC value for later setstate check. */
		cpudata->gtsc_match = state->msrs[NVMM_X64_MSR_TSC];
	}

	if (flags & NVMM_X64_STATE_INTR) {
		intstate = vmx_vmread(VMCS_GUEST_INTERRUPTIBILITY);
		state->intr.int_shadow =
		    (intstate & (INT_STATE_STI|INT_STATE_MOVSS)) != 0;
		state->intr.int_window_exiting = cpudata->int_window_exit;
		state->intr.nmi_window_exiting = cpudata->nmi_window_exit;
		state->intr.evt_pending = cpudata->evt_pending;
	}

	CTASSERT(sizeof(cpudata->gxsave.fpu) == sizeof(state->fpu));
	if (flags & NVMM_X64_STATE_FPU) {
		memcpy(&state->fpu, &cpudata->gxsave.fpu, sizeof(state->fpu));
	}

	vmx_vmcs_leave(vcpu);

	comm->state_wanted = 0;
	comm->state_cached |= flags;
}

static void
vmx_vcpu_state_provide(struct nvmm_cpu *vcpu, uint64_t flags)
{
	vcpu->comm->state_wanted = flags;
	vmx_vcpu_getstate(vcpu);
}

static void
vmx_vcpu_state_commit(struct nvmm_cpu *vcpu)
{
	vcpu->comm->state_wanted = vcpu->comm->state_commit;
	vcpu->comm->state_commit = 0;
	vmx_vcpu_setstate(vcpu);
}

/* -------------------------------------------------------------------------- */

static void
vmx_asid_alloc(struct nvmm_cpu *vcpu)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;
	size_t i, oct, bit;

	os_mtx_lock(&vmx_asidlock);

	for (i = 0; i < vmx_maxasid; i++) {
		oct = i / 8;
		bit = i % 8;

		if (vmx_asidmap[oct] & __BIT(bit)) {
			continue;
		}

		cpudata->asid = i;

		vmx_asidmap[oct] |= __BIT(bit);
		vmx_vmwrite(VMCS_VPID, i);
		os_mtx_unlock(&vmx_asidlock);
		return;
	}

	os_mtx_unlock(&vmx_asidlock);

	panic("%s: impossible", __func__);
}

static void
vmx_asid_free(struct nvmm_cpu *vcpu)
{
	size_t oct, bit;
	uint64_t asid;

	asid = vmx_vmread(VMCS_VPID);

	oct = asid / 8;
	bit = asid % 8;

	os_mtx_lock(&vmx_asidlock);
	vmx_asidmap[oct] &= ~__BIT(bit);
	os_mtx_unlock(&vmx_asidlock);
}

static void
vmx_vcpu_init(struct nvmm_machine *mach, struct nvmm_cpu *vcpu)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;
	struct vmcs *vmcs = cpudata->vmcs;
	struct msr_entry *gmsr = cpudata->gmsr;
	uint64_t rev, eptp;

	rev = vmx_get_revision();

	memset(vmcs, 0, VMCS_SIZE);
	vmcs->ident = __SHIFTIN(rev, VMCS_IDENT_REVISION);
	vmcs->abort = 0;

	vmx_vmcs_enter(vcpu);

	/* No link pointer. */
	vmx_vmwrite(VMCS_LINK_POINTER, 0xFFFFFFFFFFFFFFFF);

	/* Install the CTLSs. */
	vmx_vmwrite(VMCS_PINBASED_CTLS, vmx_pinbased_ctls);
	vmx_vmwrite(VMCS_PROCBASED_CTLS, vmx_procbased_ctls);
	vmx_vmwrite(VMCS_PROCBASED_CTLS2, vmx_procbased_ctls2);
	vmx_vmwrite(VMCS_ENTRY_CTLS, vmx_entry_ctls);
	vmx_vmwrite(VMCS_EXIT_CTLS, vmx_exit_ctls);

	/* Allow direct access to certain MSRs. */
	memset(cpudata->msrbm, 0xFF, MSRBM_SIZE);
	vmx_vcpu_msr_allow(cpudata->msrbm, MSR_EFER, true, true);
	vmx_vcpu_msr_allow(cpudata->msrbm, MSR_STAR, true, true);
	vmx_vcpu_msr_allow(cpudata->msrbm, MSR_LSTAR, true, true);
	vmx_vcpu_msr_allow(cpudata->msrbm, MSR_CSTAR, true, true);
	vmx_vcpu_msr_allow(cpudata->msrbm, MSR_SFMASK, true, true);
	vmx_vcpu_msr_allow(cpudata->msrbm, MSR_KERNELGSBASE, true, true);
	vmx_vcpu_msr_allow(cpudata->msrbm, MSR_SYSENTER_CS, true, true);
	vmx_vcpu_msr_allow(cpudata->msrbm, MSR_SYSENTER_ESP, true, true);
	vmx_vcpu_msr_allow(cpudata->msrbm, MSR_SYSENTER_EIP, true, true);
	vmx_vcpu_msr_allow(cpudata->msrbm, MSR_FSBASE, true, true);
	vmx_vcpu_msr_allow(cpudata->msrbm, MSR_GSBASE, true, true);
	vmx_vcpu_msr_allow(cpudata->msrbm, MSR_TSC, true, false);
	vmx_vmwrite(VMCS_MSR_BITMAP, (uint64_t)cpudata->msrbm_pa);

	/*
	 * List of Guest MSRs loaded on VMENTRY, saved on VMEXIT. This
	 * includes the L1D_FLUSH MSR, to mitigate L1TF.
	 */
	gmsr[VMX_MSRLIST_STAR].msr = MSR_STAR;
	gmsr[VMX_MSRLIST_STAR].val = 0;
	gmsr[VMX_MSRLIST_LSTAR].msr = MSR_LSTAR;
	gmsr[VMX_MSRLIST_LSTAR].val = 0;
	gmsr[VMX_MSRLIST_CSTAR].msr = MSR_CSTAR;
	gmsr[VMX_MSRLIST_CSTAR].val = 0;
	gmsr[VMX_MSRLIST_SFMASK].msr = MSR_SFMASK;
	gmsr[VMX_MSRLIST_SFMASK].val = 0;
	gmsr[VMX_MSRLIST_KERNELGSBASE].msr = MSR_KERNELGSBASE;
	gmsr[VMX_MSRLIST_KERNELGSBASE].val = 0;
	gmsr[VMX_MSRLIST_L1DFLUSH].msr = MSR_IA32_FLUSH_CMD;
	gmsr[VMX_MSRLIST_L1DFLUSH].val = IA32_FLUSH_CMD_L1D_FLUSH;
	vmx_vmwrite(VMCS_ENTRY_MSR_LOAD_ADDRESS, cpudata->gmsr_pa);
	vmx_vmwrite(VMCS_EXIT_MSR_STORE_ADDRESS, cpudata->gmsr_pa);
	vmx_vmwrite(VMCS_ENTRY_MSR_LOAD_COUNT, vmx_msrlist_entry_nmsr);
	vmx_vmwrite(VMCS_EXIT_MSR_STORE_COUNT, VMX_MSRLIST_EXIT_NMSR);

	/* Set the CR0 mask. Any change of these bits causes a VMEXIT. */
	vmx_vmwrite(VMCS_CR0_MASK, CR0_STATIC_MASK);

	/* Force unsupported CR4 fields to zero. */
	vmx_vmwrite(VMCS_CR4_MASK, CR4_INVALID);
	vmx_vmwrite(VMCS_CR4_SHADOW, 0);

	/* Set the Host state for resuming. */
	vmx_vmwrite(VMCS_HOST_RIP, (uint64_t)(uintptr_t)vmx_resume_rip);
	vmx_vmwrite(VMCS_HOST_CS_SELECTOR, GSEL(GCODE_SEL, SEL_KPL));
	vmx_vmwrite(VMCS_HOST_SS_SELECTOR, GSEL(GDATA_SEL, SEL_KPL));
	vmx_vmwrite(VMCS_HOST_DS_SELECTOR, GSEL(GDATA_SEL, SEL_KPL));
	vmx_vmwrite(VMCS_HOST_ES_SELECTOR, GSEL(GDATA_SEL, SEL_KPL));
	vmx_vmwrite(VMCS_HOST_FS_SELECTOR, 0);
	vmx_vmwrite(VMCS_HOST_GS_SELECTOR, 0);
	vmx_vmwrite(VMCS_HOST_IA32_SYSENTER_CS, 0);
	vmx_vmwrite(VMCS_HOST_IA32_SYSENTER_ESP, 0);
	vmx_vmwrite(VMCS_HOST_IA32_SYSENTER_EIP, 0);
	vmx_vmwrite(VMCS_HOST_IA32_PAT, rdmsr(MSR_CR_PAT));
	vmx_vmwrite(VMCS_HOST_IA32_EFER, rdmsr(MSR_EFER));
	vmx_vmwrite(VMCS_HOST_CR0, x86_get_cr0() & ~CR0_TS);

	/* Generate ASID. */
	vmx_asid_alloc(vcpu);

	/* Enable Extended Paging, 4-Level. */
	eptp =
	    __SHIFTIN(vmx_eptp_type, EPTP_TYPE) |
	    __SHIFTIN(4-1, EPTP_WALKLEN) |
	    (vmx_ept_has_ad ? EPTP_FLAGS_AD : 0) |
	    os_vmspace_pdirpa(mach->vm);
	vmx_vmwrite(VMCS_EPTP, eptp);

	/* Init IA32_MISC_ENABLE. */
	cpudata->gmsr_misc_enable = rdmsr(MSR_MISC_ENABLE);
	cpudata->gmsr_misc_enable &=
	    ~(IA32_MISC_PERFMON_EN|IA32_MISC_EISST_EN|IA32_MISC_MWAIT_EN);
	cpudata->gmsr_misc_enable |=
	    (IA32_MISC_BTS_UNAVAIL|IA32_MISC_PEBS_UNAVAIL);

	/* Init XSAVE header. */
	cpudata->gxsave.xstate_bv = vmx_xcr0_mask;
	cpudata->gxsave.xcomp_bv = 0;

	/* Install the RESET state. */
	memcpy(&vcpu->comm->state, &nvmm_x86_reset_state,
	    sizeof(nvmm_x86_reset_state));
	vcpu->comm->state_wanted = NVMM_X64_STATE_ALL;
	vcpu->comm->state_cached = 0;
	vmx_vcpu_setstate(vcpu);

	vmx_vmcs_leave(vcpu);
}

static int
vmx_vcpu_create(struct nvmm_machine *mach, struct nvmm_cpu *vcpu)
{
	struct vmx_cpudata *cpudata;
	int error;

	/* Allocate the VMX cpudata. */
	cpudata = (struct vmx_cpudata *)os_pagemem_zalloc(sizeof(*cpudata));
	if (cpudata == NULL)
		return ENOMEM;

	vcpu->cpudata = cpudata;

	/* VMCS */
	error = os_contigpa_zalloc(&cpudata->vmcs_pa,
	    (vaddr_t *)&cpudata->vmcs, VMCS_NPAGES);
	if (error)
		goto error;

	/* MSR Bitmap */
	error = os_contigpa_zalloc(&cpudata->msrbm_pa,
	    (vaddr_t *)&cpudata->msrbm, MSRBM_NPAGES);
	if (error)
		goto error;

	/* Guest MSR List */
	error = os_contigpa_zalloc(&cpudata->gmsr_pa,
	    (vaddr_t *)&cpudata->gmsr, 1);
	if (error)
		goto error;

	os_cpuset_init(&cpudata->htlb_want_flush);

	/* Init the VCPU info. */
	vmx_vcpu_init(mach, vcpu);

	return 0;

error:
	if (cpudata->vmcs_pa) {
		os_contigpa_free(cpudata->vmcs_pa, (vaddr_t)cpudata->vmcs,
		    VMCS_NPAGES);
	}
	if (cpudata->msrbm_pa) {
		os_contigpa_free(cpudata->msrbm_pa, (vaddr_t)cpudata->msrbm,
		    MSRBM_NPAGES);
	}
	if (cpudata->gmsr_pa) {
		os_contigpa_free(cpudata->gmsr_pa, (vaddr_t)cpudata->gmsr, 1);
	}
	os_pagemem_free(cpudata, sizeof(*cpudata));
	return error;
}

static void
vmx_vcpu_destroy(struct nvmm_machine *mach, struct nvmm_cpu *vcpu)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;

	vmx_vmcs_enter(vcpu);
	vmx_asid_free(vcpu);
	vmx_vmcs_destroy(vcpu);

	os_cpuset_destroy(cpudata->htlb_want_flush);

	os_contigpa_free(cpudata->vmcs_pa, (vaddr_t)cpudata->vmcs,
	    VMCS_NPAGES);
	os_contigpa_free(cpudata->msrbm_pa, (vaddr_t)cpudata->msrbm,
	    MSRBM_NPAGES);
	os_contigpa_free(cpudata->gmsr_pa, (vaddr_t)cpudata->gmsr,
	    1);
	os_pagemem_free(cpudata, sizeof(*cpudata));
}

/* -------------------------------------------------------------------------- */

static int
vmx_vcpu_configure_cpuid(struct vmx_cpudata *cpudata, void *data)
{
	struct nvmm_vcpu_conf_cpuid *cpuid = data;
	size_t i;

	if (__predict_false(cpuid->mask && cpuid->exit)) {
		return EINVAL;
	}
	if (__predict_false(cpuid->mask &&
	    ((cpuid->u.mask.set.eax & cpuid->u.mask.del.eax) ||
	     (cpuid->u.mask.set.ebx & cpuid->u.mask.del.ebx) ||
	     (cpuid->u.mask.set.ecx & cpuid->u.mask.del.ecx) ||
	     (cpuid->u.mask.set.edx & cpuid->u.mask.del.edx)))) {
		return EINVAL;
	}

	/* If unset, delete, to restore the default behavior. */
	if (!cpuid->mask && !cpuid->exit) {
		for (i = 0; i < VMX_NCPUIDS; i++) {
			if (!cpudata->cpuidpresent[i]) {
				continue;
			}
			if (cpudata->cpuid[i].leaf == cpuid->leaf) {
				cpudata->cpuidpresent[i] = false;
			}
		}
		return 0;
	}

	/* If already here, replace. */
	for (i = 0; i < VMX_NCPUIDS; i++) {
		if (!cpudata->cpuidpresent[i]) {
			continue;
		}
		if (cpudata->cpuid[i].leaf == cpuid->leaf) {
			memcpy(&cpudata->cpuid[i], cpuid,
			    sizeof(struct nvmm_vcpu_conf_cpuid));
			return 0;
		}
	}

	/* Not here, insert. */
	for (i = 0; i < VMX_NCPUIDS; i++) {
		if (!cpudata->cpuidpresent[i]) {
			cpudata->cpuidpresent[i] = true;
			memcpy(&cpudata->cpuid[i], cpuid,
			    sizeof(struct nvmm_vcpu_conf_cpuid));
			return 0;
		}
	}

	return ENOBUFS;
}

static int
vmx_vcpu_configure_tpr(struct vmx_cpudata *cpudata, void *data)
{
	struct nvmm_vcpu_conf_tpr *tpr = data;

	memcpy(&cpudata->tpr, tpr, sizeof(*tpr));
	return 0;
}

static int
vmx_vcpu_configure(struct nvmm_cpu *vcpu, uint64_t op, void *data)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;

	switch (op) {
	case NVMM_VCPU_CONF_MD(NVMM_VCPU_CONF_CPUID):
		return vmx_vcpu_configure_cpuid(cpudata, data);
	case NVMM_VCPU_CONF_MD(NVMM_VCPU_CONF_TPR):
		return vmx_vcpu_configure_tpr(cpudata, data);
	default:
		return EINVAL;
	}
}

/* -------------------------------------------------------------------------- */

#ifdef __NetBSD__
static void
vmx_tlb_flush(struct pmap *pm)
{
	struct nvmm_machine *mach = os_pmap_mach(pm);
	struct vmx_machdata *machdata = mach->machdata;

	os_atomic_inc_64(&machdata->mach_htlb_gen);

	/*
	 * Send a dummy IPI to each CPU. The IPIs cause #VMEXITs. Afterwards the
	 * VCPU loops will see that their 'vcpu_htlb_gen' is out of sync, and
	 * will each flush their own TLB.
	 */
	os_ipi_kickall();
}
#endif

static void
vmx_machine_create(struct nvmm_machine *mach)
{
	struct pmap *pmap = os_vmspace_pmap(mach->vm);
	struct vmx_machdata *machdata;

	/* Transform into an EPT pmap. */
#if defined(__NetBSD__)
	pmap_ept_transform(pmap);
	os_pmap_mach(pmap) = (void *)mach;
	pmap->pm_tlb_flush = vmx_tlb_flush;
#elif defined(__DragonFly__)
	pmap_ept_transform(pmap, vmx_ept_has_ad ? 0 : PMAP_EMULATE_AD_BITS);
#endif

	machdata = os_mem_zalloc(sizeof(struct vmx_machdata));
	mach->machdata = machdata;

	/* Start with an hTLB flush everywhere. */
	machdata->mach_htlb_gen = 1;
}

static void
vmx_machine_destroy(struct nvmm_machine *mach)
{
	os_mem_free(mach->machdata, sizeof(struct vmx_machdata));
}

static int
vmx_machine_configure(struct nvmm_machine *mach, uint64_t op, void *data)
{
	panic("%s: impossible", __func__);
}

/* -------------------------------------------------------------------------- */

#define CTLS_ONE_ALLOWED(msrval, bitoff) \
	((msrval & __BIT(32 + bitoff)) != 0)
#define CTLS_ZERO_ALLOWED(msrval, bitoff) \
	((msrval & __BIT(bitoff)) == 0)

static int
vmx_check_ctls(uint64_t msr_ctls, uint64_t msr_true_ctls, uint64_t set_one)
{
	uint64_t basic, val, true_val;
	bool has_true;
	size_t i;

	basic = rdmsr(MSR_IA32_VMX_BASIC);
	has_true = (basic & IA32_VMX_BASIC_TRUE_CTLS) != 0;

	val = rdmsr(msr_ctls);
	if (has_true) {
		true_val = rdmsr(msr_true_ctls);
	} else {
		true_val = val;
	}

	for (i = 0; i < 32; i++) {
		if (!(set_one & __BIT(i))) {
			continue;
		}
		if (!CTLS_ONE_ALLOWED(true_val, i)) {
			return -1;
		}
	}

	return 0;
}

static int
vmx_init_ctls(uint64_t msr_ctls, uint64_t msr_true_ctls,
    uint64_t set_one, uint64_t set_zero, uint64_t *res)
{
	uint64_t basic, val, true_val;
	bool one_allowed, zero_allowed, has_true;
	size_t i;

	basic = rdmsr(MSR_IA32_VMX_BASIC);
	has_true = (basic & IA32_VMX_BASIC_TRUE_CTLS) != 0;

	val = rdmsr(msr_ctls);
	if (has_true) {
		true_val = rdmsr(msr_true_ctls);
	} else {
		true_val = val;
	}

	for (i = 0; i < 32; i++) {
		one_allowed = CTLS_ONE_ALLOWED(true_val, i);
		zero_allowed = CTLS_ZERO_ALLOWED(true_val, i);

		if (zero_allowed && !one_allowed) {
			if (set_one & __BIT(i))
				return -1;
			*res &= ~__BIT(i);
		} else if (one_allowed && !zero_allowed) {
			if (set_zero & __BIT(i))
				return -1;
			*res |= __BIT(i);
		} else {
			if (set_zero & __BIT(i)) {
				*res &= ~__BIT(i);
			} else if (set_one & __BIT(i)) {
				*res |= __BIT(i);
			} else if (!has_true) {
				*res &= ~__BIT(i);
			} else if (CTLS_ZERO_ALLOWED(val, i)) {
				*res &= ~__BIT(i);
			} else if (CTLS_ONE_ALLOWED(val, i)) {
				*res |= __BIT(i);
			} else {
				return -1;
			}
		}
	}

	return 0;
}

static bool
vmx_ident(void)
{
	cpuid_desc_t descs;
	uint64_t msr;
	int ret;

	x86_get_cpuid(0x00000001, &descs);
	if (!(descs.ecx & CPUID_0_01_ECX_VMX)) {
		return false;
	}

	msr = rdmsr(MSR_IA32_FEATURE_CONTROL);
	if ((msr & IA32_FEATURE_CONTROL_LOCK) != 0 &&
	    (msr & IA32_FEATURE_CONTROL_OUT_SMX) == 0) {
		os_printf("nvmm: VMX disabled in BIOS\n");
		return false;
	}

	msr = rdmsr(MSR_IA32_VMX_BASIC);
	if ((msr & IA32_VMX_BASIC_IO_REPORT) == 0) {
		os_printf("nvmm: I/O reporting not supported\n");
		return false;
	}
	if (__SHIFTOUT(msr, IA32_VMX_BASIC_MEM_TYPE) != MEM_TYPE_WB) {
		os_printf("nvmm: WB memory not supported\n");
		return false;
	}

	/* PG and PE are reported, even if Unrestricted Guests is supported. */
	vmx_cr0_fixed0 = rdmsr(MSR_IA32_VMX_CR0_FIXED0) & ~(CR0_PG|CR0_PE);
	vmx_cr0_fixed1 = rdmsr(MSR_IA32_VMX_CR0_FIXED1) | (CR0_PG|CR0_PE);
	ret = vmx_check_cr(x86_get_cr0(), vmx_cr0_fixed0, vmx_cr0_fixed1);
	if (ret == -1) {
		os_printf("nvmm: CR0 requirements not satisfied\n");
		return false;
	}

	vmx_cr4_fixed0 = rdmsr(MSR_IA32_VMX_CR4_FIXED0);
	vmx_cr4_fixed1 = rdmsr(MSR_IA32_VMX_CR4_FIXED1);
	ret = vmx_check_cr(x86_get_cr4() | CR4_VMXE, vmx_cr4_fixed0,
	    vmx_cr4_fixed1);
	if (ret == -1) {
		os_printf("nvmm: CR4 requirements not satisfied\n");
		return false;
	}

	/* Init the CTLSs right now, and check for errors. */
	ret = vmx_init_ctls(
	    MSR_IA32_VMX_PINBASED_CTLS, MSR_IA32_VMX_TRUE_PINBASED_CTLS,
	    VMX_PINBASED_CTLS_ONE, VMX_PINBASED_CTLS_ZERO,
	    &vmx_pinbased_ctls);
	if (ret == -1) {
		os_printf("nvmm: pin-based-ctls requirements not satisfied\n");
		return false;
	}
	ret = vmx_init_ctls(
	    MSR_IA32_VMX_PROCBASED_CTLS, MSR_IA32_VMX_TRUE_PROCBASED_CTLS,
	    VMX_PROCBASED_CTLS_ONE, VMX_PROCBASED_CTLS_ZERO,
	    &vmx_procbased_ctls);
	if (ret == -1) {
		os_printf("nvmm: proc-based-ctls requirements not satisfied\n");
		return false;
	}
	ret = vmx_init_ctls(
	    MSR_IA32_VMX_PROCBASED_CTLS2, MSR_IA32_VMX_PROCBASED_CTLS2,
	    VMX_PROCBASED_CTLS2_ONE, VMX_PROCBASED_CTLS2_ZERO,
	    &vmx_procbased_ctls2);
	if (ret == -1) {
		os_printf("nvmm: proc-based-ctls2 requirements not satisfied\n");
		return false;
	}
	ret = vmx_check_ctls(
	    MSR_IA32_VMX_PROCBASED_CTLS2, MSR_IA32_VMX_PROCBASED_CTLS2,
	    PROC_CTLS2_INVPCID_ENABLE);
	if (ret != -1) {
		vmx_procbased_ctls2 |= PROC_CTLS2_INVPCID_ENABLE;
	}
	ret = vmx_init_ctls(
	    MSR_IA32_VMX_ENTRY_CTLS, MSR_IA32_VMX_TRUE_ENTRY_CTLS,
	    VMX_ENTRY_CTLS_ONE, VMX_ENTRY_CTLS_ZERO,
	    &vmx_entry_ctls);
	if (ret == -1) {
		os_printf("nvmm: entry-ctls requirements not satisfied\n");
		return false;
	}
	ret = vmx_init_ctls(
	    MSR_IA32_VMX_EXIT_CTLS, MSR_IA32_VMX_TRUE_EXIT_CTLS,
	    VMX_EXIT_CTLS_ONE, VMX_EXIT_CTLS_ZERO,
	    &vmx_exit_ctls);
	if (ret == -1) {
		os_printf("nvmm: exit-ctls requirements not satisfied\n");
		return false;
	}

	msr = rdmsr(MSR_IA32_VMX_EPT_VPID_CAP);
	if ((msr & IA32_VMX_EPT_VPID_WALKLENGTH_4) == 0) {
		os_printf("nvmm: 4-level page tree not supported\n");
		return false;
	}
	if ((msr & IA32_VMX_EPT_VPID_INVEPT) == 0) {
		os_printf("nvmm: INVEPT not supported\n");
		return false;
	}
	if ((msr & IA32_VMX_EPT_VPID_INVVPID) == 0) {
		os_printf("nvmm: INVVPID not supported\n");
		return false;
	}
	if ((msr & IA32_VMX_EPT_VPID_FLAGS_AD) != 0) {
		vmx_ept_has_ad = true;
	} else {
		vmx_ept_has_ad = false;
	}
#ifdef __NetBSD__
	pmap_ept_has_ad = vmx_ept_has_ad;
#endif
	if (!(msr & IA32_VMX_EPT_VPID_UC) && !(msr & IA32_VMX_EPT_VPID_WB)) {
		os_printf("nvmm: EPT UC/WB memory types not supported\n");
		return false;
	}

	return true;
}

static void
vmx_init_asid(uint32_t maxasid)
{
	size_t allocsz;

	os_mtx_init(&vmx_asidlock);

	vmx_maxasid = maxasid;
	allocsz = roundup(maxasid, 8) / 8;
	vmx_asidmap = os_mem_zalloc(allocsz);

	/* ASID 0 is reserved for the host. */
	vmx_asidmap[0] |= __BIT(0);
}

static
OS_IPI_FUNC(vmx_change_cpu)
{
	bool enable = arg != NULL;
	uint64_t msr, cr4;

	if (enable) {
		msr = rdmsr(MSR_IA32_FEATURE_CONTROL);
		if ((msr & IA32_FEATURE_CONTROL_LOCK) == 0) {
			/* Lock now, with VMX-outside-SMX enabled. */
			wrmsr(MSR_IA32_FEATURE_CONTROL, msr |
			    IA32_FEATURE_CONTROL_LOCK |
			    IA32_FEATURE_CONTROL_OUT_SMX);
		}
	}

	if (!enable) {
		vmx_vmxoff();
	}

	cr4 = x86_get_cr4();
	if (enable) {
		cr4 |= CR4_VMXE;
	} else {
		cr4 &= ~CR4_VMXE;
	}
	x86_set_cr4(cr4);

	if (enable) {
		vmx_vmxon(&vmxoncpu[os_curcpu_number()].pa);
	}
}

static void
vmx_init_l1tf(void)
{
	cpuid_desc_t descs;
	uint64_t msr;

	x86_get_cpuid(0x00000000, &descs);
	if (descs.eax < 7) {
		return;
	}

	x86_get_cpuid(0x00000007, &descs);

	if (descs.edx & CPUID_0_07_EDX_ARCH_CAP) {
		msr = rdmsr(MSR_IA32_ARCH_CAPABILITIES);
		if (msr & IA32_ARCH_SKIP_L1DFL_VMENTRY) {
			/* No mitigation needed. */
			return;
		}
	}

	if (descs.edx & CPUID_0_07_EDX_L1D_FLUSH) {
		/* Enable hardware mitigation. */
		vmx_msrlist_entry_nmsr += 1;
	}
}

static void
vmx_init(void)
{
	struct vmxon *vmxon;
	uint32_t revision;
	cpuid_desc_t descs;
	os_cpu_t *cpu;
	uint64_t msr;
	paddr_t pa;
	vaddr_t va;
	int error;

	/* Init the ASID bitmap (VPID). */
	vmx_init_asid(VPID_MAX);

	/* Init the XCR0 mask. */
	vmx_xcr0_mask = VMX_XCR0_MASK_DEFAULT & x86_xsave_features;

	/* Init the max basic CPUID leaf. */
	x86_get_cpuid(0x00000000, &descs);
	vmx_cpuid_max_basic = uimin(descs.eax, VMX_CPUID_MAX_BASIC);

	/* Init the max extended CPUID leaf. */
	x86_get_cpuid(0x80000000, &descs);
	vmx_cpuid_max_extended = uimin(descs.eax, VMX_CPUID_MAX_EXTENDED);

	/* Init the TLB flush op, the EPT flush op and the EPTP type. */
	msr = rdmsr(MSR_IA32_VMX_EPT_VPID_CAP);
	if ((msr & IA32_VMX_EPT_VPID_INVVPID_CONTEXT) != 0) {
		vmx_tlb_flush_op = VMX_INVVPID_CONTEXT;
	} else {
		vmx_tlb_flush_op = VMX_INVVPID_ALL;
	}
	if ((msr & IA32_VMX_EPT_VPID_INVEPT_CONTEXT) != 0) {
		vmx_ept_flush_op = VMX_INVEPT_CONTEXT;
	} else {
		vmx_ept_flush_op = VMX_INVEPT_ALL;
	}
	if ((msr & IA32_VMX_EPT_VPID_WB) != 0) {
		vmx_eptp_type = EPTP_TYPE_WB;
	} else {
		vmx_eptp_type = EPTP_TYPE_UC;
	}

	/* Init the L1TF mitigation. */
	vmx_init_l1tf();

	/* Init the global host state. */
	if (vmx_xcr0_mask != 0) {
		vmx_global_hstate.xcr0 = x86_get_xcr(0);
	}
	vmx_global_hstate.star = rdmsr(MSR_STAR);
	vmx_global_hstate.lstar = rdmsr(MSR_LSTAR);
	vmx_global_hstate.cstar = rdmsr(MSR_CSTAR);
	vmx_global_hstate.sfmask = rdmsr(MSR_SFMASK);

	memset(vmxoncpu, 0, sizeof(vmxoncpu));
	revision = vmx_get_revision();

	OS_CPU_FOREACH(cpu) {
		error = os_contigpa_zalloc(&pa, &va, 1);
		if (error) {
			panic("%s: out of memory", __func__);
		}
		vmxoncpu[os_cpu_number(cpu)].pa = pa;
		vmxoncpu[os_cpu_number(cpu)].va = va;

		vmxon = (struct vmxon *)vmxoncpu[os_cpu_number(cpu)].va;
		vmxon->ident = __SHIFTIN(revision, VMXON_IDENT_REVISION);
	}

	os_ipi_broadcast(vmx_change_cpu, (void *)true);
}

static void
vmx_fini_asid(void)
{
	size_t allocsz;

	allocsz = roundup(vmx_maxasid, 8) / 8;
	os_mem_free(vmx_asidmap, allocsz);

	os_mtx_destroy(&vmx_asidlock);
}

static void
vmx_fini(void)
{
	size_t i;

	os_ipi_broadcast(vmx_change_cpu, (void *)false);

	for (i = 0; i < OS_MAXCPUS; i++) {
		if (vmxoncpu[i].pa != 0)
			os_contigpa_free(vmxoncpu[i].pa, vmxoncpu[i].va, 1);
	}

	vmx_fini_asid();
}

static void
vmx_capability(struct nvmm_capability *cap)
{
	cap->arch.mach_conf_support = 0;
	cap->arch.vcpu_conf_support =
	    NVMM_CAP_ARCH_VCPU_CONF_CPUID |
	    NVMM_CAP_ARCH_VCPU_CONF_TPR;
	cap->arch.xcr0_mask = vmx_xcr0_mask;
	cap->arch.mxcsr_mask = x86_fpu_mxcsr_mask;
	cap->arch.conf_cpuid_maxops = VMX_NCPUIDS;
}

const struct nvmm_impl nvmm_x86_vmx = {
	.name = "x86-vmx",
	.ident = vmx_ident,
	.init = vmx_init,
	.fini = vmx_fini,
	.capability = vmx_capability,
	.mach_conf_max = NVMM_X86_MACH_NCONF,
	.mach_conf_sizes = NULL,
	.vcpu_conf_max = NVMM_X86_VCPU_NCONF,
	.vcpu_conf_sizes = vmx_vcpu_conf_sizes,
	.state_size = sizeof(struct nvmm_x64_state),
	.machine_create = vmx_machine_create,
	.machine_destroy = vmx_machine_destroy,
	.machine_configure = vmx_machine_configure,
	.vcpu_create = vmx_vcpu_create,
	.vcpu_destroy = vmx_vcpu_destroy,
	.vcpu_configure = vmx_vcpu_configure,
	.vcpu_setstate = vmx_vcpu_setstate,
	.vcpu_getstate = vmx_vcpu_getstate,
	.vcpu_inject = vmx_vcpu_inject,
	.vcpu_run = vmx_vcpu_run
};
