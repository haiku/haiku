/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef BOOT_ARCH_CPU_H
#define BOOT_ARCH_CPU_H


#include <SupportDefs.h>


#ifdef __cplusplus
extern "C" {
#endif


void calculate_cpu_conversion_factor(uint8 channel);


#ifdef __cplusplus
}
#endif

#include <boot/vfs.h>

void ucode_load(BootVolume& volume);


#endif	/* BOOT_ARCH_CPU_H */
