/*
 * Copyright 2021-2022, Haiku, Inc. All rights reserved.
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

#include "mmu.h"
#include "aarch64.h"


//#define TRACE_SMP
#ifdef TRACE_SMP
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


#define PSCI_CPU_ON 0xc4000003UL


extern "C" void arch_enter_kernel(struct kernel_args* kernelArgs,
	addr_t kernelEntry, addr_t kernelStackTop, uint32 cpu);
void arm64_common_cpu_startup();


struct secondary_startup_info {
	uint64 ttbr0;
	uint64 ttbr1;
	uint64 sctlr;
	uint64 tcr;
	uint64 mair;
	uint64 stack;
	uint64 kernelEntry;
	uint64 kernelArgs;
	uint32 id;
};


static platform_cpu_info sCpus[SMP_MAX_CPUS];
uint32 sCpuCount = 0;


void
arch_smp_register_cpu(platform_cpu_info** cpu)
{
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
	return 0;
}


void
arch_smp_init_other_cpus(void)
{
	gKernelArgs.num_cpus = sCpuCount;

	if (get_safemode_boolean(B_SAFEMODE_DISABLE_SMP, false)) {
		// SMP has been disabled!
		TRACE("smp disabled per safemode setting\n");
		gKernelArgs.num_cpus = 1;
	}

	if (gKernelArgs.num_cpus < 2)
		return;

	for (uint32 i = 1; i < gKernelArgs.num_cpus; i++) {
		// create a final stack the trampoline code will put the ap processor on
		void * stack = NULL;
		const size_t size = KERNEL_STACK_SIZE + KERNEL_STACK_GUARD_PAGES * B_PAGE_SIZE;
		if (platform_allocate_region(&stack, size, 0) != B_OK) {
			panic("Unable to allocate AP stack");
		}
		memset(stack, 0, size);
		gKernelArgs.cpu_kstack[i].start = fix_address((uint64_t)stack);
		gKernelArgs.cpu_kstack[i].size = size;
	}
	return;
}


static void
arm64_secondary_startup()
{
	asm("ldr x1, [x0, #0]");
	asm("msr TTBR0_EL1, x1");
	asm("ldr x1, [x0, #8]");
	asm("msr TTBR1_EL1, x1");
	asm("ldr x1, [x0, #16]");
	asm("msr SCTLR_EL1, x1");
	asm("ldr x1, [x0, #24]");
	asm("msr TCR_EL1, x1");
	asm("ldr x1, [x0, #32]");
	asm("msr MAIR_EL1, x1");
	asm("ldr x1, [x0, #40]");
	asm("mov sp, x1");
	asm("ldr x1, =0x300000");
	asm("msr CPACR_EL1, x1");
	asm("b arm64_secondary_startup2");
}


extern "C" void
arm64_secondary_startup2(secondary_startup_info *startup)
{
	arm64_common_cpu_startup();

	arch_enter_kernel((struct kernel_args *)startup->kernelArgs, startup->kernelEntry,
		gKernelArgs.cpu_kstack[startup->id].start + gKernelArgs.cpu_kstack[startup->id].size,
		startup->id);
}


void
arch_smp_boot_other_cpus(addr_t ttbr1, uint64 kernelEntry, addr_t virtKernelArgs)
{
	for (uint32 i = 0; i < sCpuCount; i++) {
		platform_cpu_info* cpu = &sCpus[i];

		if (cpu->id == 0)
			continue;

		void* stack = NULL;
		platform_kernel_address_to_bootloader_address(
			gKernelArgs.cpu_kstack[cpu->id].start,
			&stack);
		secondary_startup_info* info = new secondary_startup_info {
			.ttbr0 = READ_SPECIALREG(TTBR0_EL1),
			.ttbr1 = ttbr1,
			.sctlr = READ_SPECIALREG(SCTLR_EL1)
				& ~(SCTLR_C | SCTLR_M),
			.tcr = READ_SPECIALREG(TCR_EL1),
			.mair = READ_SPECIALREG(MAIR_EL1),
			.stack = ((uint64)stack) + gKernelArgs.cpu_kstack[cpu->id].size,
			.kernelEntry = kernelEntry,
			.kernelArgs = virtKernelArgs,
			.id = cpu->id
		};

		asm(
			"mov x0, %0\n"
			"mov x1, %1\n"
			"mov x2, %2\n"
			"mov x3, %3\n"
			"hvc #0"
			::
			"r" (PSCI_CPU_ON),
			"r" (cpu->mpidr),
			"r" ((uint64)&arm64_secondary_startup),
			"r" ((uint64)info)
			: "x0", "x1", "x2", "x3"
		);
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
