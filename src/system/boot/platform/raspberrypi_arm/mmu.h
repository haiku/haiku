/*
 * Copyright 2009 Jonas Sundström, jonas@kirilla.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_BOOT_PLATFORM_PI_MMU_H
#define _SYSTEM_BOOT_PLATFORM_PI_MMU_H


#include <SupportDefs.h>


#ifdef __cplusplus
extern "C" {
#endif

extern void mmu_init(void);
extern void mmu_init_for_kernel(void);

extern addr_t mmu_map_physical_memory(addr_t physicalAddress, size_t size,
	uint32 flags);

extern void* mmu_allocate(void* virtualAddress, size_t size);
extern void mmu_free(void* virtualAddress, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* _SYSTEM_BOOT_PLATFORM_PI_MMU_H */
