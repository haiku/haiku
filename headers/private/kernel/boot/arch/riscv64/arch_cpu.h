/*
 * Copyright 2013-2020 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef BOOT_ARCH_CPU_H
#define BOOT_ARCH_CPU_H


#include <SupportDefs.h>
#include <boot/vfs.h>


#ifdef __cplusplus
extern "C" {
#endif

status_t boot_arch_cpu_init(void);
void arch_ucode_load(BootVolume& volume);

#ifdef __cplusplus
}
#endif


#endif /* BOOT_ARCH_CPU_H */
