/*
 * Copyright 2003-2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "menu.h"
#include "loader.h"
#include "load_driver_settings.h"

#include <boot/stage2.h>
#include <boot/vfs.h>
#include <boot/platform.h>
#include <boot/heap.h>
#include <boot/stdio.h>

#include <util/kernel_cpp.h>


//#define TRACE_MAIN
#ifdef TRACE_MAIN
#	define TRACE(x) dprintf x
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

	// set debug syslog default
#if KDEBUG_ENABLE_DEBUG_SYSLOG
	gKernelArgs.keep_debug_output_buffer = true;
#endif

	// construct boot_volume KMessage explicitely
	new(&gKernelArgs.boot_volume) KMessage;

	add_stage2_driver_settings(args);

	platform_init_video();

	// the main platform dependent initialisation
	// has already taken place at this point.

	if (vfs_init(args) < B_OK)
		panic("Could not initialize VFS!\n");

	dprintf("Welcome to the Haiku boot loader!\n");

	bool mountedAllVolumes = false;

	BootVolume bootVolume;

	if (get_boot_file_system(args, bootVolume) != B_OK
		|| (platform_boot_options() & BOOT_OPTION_MENU) != 0) {
		if (!bootVolume.IsValid())
			puts("\tno boot path found, scan for all partitions...\n");

		if (mount_file_systems(args) < B_OK) {
			// That's unfortunate, but we still give the user the possibility
			// to insert a CD-ROM or just rescan the available devices
			puts("Could not locate any supported boot devices!\n");
		}

		// ToDo: check if there is only one bootable volume!

		mountedAllVolumes = true;

		if (user_menu(bootVolume) < B_OK) {
			// user requested to quit the loader
			goto out;
		}
	}

	if (bootVolume.IsValid()) {
		// we got a volume to boot from!
		status_t status;
		while ((status = load_kernel(args, bootVolume)) < B_OK) {
			// loading the kernel failed, so let the user choose another
			// volume to boot from until it works
			bootVolume.Unset();

			if (!mountedAllVolumes) {
				// mount all other file systems, if not already happened
				if (mount_file_systems(args) < B_OK)
					panic("Could not locate any supported boot devices!\n");

				mountedAllVolumes = true;
			}

			if (user_menu(bootVolume) < B_OK || !bootVolume.IsValid()) {
				// user requested to quit the loader
				goto out;
			}
		}

		// if everything is okay, continue booting; the kernel
		// is already loaded at this point and we definitely
		// know our boot volume, too
		if (status == B_OK) {
			register_boot_file_system(bootVolume.RootDirectory());

			if ((platform_boot_options() & BOOT_OPTION_DEBUG_OUTPUT) == 0)
				platform_switch_to_logo();

			load_modules(args, bootVolume);
			load_driver_settings(args, bootVolume.RootDirectory());

			// apply boot settings
			apply_boot_settings();

			// set up kernel args version info
			gKernelArgs.kernel_args_size = sizeof(kernel_args);
			gKernelArgs.version = CURRENT_KERNEL_ARGS_VERSION;

			// clone the boot_volume KMessage into kernel accessible memory
			// note, that we need to 4 byte align the buffer and thus allocate
			// 3 more bytes
			KMessage& bootVolume = gKernelArgs.boot_volume;
			void* buffer = kernel_args_malloc(bootVolume.ContentSize() + 3);
			if (!buffer) {
				panic("Could not allocate memory for the boot volume kernel "
					"arguments");
			}

			buffer = (void*)(((addr_t)buffer + 3) & ~(addr_t)0x3);
			memcpy(buffer, bootVolume.Buffer(), bootVolume.ContentSize());
			bootVolume.SetTo(buffer, bootVolume.ContentSize());

			// ToDo: cleanup, heap_release() etc.
			platform_start_kernel();
		}
	}

out:
	heap_release(args);
	return 0;
}
