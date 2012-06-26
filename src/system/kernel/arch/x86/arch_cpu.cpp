/*
 * Copyright 2002-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <cpu.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <ACPI.h>

#include <boot_device.h>
#include <commpage.h>
#include <debug.h>
#include <elf.h>
#include <smp.h>
#include <tls.h>
#include <vm/vm.h>
#include <vm/vm_types.h>
#include <vm/VMAddressSpace.h>

#include <arch_system_info.h>
#include <arch/x86/apic.h>
#include <arch/x86/selector.h>
#include <boot/kernel_args.h>

#include "interrupts.h"
#include "paging/X86PagingStructures.h"
#include "paging/X86VMTranslationMap.h"


#define DUMP_FEATURE_STRING 1


/* cpu vendor info */
struct cpu_vendor_info {
	const char *vendor;
	const char *ident_string[2];
};

static const struct cpu_vendor_info vendor_info[VENDOR_NUM] = {
	{ "Intel", { "GenuineIntel" } },
	{ "AMD", { "AuthenticAMD" } },
	{ "Cyrix", { "CyrixInstead" } },
	{ "UMC", { "UMC UMC UMC" } },
	{ "NexGen", { "NexGenDriven" } },
	{ "Centaur", { "CentaurHauls" } },
	{ "Rise", { "RiseRiseRise" } },
	{ "Transmeta", { "GenuineTMx86", "TransmetaCPU" } },
	{ "NSC", { "Geode by NSC" } },
};

#define CR0_CACHE_DISABLE		(1UL << 30)
#define CR0_NOT_WRITE_THROUGH	(1UL << 29)
#define CR0_FPU_EMULATION		(1UL << 2)
#define CR0_MONITOR_FPU			(1UL << 1)

#define CR4_OS_FXSR				(1UL << 9)
#define CR4_OS_XMM_EXCEPTION	(1UL << 10)

#define K8_SMIONCMPHALT			(1ULL << 27)
#define K8_C1EONCMPHALT			(1ULL << 28)

#define K8_CMPHALT				(K8_SMIONCMPHALT | K8_C1EONCMPHALT)

struct set_mtrr_parameter {
	int32	index;
	uint64	base;
	uint64	length;
	uint8	type;
};

struct set_mtrrs_parameter {
	const x86_mtrr_info*	infos;
	uint32					count;
	uint8					defaultType;
};


extern "C" void reboot(void);
	// from arch_x86.S

void (*gX86SwapFPUFunc)(void *oldState, const void *newState);
void (*gCpuIdleFunc)(void);
bool gHasSSE = false;

static uint32 sCpuRendezvous;
static uint32 sCpuRendezvous2;
static uint32 sCpuRendezvous3;
static vint32 sTSCSyncRendezvous;

segment_descriptor *gGDT = NULL;

/* Some specials for the double fault handler */
static uint8* sDoubleFaultStacks;
static const size_t kDoubleFaultStackSize = 4096;	// size per CPU

static x86_cpu_module_info *sCpuModule;


extern "C" void memcpy_generic(void* dest, const void* source, size_t count);
extern int memcpy_generic_end;
extern "C" void memset_generic(void* dest, int value, size_t count);
extern int memset_generic_end;

x86_optimized_functions gOptimizedFunctions = {
	memcpy_generic,
	&memcpy_generic_end,
	memset_generic,
	&memset_generic_end
};


static status_t
acpi_shutdown(bool rebootSystem)
{
	if (debug_debugger_running() || !are_interrupts_enabled())
		return B_ERROR;

	acpi_module_info* acpi;
	if (get_module(B_ACPI_MODULE_NAME, (module_info**)&acpi) != B_OK)
		return B_NOT_SUPPORTED;

	status_t status;
	if (rebootSystem) {
		status = acpi->reboot();
	} else {
		// Make sure we run on the boot CPU (apparently needed for some ACPI
		// implementations)
		_user_set_cpu_enabled(0, true);
		for (int32 cpu = 1; cpu < smp_get_num_cpus(); cpu++) {
			_user_set_cpu_enabled(cpu, false);
		}
		// TODO: must not be called from the idle thread!
		thread_yield(true);

		status = acpi->prepare_sleep_state(ACPI_POWER_STATE_OFF, NULL, 0);
		if (status == B_OK) {
			//cpu_status state = disable_interrupts();
			status = acpi->enter_sleep_state(ACPI_POWER_STATE_OFF);
			//restore_interrupts(state);
		}
	}

	put_module(B_ACPI_MODULE_NAME);
	return status;
}


/*!	Disable CPU caches, and invalidate them. */
static void
disable_caches()
{
	x86_write_cr0((x86_read_cr0() | CR0_CACHE_DISABLE)
		& ~CR0_NOT_WRITE_THROUGH);
	wbinvd();
	arch_cpu_global_TLB_invalidate();
}


/*!	Invalidate CPU caches, and enable them. */
static void
enable_caches()
{
	wbinvd();
	arch_cpu_global_TLB_invalidate();
	x86_write_cr0(x86_read_cr0()
		& ~(CR0_CACHE_DISABLE | CR0_NOT_WRITE_THROUGH));
}


