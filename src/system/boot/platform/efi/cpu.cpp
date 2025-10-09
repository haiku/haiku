/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <boot/kernel_args.h>
#include <boot/platform.h>
#include <boot/stage2.h>
#include <boot/vfs.h>
#include <arch/cpu.h>

#include <arch_cpu.h>

#include "efi_platform.h"


static void
get_smbios_tables()
{
	const efi_configuration_table *table = kSystemTable->ConfigurationTable;
	const size_t entries = kSystemTable->NumberOfTableEntries;
	// Try to find any SMBIOS table.
	for (uint32 i = 0; i < entries; i++) {
		void* vendorTable = table[i].VendorTable;
		if (table[i].VendorGuid.equals(SMBIOS_TABLE_GUID)) {
			gBootParams.SetInt64(BOOT_EFI_SMBIOS_V2_ROOT, (addr_t)vendorTable);
			dprintf("smbios: found v2 at %p\n", vendorTable);
			continue;
		}
		if (table[i].VendorGuid.equals(SMBIOS3_TABLE_GUID)) {
			gBootParams.SetInt64(BOOT_EFI_SMBIOS_V3_ROOT, (addr_t)vendorTable);
			dprintf("smbios: found v3 at %p\n", vendorTable);
			continue;
		}
	}
}


void
cpu_init()
{
	gKernelArgs.num_cpus = 1;
		// this will eventually be corrected later on

	boot_arch_cpu_init();
}


extern "C" void
platform_load_ucode(BootVolume& volume)
{
	arch_ucode_load(volume);

	get_smbios_tables();
}
