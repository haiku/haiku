/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "Handle.h"
#include "openfirmware.h"

#include <boot/platform.h>
#include <boot/vfs.h>
#include <boot/stdio.h>
#include <util/kernel_cpp.h>

#include <string.h>


char sBootPath[192];


/** Gets all device types of the specified type by doing a 
 *	depth-first searchi of the OpenFirmware device tree.
 *
 *	The cookie has to be initialized to zero.
 */

static status_t
get_next_device(int *_cookie, const char *type, char *path, size_t pathSize)
{
	int node = *_cookie;

	if (node == 0)
		node = of_peer(0);

	while (true) {
		int next = of_child(node);
		if (next == OF_FAILED)
			return B_ERROR;

		if (next == 0) {
			// no child node found
			next = of_peer(node);
			if (next == OF_FAILED)
				return B_ERROR;

			while (next == 0) {
				// no peer node found, we are using the device
				// tree itself as our search stack

				next = of_parent(node);
				if (next == OF_FAILED)
					return B_ERROR;

				if (next == 0) {
					// We have searched the whole device tree
					return B_ENTRY_NOT_FOUND;
				}

				// look into the next tree
				node = next;
				next = of_peer(node);
			}
		}

		*_cookie = node = next;

		char nodeType[16];
		int length;
		if (of_getprop(node, "device_type", nodeType, sizeof(nodeType)) == OF_FAILED
			|| strcmp(nodeType, type)
			|| (length = of_package_to_path(node, path, pathSize - 1)) == OF_FAILED)
			continue;

		path[length] = '\0';
		return B_OK;
	}
}


//	#pragma mark -


status_t 
platform_get_boot_device(struct stage2_args *args, Node **_device)
{
	// print out the boot path (to be removed later?)

	of_getprop(gChosen, "bootpath", sBootPath, sizeof(sBootPath));
	printf("boot path = \"%s\"\n", sBootPath);

	int node = of_finddevice(sBootPath);
	if (node != OF_FAILED) {
		char type[16];
		of_getprop(node, "device_type", type, sizeof(type));
		printf("boot type = %s\n", type);
	} else
		printf("could not open boot path.\n");

	int handle = of_open(sBootPath);
	if (handle == OF_FAILED) {
		puts("\t\t(open failed)");
		return B_ERROR;
	}

	Handle *device = new Handle(handle);
	if (device == NULL)
		return B_NO_MEMORY;

	*_device = device;
	return B_OK;
}


status_t
platform_get_boot_partition(struct stage2_args *args, struct list *list,
	boot::Partition **_partition)
{
	boot::Partition *partition = NULL;
	while ((partition = (boot::Partition *)list_get_next_item(list, partition)) != NULL) {
		// ToDo: just take the first partition for now
		*_partition = partition;
		return B_OK;
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
platform_add_block_devices(stage2_args *args, list *devicesList)
{
	// add all block devices to the list of possible boot devices

	int cookie = 0;
	char path[256];
	status_t status;
	while ((status = get_next_device(&cookie, "block", path, sizeof(path))) == B_OK) {
		if (!strcmp(path, sBootPath)) {
			// don't add the boot device twice
			continue;
		}

		printf("\t%s\n", path);
		int handle = of_open(path);
		if (handle == OF_FAILED) {
			puts("\t\t(failed)");
			continue;
		}

		printf("\t\t(could open device, handle = %p)\n", (void *)handle);
		Handle *device = new Handle(handle);

		list_add_item(devicesList, device);
	}
	printf("\t(loop ended with %ld)\n", status);

	return B_OK;
}

