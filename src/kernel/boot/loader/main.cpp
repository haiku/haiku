/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <boot/stage2.h>
#include <boot/vfs.h>
#include <boot/platform.h>
#include <boot/heap.h>
#include <boot/stdio.h>

#include <util/kernel_cpp.h>


extern "C" int
boot(stage2_args *args)
{
	puts("Oh yes, we are\n");
	if (heap_init(args) < B_OK)
		panic("Could not initialize heap!\n");

	puts("heap initialized...");

	// the main platform dependent initialisation
	// has already taken place at this point.

	if (vfs_init(args) < B_OK)
		panic("Could not initialize VFS!\n");

	puts("Welcome to the OpenBeOS boot loader!");

	if (mount_boot_file_systems() < B_OK)
		panic("Could not locate any supported boot devices!\n");

	heap_release();
	return 0;
}

#if 0
void *get_boot_device(stage2_args *args);
void *user_menu();
void load_boot_drivers(void *device);
void load_boot_modules(void *device);
void load_driver_settings(void *device);


void
load_kernel(void *deviceHandle)
{
	void *handle = open(deviceHandle, "system/kernel");
	if (handle == NULL)
		panic("Could not open kernel from boot device!\n");

	/* load kernel into memory and relocate it */
}


int
main(stage2_args *args)
{
	/* pre-state2 initialization is already done at this point */

	void *device = get_boot_device(args);
	if (device == NULL)
		device = user_menu();

	if (device == NULL)
		panic("No boot partition found");

	load_kernel(device);
	load_boot_drivers(device);
	load_boot_modules(device);
	load_driver_settings(device);

	start_kernel();
}
#endif

