/*
 * Copyright 2008, Dustin Howett, dustin.howett@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
*/


#include "arch_timer.h"

#include "mmu.h"
#include "acpi.h"

#include <KernelExport.h>
#include <SupportDefs.h>

#include <kernel.h>
#include <safemode.h>
#include <boot/stage2.h>
#include <boot/menu.h>
#include <boot/arch/x86/arch_cpu.h>
#include <arch/x86/arch_acpi.h>
#include <arch/x86/arch_hpet.h>
#include <arch/x86/arch_system_info.h>

#include <string.h>

//#define TRACE_TIMER
#ifdef TRACE_TIMER
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


static void
hpet_init(void)
{
	// Try to find the HPET ACPI table.
	TRACE(("hpet_init: Looking for HPET...\n"));
	acpi_hpet *hpet = (acpi_hpet *)acpi_find_table(ACPI_HPET_SIGNATURE);

	// Clear hpet kernel args to known invalid state;
	gKernelArgs.arch_args.hpet_phys = 0;
	gKernelArgs.arch_args.hpet = NULL;

	if (hpet == NULL) {
		// No HPET table in the RSDT.
		// Since there are no other methods for finding it,
		// assume we don't have one.
		TRACE(("hpet_init: HPET not found.\n"));
		return;
	}

	TRACE(("hpet_init: found HPET at %x.\n", hpet->hpet_address.address));
	gKernelArgs.arch_args.hpet_phys = hpet->hpet_address.address;
	gKernelArgs.arch_args.hpet = (void *)mmu_map_physical_memory(
		gKernelArgs.arch_args.hpet_phys, B_PAGE_SIZE, EfiACPIReclaimMemory);
}


void
arch_timer_init(void)
{
	calculate_cpu_conversion_factor(2);
	hpet_init();
}
