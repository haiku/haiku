/*
 * Copyright 2013-2019 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_BOOT_PLATFORM_EFI_ARCH_SMP_H
#define KERNEL_BOOT_PLATFORM_EFI_ARCH_SMP_H


#include <boot/menu.h>


#if defined(__riscv) || defined(__ARM__) || defined(__ARM64__)
// These platforms take inventory of cpu cores from fdt

struct platform_cpu_info {
	uint32 id;
};

void arch_smp_register_cpu(platform_cpu_info** cpu);
#endif


int arch_smp_get_current_cpu(void);
void arch_smp_init_other_cpus(void);
#ifdef __riscv
void arch_smp_boot_other_cpus(uint64 satp, uint64 kernel_entry);
#else
void arch_smp_boot_other_cpus(uint32 pml4, uint64 kernel_entry);
#endif
void arch_smp_add_safemode_menus(Menu *menu);
void arch_smp_init(void);


#endif /* KERNEL_BOOT_PLATFORM_EFI_ARCH_SMP_H */