static void
set_mtrr(void *_parameter, int cpu)
{
	struct set_mtrr_parameter *parameter
		= (struct set_mtrr_parameter *)_parameter;

	// wait until all CPUs have arrived here
	smp_cpu_rendezvous(&sCpuRendezvous, cpu);

	// One CPU has to reset sCpuRendezvous3 -- it is needed to prevent the CPU
	// that initiated the call_all_cpus() from doing that again and clearing
	// sCpuRendezvous2 before the last CPU has actually left the loop in
	// smp_cpu_rendezvous();
	if (cpu == 0)
		atomic_set((vint32*)&sCpuRendezvous3, 0);

	disable_caches();

	sCpuModule->set_mtrr(parameter->index, parameter->base, parameter->length,
		parameter->type);

	enable_caches();

	// wait until all CPUs have arrived here
	smp_cpu_rendezvous(&sCpuRendezvous2, cpu);
	smp_cpu_rendezvous(&sCpuRendezvous3, cpu);
}


static void
set_mtrrs(void* _parameter, int cpu)
{
	set_mtrrs_parameter* parameter = (set_mtrrs_parameter*)_parameter;

	// wait until all CPUs have arrived here
	smp_cpu_rendezvous(&sCpuRendezvous, cpu);

	// One CPU has to reset sCpuRendezvous3 -- it is needed to prevent the CPU
	// that initiated the call_all_cpus() from doing that again and clearing
	// sCpuRendezvous2 before the last CPU has actually left the loop in
	// smp_cpu_rendezvous();
	if (cpu == 0)
		atomic_set((vint32*)&sCpuRendezvous3, 0);

	disable_caches();

	sCpuModule->set_mtrrs(parameter->defaultType, parameter->infos,
		parameter->count);

	enable_caches();

	// wait until all CPUs have arrived here
	smp_cpu_rendezvous(&sCpuRendezvous2, cpu);
	smp_cpu_rendezvous(&sCpuRendezvous3, cpu);
}


static void
init_mtrrs(void *_unused, int cpu)
{
	// wait until all CPUs have arrived here
	smp_cpu_rendezvous(&sCpuRendezvous, cpu);

	// One CPU has to reset sCpuRendezvous3 -- it is needed to prevent the CPU
	// that initiated the call_all_cpus() from doing that again and clearing
	// sCpuRendezvous2 before the last CPU has actually left the loop in
	// smp_cpu_rendezvous();
	if (cpu == 0)
		atomic_set((vint32*)&sCpuRendezvous3, 0);

	disable_caches();

	sCpuModule->init_mtrrs();

	enable_caches();

	// wait until all CPUs have arrived here
	smp_cpu_rendezvous(&sCpuRendezvous2, cpu);
	smp_cpu_rendezvous(&sCpuRendezvous3, cpu);
}


uint32
x86_count_mtrrs(void)
{
	if (sCpuModule == NULL)
		return 0;

	return sCpuModule->count_mtrrs();
}


void
x86_set_mtrr(uint32 index, uint64 base, uint64 length, uint8 type)
{
	struct set_mtrr_parameter parameter;
	parameter.index = index;
	parameter.base = base;
	parameter.length = length;
	parameter.type = type;

	sCpuRendezvous = sCpuRendezvous2 = 0;
	call_all_cpus(&set_mtrr, &parameter);
}


status_t
x86_get_mtrr(uint32 index, uint64 *_base, uint64 *_length, uint8 *_type)
{
	// the MTRRs are identical on all CPUs, so it doesn't matter
	// on which CPU this runs
	return sCpuModule->get_mtrr(index, _base, _length, _type);
}


void
x86_set_mtrrs(uint8 defaultType, const x86_mtrr_info* infos, uint32 count)
{
	if (sCpuModule == NULL)
		return;

	struct set_mtrrs_parameter parameter;
	parameter.defaultType = defaultType;
	parameter.infos = infos;
	parameter.count = count;

	sCpuRendezvous = sCpuRendezvous2 = 0;
	call_all_cpus(&set_mtrrs, &parameter);
}


void
x86_init_fpu(void)
{
	if (!x86_check_feature(IA32_FEATURE_FPU, FEATURE_COMMON)) {
		// No FPU... time to install one in your 386?
		dprintf("%s: Warning: CPU has no reported FPU.\n", __func__);
		gX86SwapFPUFunc = i386_noop_swap;
		return;
	}

	if (!x86_check_feature(IA32_FEATURE_SSE, FEATURE_COMMON)
		|| !x86_check_feature(IA32_FEATURE_FXSR, FEATURE_COMMON)) {
		dprintf("%s: CPU has no SSE... just enabling FPU.\n", __func__);
		// we don't have proper SSE support, just enable FPU
		x86_write_cr0(x86_read_cr0() & ~(CR0_FPU_EMULATION | CR0_MONITOR_FPU));
		gX86SwapFPUFunc = i386_fnsave_swap;
		return;
	}
	dprintf("%s: CPU has SSE... enabling FXSR and XMM.\n", __func__);

	// enable OS support for SSE
	x86_write_cr4(x86_read_cr4() | CR4_OS_FXSR | CR4_OS_XMM_EXCEPTION);
	x86_write_cr0(x86_read_cr0() & ~(CR0_FPU_EMULATION | CR0_MONITOR_FPU));

	gX86SwapFPUFunc = i386_fxsave_swap;
	gHasSSE = true;
}


