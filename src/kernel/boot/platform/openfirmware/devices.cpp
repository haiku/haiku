/*
** Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "Handle.h"
#include "openfirmware.h"
#include "machine.h"

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

	int length = of_getprop(gChosen, "bootpath", sBootPath, sizeof(sBootPath));
	if (length <= 1)
		return B_ENTRY_NOT_FOUND;
	printf("boot path = \"%s\"\n", sBootPath);

	int node = of_finddevice(sBootPath);
	if (node != OF_FAILED) {
		char type[16];
		of_getprop(node, "device_type", type, sizeof(type));
		printf("boot type = %s\n", type);
		if (strcmp("block", type)) {
			printf("boot device is not a block device!\n");
			return B_ENTRY_NOT_FOUND;
		}
	} else
		printf("could not open boot path.\n");

/*	char name[256];
	strcpy(name, sBootPath);
	strcat(name, ":kernel_ppc");
	int kernel = of_open(name);
	if (kernel == OF_FAILED) {
		puts("open kernel failed");
	} else
		puts("open kernel succeeded");
*/
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
platform_get_boot_partition(struct stage2_args *args, Node *device,
	NodeList *list, boot::Partition **_partition)
{
	NodeIterator iterator = list->Iterator();
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
	status_t status;
	while ((status = get_next_device(&cookie, "block", path, sizeof(path))) == B_OK) {
		if (!strcmp(path, sBootPath)) {
			// don't add the boot device twice
			continue;
		}

		// Adjust the arguments passed to the open command so that
		// the disk-label package is by-passed - unfortunately,
		// this is implementation specific (and I found no docs
		// for the Apple OF disk-label usage, of course)

		// SUN's OpenBoot:
		//strcpy(path + strlen(path), ":nolabel");
		// Apple:
		if (gMachine & MACHINE_MAC)
			strcpy(path + strlen(path), ":0");

		printf("\t%s\n", path);

		int handle = of_open(path);
		if (handle == OF_FAILED) {
			puts("\t\t(failed)");
			continue;
		}

		Handle *device = new Handle(handle);
		printf("\t\t(could open device, handle = %p, node = %p)\n", (void *)handle, device);

		devicesList->Add(device);
	}
	printf("\t(loop ended with %ld)\n", status);

	return B_OK;
}

