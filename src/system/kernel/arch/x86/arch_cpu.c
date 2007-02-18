/*
 * Copyright 2002-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <boot_device.h>
#include <smp.h>
#include <tls.h>
#include <vm.h>

#include <arch_system_info.h>
#include <cpu.h>
#include <arch/cpu.h>
#include <arch/x86/selector.h>
#include <boot/kernel_args.h>

#include "interrupts.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>


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

struct set_mtrr_parameter {
	int32	index;
	uint64	base;
	uint64	length;
	uint8	type;
};


extern void reboot(void);
	// from arch_x86.S

void (*gX86SwapFPUFunc)(void *oldState, const void *newState);
bool gHasSSE = false;

static vint32 sWaitAllCPUs;

segment_descriptor *gGDT = NULL;

/* Some specials for the double fault handler */
//static struct tss sDoubleFaultTSS;
static uint32 sDoubleFaultStack[10240];

static x86_cpu_module_info *sCpuModule;

/**	Disable CPU caches, and invalidate them. */

static void
disable_caches()
{
	x86_write_cr0((x86_read_cr0() | CR0_CACHE_DISABLE) & ~CR0_NOT_WRITE_THROUGH);
	wbinvd();
	arch_cpu_global_TLB_invalidate();
}


/**	Invalidate CPU caches, and enable them. */

static void
enable_caches()
{
	wbinvd();
	arch_cpu_global_TLB_invalidate();
	x86_write_cr0(x86_read_cr0() & ~(CR0_CACHE_DISABLE | CR0_NOT_WRITE_THROUGH));
}


static void
set_mtrr(void *_parameter, int cpu)
{
	struct set_mtrr_parameter *parameter = (struct set_mtrr_parameter *)_parameter;

	// wait until all CPUs have arrived here
	atomic_add(&sWaitAllCPUs, 1);
	while (sWaitAllCPUs != smp_get_num_cpus())
		;

	disable_caches();

	sCpuModule->set_mtrr(parameter->index, parameter->base, parameter->length,
		parameter->type);

	enable_caches();

	// wait until all CPUs have arrived here
	atomic_add(&sWaitAllCPUs, -1);
	while (sWaitAllCPUs != 0)
		;
}


static void
init_mtrrs(void *_unused, int cpu)
{
	// wait until all CPUs have arrived here
	atomic_add(&sWaitAllCPUs, 1);
	while (sWaitAllCPUs != smp_get_num_cpus())
		;

	disable_caches();

	sCpuModule->init_mtrrs();

	enable_caches();

	// wait until all CPUs have arrived here
	atomic_add(&sWaitAllCPUs, -1);
	while (sWaitAllCPUs != 0)
		;
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
	cpu_status state;

	struct set_mtrr_parameter parameter;
	parameter.index = index;
	parameter.base = base;
	parameter.length = length;
	parameter.type = type;

	call_all_cpus(&set_mtrr, &parameter);
}


status_t
x86_get_mtrr(uint32 index, uint64 *_base, uint64 *_length, uint8 *_type)
{
	// the MTRRs are identical on all CPUs, so it doesn't matter
	// on which CPU this runs
	return sCpuModule->get_mtrr(index, _base, _length, _type);
}


static void
init_sse(void)
{
	if (!x86_check_feature(IA32_FEATURE_SSE, FEATURE_COMMON)
		|| !x86_check_feature(IA32_FEATURE_FXSR, FEATURE_COMMON)) {
		// we don't have proper SSE support
		return;
	}

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
	/* set up the double fault tss */
	/* TODO: Axel - fix SMP support */
	struct tss *tss = &gCPU[cpuNum].arch.tss;
	
	memset(tss, 0, sizeof(struct tss));
	tss->sp0 = (uint32)sDoubleFaultStack + sizeof(sDoubleFaultStack);
	tss->ss0 = KERNEL_DATA_SEG;
	read_cr3(tss->cr3); // copy the current cr3 to the double fault cr3
	tss->eip = (uint32)&double_fault;
	tss->es = KERNEL_DATA_SEG;
	tss->cs = KERNEL_CODE_SEG;
	tss->ss = KERNEL_DATA_SEG;
	tss->esp = tss->sp0;
	tss->ds = KERNEL_DATA_SEG;
	tss->fs = KERNEL_DATA_SEG;
	tss->gs = KERNEL_DATA_SEG;
	tss->ldt_seg_selector = 0;

	// add TSS descriptor for this new TSS
	set_tss_descriptor(&gGDT[DOUBLE_FAULT_TSS_BASE_SEGMENT + cpuNum], (addr_t)tss, sizeof(struct tss));

//	set_tss_descriptor(&gGDT[DOUBLE_FAULT_TSS_SEGMENT + cpuNum], (addr_t)sDoubleFaultTSS[cpuNum], sizeof(struct tss));
}