static void
load_tss(int cpu)
{
	short seg = ((TSS_BASE_SEGMENT + cpu) << 3) | DPL_KERNEL;
	asm("movw  %0, %%ax;"
		"ltr %%ax;" : : "r" (seg) : "eax");
}


static void
init_double_fault(int cpuNum)
{
	// set up the double fault TSS
	struct tss *tss = &gCPU[cpuNum].arch.double_fault_tss;

	memset(tss, 0, sizeof(struct tss));
	size_t stackSize;
	tss->sp0 = (uint32)x86_get_double_fault_stack(cpuNum, &stackSize);
	tss->sp0 += stackSize;
	tss->ss0 = KERNEL_DATA_SEG;
	read_cr3(tss->cr3);
		// copy the current cr3 to the double fault cr3
	tss->eip = (uint32)&double_fault;
	tss->es = KERNEL_DATA_SEG;
	tss->cs = KERNEL_CODE_SEG;
	tss->ss = KERNEL_DATA_SEG;
	tss->esp = tss->sp0;
	tss->ds = KERNEL_DATA_SEG;
	tss->fs = KERNEL_DATA_SEG;
	tss->gs = KERNEL_DATA_SEG;
	tss->ldt_seg_selector = 0;
	tss->io_map_base = sizeof(struct tss);

	// add TSS descriptor for this new TSS
	uint16 tssSegmentDescriptorIndex = DOUBLE_FAULT_TSS_BASE_SEGMENT + cpuNum;
	set_tss_descriptor(&gGDT[tssSegmentDescriptorIndex],
		(addr_t)tss, sizeof(struct tss));

	x86_set_task_gate(cpuNum, 8, tssSegmentDescriptorIndex << 3);
}


