/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef MMU_H
#define MMU_H


#include <arch/x86/descriptors.h>


#ifndef _ASSEMBLER


#include <SupportDefs.h>


extern segment_descriptor gBootGDT[BOOT_GDT_SEGMENT_COUNT];


// For use with mmu_map_physical_memory()
static const uint32 kDefaultPageFlags = 0x3;	// present, R/W

#ifdef __cplusplus
extern "C" {
#endif

extern void mmu_init(void);
extern void mmu_init_for_kernel(void);
extern addr_t mmu_map_physical_memory(addr_t physicalAddress, size_t size, uint32 flags);
extern void *mmu_allocate(void *virtualAddress, size_t size);
extern void *mmu_allocate_page(addr_t *_physicalAddress);
extern bool mmu_allocate_physical(addr_t base, size_t size);
extern void mmu_free(void *virtualAddress, size_t size);

// Used by the long mode switch code
extern size_t mmu_get_virtual_usage();
extern bool mmu_get_virtual_mapping(addr_t virtualAddress,
	addr_t *_physicalAddress);

#ifdef __cplusplus
}
#endif

#endif	// !_ASSEMBLER

#endif	/* MMU_H */
