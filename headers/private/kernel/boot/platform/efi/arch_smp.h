/*
 * Copyright 2013-2022 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_BOOT_PLATFORM_EFI_ARCH_SMP_H
#define KERNEL_BOOT_PLATFORM_EFI_ARCH_SMP_H


#include <boot/menu.h>


#if defined(__riscv) || defined(__ARM__) || defined(__aarch64__)
// These platforms take inventory of cpu cores from fdt

struct platform_cpu_info {
	uint32 id; // hart id on riscv
#if defined(__riscv)
	uint32 phandle;
	uint32 plicContext;
#endif
};

#if defined(__riscv)
extern uint32 gBootHart;
#endif

void arch_smp_register_cpu(platform_cpu_info** cpu);
#endif


int arch_smp_get_current_cpu(void);
void arch_smp_init_other_cpus(void);
#ifdef __riscv
platform_cpu_info* arch_smp_find_cpu(uint32 phandle);
void arch_smp_boot_other_cpus(uint64 satp, uint64 kernelEntry, addr_t virtKernelArgs);
#else
void arch_smp_boot_other_cpus(uint32 pml4, uint64 kernelEntry, addr_t virtKernelArgs);
#endif
void arch_smp_add_safemode_menus(Menu *menu);
void arch_smp_init(void);


#endif /* KERNEL_BOOT_PLATFORM_EFI_ARCH_SMP_H */