#if DUMP_FEATURE_STRING
static void
dump_feature_string(int currentCPU, cpu_ent *cpu)
{
	char features[256];
	features[0] = 0;

	if (cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_FPU)
		strlcat(features, "fpu ", sizeof(features));
	if (cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_VME)
		strlcat(features, "vme ", sizeof(features));
	if (cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_DE)
		strlcat(features, "de ", sizeof(features));
	if (cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_PSE)
		strlcat(features, "pse ", sizeof(features));
	if (cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_TSC)
		strlcat(features, "tsc ", sizeof(features));
	if (cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_MSR)
		strlcat(features, "msr ", sizeof(features));
	if (cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_PAE)
		strlcat(features, "pae ", sizeof(features));
	if (cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_MCE)
		strlcat(features, "mce ", sizeof(features));
	if (cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_CX8)
		strlcat(features, "cx8 ", sizeof(features));
	if (cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_APIC)
		strlcat(features, "apic ", sizeof(features));
	if (cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_SEP)
		strlcat(features, "sep ", sizeof(features));
	if (cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_MTRR)
		strlcat(features, "mtrr ", sizeof(features));
	if (cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_PGE)
		strlcat(features, "pge ", sizeof(features));
	if (cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_MCA)
		strlcat(features, "mca ", sizeof(features));
	if (cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_CMOV)
		strlcat(features, "cmov ", sizeof(features));
	if (cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_PAT)
		strlcat(features, "pat ", sizeof(features));
	if (cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_PSE36)
		strlcat(features, "pse36 ", sizeof(features));
	if (cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_PSN)
		strlcat(features, "psn ", sizeof(features));
	if (cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_CLFSH)
		strlcat(features, "clfsh ", sizeof(features));
	if (cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_DS)
		strlcat(features, "ds ", sizeof(features));
	if (cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_ACPI)
		strlcat(features, "acpi ", sizeof(features));
	if (cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_MMX)
		strlcat(features, "mmx ", sizeof(features));
	if (cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_FXSR)
		strlcat(features, "fxsr ", sizeof(features));
	if (cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_SSE)
		strlcat(features, "sse ", sizeof(features));
	if (cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_SSE2)
		strlcat(features, "sse2 ", sizeof(features));
	if (cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_SS)
		strlcat(features, "ss ", sizeof(features));
	if (cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_HTT)
		strlcat(features, "htt ", sizeof(features));
	if (cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_TM)
		strlcat(features, "tm ", sizeof(features));
	if (cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_PBE)
		strlcat(features, "pbe ", sizeof(features));
	if (cpu->arch.feature[FEATURE_EXT] & IA32_FEATURE_EXT_SSE3)
		strlcat(features, "sse3 ", sizeof(features));
	if (cpu->arch.feature[FEATURE_EXT] & IA32_FEATURE_EXT_PCLMULQDQ)
		strlcat(features, "pclmulqdq ", sizeof(features));
	if (cpu->arch.feature[FEATURE_EXT] & IA32_FEATURE_EXT_DTES64)
		strlcat(features, "dtes64 ", sizeof(features));
	if (cpu->arch.feature[FEATURE_EXT] & IA32_FEATURE_EXT_MONITOR)
		strlcat(features, "monitor ", sizeof(features));
	if (cpu->arch.feature[FEATURE_EXT] & IA32_FEATURE_EXT_DSCPL)
		strlcat(features, "dscpl ", sizeof(features));
	if (cpu->arch.feature[FEATURE_EXT] & IA32_FEATURE_EXT_VMX)
		strlcat(features, "vmx ", sizeof(features));
	if (cpu->arch.feature[FEATURE_EXT] & IA32_FEATURE_EXT_SMX)
		strlcat(features, "smx ", sizeof(features));
	if (cpu->arch.feature[FEATURE_EXT] & IA32_FEATURE_EXT_EST)
		strlcat(features, "est ", sizeof(features));
	if (cpu->arch.feature[FEATURE_EXT] & IA32_FEATURE_EXT_TM2)
		strlcat(features, "tm2 ", sizeof(features));
	if (cpu->arch.feature[FEATURE_EXT] & IA32_FEATURE_EXT_SSSE3)
		strlcat(features, "ssse3 ", sizeof(features));
	if (cpu->arch.feature[FEATURE_EXT] & IA32_FEATURE_EXT_CNXTID)
		strlcat(features, "cnxtid ", sizeof(features));
	if (cpu->arch.feature[FEATURE_EXT] & IA32_FEATURE_EXT_FMA)
		strlcat(features, "fma ", sizeof(features));
	if (cpu->arch.feature[FEATURE_EXT] & IA32_FEATURE_EXT_CX16)
		strlcat(features, "cx16 ", sizeof(features));
	if (cpu->arch.feature[FEATURE_EXT] & IA32_FEATURE_EXT_XTPR)
		strlcat(features, "xtpr ", sizeof(features));
	if (cpu->arch.feature[FEATURE_EXT] & IA32_FEATURE_EXT_PDCM)
		strlcat(features, "pdcm ", sizeof(features));
	if (cpu->arch.feature[FEATURE_EXT] & IA32_FEATURE_EXT_PCID)
		strlcat(features, "pcid ", sizeof(features));
	if (cpu->arch.feature[FEATURE_EXT] & IA32_FEATURE_EXT_DCA)
		strlcat(features, "dca ", sizeof(features));
	if (cpu->arch.feature[FEATURE_EXT] & IA32_FEATURE_EXT_SSE4_1)
		strlcat(features, "sse4_1 ", sizeof(features));
	if (cpu->arch.feature[FEATURE_EXT] & IA32_FEATURE_EXT_SSE4_2)
		strlcat(features, "sse4_2 ", sizeof(features));
	if (cpu->arch.feature[FEATURE_EXT] & IA32_FEATURE_EXT_X2APIC)
		strlcat(features, "x2apic ", sizeof(features));
	if (cpu->arch.feature[FEATURE_EXT] & IA32_FEATURE_EXT_MOVBE)
		strlcat(features, "movbe ", sizeof(features));
	if (cpu->arch.feature[FEATURE_EXT] & IA32_FEATURE_EXT_POPCNT)
		strlcat(features, "popcnt ", sizeof(features));
	if (cpu->arch.feature[FEATURE_EXT] & IA32_FEATURE_EXT_TSCDEADLINE)
		strlcat(features, "tscdeadline ", sizeof(features));
	if (cpu->arch.feature[FEATURE_EXT] & IA32_FEATURE_EXT_AES)
		strlcat(features, "aes ", sizeof(features));
	if (cpu->arch.feature[FEATURE_EXT] & IA32_FEATURE_EXT_XSAVE)
		strlcat(features, "xsave ", sizeof(features));
	if (cpu->arch.feature[FEATURE_EXT] & IA32_FEATURE_EXT_OSXSAVE)
		strlcat(features, "osxsave ", sizeof(features));
	if (cpu->arch.feature[FEATURE_EXT] & IA32_FEATURE_EXT_AVX)
		strlcat(features, "avx ", sizeof(features));
	if (cpu->arch.feature[FEATURE_EXT] & IA32_FEATURE_EXT_F16C)
		strlcat(features, "f16c ", sizeof(features));
	if (cpu->arch.feature[FEATURE_EXT] & IA32_FEATURE_EXT_RDRND)
		strlcat(features, "rdrnd ", sizeof(features));
	if (cpu->arch.feature[FEATURE_EXT] & IA32_FEATURE_EXT_HYPERVISOR)
		strlcat(features, "hypervisor ", sizeof(features));
	if (cpu->arch.feature[FEATURE_EXT_AMD] & IA32_FEATURE_AMD_EXT_SYSCALL)
		strlcat(features, "syscall ", sizeof(features));
	if (cpu->arch.feature[FEATURE_EXT_AMD] & IA32_FEATURE_AMD_EXT_NX)
		strlcat(features, "nx ", sizeof(features));
	if (cpu->arch.feature[FEATURE_EXT_AMD] & IA32_FEATURE_AMD_EXT_MMXEXT)
		strlcat(features, "mmxext ", sizeof(features));
	if (cpu->arch.feature[FEATURE_EXT_AMD] & IA32_FEATURE_AMD_EXT_FFXSR)
		strlcat(features, "ffxsr ", sizeof(features));
	if (cpu->arch.feature[FEATURE_EXT_AMD] & IA32_FEATURE_AMD_EXT_LONG)
		strlcat(features, "long ", sizeof(features));
	if (cpu->arch.feature[FEATURE_EXT_AMD] & IA32_FEATURE_AMD_EXT_3DNOWEXT)
		strlcat(features, "3dnowext ", sizeof(features));
	if (cpu->arch.feature[FEATURE_EXT_AMD] & IA32_FEATURE_AMD_EXT_3DNOW)
		strlcat(features, "3dnow ", sizeof(features));
	if (cpu->arch.feature[FEATURE_6_EAX] & IA32_FEATURE_DTS)
		strlcat(features, "dts ", sizeof(features));
	if (cpu->arch.feature[FEATURE_6_EAX] & IA32_FEATURE_ITB)
		strlcat(features, "itb ", sizeof(features));
	if (cpu->arch.feature[FEATURE_6_EAX] & IA32_FEATURE_ARAT)
		strlcat(features, "arat ", sizeof(features));
	if (cpu->arch.feature[FEATURE_6_EAX] & IA32_FEATURE_PLN)
		strlcat(features, "pln ", sizeof(features));
	if (cpu->arch.feature[FEATURE_6_EAX] & IA32_FEATURE_ECMD)
		strlcat(features, "ecmd ", sizeof(features));
	if (cpu->arch.feature[FEATURE_6_EAX] & IA32_FEATURE_PTM)
		strlcat(features, "ptm ", sizeof(features));
	if (cpu->arch.feature[FEATURE_6_ECX] & IA32_FEATURE_APERFMPERF)
		strlcat(features, "aperfmperf ", sizeof(features));
	if (cpu->arch.feature[FEATURE_6_ECX] & IA32_FEATURE_EPB)
		strlcat(features, "epb ", sizeof(features));

	dprintf("CPU %d: features: %s\n", currentCPU, features);
}
#endif	// DUMP_FEATURE_STRING


