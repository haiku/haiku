/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MMU_H
#define MMU_H


#include <SupportDefs.h>


// For use with mmu_map_physical_memory()
static const uint32 kDefaultPageFlags = 0x3;
	// present, R/W


#ifdef __cplusplus
extern "C" {
#endif

extern void mmu_init(void);
extern void mmu_init_for_kernel(void);
extern addr_t mmu_map_physical_memory(addr_t physicalAddress,
	size_t size, uint32 flags);
extern void *mmu_allocate(void *virtualAddress, size_t size);
extern void mmu_free(void *virtualAddress, size_t size);

#ifdef __cplusplus
}
#endif


#endif	/* MMU_H */
