/*
 * Copyright 2009, Johannes Wischert, johanneswi@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <KernelExport.h>
#include <boot/platform.h>
#include <boot/vfs.h>
#include <boot/stdio.h>
#include <boot/stage2.h>

#define TRACE_DEVICES
#ifdef TRACE_DEVICES
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...)
#endif




status_t 
platform_add_boot_device(struct stage2_args *args, NodeList *devicesList)
{
	TRACE("platform_add_boot_device\n");

	if (!args->platform.boot_tgz_data || !args->platform.boot_tgz_size)
		return B_DEVICE_NOT_FOUND;

	TRACE("Memory Disk at: %lx size: %lx\n", args->platform.boot_tgz_data,
		args->platform.boot_tgz_size);

	MemoryDisk* disk = new(nothrow) MemoryDisk(
		(const uint8 *)args->platform.boot_tgz_data,
		args->platform.boot_tgz_size, "boot.tgz");
	if (!disk) {
		dprintf("platform_add_boot_device(): Could not create MemoryDisk !\n");
		return B_NO_MEMORY;
	}

	devicesList->Add(disk);
	return B_OK;
}


status_t
platform_get_boot_partition(struct stage2_args *args, Node *device,
	NodeList *list, boot::Partition **_partition)
{
	TRACE("platform_get_boot_partition\n");

	NodeIterator iterator = list->GetIterator();
	boot::Partition *partition = NULL;
	while ((partition = (boot::Partition *)iterator.Next()) != NULL) {
		// ToDo: just take the first partition for now
		*_partition = partition;
		return B_OK;
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
platform_add_block_devices(stage2_args *args, NodeList *devicesList)
{
	TRACE("platform_add_block_devices\n");
	return B_OK;
}


status_t 
platform_register_boot_device(Node *device)
{
	TRACE("platform_register_boot_device\n");
	return B_OK;
}