static int
detect_cpu(int currentCPU)
{
	cpu_ent *cpu = get_cpu_struct();
	char vendorString[17];
	cpuid_info cpuid;

	// clear out the cpu info data
	cpu->arch.vendor = VENDOR_UNKNOWN;
	cpu->arch.vendor_name = "UNKNOWN VENDOR";
	cpu->arch.feature[FEATURE_COMMON] = 0;
	cpu->arch.feature[FEATURE_EXT] = 0;
	cpu->arch.feature[FEATURE_EXT_AMD] = 0;
	cpu->arch.model_name[0] = 0;

	// print some fun data
	get_current_cpuid(&cpuid, 0);

	// build the vendor string
	memset(vendorString, 0, sizeof(vendorString));
	memcpy(vendorString, cpuid.eax_0.vendor_id, sizeof(cpuid.eax_0.vendor_id));

	// get the family, model, stepping
	get_current_cpuid(&cpuid, 1);
	cpu->arch.type = cpuid.eax_1.type;
	cpu->arch.family = cpuid.eax_1.family;
	cpu->arch.extended_family = cpuid.eax_1.extended_family;
	cpu->arch.model = cpuid.eax_1.model;
	cpu->arch.extended_model = cpuid.eax_1.extended_model;
	cpu->arch.stepping = cpuid.eax_1.stepping;
	dprintf("CPU %d: type %d family %d extended_family %d model %d "
		"extended_model %d stepping %d, string '%s'\n",
		currentCPU, cpu->arch.type, cpu->arch.family,
		cpu->arch.extended_family, cpu->arch.model,
		cpu->arch.extended_model, cpu->arch.stepping, vendorString);

	// figure out what vendor we have here

	for (int32 i = 0; i < VENDOR_NUM; i++) {
		if (vendor_info[i].ident_string[0]
			&& !strcmp(vendorString, vendor_info[i].ident_string[0])) {
			cpu->arch.vendor = (x86_vendors)i;
			cpu->arch.vendor_name = vendor_info[i].vendor;
			break;
		}
		if (vendor_info[i].ident_string[1]
			&& !strcmp(vendorString, vendor_info[i].ident_string[1])) {
			cpu->arch.vendor = (x86_vendors)i;
			cpu->arch.vendor_name = vendor_info[i].vendor;
			break;
		}
	}

	// see if we can get the model name
	get_current_cpuid(&cpuid, 0x80000000);
	if (cpuid.eax_0.max_eax >= 0x80000004) {
		// build the model string (need to swap ecx/edx data before copying)
		unsigned int temp;
		memset(cpu->arch.model_name, 0, sizeof(cpu->arch.model_name));

		get_current_cpuid(&cpuid, 0x80000002);
		temp = cpuid.regs.edx;
		cpuid.regs.edx = cpuid.regs.ecx;
		cpuid.regs.ecx = temp;
		memcpy(cpu->arch.model_name, cpuid.as_chars, sizeof(cpuid.as_chars));

		get_current_cpuid(&cpuid, 0x80000003);
		temp = cpuid.regs.edx;
		cpuid.regs.edx = cpuid.regs.ecx;
		cpuid.regs.ecx = temp;
		memcpy(cpu->arch.model_name + 16, cpuid.as_chars,
			sizeof(cpuid.as_chars));

		get_current_cpuid(&cpuid, 0x80000004);
		temp = cpuid.regs.edx;
		cpuid.regs.edx = cpuid.regs.ecx;
		cpuid.regs.ecx = temp;
		memcpy(cpu->arch.model_name + 32, cpuid.as_chars,
			sizeof(cpuid.as_chars));

		// some cpus return a right-justified string
		int32 i = 0;
		while (cpu->arch.model_name[i] == ' ')
			i++;
		if (i > 0) {
			memmove(cpu->arch.model_name, &cpu->arch.model_name[i],
				strlen(&cpu->arch.model_name[i]) + 1);
		}

		dprintf("CPU %d: vendor '%s' model name '%s'\n",
			currentCPU, cpu->arch.vendor_name, cpu->arch.model_name);
	} else {
		strlcpy(cpu->arch.model_name, "unknown", sizeof(cpu->arch.model_name));
	}

	// load feature bits
	get_current_cpuid(&cpuid, 1);
	cpu->arch.feature[FEATURE_COMMON] = cpuid.eax_1.features; // edx
	cpu->arch.feature[FEATURE_EXT] = cpuid.eax_1.extended_features; // ecx
	if (cpu->arch.vendor == VENDOR_AMD) {
		get_current_cpuid(&cpuid, 0x80000001);
		cpu->arch.feature[FEATURE_EXT_AMD] = cpuid.regs.edx; // edx
	}
	get_current_cpuid(&cpuid, 6);
	cpu->arch.feature[FEATURE_6_EAX] = cpuid.regs.eax;
	cpu->arch.feature[FEATURE_6_ECX] = cpuid.regs.ecx;

#if DUMP_FEATURE_STRING
	dump_feature_string(currentCPU, cpu);
#endif

	return 0;
}


