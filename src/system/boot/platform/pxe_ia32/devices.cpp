/*
 * Copyright 2003-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2006, Marcus Overhagen, marcus@overhagen.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "bios.h"
#include "pxe_undi.h"

#include <KernelExport.h>
#include <boot/platform.h>
#include <boot/vfs.h>
#include <boot/stdio.h>
#include <boot/stage2.h>
#include <boot/net/NetStack.h>
#include <boot/net/RemoteDisk.h>
#include <util/kernel_cpp.h>

#include <string.h>

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

	status_t error = net_stack_init();
	if (error != B_OK)
		return error;

	// init a remote disk, if possible
	RemoteDisk *remoteDisk = RemoteDisk::FindAnyRemoteDisk();
	if (!remoteDisk)
		return B_ENTRY_NOT_FOUND;

	devicesList->Add(remoteDisk);
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
	disk_identifier &disk = gKernelArgs.boot_disk.identifier;

	disk.bus_type = UNKNOWN_BUS;
	disk.device_type = UNKNOWN_DEVICE;
	disk.device.unknown.size = device->Size();

	return B_OK;
}		
