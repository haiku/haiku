/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <boot/platform.h>
#include <boot/vfs.h>
#include <boot/stdio.h>
#include <util/kernel_cpp.h>

#include "Handle.h"
#include "openfirmware.h"


status_t
platform_get_boot_devices(stage2_args *args, list *devicesList)
{
	int handle;

	if ((handle = of_finddevice("ide0")) != OF_FAILED) {
		Handle *device = new Handle(handle, false);

		printf("Found IDE device, handle = %d.\n", handle);
		list_add_item(devicesList, device);
	}
	puts("platform_get_boot_devices() end.");
	return B_OK;
}

