/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef KERNEL_BOOT_PLATFORM_OPENFIRMWARE_ARCH_H
#define KERNEL_BOOT_PLATFORM_OPENFIRMWARE_ARCH_H


#include <SupportDefs.h>


#ifdef __cplusplus
extern "C" {
#endif
	
extern void *arch_mmu_alloc_at(void *address, size_t size, uint8 protection);
extern void *arch_mmu_alloc(size_t size, uint8 protection);
extern status_t arch_mmu_init(void);

#ifdef __cplusplus
}
#endif

#endif	/* KERNEL_BOOT_PLATFORM_OPENFIRMWARE_ARCH_H */
