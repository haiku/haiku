/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_BOOT_ADDR_RANGE_H
#define KERNEL_BOOT_ADDR_RANGE_H


#include <SupportDefs.h>


typedef struct addr_range {
	uint64 start;
	uint64 size;
} addr_range;


typedef struct phys_addr_range {
	phys_addr_t	start;
	phys_size_t	size;
} phys_addr_range;


#ifdef __cplusplus
extern "C" {
#endif

status_t insert_address_range(addr_range* ranges, uint32* _numRanges,
	uint32 maxRanges, uint64 start, uint64 size);
status_t remove_address_range(addr_range* ranges, uint32* _numRanges,
	uint32 maxRanges, uint64 start, uint64 size);
bool get_free_address_range(addr_range* ranges, uint32 numRanges, uint64 base,
	uint64 size, uint64* _rangeBase);
bool is_address_range_covered(addr_range* ranges, uint32 numRanges, uint64 base,
	uint64 size);
void sort_address_ranges(addr_range* ranges, uint32 numRanges);

status_t insert_physical_address_range(phys_addr_range* ranges,
	uint32* _numRanges, uint32 maxRanges, phys_addr_t start, phys_size_t size);
status_t remove_physical_address_range(phys_addr_range* ranges,
	uint32* _numRanges, uint32 maxRanges, phys_addr_t start, phys_size_t size);
bool get_free_physical_address_range(phys_addr_range* ranges, uint32 numRanges,
	phys_addr_t base, phys_size_t size, phys_addr_t* _rangeBase);
bool is_physical_address_range_covered(phys_addr_range* ranges,
	uint32 numRanges, phys_addr_t base, phys_size_t size);
void sort_physical_address_ranges(phys_addr_range* ranges, uint32 numRanges);

status_t insert_physical_memory_range(phys_addr_t start, phys_size_t size);
status_t insert_physical_allocated_range(phys_addr_t start, phys_size_t size);
status_t insert_virtual_allocated_range(uint64 start, uint64 size);
void ignore_physical_memory_ranges_beyond_4gb();

#ifdef __cplusplus
}
#endif

#endif	/* KERNEL_BOOT_ADDR_RANGE_H */