bool
x86_check_feature(uint32 feature, enum x86_feature_type type)
{
	cpu_ent *cpu = get_cpu_struct();

#if 0
	int i;
	dprintf("x86_check_feature: feature 0x%x, type %d\n", feature, type);
	for (i = 0; i < FEATURE_NUM; i++) {
		dprintf("features %d: 0x%x\n", i, cpu->arch.feature[i]);
	}
#endif

	return (cpu->arch.feature[type] & feature) != 0;
}


void*
x86_get_double_fault_stack(int32 cpu, size_t* _size)
{
	*_size = kDoubleFaultStackSize;
	return sDoubleFaultStacks + kDoubleFaultStackSize * cpu;
}


/*!	Returns the index of the current CPU. Can only be called from the double
	fault handler.
*/
int32
x86_double_fault_get_cpu(void)
{
	uint32 stack = x86_read_ebp();
	return (stack - (uint32)sDoubleFaultStacks) / kDoubleFaultStackSize;
}


//	#pragma mark -


status_t
arch_cpu_preboot_init_percpu(kernel_args *args, int cpu)
{
	// A simple nop FPU call until x86_init_fpu
	gX86SwapFPUFunc = i386_noop_swap;

	// On SMP system we want to synchronize the CPUs' TSCs, so system_time()
	// will return consistent values.
	if (smp_get_num_cpus() > 1) {
		// let the first CPU prepare the rendezvous point
		if (cpu == 0)
			sTSCSyncRendezvous = smp_get_num_cpus() - 1;

		// One CPU after the other will drop out of this loop and be caught by
		// the loop below, until the last CPU (0) gets there. Save for +/- a few
		// cycles the CPUs should pass the second loop at the same time.
		while (sTSCSyncRendezvous != cpu) {
		}

		sTSCSyncRendezvous = cpu - 1;

		while (sTSCSyncRendezvous != -1) {
		}

		// reset TSC to 0
		x86_write_msr(IA32_MSR_TSC, 0);
	}

	return B_OK;
}


static void
halt_idle(void)
{
	asm("hlt");
}


static void
amdc1e_noarat_idle(void)
{
	uint64 msr = x86_read_msr(K8_MSR_IPM);
	if (msr & K8_CMPHALT)
		x86_write_msr(K8_MSR_IPM, msr & ~K8_CMPHALT);
	halt_idle();
}


static bool
detect_amdc1e_noarat()
{
	cpu_ent *cpu = get_cpu_struct();

	if (cpu->arch.vendor != VENDOR_AMD)
		return false;

	// Family 0x12 and higher processors support ARAT
	// Family lower than 0xf processors doesn't support C1E
	// Family 0xf with model <= 0x40 procssors doesn't support C1E
	uint32 family = cpu->arch.family + cpu->arch.extended_family;
	uint32 model = (cpu->arch.extended_model << 4) | cpu->arch.model;
	return (family < 0x12 && family > 0xf) || (family == 0xf && model > 0x40);
}


