/*
 * Copyright 2019-2022, Haiku, Inc. All rights reserved.
 * Released under the terms of the MIT License.
*/


#include "arch_smp.h"

#include <algorithm>
#include <string.h>

#include <KernelExport.h>

#include <kernel.h>
#include <safemode.h>
#include <boot/platform.h>
#include <boot/stage2.h>
#include <boot/menu.h>
#include <platform/sbi/sbi_syscalls.h>

#include "mmu.h"


//#define TRACE_SMP
#ifdef TRACE_SMP
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


typedef status_t (*KernelEntry) (kernel_args *bootKernelArgs, int currentCPU);


struct CpuEntryInfo {
	uint64 satp;		//  0
	uint64 stackBase;	//  8
	uint64 stackSize;	// 16
	KernelEntry kernelEntry;// 24
};


static platform_cpu_info sCpus[SMP_MAX_CPUS];
uint32 sCpuCount = 0;


static void
arch_cpu_dump_hart_status(uint64 status)
{
	switch (status) {
		case SBI_HART_STATE_STARTED:
			dprintf("started");
			break;
		case SBI_HART_STATE_STOPPED:
			dprintf("stopped");
			break;
		case SBI_HART_STATE_START_PENDING:
			dprintf("startPending");
			break;
		case SBI_HART_STATE_STOP_PENDING:
			dprintf("stopPending");
			break;
		case SBI_HART_STATE_SUSPENDED:
			dprintf("suspended");
			break;
		case SBI_HART_STATE_SUSPEND_PENDING:
			dprintf("suspendPending");
			break;
		case SBI_HART_STATE_RESUME_PENDING:
			dprintf("resumePending");
			break;
		default:
			dprintf("?(%" B_PRIu64 ")", status);
	}
}


static void
arch_cpu_dump_hart()
{
	dprintf("  hart status:\n");
	for (uint32 i = 0; i < sCpuCount; i++) {
		dprintf("    hart %" B_PRIu32 ": ", i);
		sbiret res = sbi_hart_get_status(sCpus[i].id);
		if (res.error < 0)
			dprintf("error: %" B_PRIu64 , res.error);
		else {
			arch_cpu_dump_hart_status(res.value);
		}
		dprintf("\n");
	}
}


static void __attribute__((naked))
arch_cpu_entry(int hartId, CpuEntryInfo* info)
{
	// enable MMU
	asm("ld t0, 0(a1)");   // CpuEntryInfo::satp
	asm("csrw satp, t0");
	asm("sfence.vma");

	// setup stack
	asm("ld sp, 8(a1)");   // CpuEntryInfo::stackBase
	asm("ld t0, 16(a1)");  // CpuEntryInfo::stackSize
	asm("add sp, sp, t0");
	asm("li fp, 0");

	asm("tail arch_cpu_entry2");
}


extern "C" void
arch_cpu_entry2(int hartId, CpuEntryInfo* info)
{
	dprintf("%s(%d)\n", __func__, hartId);

	uint32 cpu = 0;
	while (cpu < sCpuCount && !(sCpus[cpu].id == (uint32)hartId))
		cpu++;

	if (!(cpu < sCpuCount))
		panic("CPU for hart id %d not found\n", hartId);

	info->kernelEntry(&gKernelArgs, cpu);
	for (;;) {}
}


void
arch_smp_register_cpu(platform_cpu_info** cpu)
{
	dprintf("arch_smp_register_cpu()\n");
	uint32 newCount = sCpuCount + 1;
	if (newCount > SMP_MAX_CPUS) {
		*cpu = NULL;
		return;
	}
	*cpu = &sCpus[sCpuCount];
	sCpuCount = newCount;
}


platform_cpu_info*
arch_smp_find_cpu(uint32 phandle)
{
	for (uint32 i = 0; i < sCpuCount; i++) {
		if (sCpus[i].phandle == phandle)
			return &sCpus[i];
	}
	return NULL;
}


int
arch_smp_get_current_cpu(void)
{
	return Mhartid();
}


void
arch_smp_init_other_cpus(void)
{
	gKernelArgs.num_cpus = sCpuCount;

	// make boot CPU first as expected by kernel
	for (uint32 i = 1; i < sCpuCount; i++) {
		if (sCpus[i].id == gBootHart)
			std::swap(sCpus[i], sCpus[0]);
	}

	for (uint32 i = 0; i < sCpuCount; i++) {
		gKernelArgs.arch_args.hartIds[i] = sCpus[i].id;
		gKernelArgs.arch_args.plicContexts[i] = sCpus[i].plicContext;
	}

	if (get_safemode_boolean(B_SAFEMODE_DISABLE_SMP, false)) {
		// SMP has been disabled!
		TRACE(("smp disabled per safemode setting\n"));
		gKernelArgs.num_cpus = 1;
	}

	if (gKernelArgs.num_cpus < 2)
		return;

	for (uint32 i = 1; i < gKernelArgs.num_cpus; i++) {
		// create a final stack the trampoline code will put the ap processor on
		void * stack = NULL;
		const size_t size = KERNEL_STACK_SIZE
			+ KERNEL_STACK_GUARD_PAGES * B_PAGE_SIZE;
		if (platform_allocate_region(&stack, size, 0) != B_OK)
			panic("Unable to allocate AP stack");

		memset(stack, 0, size);
		gKernelArgs.cpu_kstack[i].start = fix_address((uint64_t)stack);
		gKernelArgs.cpu_kstack[i].size = size;
	}
}


void
arch_smp_boot_other_cpus(uint64 satp, uint64 kernel_entry, addr_t virtKernelArgs)
{
	dprintf("arch_smp_boot_other_cpus(%p, %p)\n", (void*)satp, (void*)kernel_entry);

	arch_cpu_dump_hart();
	for (uint32 i = 0; i < sCpuCount; i++) {
		if (sCpus[i].id != gBootHart) {
			sbiret res;
			dprintf("  starting CPU %" B_PRIu32 "\n", sCpus[i].id);

			dprintf("  stack: %#" B_PRIx64 " - %#" B_PRIx64 "\n",
				gKernelArgs.cpu_kstack[i].start, gKernelArgs.cpu_kstack[i].start
				+ gKernelArgs.cpu_kstack[i].size - 1);

			CpuEntryInfo* info = new(std::nothrow) CpuEntryInfo{
				.satp = satp,
				.stackBase = gKernelArgs.cpu_kstack[i].start,
				.stackSize = gKernelArgs.cpu_kstack[i].size,
				.kernelEntry = (KernelEntry)kernel_entry
			};
			res = sbi_hart_start(sCpus[i].id, (addr_t)&arch_cpu_entry, (addr_t)info);

			for (;;) {
				res = sbi_hart_get_status(sCpus[i].id);
				if (res.error < 0 || res.value == SBI_HART_STATE_STARTED)
					break;
			}
		}
	}
	arch_cpu_dump_hart();
}


void
arch_smp_add_safemode_menus(Menu *menu)
{
	MenuItem *item;

	if (gKernelArgs.num_cpus < 2)
		return;

	item = new(nothrow) MenuItem("Disable SMP");
	menu->AddItem(item);
	item->SetData(B_SAFEMODE_DISABLE_SMP);
	item->SetType(MENU_ITEM_MARKABLE);
	item->SetHelpText("Disables all but one CPU core.");
}


void
arch_smp_init(void)
{
}