static void make_feature_string(cpu_ent *cpu, char *str, size_t strlen)
{
	str[0] = 0;

	if(cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_FPU)
		strlcat(str, "fpu ", strlen);
	if(cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_VME)
		strlcat(str, "vme ", strlen);
	if(cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_DE)
		strlcat(str, "de ", strlen);
	if(cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_PSE)
		strlcat(str, "pse ", strlen);
	if(cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_TSC)
		strlcat(str, "tsc ", strlen);
	if(cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_MSR)
		strlcat(str, "msr ", strlen);
	if(cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_PAE)
		strlcat(str, "pae ", strlen);
	if(cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_MCE)
		strlcat(str, "mce ", strlen);
	if(cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_CX8)
		strlcat(str, "cx8 ", strlen);
	if(cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_APIC)
		strlcat(str, "apic ", strlen);
	if(cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_SEP)
		strlcat(str, "sep ", strlen);
	if(cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_MTRR)
		strlcat(str, "mtrr ", strlen);
	if(cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_PGE)
		strlcat(str, "pge ", strlen);
	if(cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_MCA)
		strlcat(str, "mca ", strlen);
	if(cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_CMOV)
		strlcat(str, "cmov ", strlen);
	if(cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_PAT)
		strlcat(str, "pat ", strlen);
	if(cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_PSE36)
		strlcat(str, "pse36 ", strlen);
	if(cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_PSN)
		strlcat(str, "psn ", strlen);
	if(cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_CLFSH)
		strlcat(str, "clfsh ", strlen);
	if(cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_DS)
		strlcat(str, "ds ", strlen);
	if(cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_ACPI)
		strlcat(str, "acpi ", strlen);
	if(cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_MMX)
		strlcat(str, "mmx ", strlen);
	if(cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_FXSR)
		strlcat(str, "fxsr ", strlen);
	if(cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_SSE)
		strlcat(str, "sse ", strlen);
	if(cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_SSE2)
		strlcat(str, "sse2 ", strlen);
	if(cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_SS)
		strlcat(str, "ss ", strlen);
	if(cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_HTT)
		strlcat(str, "htt ", strlen);
	if(cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_TM)
		strlcat(str, "tm ", strlen);
	if(cpu->arch.feature[FEATURE_COMMON] & IA32_FEATURE_PBE)
		strlcat(str, "pbe ", strlen);
	if(cpu->arch.feature[FEATURE_EXT] & IA32_FEATURE_EXT_SSE3)
		strlcat(str, "sse3 ", strlen);
	if(cpu->arch.feature[FEATURE_EXT] & IA32_FEATURE_EXT_MONITOR)
		strlcat(str, "monitor ", strlen);
	if(cpu->arch.feature[FEATURE_EXT] & IA32_FEATURE_EXT_DSCPL)
		strlcat(str, "dscpl ", strlen);
	if(cpu->arch.feature[FEATURE_EXT] & IA32_FEATURE_EXT_EST)
		strlcat(str, "est ", strlen);
	if(cpu->arch.feature[FEATURE_EXT] & IA32_FEATURE_EXT_TM2)
		strlcat(str, "tm2 ", strlen);
	if(cpu->arch.feature[FEATURE_EXT] & IA32_FEATURE_EXT_CNXTID)
		strlcat(str, "cnxtid ", strlen);
	if(cpu->arch.feature[FEATURE_EXT_AMD] & IA32_FEATURE_AMD_EXT_SYSCALL)
		strlcat(str, "syscall ", strlen);
	if(cpu->arch.feature[FEATURE_EXT_AMD] & IA32_FEATURE_AMD_EXT_NX)
		strlcat(str, "nx ", strlen);
	if(cpu->arch.feature[FEATURE_EXT_AMD] & IA32_FEATURE_AMD_EXT_MMXEXT)
		strlcat(str, "mmxext ", strlen);
	if(cpu->arch.feature[FEATURE_EXT_AMD] & IA32_FEATURE_AMD_EXT_FFXSR)
		strlcat(str, "ffxsr ", strlen);
	if(cpu->arch.feature[FEATURE_EXT_AMD] & IA32_FEATURE_AMD_EXT_LONG)
		strlcat(str, "long ", strlen);
	if(cpu->arch.feature[FEATURE_EXT_AMD] & IA32_FEATURE_AMD_EXT_3DNOWEXT)
		strlcat(str, "3dnowext ", strlen);
	if(cpu->arch.feature[FEATURE_EXT_AMD] & IA32_FEATURE_AMD_EXT_3DNOW)
		strlcat(str, "3dnow ", strlen);
}

