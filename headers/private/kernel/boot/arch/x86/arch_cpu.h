/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef BOOT_ARCH_CPU_H
#define BOOT_ARCH_CPU_H


#include <SupportDefs.h>
#include <boot/vfs.h>

#include "../../../arch/x86/arch_cpuasm.h"


#ifdef __cplusplus
extern "C" {
#endif


void calculate_cpu_conversion_factor(uint8 channel);
status_t boot_arch_cpu_init(void);
void arch_ucode_load(BootVolume& volume);


#ifdef __cplusplus
}
#endif


#endif	/* BOOT_ARCH_CPU_H */