status_t
arch_cpu_init_percpu(kernel_args *args, int cpu)
{
	detect_cpu(cpu);

	// load the TSS for this cpu
	// note the main cpu gets initialized in arch_cpu_init_post_vm()
	if (cpu != 0) {
		load_tss(cpu);

		// set the IDT
		struct {
			uint16	limit;
			void*	address;
		} _PACKED descriptor = {
			256 * 8 - 1,	// 256 descriptors, 8 bytes each (-1 for "limit")
			x86_get_idt(cpu)
		};

		asm volatile("lidt	%0" : : "m"(descriptor));
	}

	if (!gCpuIdleFunc) {
		if (detect_amdc1e_noarat())
			gCpuIdleFunc = amdc1e_noarat_idle;
		else
			gCpuIdleFunc = halt_idle;
	}
	return 0;
}


status_t
arch_cpu_init(kernel_args *args)
{
	// init the TSC -> system_time() conversion factors

	uint32 conversionFactor = args->arch_args.system_time_cv_factor;
	uint64 conversionFactorNsecs = (uint64)conversionFactor * 1000;

	if (conversionFactorNsecs >> 32 != 0) {
		// the TSC frequency is < 1 GHz, which forces us to shift the factor
		__x86_setup_system_time(conversionFactor, conversionFactorNsecs >> 16,
			true);
	} else {
		// the TSC frequency is >= 1 GHz
		__x86_setup_system_time(conversionFactor, conversionFactorNsecs, false);
	}

	return B_OK;
}


status_t
arch_cpu_init_post_vm(kernel_args *args)
{
	uint32 i;

	// account for the segment descriptors
	gGDT = (segment_descriptor *)(addr_t)args->arch_args.vir_gdt;
	create_area("gdt", (void **)&gGDT, B_EXACT_ADDRESS, B_PAGE_SIZE,
		B_ALREADY_WIRED, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);

	// currently taken out of the build, because it's not yet used (and assumes
	// (a fixed number of used GDT entries)
	//i386_selector_init(gGDT);  // pass the new gdt

	// allocate an area for the double fault stacks
	virtual_address_restrictions virtualRestrictions = {};
	virtualRestrictions.address_specification = B_ANY_KERNEL_ADDRESS;
	physical_address_restrictions physicalRestrictions = {};
	create_area_etc(B_SYSTEM_TEAM, "double fault stacks",
		kDoubleFaultStackSize * smp_get_num_cpus(), B_FULL_LOCK,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, CREATE_AREA_DONT_WAIT,
		&virtualRestrictions, &physicalRestrictions,
		(void**)&sDoubleFaultStacks);

	X86PagingStructures* kernelPagingStructures
		= static_cast<X86VMTranslationMap*>(
			VMAddressSpace::Kernel()->TranslationMap())->PagingStructures();

	// setup task-state segments
	for (i = 0; i < args->num_cpus; i++) {
		// initialize the regular and double fault tss stored in the per-cpu
		// structure
		memset(&gCPU[i].arch.tss, 0, sizeof(struct tss));
		gCPU[i].arch.tss.ss0 = KERNEL_DATA_SEG;
		gCPU[i].arch.tss.io_map_base = sizeof(struct tss);

		// add TSS descriptor for this new TSS
		set_tss_descriptor(&gGDT[TSS_BASE_SEGMENT + i],
			(addr_t)&gCPU[i].arch.tss, sizeof(struct tss));

		// initialize the double fault tss
		init_double_fault(i);

		// init active translation map
		gCPU[i].arch.active_paging_structures = kernelPagingStructures;
		kernelPagingStructures->AddReference();
	}

	// set the current hardware task on cpu 0
	load_tss(0);

	// setup TLS descriptors (one for every CPU)

	for (i = 0; i < args->num_cpus; i++) {
		set_segment_descriptor(&gGDT[TLS_BASE_SEGMENT + i], 0, TLS_SIZE,
			DT_DATA_WRITEABLE, DPL_USER);
	}

	if (!apic_available())
		x86_init_fpu();
	// else fpu gets set up in smp code

	return B_OK;
}


