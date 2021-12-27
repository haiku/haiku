/*
 * Copyright 2021 Haiku, Inc. All rights reserved.
 * Released under the terms of the MIT License.
 */


#include "arch_smp.h"


int
arch_smp_get_current_cpu(void)
{
	return 0;
}


void
arch_smp_init_other_cpus(void)
{
}


void
arch_smp_boot_other_cpus(uint32 pml4, uint64 kernel_entry)
{
}


void
arch_smp_add_safemode_menus(Menu *menu)
{
}


void
arch_smp_init(void)
{
}
