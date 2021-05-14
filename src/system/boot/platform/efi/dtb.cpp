/*
 * Copyright 2019-2020 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *   Alexander von Gluck IV <kallisti5@unixzen.com>
 */


#include <boot/addr_range.h>
#include <boot/platform.h>
#include <boot/stage2.h>
#include <kernel/kernel.h>

#include <ByteOrder.h>

#include "efi_platform.h"


#define INFO(x...) dprintf("efi/fdt: " x)
#define ERROR(x...) dprintf("efi/fdt: " x)


static void* sDtbTable = NULL;
static uint32 sDtbSize = 0;


static bool
fdt_valid(void* fdt, uint32* size)
{
	if (fdt == NULL)
		return false;
	uint32* words = (uint32*)fdt;
	if (B_BENDIAN_TO_HOST_INT32(words[0]) != 0xd00dfeed)
		return false;
	*size = B_BENDIAN_TO_HOST_INT32(words[1]);
	if (size == 0)
		return false;

	return true;
}


void
dtb_init()
{
	efi_configuration_table *table = kSystemTable->ConfigurationTable;
	size_t entries = kSystemTable->NumberOfTableEntries;

	INFO("Probing for device trees from UEFI...\n");

	// Try to find an FDT
	for (uint32 i = 0; i < entries; i++) {
		if (!table[i].VendorGuid.equals(DEVICE_TREE_GUID))
			continue;

		void* dtbPtr = (void*)(table[i].VendorTable);

		uint32 fdtSize = 0;
		if (!fdt_valid(dtbPtr, &fdtSize)) {
			ERROR("Invalid FDT from UEFI table %d\n", i);
			break;
		} else {
			INFO("Valid FDT from UEFI table %d (%d)\n", i, fdtSize);

			sDtbTable = dtbPtr;
			sDtbSize = fdtSize;
			break;
		}
	}
}


void
dtb_set_kernel_args()
{
	// pack into proper location if the architecture cares
	if (sDtbTable != NULL) {
		#ifdef __ARM__
		gKernelArgs.arch_args.fdt = kernel_args_malloc(sDtbSize);
		if (gKernelArgs.arch_args.fdt != NULL)
			memcpy(gKernelArgs.arch_args.fdt, sDtbTable, sDtbSize);
		else
			ERROR("unable to malloc for fdt!\n");
		#endif
	}
}
