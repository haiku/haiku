/*
 * Copyright 2014, Henry Harrington, henry.harrington@gmail.com.
 * Distributed under the terms of the MIT License.
 */

#ifndef MMU_H
#define MMU_H

#include <arch/x86/descriptors.h>

#undef BOOT_GDT_SEGMENT_COUNT
#define BOOT_GDT_SEGMENT_COUNT	(USER_DATA_SEGMENT + 1)

#ifndef _ASSEMBLER

#include "efi_platform.h"

#include <util/FixedWidthPointer.h>


extern segment_descriptor gBootGDT[BOOT_GDT_SEGMENT_COUNT];

static const uint32 kDefaultPageFlags = 0x3;
	// present, R/W
static const uint64 kTableMappingFlags = 0x7;
	// present, R/W, user
static const uint64 kLargePageMappingFlags = 0x183;
	// present, R/W, user, global, large
static const uint64 kPageMappingFlags = 0x103;
	// present, R/W, user, global


#ifdef __cplusplus
extern "C" {
#endif

extern addr_t mmu_map_physical_memory(addr_t physicalAddress,
	size_t size, uint32 flags);
extern void mmu_free(void *virtualAddress, size_t size);

extern void mmu_post_efi_setup(size_t memory_map_size,
	efi_memory_descriptor *memory_map, size_t descriptor_size,
	uint32_t descriptor_version);

extern uint64_t mmu_generate_post_efi_page_tables(size_t memory_map_size,
	efi_memory_descriptor *memory_map, size_t descriptor_size,
	uint32_t descriptor_version);

extern status_t platform_kernel_address_to_bootloader_address(uint64_t address,
	void **_result);

extern status_t platform_bootloader_address_to_kernel_address(void *address,
	uint64_t *_result);

#ifdef __cplusplus
}
#endif


/*! Convert a 32-bit address to a 64-bit address. */
inline uint64
fix_address(uint64 address)
{
	uint64 result;
	if (platform_bootloader_address_to_kernel_address((void *)address, &result)
		!= B_OK) {
		return address;
	} else
		return result;
}


template<typename Type>
inline void
fix_address(FixedWidthPointer<Type>& p)
{
	if (p != NULL)
		p.SetTo(fix_address(p.Get()));
}


#endif	// !_ASSEMBLER

#endif	/* MMU_H */
