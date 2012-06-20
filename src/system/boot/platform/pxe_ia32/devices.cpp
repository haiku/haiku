/*
 * Copyright 2003-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2006, Marcus Overhagen, marcus@overhagen.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "bios.h"
#include "pxe_undi.h"
#include "network.h"

#include <KernelExport.h>
#include <boot/platform.h>
#include <boot/vfs.h>
#include <boot/stdio.h>
#include <boot/stage2.h>
#include <boot/net/NetStack.h>
#include <boot/net/RemoteDisk.h>
#include <util/kernel_cpp.h>
#include <util/KMessage.h>

#include <string.h>

#define TRACE_DEVICES
#ifdef TRACE_DEVICES
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...)
#endif


//extern unsigned char* gBuiltinBootArchive;
//extern long long gBuiltinBootArchiveSize;

static TFTP sTFTP;


status_t 
platform_add_boot_device(struct stage2_args *args, NodeList *devicesList)
{
	TRACE("platform_add_boot_device\n");

	// get the boot archive containing kernel and drivers via TFTP
	status_t error = sTFTP.Init();
	if (error == B_OK) {
		uint8* data;
		size_t size;
		// The root path in the DHCP packet from the server might contain the
		// name of the archive. It would come first, then separated by semicolon
		// the actual root path.
		const char* fileName = "haiku-netboot.tgz";	// default
		char stackFileName[1024];
		const char* rootPath = sTFTP.RootPath();
		if (rootPath) {
			if (char* fileNameEnd = strchr(rootPath, ';')) {
				size_t len = min_c(fileNameEnd - rootPath,
					(int)sizeof(stackFileName) - 1);
				memcpy(stackFileName, rootPath, len);
				stackFileName[len] = '\0';
				fileName = stackFileName;
			}
		}

		// get the file
		error = sTFTP.ReceiveFile(fileName, &data, &size);
		if (error == B_OK) {
			char name[64];
			ip_addr_t serverAddress = sTFTP.ServerIPAddress();
			snprintf(name, sizeof(name), "%lu.%lu.%lu.%lu:%s",
				(serverAddress >> 24), (serverAddress >> 16) & 0xff,
				(serverAddress >> 8) & 0xff, serverAddress & 0xff, fileName);

			MemoryDisk* disk = new(nothrow) MemoryDisk(data, size, name);
			if (!disk) {
				dprintf("platform_add_boot_device(): Out of memory!\n");
				platform_free_region(data, size);
				return B_NO_MEMORY;
			}
	
			devicesList->Add(disk);
			return B_OK;
		} else {
			dprintf("platform_add_boot_device(): Failed to load file \"%s\" "
				"via TFTP\n", fileName);
		}
	}

	return B_ENTRY_NOT_FOUND;

// 	// built-in boot archive?
// 	if (gBuiltinBootArchiveSize > 0) {
// 		MemoryDisk* disk = new(nothrow) MemoryDisk(gBuiltinBootArchive,
// 			gBuiltinBootArchiveSize);
// 		if (!disk)
// 			return B_NO_MEMORY;
// 
// 		devicesList->Add(disk);
// 		return B_OK;
// 	}

// 	error = net_stack_init();
// 	if (error != B_OK)
// 		return error;
// 
// 	// init a remote disk, if possible
// 	RemoteDisk *remoteDisk = RemoteDisk::FindAnyRemoteDisk();
// 	if (!remoteDisk) {
// 		unsigned ip = NetStack::Default()->GetEthernetInterface()->IPAddress();
// 		panic("PXE boot: can't find remote disk on server %u.%u.%u.%u\n",
// 			(ip >> 24) & 0xff, (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip & 0xff);
// 		return B_ENTRY_NOT_FOUND;
// 	}
// 
// 	devicesList->Add(remoteDisk);
// 	return B_OK;
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

	// get the root path -- chop off the file name of the archive we loaded
	const char* rootPath = sTFTP.RootPath();
	if (rootPath) {
		if (char* fileNameEnd = strchr(rootPath, ';'))
			rootPath = fileNameEnd + 1;
	}

	if (gBootVolume.SetInt32(BOOT_METHOD, BOOT_METHOD_NET) != B_OK
		|| gBootVolume.AddInt64("client MAC",
			sTFTP.MACAddress().ToUInt64()) != B_OK
		|| gBootVolume.AddInt32("client IP", sTFTP.IPAddress()) != B_OK
		|| gBootVolume.AddInt32("server IP", sTFTP.ServerIPAddress()) != B_OK
		|| gBootVolume.AddInt32("server port", sTFTP.ServerPort()) != B_OK
		|| (sTFTP.RootPath()
			&& gBootVolume.AddString("net root path", rootPath)
				!= B_OK)) {
		return B_NO_MEMORY;
	}

// 	RemoteDisk *rd = static_cast<RemoteDisk *>(device);
// 	UNDI *undi = static_cast<UNDI *>(NetStack::Default()->GetEthernetInterface());
//
// 	gKernelArgs.boot_disk.identifier.bus_type = UNKNOWN_BUS;
// 	gKernelArgs.boot_disk.identifier.device_type = NETWORK_DEVICE;
// 	gKernelArgs.boot_disk.identifier.device.network.client_ip = undi->IPAddress();
// 	gKernelArgs.boot_disk.identifier.device.network.server_ip = rd->ServerIPAddress();
// 	gKernelArgs.boot_disk.identifier.device.network.server_port = rd->ServerPort();
// 	gKernelArgs.boot_disk.partition_offset = 0;
// 	gKernelArgs.boot_disk.user_selected = false;
// 	gKernelArgs.boot_disk.booted_from_image = false;
// 	gKernelArgs.boot_disk.booted_from_network = true;
// 	gKernelArgs.boot_disk.cd = false;

	return B_OK;
}
