/*
 * Copyright 2022 Haiku, Inc. All rights reserved.
 * Released under the terms of the MIT License.
 */
#ifndef EFI_GENERIC_MMU_H
#define EFI_GENERIC_MMU_H


#include <SupportDefs.h>
#include <efi/types.h>
#include <efi/boot-services.h>


void build_physical_memory_list(size_t memoryMapSize, efi_memory_descriptor *memoryMap,
	size_t descriptorSize, uint32_t descriptorVersion,
	uint64_t physicalMemoryLow, uint64_t physicalMemoryHigh);

void build_physical_allocated_list(size_t memoryMapSize, efi_memory_descriptor *memoryMap,
	size_t descriptorSize, uint32_t descriptorVersion);

const char* memory_region_type_str(int type);


#endif /* EFI_GENERIC_MMU_H */
