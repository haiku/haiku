/*
 * Copyright 2003-2006, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2010, Andreas Färber <andreas.faerber@web.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <string.h>

#include <boot/platform.h>
#include <boot/vfs.h>
#include <boot/stdio.h>
#include <boot/stage2.h>
#include <boot/net/IP.h>
#include <boot/net/iSCSITarget.h>
#include <boot/net/NetStack.h>
#include <boot/net/RemoteDisk.h>
//#include <platform/cfe/devices.h>
#include <boot/platform/cfe/cfe.h>
#include <util/kernel_cpp.h>

#include "Handle.h"


char sBootPath[192];


status_t 
platform_add_boot_device(struct stage2_args *args, NodeList *devicesList)
{
#warning PPC:TODO
	return B_ERROR;
}


status_t
platform_get_boot_partition(struct stage2_args *args, Node *device,
	NodeList *list, boot::Partition **_partition)
{
	NodeIterator iterator = list->GetIterator();
	boot::Partition *partition = NULL;
	while ((partition = (boot::Partition *)iterator.Next()) != NULL) {
		// ToDo: just take the first partition for now
		*_partition = partition;
		return B_OK;
	}

	return B_ENTRY_NOT_FOUND;
}


#define DUMPED_BLOCK_SIZE 16

void
dumpBlock(const char *buffer, int size, const char *prefix)
{
	int i;
	
	for (i = 0; i < size;) {
		int start = i;

		printf(prefix);
		for (; i < start+DUMPED_BLOCK_SIZE; i++) {
			if (!(i % 4))
				printf(" ");

			if (i >= size)
				printf("  ");
			else
				printf("%02x", *(unsigned char *)(buffer + i));
		}
		printf("  ");

		for (i = start; i < start + DUMPED_BLOCK_SIZE; i++) {
			if (i < size) {
				char c = buffer[i];

				if (c < 30)
					printf(".");
				else
					printf("%c", c);
			} else
				break;
		}
		printf("\n");
	}
}


status_t
platform_add_block_devices(stage2_args *args, NodeList *devicesList)
{
	// add all block devices to the list of possible boot devices

	int cookie = 0;
	char path[256];
	int status;
	for (; (status = cfe_enumdev(cookie, path, sizeof(path))) == B_OK; 				cookie++) {
		if (!strcmp(path, sBootPath)) {
			// don't add the boot device twice
			continue;
		}

		printf("\t%s\n", path);

		int handle = cfe_open(path);
		if (handle < CFE_OK) {
			puts("\t\t(failed)");
			continue;
		}

		Handle *device = new(nothrow) Handle(handle);
		printf("\t\t(could open device, handle = %d, node = %p)\n",
			handle, device);

		devicesList->Add(device);
	}
	printf("\t(loop ended with %ld)\n", status);

	return B_OK;
}


status_t 
platform_register_boot_device(Node *device)
{
	disk_identifier disk;

	disk.bus_type = UNKNOWN_BUS;
	disk.device_type = UNKNOWN_DEVICE;
	disk.device.unknown.size = device->Size();

	gBootVolume.SetData(BOOT_VOLUME_DISK_IDENTIFIER, B_RAW_TYPE, &disk,
		sizeof(disk_identifier));

	return B_OK;
}		

