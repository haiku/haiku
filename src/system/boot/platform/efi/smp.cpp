/*
 * Copyright 2008, Dustin Howett, dustin.howett@gmail.com. All rights reserved.
 * Copyright 2004-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
*/


#include "smp.h"

#include <string.h>

#include <KernelExport.h>

#include <kernel.h>
#include <safemode.h>
#include <boot/platform.h>
#include <boot/stage2.h>
#include <boot/menu.h>

#include "arch_smp.h"


#define NO_SMP 0

//#define TRACE_SMP
#ifdef TRACE_SMP
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


int
smp_get_current_cpu(void)
{
	return arch_smp_get_current_cpu();
}


void
smp_init_other_cpus(void)
{
	arch_smp_init_other_cpus();
}


void
smp_boot_other_cpus(uint32 pml4, uint64 kernel_entry)
{
	if (gKernelArgs.num_cpus < 2)
		return;

	arch_smp_boot_other_cpus(pml4, kernel_entry);
}


void
smp_add_safemode_menus(Menu *menu)
{
	arch_smp_add_safemode_menus(menu);
}


void
smp_init(void)
{
#if NO_SMP
	gKernelArgs.num_cpus = 1;
	return;
#endif

	arch_smp_init();
}
