/*
 * Copyright 2022 Haiku, Inc. All rights reserved.
 * Released under the terms of the MIT License.
 */

#include "generic_mmu.h"

#include <algorithm>
#include <boot/stage2.h>


void
build_physical_memory_list(size_t memoryMapSize, efi_memory_descriptor* memoryMap,
	size_t descriptorSize, uint32_t descriptorVersion,
	uint64_t physicalMemoryLow, uint64_t physicalMemoryHigh)
{
	addr_t addr = (addr_t)memoryMap;

	gKernelArgs.num_physical_memory_ranges = 0;

	// First scan: Add all usable ranges
	for (size_t i = 0; i < memoryMapSize / descriptorSize; ++i) {
		efi_memory_descriptor* entry = (efi_memory_descriptor *)(addr + i * descriptorSize);
		switch (entry->Type) {
			case EfiLoaderCode:
			case EfiLoaderData:
			case EfiBootServicesCode:
			case EfiBootServicesData:
			case EfiConventionalMemory: {
				uint64_t base = entry->PhysicalStart;
				uint64_t end = entry->PhysicalStart + entry->NumberOfPages * B_PAGE_SIZE;
				uint64_t originalSize = end - base;
				base = std::max(base, physicalMemoryLow);
				end = std::min(end, physicalMemoryHigh);

				gKernelArgs.ignored_physical_memory
					+= originalSize - (max_c(end, base) - base);

				if (base >= end)
					break;
				uint64_t size = end - base;

				insert_physical_memory_range(base, size);
				break;
			}
			default:
				break;
		}
	}

	uint64_t initialPhysicalMemory = total_physical_memory();

	// Second scan: Remove everything reserved that may overlap
	for (size_t i = 0; i < memoryMapSize / descriptorSize; ++i) {
		efi_memory_descriptor* entry = (efi_memory_descriptor *)(addr + i * descriptorSize);
		switch (entry->Type) {
			case EfiLoaderCode:
			case EfiLoaderData:
			case EfiBootServicesCode:
			case EfiBootServicesData:
			case EfiConventionalMemory:
				break;
			default:
				uint64_t base = entry->PhysicalStart;
				uint64_t size = entry->NumberOfPages * B_PAGE_SIZE;
				remove_physical_memory_range(base, size);
		}
	}

	gKernelArgs.ignored_physical_memory
		+= initialPhysicalMemory - total_physical_memory();

	sort_address_ranges(gKernelArgs.physical_memory_range,
		gKernelArgs.num_physical_memory_ranges);
}


void
build_physical_allocated_list(size_t memoryMapSize, efi_memory_descriptor* memoryMap,
	size_t descriptorSize, uint32_t descriptorVersion)
{
	gKernelArgs.num_physical_allocated_ranges = 0;

	addr_t addr = (addr_t)memoryMap;
	for (size_t i = 0; i < memoryMapSize / descriptorSize; ++i) {
		efi_memory_descriptor* entry = (efi_memory_descriptor *)(addr + i * descriptorSize);
		switch (entry->Type) {
			case EfiLoaderData: {
				uint64_t base = entry->PhysicalStart;
				uint64_t size = entry->NumberOfPages * B_PAGE_SIZE;
				insert_physical_allocated_range(base, size);
				break;
			}
			default:
				;
		}
	}

	sort_address_ranges(gKernelArgs.physical_allocated_range,
		gKernelArgs.num_physical_allocated_ranges);
}


const char*
memory_region_type_str(int type)
{
	switch (type)	{
		case EfiReservedMemoryType:
			return "EfiReservedMemoryType";
		case EfiLoaderCode:
			return "EfiLoaderCode";
		case EfiLoaderData:
			return "EfiLoaderData";
		case EfiBootServicesCode:
			return "EfiBootServicesCode";
		case EfiBootServicesData:
			return "EfiBootServicesData";
		case EfiRuntimeServicesCode:
			return "EfiRuntimeServicesCode";
		case EfiRuntimeServicesData:
			return "EfiRuntimeServicesData";
		case EfiConventionalMemory:
			return "EfiConventionalMemory";
		case EfiUnusableMemory:
			return "EfiUnusableMemory";
		case EfiACPIReclaimMemory:
			return "EfiACPIReclaimMemory";
		case EfiACPIMemoryNVS:
			return "EfiACPIMemoryNVS";
		case EfiMemoryMappedIO:
			return "EfiMemoryMappedIO";
		case EfiMemoryMappedIOPortSpace:
			return "EfiMemoryMappedIOPortSpace";
		case EfiPalCode:
			return "EfiPalCode";
		case EfiPersistentMemory:
			return "EfiPersistentMemory";
		default:
			return "unknown";
	}
}