static int detect_cpu(kernel_args *ka, int curr_cpu) 
{
	cpuid_info cpuid;
	unsigned int data[4];
	char vendor_str[17];
	int i;
	cpu_ent *cpu = get_cpu_struct();

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
	memset(vendor_str, 0, sizeof(vendor_str));
	memcpy(vendor_str, cpuid.eax_0.vendor_id, sizeof(cpuid.eax_0.vendor_id));

	// get the family, model, stepping
	get_current_cpuid(&cpuid, 1);
	cpu->arch.type = cpuid.eax_1.type;
	cpu->arch.family = cpuid.eax_1.family;
	cpu->arch.model = cpuid.eax_1.model;
	cpu->arch.stepping = cpuid.eax_1.stepping;
	dprintf("CPU %d: type %d family %d model %d stepping %d, string '%s'\n",
		curr_cpu, cpu->arch.type, cpu->arch.family, cpu->arch.model, cpu->arch.stepping, vendor_str);

	// figure out what vendor we have here

	for(i=0; i<VENDOR_NUM; i++) {
		if(vendor_info[i].ident_string[0] && !strcmp(vendor_str, vendor_info[i].ident_string[0])) {
			cpu->arch.vendor = i;
			cpu->arch.vendor_name = vendor_info[i].vendor;
			break;
		}
		if(vendor_info[i].ident_string[1] && !strcmp(vendor_str, vendor_info[i].ident_string[1])) {
			cpu->arch.vendor = i;
			cpu->arch.vendor_name = vendor_info[i].vendor;
			break;
		}
	}

	// see if we can get the model name
	get_current_cpuid(&cpuid, 0x80000000);
	if(cpuid.eax_0.max_eax >= 0x80000004) {
		// build the model string (need to swap ecx/edx data before copying)
		unsigned int temp;
		memset(cpu->arch.model_name, 0, sizeof(cpu->arch.model_name));
		get_current_cpuid(&cpuid, 0x80000002);
		temp = cpuid.regs.edx; cpuid.regs.edx = cpuid.regs.ecx; cpuid.regs.ecx = temp;
		memcpy(cpu->arch.model_name, cpuid.as_chars, sizeof(cpuid.as_chars));
		get_current_cpuid(&cpuid, 0x80000003);
		temp = cpuid.regs.edx; cpuid.regs.edx = cpuid.regs.ecx; cpuid.regs.ecx = temp;
		memcpy(cpu->arch.model_name + 16, cpuid.as_chars, sizeof(cpuid.as_chars));
		get_current_cpuid(&cpuid, 0x80000004);
		temp = cpuid.regs.edx; cpuid.regs.edx = cpuid.regs.ecx; cpuid.regs.ecx = temp;
		memcpy(cpu->arch.model_name + 32, cpuid.as_chars, sizeof(cpuid.as_chars));

		// some cpus return a right-justified string
		for(i = 0; cpu->arch.model_name[i] == ' '; i++)
			;
		if(i > 0) {
			memmove(cpu->arch.model_name, 
				&cpu->arch.model_name[i], 
				strlen(&cpu->arch.model_name[i]) + 1);
		}

		dprintf("CPU %d: vendor '%s' model name '%s'\n", 
			curr_cpu, cpu->arch.vendor_name, cpu->arch.model_name);
	} else {
		strcpy(cpu->arch.model_name, "unknown");
	}

	// load feature bits
	get_current_cpuid(&cpuid, 1);
	cpu->arch.feature[FEATURE_COMMON] = cpuid.eax_1.features; // edx
	cpu->arch.feature[FEATURE_EXT] = cpuid.eax_1.extended_features; // ecx
	if(cpu->arch.vendor == VENDOR_AMD) {
		get_current_cpuid(&cpuid, 0x80000001);
		cpu->arch.feature[FEATURE_EXT_AMD] = cpuid.regs.edx; // edx
	}

	make_feature_string(cpu, cpu->arch.feature_string, sizeof(cpu->arch.feature_string));
	dprintf("CPU %d: features: %s\n", curr_cpu, cpu->arch.feature_string);

	return 0;
}

