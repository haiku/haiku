/*
 * Copyright 2013-2019 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_BOOT_PLATFORM_EFI_ARCH_SMP_H
#define KERNEL_BOOT_PLATFORM_EFI_ARCH_SMP_H

#include <boot/menu.h>


int arch_smp_get_current_cpu(void);
void arch_smp_init_other_cpus(void);
void arch_smp_boot_other_cpus(uint32 pml4, uint64 kernel_entry);
void arch_smp_add_safemode_menus(Menu *menu);
void arch_smp_init(void);


#endif /* KERNEL_BOOT_PLATFORM_EFI_ARCH_SMP_H */