status_t
arch_cpu_init_post_modules(kernel_args *args)
{
	// initialize CPU module

	void *cookie = open_module_list("cpu");

	while (true) {
		char name[B_FILE_NAME_LENGTH];
		size_t nameLength = sizeof(name);

		if (read_next_module_name(cookie, name, &nameLength) != B_OK
			|| get_module(name, (module_info **)&sCpuModule) == B_OK)
			break;
	}

	close_module_list(cookie);

	// initialize MTRRs if available
	if (x86_count_mtrrs() > 0) {
		sCpuRendezvous = sCpuRendezvous2 = 0;
		call_all_cpus(&init_mtrrs, NULL);
	}

	// get optimized functions from the CPU module
	if (sCpuModule != NULL && sCpuModule->get_optimized_functions != NULL) {
		x86_optimized_functions functions;
		memset(&functions, 0, sizeof(functions));

		sCpuModule->get_optimized_functions(&functions);

		if (functions.memcpy != NULL) {
			gOptimizedFunctions.memcpy = functions.memcpy;
			gOptimizedFunctions.memcpy_end = functions.memcpy_end;
		}

		if (functions.memset != NULL) {
			gOptimizedFunctions.memset = functions.memset;
			gOptimizedFunctions.memset_end = functions.memset_end;
		}
	}

	// put the optimized functions into the commpage
	size_t memcpyLen = (addr_t)gOptimizedFunctions.memcpy_end
		- (addr_t)gOptimizedFunctions.memcpy;
	fill_commpage_entry(COMMPAGE_ENTRY_X86_MEMCPY,
		(const void*)gOptimizedFunctions.memcpy, memcpyLen);
	size_t memsetLen = (addr_t)gOptimizedFunctions.memset_end
		- (addr_t)gOptimizedFunctions.memset;
	fill_commpage_entry(COMMPAGE_ENTRY_X86_MEMSET,
		(const void*)gOptimizedFunctions.memset, memsetLen);

	// add the functions to the commpage image
	image_id image = get_commpage_image();
	elf_add_memory_image_symbol(image, "commpage_memcpy",
		((addr_t*)USER_COMMPAGE_ADDR)[COMMPAGE_ENTRY_X86_MEMCPY], memcpyLen,
		B_SYMBOL_TYPE_TEXT);
	elf_add_memory_image_symbol(image, "commpage_memset",
		((addr_t*)USER_COMMPAGE_ADDR)[COMMPAGE_ENTRY_X86_MEMSET], memsetLen,
		B_SYMBOL_TYPE_TEXT);

	return B_OK;
}


void
i386_set_tss_and_kstack(addr_t kstack)
{
	get_cpu_struct()->arch.tss.sp0 = kstack;
}


void
arch_cpu_global_TLB_invalidate(void)
{
	uint32 flags = x86_read_cr4();

	if (flags & IA32_CR4_GLOBAL_PAGES) {
		// disable and reenable the global pages to flush all TLBs regardless
		// of the global page bit
		x86_write_cr4(flags & ~IA32_CR4_GLOBAL_PAGES);
		x86_write_cr4(flags | IA32_CR4_GLOBAL_PAGES);
	} else {
		cpu_status state = disable_interrupts();
		arch_cpu_user_TLB_invalidate();
		restore_interrupts(state);
	}
}


void
arch_cpu_invalidate_TLB_range(addr_t start, addr_t end)
{
	int32 num_pages = end / B_PAGE_SIZE - start / B_PAGE_SIZE;
	while (num_pages-- >= 0) {
		invalidate_TLB(start);
		start += B_PAGE_SIZE;
	}
}


void
arch_cpu_invalidate_TLB_list(addr_t pages[], int num_pages)
{
	int i;
	for (i = 0; i < num_pages; i++) {
		invalidate_TLB(pages[i]);
	}
}


ssize_t
arch_cpu_user_strlcpy(char *to, const char *from, size_t size,
	addr_t *faultHandler)
{
	int fromLength = 0;
	addr_t oldFaultHandler = *faultHandler;

	// this check is to trick the gcc4 compiler and have it keep the error label
	if (to == NULL && size > 0)
		goto error;

	*faultHandler = (addr_t)&&error;

	if (size > 0) {
		to[--size] = '\0';
		// copy
		for ( ; size; size--, fromLength++, to++, from++) {
			if ((*to = *from) == '\0')
				break;
		}
	}
	// count any leftover from chars
	while (*from++ != '\0') {
		fromLength++;
	}

	*faultHandler = oldFaultHandler;
	return fromLength;

error:
	*faultHandler = oldFaultHandler;
	return B_BAD_ADDRESS;
}


status_t
arch_cpu_user_memset(void *s, char c, size_t count, addr_t *faultHandler)
{
	char *xs = (char *)s;
	addr_t oldFaultHandler = *faultHandler;

	// this check is to trick the gcc4 compiler and have it keep the error label
	if (s == NULL)
		goto error;

	*faultHandler = (addr_t)&&error;

	while (count--)
		*xs++ = c;

	*faultHandler = oldFaultHandler;
	return 0;

error:
	*faultHandler = oldFaultHandler;
	return B_BAD_ADDRESS;
}


status_t
arch_cpu_shutdown(bool rebootSystem)
{
	if (acpi_shutdown(rebootSystem) == B_OK)
		return B_OK;

	if (!rebootSystem)
		return apm_shutdown();

	cpu_status state = disable_interrupts();

	// try to reset the system using the keyboard controller
	out8(0xfe, 0x64);

	// Give some time to the controller to do its job (0.5s)
	snooze(500000);

	// if that didn't help, try it this way
	reboot();

	restore_interrupts(state);
	return B_ERROR;
}


void
arch_cpu_idle(void)
{
	gCpuIdleFunc();
}


void
arch_cpu_sync_icache(void *address, size_t length)
{
	// instruction cache is always consistent on x86
}


void
arch_cpu_memory_read_barrier(void)
{
	asm volatile ("lock;" : : : "memory");
	asm volatile ("addl $0, 0(%%esp);" : : : "memory");
}


void
arch_cpu_memory_write_barrier(void)
{
	asm volatile ("lock;" : : : "memory");
	asm volatile ("addl $0, 0(%%esp);" : : : "memory");
}

