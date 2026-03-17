/*
 * Copyright 2013-2022 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_BOOT_PLATFORM_EFI_ARCH_SMP_H
#define KERNEL_BOOT_PLATFORM_EFI_ARCH_SMP_H


#include <boot/menu.h>


#if defined(__riscv)
struct platform_cpu_info {
	uint32 id;
	uint32 phandle;
	uint32 plicContext;
};

extern uint32 gBootHart;
void arch_smp_register_cpu(platform_cpu_info** cpu);
platform_cpu_info* arch_smp_find_cpu(uint32 phandle);
#elif defined(__ARM__)
struct platform_cpu_info {
	uint32 id;
};

void arch_smp_register_cpu(platform_cpu_info** cpu);
#elif defined(__aarch64__)
struct platform_cpu_info {
	uint32 id;
	uint64 mpidr;
};

void arch_smp_register_cpu(platform_cpu_info** cpu);
#endif

int arch_smp_get_current_cpu(void);
void arch_smp_init_other_cpus(void);
void arch_smp_boot_other_cpus(addr_t pageTable, uint64 kernelEntry, addr_t virtKernelArgs);
void arch_smp_add_safemode_menus(Menu *menu);
void arch_smp_init(void);


#endif /* KERNEL_BOOT_PLATFORM_EFI_ARCH_SMP_H */
