/*
** Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "menu.h"
#include "loader.h"

#include <boot/stage2.h>
#include <boot/vfs.h>
#include <boot/platform.h>
#include <boot/heap.h>
#include <boot/stdio.h>

#include <util/kernel_cpp.h>


//#define TRACE_MAIN
#ifdef TRACE_MAIN
#	define TRACE(x) printf x
#else
#	define TRACE(x) ;
#endif


extern "C" int
main(stage2_args *args)
{
	TRACE(("boot(): enter\n"));

	if (heap_init(args) < B_OK)
		panic("Could not initialize heap!\n");

	TRACE(("boot(): heap initialized...\n"));

	// the main platform dependent initialisation
	// has already taken place at this point.

	if (vfs_init(args) < B_OK)
		panic("Could not initialize VFS!\n");

	puts("Welcome to the OpenBeOS boot loader!");

	bool mountedAllVolumes = false;

	Directory *volume = get_boot_file_system(args);

	if (volume == NULL || platform_user_menu_requested()) {
		if (volume == NULL)
			puts("\tno boot path found, scan for all partitions...\n");

		if (mount_file_systems(args) < B_OK)
			panic("Could not locate any supported boot devices!\n");

		// ToDo: check if there is only one bootable volume!

		mountedAllVolumes = true;

		if (user_menu(&volume) < B_OK) {
			// user requested to quit the loader
			goto out;
		}
	}

	if (volume != NULL) {
		// we got a volume to boot from!
		status_t status;
		while ((status = load_kernel(args, volume)) < B_OK) {
			// loading the kernel failed, so let the user choose another
			// volume to boot from until it works
			volume = NULL;

			if (!mountedAllVolumes) {
				// mount all other file systems, if not already happened
				if (mount_file_systems(args) < B_OK)
					panic("Could not locate any supported boot devices!\n");

				mountedAllVolumes = true;
			}

			if (user_menu(&volume) < B_OK || volume == NULL) {
				// user requested to quit the loader
				goto out;
			}
		}

		// if everything is okay, continue booting; the kernel
		// is already loaded at this point and we definitely
		// know our boot volume, too
		if (status == B_OK) {
			register_boot_file_system(volume);
			load_modules(args, volume);

			// set up kernel args version info
			gKernelArgs.kernel_args_size = sizeof(kernel_args);
			gKernelArgs.version = CURRENT_KERNEL_ARGS_VERSION;

			// ToDo: cleanup, heap_release() etc.
			platform_start_kernel();
		}
	}

out:
	heap_release(args);
	return 0;
}