bool x86_check_feature(uint32 feature, enum x86_feature_type type)
{
	cpu_ent *cpu = get_cpu_struct();

#if 0
	int i;
	dprintf("x86_check_feature: feature 0x%x, type %d\n", feature, type);
	for (i = 0; i < FEATURE_NUM; i++) {
		dprintf("features %d: 0x%x\n", i, cpu->arch.feature[i]);
	}
#endif

	return (cpu->arch.feature[type] & feature) ? TRUE : FALSE;
}

//	#pragma mark -

status_t
arch_cpu_preboot_init(kernel_args *args)
{
	write_dr3(0);

	x86_write_cr0(x86_read_cr0() & ~(CR0_FPU_EMULATION | CR0_MONITOR_FPU));
	gX86SwapFPUFunc = i386_fnsave_swap;

	return B_OK;
}


status_t 
arch_cpu_init_percpu(kernel_args *args, int curr_cpu)
{
	detect_cpu(args, curr_cpu);

	// load the TSS for this cpu
	// note the main cpu gets initialized in arch_cpu_init_post_vm()
	if (curr_cpu != 0)
		load_tss(curr_cpu);

	return 0;
}

status_t
arch_cpu_init(kernel_args *args)
{
	__x86_setup_system_time(args->arch_args.system_time_cv_factor);

	return B_OK;
}


status_t
arch_cpu_init_post_vm(kernel_args *args)
{
	uint32 i;

	// account for the segment descriptors

	gGDT = (segment_descriptor *)args->arch_args.vir_gdt;
	create_area("gdt", (void **)&gGDT, B_EXACT_ADDRESS, B_PAGE_SIZE, B_ALREADY_WIRED,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);

	// currently taken out of the build, because it's not yet used (and assumes
	// (a fixed number of used GDT entries)
	//i386_selector_init(gGDT);  // pass the new gdt

	// setup task-state segments
	for (i = 0; i < args->num_cpus; i++) {
		// initialize the regular and double fault tss stored in the per-cpu structure
		memset(&gCPU[i].arch.tss, 0, sizeof(struct tss));
		gCPU[i].arch.tss.ss0 = KERNEL_DATA_SEG;

		// add TSS descriptor for this new TSS
		set_tss_descriptor(&gGDT[TSS_BASE_SEGMENT + i], (addr_t)&gCPU[i].arch.tss, sizeof(struct tss));

		// initialize the double fault tss
		init_double_fault(i);
	}

	// set the current hardware task on cpu 0
	load_tss(0);

	x86_set_task_gate(8, DOUBLE_FAULT_TSS_BASE_SEGMENT << 3);

	// setup TLS descriptors (one for every CPU)

	for (i = 0; i < args->num_cpus; i++) {
		set_segment_descriptor(&gGDT[TLS_BASE_SEGMENT + i], 0, TLS_SIZE,
			DT_DATA_WRITEABLE, DPL_USER);
	}

	// setup SSE2/3 support
	init_sse();

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
	if (x86_count_mtrrs() > 0)
		call_all_cpus(&init_mtrrs, NULL);

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
	} else
		arch_cpu_user_TLB_invalidate();
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


status_t
arch_cpu_user_memcpy(void *to, const void *from, size_t size, addr_t *faultHandler)
{
	char *tmp = (char *)to;
	char *s = (char *)from;

	*faultHandler = (addr_t)&&error;

	while (size--)
		*tmp++ = *s++;

	*faultHandler = 0;
	return 0;

error:
	*faultHandler = 0;
	return B_BAD_ADDRESS;
}


ssize_t
arch_cpu_user_strlcpy(char *to, const char *from, size_t size, addr_t *faultHandler)
{
	int fromLength = 0;

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

	*faultHandler = 0;
	return fromLength;

error:
	*faultHandler = 0;
	return B_BAD_ADDRESS;
}


status_t
arch_cpu_user_memset(void *s, char c, size_t count, addr_t *faultHandler)
{
	char *xs = (char *)s;

	*faultHandler = (addr_t)&&error;

	while (count--)
		*xs++ = c;

	*faultHandler = 0;
	return 0;

error:
	*faultHandler = 0;
	return B_BAD_ADDRESS;
}


status_t
arch_cpu_shutdown(bool rebootSystem)
{
	cpu_status state = disable_interrupts();

	if (!rebootSystem) {
		status_t status = apm_shutdown();

		restore_interrupts(state);
		return status;
	}

	// try to reset the system using the keyboard controller
	out8(0xfe, 0x64);

	// if that didn't help, try it this way
	reboot();

	restore_interrupts(state);
	return B_ERROR;
}


void
arch_cpu_idle(void)
{
	asm("hlt");
}


void
arch_cpu_sync_icache(void *address, size_t length)
{
	// instruction cache is always consistent on x86
}

