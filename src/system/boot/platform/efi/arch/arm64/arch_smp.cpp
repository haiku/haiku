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

extern "C" {
#include <libfdt.h>
}


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

static void arm64_psci_call_smc(uint64 func, uint64 arg0, uint64 arg1, uint64 arg2);
static void arm64_psci_call_hvc(uint64 func, uint64 arg0, uint64 arg1, uint64 arg2);


static platform_cpu_info sCpus[SMP_MAX_CPUS];
static uint32 sCpuCount = 0;

static struct kernel_args* sKernelArgs;
static uint64 sKernelEntry;
static uint64 sSecondaryStacks[SMP_MAX_CPUS];
static void (*sPsciCallFn)(uint64, uint64, uint64, uint64);

enum class CpuEnableMethod {
	Unknown,
	Psci,
	SpinTable
};


static CpuEnableMethod sCpuEnableMethod = CpuEnableMethod::Unknown;


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
	if (sCpuEnableMethod == CpuEnableMethod::Unknown)
		sCpuCount = 1;
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
		sSecondaryStacks[i] = (uint64)stack + size;
	}

	return;
}


void
arm64_secondary_startup()
{
	// We may or may not have a stack at this point, and
	// the bootstrap processor couldn't pass us any information
	// so we need to configure ourselves using static variables
	// without using a stack
	asm(
	//       Put our MPIDR in x0 and clear bits 31 and 24
	//       (which aren't used for identifying the CPU)
		"    mrs x0, MPIDR_EL1\n"
		"    mov x1, #((1 << 31) | (1 << 24))\n"
		"    bic x0, x0, x1\n"
		"    mov x1, %0\n"
		"    ldr x2, =24\n"
	//       Search through sCpus to find the entry corresponding to
	//       our MPIDR
		"0:  ldr x3, [x1, 8]\n"
		"    cmp x0, x3\n"
		"    beq 1f\n"
		"    add x1, x1, x2\n"
		"    b 0b\n"
		"1:  ldr w0, [x1]\n"
	//       Use the id in our sCpus entry to get our stack pointer
	//       from sSecondaryStacks
		"	 lsl x1, x0, #3\n"
		"    mov x2, %1\n"
		"    add x1, x2, x1\n"
		"    ldr x1, [x1]\n"
		"    mov sp, x1\n"
	//       Enable the FPU, jump to arm64_secondary_startup2
		"    mov x1, #0x300000\n"
		"    msr CPACR_EL1, x1\n"
		"    b arm64_secondary_startup2"
		:
		: "r" (sCpus), "r" (sSecondaryStacks)
		: "x0", "x1", "x2", "x3"
	);
}


extern "C" void
arm64_secondary_startup2(uint32 cpu)
{
	arm64_common_cpu_startup();

	arch_enter_kernel(sKernelArgs, sKernelEntry,
		gKernelArgs.cpu_kstack[cpu].start + gKernelArgs.cpu_kstack[cpu].size,
		cpu);
}


void
arch_smp_boot_other_cpus(addr_t ttbr1, uint64 kernelEntry, addr_t virtKernelArgs)
{
	sKernelEntry = kernelEntry;
	sKernelArgs = (struct kernel_args*)virtKernelArgs;

	for (uint32 i = 0; i < sCpuCount; i++) {
		platform_cpu_info* cpu = &sCpus[i];

		if (cpu->id == 0)
			continue;

		switch (sCpuEnableMethod) {
		case CpuEnableMethod::Psci:
			sPsciCallFn(PSCI_CPU_ON, cpu->mpidr, (uint64)arm64_secondary_startup, 0);
			break;
		case CpuEnableMethod::SpinTable:
			*((uint64*)cpu->releaseAddr) = (uint64)arm64_secondary_startup;
			asm("sev");
			break;
		default:
			// Unreachable, we set sCpuCount to 0 already
			break;
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


void
arm64_handle_fdt_cpu_node(const void *fdt, int node)
{
	platform_cpu_info* info = NULL;
	arch_smp_register_cpu(&info);
	if (info == NULL)
		return;
	info->id = sCpuCount - 1;

	int parent = fdt_parent_offset(fdt, node);
	if (fdt32_to_cpu(*(uint32*)fdt_getprop(fdt, parent,
		"#address-cells", NULL)) == 1) {
		info->mpidr = fdt32_to_cpu(*(uint32*)fdt_getprop(fdt, node,
			"reg", NULL));
	} else {
		info->mpidr = fdt64_to_cpu(*(uint64*)fdt_getprop(fdt, node,
			"reg", NULL));
	}

	const char* enableMethod = (const char*)fdt_getprop(fdt, node,
		"enable-method", NULL);
	if (enableMethod == NULL)
		return;

	if (strcmp(enableMethod, "spin-table") == 0) {
		sCpuEnableMethod = CpuEnableMethod::SpinTable;
		uint64* releaseAddr = (uint64*)fdt_getprop(fdt, node,
			"cpu-release-addr", NULL);
		info->releaseAddr = fdt64_to_cpu(*releaseAddr);
	} else if (strcmp(enableMethod, "psci") == 0) {
		sCpuEnableMethod = CpuEnableMethod::Psci;
	}
}


void
arm64_handle_fdt_psci_node(const void *fdt, int node)
{
	const char* method = (const char*)fdt_getprop(fdt, node,
		"method", NULL);

	if (strcmp(method, "smc") == 0) {
		sPsciCallFn = arm64_psci_call_smc;
	} else if (strcmp(method, "hvc") == 0) {
		sPsciCallFn = arm64_psci_call_hvc;
	}
}


static void
arm64_psci_call_smc(uint64 func, uint64 arg0, uint64 arg1, uint64 arg2)
{
	register uint64 x0 asm("x0") = func;
	register uint64 x1 asm("x1") = arg0;
	register uint64 x2 asm("x2") = arg1;
	register uint64 x3 asm("x3") = arg2;
	asm("smc #0" :: "r" (x0), "r" (x1), "r" (x2), "r" (x3));
}


static void
arm64_psci_call_hvc(uint64 func, uint64 arg0, uint64 arg1, uint64 arg2)
{
	register uint64 x0 asm("x0") = func;
	register uint64 x1 asm("x1") = arg0;
	register uint64 x2 asm("x2") = arg1;
	register uint64 x3 asm("x3") = arg2;
	asm("hvc #0" :: "r" (x0), "r" (x1), "r" (x2), "r" (x3));
}
