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
	
extern status_t arch_set_callback(void);
extern void *arch_mmu_allocate(void *address, size_t size, uint8 protection,
	bool exactAddress);
extern status_t arch_mmu_free(void *address, size_t size);
extern status_t arch_mmu_init(void);

#ifdef __cplusplus
}
#endif

#endif	/* KERNEL_BOOT_PLATFORM_OPENFIRMWARE_ARCH_H */
