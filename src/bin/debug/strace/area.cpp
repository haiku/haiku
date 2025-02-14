/*
 * Copyright 2023, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>
#include <vm_defs.h>

#include "strace.h"
#include "Syscall.h"
#include "TypeHandler.h"


#define FLAG_INFO_ENTRY(name) \
	{ name, #name }

static const FlagsTypeHandler::FlagInfo kAreaProtectionFlagInfos[] = {
	FLAG_INFO_ENTRY(B_READ_AREA),
	FLAG_INFO_ENTRY(B_WRITE_AREA),
	FLAG_INFO_ENTRY(B_EXECUTE_AREA),
	FLAG_INFO_ENTRY(B_STACK_AREA),

	FLAG_INFO_ENTRY(B_CLONEABLE_AREA),
	FLAG_INFO_ENTRY(B_OVERCOMMITTING_AREA),

	{ 0, NULL }
};


struct enum_info {
	unsigned int index;
	const char *name;
};

#define ENUM_INFO_ENTRY(name) \
	{ name, #name }

static const enum_info kAddressSpecs[] = {
	ENUM_INFO_ENTRY(B_ANY_ADDRESS),
	ENUM_INFO_ENTRY(B_EXACT_ADDRESS),
	ENUM_INFO_ENTRY(B_BASE_ADDRESS),
	ENUM_INFO_ENTRY(B_CLONE_ADDRESS),
	ENUM_INFO_ENTRY(B_ANY_KERNEL_ADDRESS),
	ENUM_INFO_ENTRY(B_RANDOMIZED_ANY_ADDRESS),
	ENUM_INFO_ENTRY(B_RANDOMIZED_BASE_ADDRESS),

	{ 0, NULL }
};


static const enum_info kAreaMappings[] = {
	ENUM_INFO_ENTRY(REGION_NO_PRIVATE_MAP),
	ENUM_INFO_ENTRY(REGION_PRIVATE_MAP),

	{ 0, NULL }
};

static FlagsTypeHandler::FlagsList kAreaProtectionFlags;
static EnumTypeHandler::EnumMap kAddressSpecsMap;
static EnumTypeHandler::EnumMap kAreaMappingMap;


void
patch_area()
{
	for (int i = 0; kAreaProtectionFlagInfos[i].name != NULL; i++) {
		kAreaProtectionFlags.push_back(kAreaProtectionFlagInfos[i]);
	}
	for (int i = 0; kAddressSpecs[i].name != NULL; i++) {
		kAddressSpecsMap[kAddressSpecs[i].index] = kAddressSpecs[i].name;
	}
	for (int i = 0; kAreaMappings[i].name != NULL; i++) {
		kAreaMappingMap[kAreaMappings[i].index] = kAreaMappings[i].name;
	}

	Syscall *create = get_syscall("_kern_create_area");
	create->GetParameter("address")->SetInOut(true);
	create->GetParameter("addressSpec")->SetHandler(
		new EnumTypeHandler(kAddressSpecsMap));
	create->GetParameter("protection")->SetHandler(
		new FlagsTypeHandler(kAreaProtectionFlags));

	Syscall *transfer = get_syscall("_kern_transfer_area");
	transfer->GetParameter("_address")->SetInOut(true);
	transfer->GetParameter("addressSpec")->SetHandler(
		new EnumTypeHandler(kAddressSpecsMap));

	Syscall *clone = get_syscall("_kern_clone_area");
	clone->GetParameter("_address")->SetInOut(true);
	clone->GetParameter("addressSpec")->SetHandler(
		new EnumTypeHandler(kAddressSpecsMap));
	clone->GetParameter("protection")->SetHandler(
		new FlagsTypeHandler(kAreaProtectionFlags));

	Syscall *reserve_address_range = get_syscall("_kern_reserve_address_range");
	reserve_address_range->GetParameter("_address")->SetInOut(true);
	reserve_address_range->GetParameter("addressSpec")->SetHandler(
		new EnumTypeHandler(kAddressSpecsMap));

	Syscall *set_area_protection = get_syscall("_kern_set_area_protection");
	set_area_protection->GetParameter("newProtection")->SetHandler(
		new FlagsTypeHandler(kAreaProtectionFlags));

	Syscall *map_file = get_syscall("_kern_map_file");
	map_file->GetParameter("address")->SetInOut(true);
	map_file->GetParameter("addressSpec")->SetHandler(
		new EnumTypeHandler(kAddressSpecsMap));
	map_file->GetParameter("protection")->SetHandler(
		new FlagsTypeHandler(kAreaProtectionFlags));
	map_file->GetParameter("mapping")->SetHandler(
		new EnumTypeHandler(kAreaMappingMap));

	Syscall *set_memory_protection = get_syscall("_kern_set_memory_protection");
	set_memory_protection->GetParameter("protection")->SetHandler(
		new FlagsTypeHandler(kAreaProtectionFlags));
}
