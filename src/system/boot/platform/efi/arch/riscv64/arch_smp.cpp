/*
 * Copyright 2019-2020, Haiku, Inc. All rights reserved.
 * Released under the terms of the MIT License.
*/


#include "arch_smp.h"

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


extern "C" void arch_enter_kernel(uint64 satp, struct kernel_args *kernelArgs,
        addr_t kernelEntry, addr_t kernelStackTop);


struct CpuEntryInfo {
	uint64 satp;
	uint64 kernelEntry;
};


static CpuInfo sCpus[SMP_MAX_CPUS];
uint32 sCpuCount = 0;


static void
CpuEntry(int hartId, CpuEntryInfo* info)
{
	arch_enter_kernel(info->satp, &gKernelArgs, info->kernelEntry,
		gKernelArgs.cpu_kstack[hartId].start
		+ gKernelArgs.cpu_kstack[hartId].size);
}


void
arch_smp_register_cpu(CpuInfo** cpu)
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


int
arch_smp_get_current_cpu(void)
{
	return Mhartid();
}


void
arch_smp_init_other_cpus(void)
{
	// TODO: SMP code disabled for now
	gKernelArgs.num_cpus = 1;
	return;

	if (get_safemode_boolean(B_SAFEMODE_DISABLE_SMP, false)) {
		// SMP has been disabled!
		TRACE(("smp disabled per safemode setting\n"));
		gKernelArgs.num_cpus = 1;
	}

	gKernelArgs.num_cpus = sCpuCount;

	if (gKernelArgs.num_cpus < 2)
		return;

	for (uint32 i = 1; i < gKernelArgs.num_cpus; i++) {
		// create a final stack the trampoline code will put the ap processor on
		void * stack = NULL;
		const size_t size = KERNEL_STACK_SIZE
			+ KERNEL_STACK_GUARD_PAGES * B_PAGE_SIZE;
		if (platform_allocate_region(&stack, size, 0, false) != B_OK) {
			panic("Unable to allocate AP stack");
		}
		memset(stack, 0, size);
		gKernelArgs.cpu_kstack[i].start = fix_address((uint64_t)stack);
		gKernelArgs.cpu_kstack[i].size = size;
	}
}


void
arch_smp_boot_other_cpus(uint64 satp, uint64 kernel_entry)
{
	// TODO: SMP code disabled for now
	return;

	dprintf("arch_smp_boot_other_cpus()\n");
	for (uint32 i = 0; i < sCpuCount; i++) {
		// TODO: mhartid 0 may not exist, or it may not be a core
		// you're interested in (FU540/FU740 hart 0 is mgmt core.)
		if (0 != sCpus[i].id) {
			sbiret res;
			dprintf("starting CPU %" B_PRIu32 "\n", sCpus[i].id);

			res = sbi_hart_get_status(sCpus[i].id);
			dprintf("[PRE] sbi_hart_get_status() -> (%ld, %ld)\n",
				res.error, res.value);

			CpuEntryInfo info = {.satp = satp, .kernelEntry = kernel_entry};
			res = sbi_hart_start(sCpus[i].id, (addr_t)&CpuEntry, (addr_t)&info);
			dprintf("sbi_hart_start() -> (%ld, %ld)\n", res.error, res.value);

			for (;;) {
				res = sbi_hart_get_status(sCpus[i].id);
				if (res.error < 0 || res.value == SBI_HART_STATE_STARTED)
					break;
			}

			dprintf("[POST] sbi_hart_get_status() -> (%ld, %ld)\n",
				res.error, res.value);
		}
	}
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
