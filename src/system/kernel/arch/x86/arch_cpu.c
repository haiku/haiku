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

#include <arch/cpu.h>
#include <arch/x86/selector.h>
#include <boot/kernel_args.h>

#include "interrupts.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>


#define CR0_CACHE_DISABLE		(1UL << 30)
#define CR0_NOT_WRITE_THROUGH	(1UL << 29)

struct set_mtrr_parameter {
	int32	index;
	uint64	base;
	uint64	length;
	uint8	type;
};


extern void reboot(void);
	// from arch_x86.S

static struct tss **sTSS;
//static struct tss **sDoubleFaultTSS;
struct tss **sDoubleFaultTSS;
static int *sIsTSSLoaded;
static vint32 sWaitAllCPUs;

segment_descriptor *gGDT = NULL;

/* Some specials for the double fault handler */
//static struct tss sDoubleFaultTSS;
static uint32 sDoubleFaultStack[10240];

static x86_cpu_module_info *sCpuModule;


struct tss *
x86_get_main_tss(void)
{
	int cpuNum = smp_get_current_cpu();
	
	return sTSS[cpuNum];
}


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
load_tss(void *data, int cpu)
{
	short seg = ((TSS_BASE_SEGMENT + cpu) << 3) | DPL_KERNEL;
	asm("movw  %0, %%ax;"
		"ltr %%ax;" : : "r" (seg) : "eax");

	sIsTSSLoaded[cpu] = true;
}


static void
init_double_fault(int cpuNum)
{
	/* set up the double fault tss */
	/* TODO: Axel - fix SMP support */
	struct tss *tss = sDoubleFaultTSS[cpuNum];
	
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


status_t
arch_cpu_preboot_init(kernel_args *args)
{
	write_dr3(0);
	return B_OK;
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

	sTSS = malloc(sizeof(struct tss *) * args->num_cpus);
	if (sTSS == NULL) {
		panic("arch_cpu_init_post_vm: could not allocate buffer for tss pointers\n");
		return B_NO_MEMORY;
	}
	
	sDoubleFaultTSS = malloc(sizeof(struct tss *) * args->num_cpus);
	if (sDoubleFaultTSS == NULL) {
		panic("arch_cpu_init_post_vm: could not allocate buffer for double fault tss pointers\n");
		return B_NO_MEMORY;
	}

	sIsTSSLoaded = malloc(sizeof(int) * args->num_cpus);
	if (sIsTSSLoaded == NULL) {
		panic("arch_cpu_init_post_vm: could not allocate buffer for tss booleans\n");
		return B_NO_MEMORY;
	}
	memset(sIsTSSLoaded, 0, sizeof(int) * args->num_cpus);

	for (i = 0; i < args->num_cpus; i++) {
		char tssName[32];
		area_id area;

		// create standard tasks
		sprintf(tssName, "tss%lu", i);
		area = create_area(tssName, (void **)&sTSS[i], B_ANY_KERNEL_ADDRESS, B_PAGE_SIZE,
			B_FULL_LOCK, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
		if (area < 0) {
			panic("arch_cpu_init2: unable to create region for tss\n");
			return B_NO_MEMORY;
		}

		// initialize TSS
		memset(sTSS[i], 0, sizeof(struct tss));
		sTSS[i]->ss0 = KERNEL_DATA_SEG;

		// add TSS descriptor for this new TSS
		set_tss_descriptor(&gGDT[TSS_BASE_SEGMENT + i], (addr_t)sTSS[i], sizeof(struct tss));

		// create double-fault task
		sprintf(tssName, "double_fault_tss%lu", i);
		area = create_area(tssName, (void **)&sDoubleFaultTSS[i], B_ANY_KERNEL_ADDRESS, B_PAGE_SIZE,
			B_FULL_LOCK, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
		if (area < 0) {
			panic("arch_cpu_init2: unable to create region for tss\n");
			return B_NO_MEMORY;
		}

		// initialize TSS
		init_double_fault(i);
	}

	call_all_cpus(&load_tss, NULL);

	x86_set_task_gate(8, DOUBLE_FAULT_TSS_BASE_SEGMENT << 3);

	// setup TLS descriptors (one for every CPU)

	for (i = 0; i < args->num_cpus; i++) {
		set_segment_descriptor(&gGDT[TLS_BASE_SEGMENT + i], 0, TLS_SIZE,
			DT_DATA_WRITEABLE, DPL_USER);
	}

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

	if (sCpuModule == NULL)
		return B_OK;

	// initialize MTRRs
	call_all_cpus(&init_mtrrs, NULL);

	return B_OK;
}


void
i386_set_tss_and_kstack(addr_t kstack)
{
	int32 currentCPU = smp_get_current_cpu();

	if (!sIsTSSLoaded[currentCPU]) {
		short seg = ((TSS_BASE_SEGMENT + currentCPU) << 3) | DPL_KERNEL;
		asm("movw  %0, %%ax;"
			"ltr %%ax;" : : "r" (seg) : "eax");
		sIsTSSLoaded[currentCPU] = true;
	}

	sTSS[currentCPU]->sp0 = kstack;
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
arch_cpu_user_memcpy(void *to, const void *from, size_t size, addr_t *fault_handler)
{
	char *tmp = (char *)to;
	char *s = (char *)from;

	*fault_handler = (addr_t)&&error;

	while (size--)
		*tmp++ = *s++;

	*fault_handler = 0;
	return 0;

error:
	*fault_handler = 0;
	return B_BAD_ADDRESS;
}


ssize_t
arch_cpu_user_strlcpy(char *to, const char *from, size_t size, addr_t *faultHandler)
{
	int from_length = 0;

	*faultHandler = (addr_t)&&error;

	if (size > 0) {
		to[--size] = '\0';
		// copy 
		for ( ; size; size--, from_length++, to++, from++) {
			if ((*to = *from) == '\0')
				break;
		}
	}
	// count any leftover from chars
	while (*from++ != '\0')
		from_length++;

	*faultHandler = 0;
	return from_length;

error:
	*faultHandler = 0;
	return B_BAD_ADDRESS;
}


status_t
arch_cpu_user_memset(void *s, char c, size_t count, addr_t *fault_handler)
{
	char *xs = (char *)s;

	*fault_handler = (addr_t)&&error;

	while (count--)
		*xs++ = c;

	*fault_handler = 0;
	return 0;

error:
	*fault_handler = 0;
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

