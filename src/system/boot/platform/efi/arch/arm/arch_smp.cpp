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


//#define TRACE_SMP
#ifdef TRACE_SMP
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


static platform_cpu_info sCpus[SMP_MAX_CPUS];
uint32 sCpuCount = 0;


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


int
arch_smp_get_current_cpu(void)
{
	// One cpu for now.
	return 0;
}


void
arch_smp_init_other_cpus(void)
{
	// One cpu for now.
	gKernelArgs.num_cpus = 1;
	return;
}


void
arch_smp_boot_other_cpus(uint32 pml4, uint64 kernel_entry)
{
	// One cpu for now.
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
	// One cpu for now.
}
