/* Contains code to load dev module and bootstrap it */

/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <kernel.h>
#include <boot/stage2.h>
#include <dev.h>
#include <vfs.h>
#include <debug.h>
#include <elf.h>
#include <Errors.h>
#include <devfs.h>
#include <Drivers.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef ARCH_x86
#	include <arch/x86/console_dev.h>
#endif

#include <fb_console.h>

#include <stdio.h>

/* XXX - move this into the core
 * These are mainly here to allow testing and this needs to be revisited
 */
const char *device_paths[] = {
	"/boot/beos/system/add-ons/kernel/drivers/dev",
	"/boot/beos/system/add-ons/kernel/drivers/dev/audio",
	"/boot/beos/system/add-ons/kernel/drivers/dev/misc",
	"/boot/beos/system/add-ons/kernel/drivers/dev/net",
	"/boot/drivers/dev",
	NULL
};


int
dev_init(kernel_args *ka)
{
	const char **ptr;
	
	dprintf("dev_init: entry\n");

	for (ptr = device_paths; (*ptr); ptr++) {
		DIR *dir = opendir(*ptr);
		if (dir != NULL) {
			struct dirent *dirent;

			while ((dirent = readdir(dir)) != NULL) {
				if (!strcmp(dirent->d_name, ".") || !strcmp(dirent->d_name, ".."))
					continue;

				dprintf("loading '%s' dev module\n", dirent->d_name);
				dev_load_dev_module(dirent->d_name, *ptr);
				dprintf("loaded dev module!\n");
			}
			closedir(dir);
		}
	}

	/* now run the oddballs we have, consoles at present */
	/* XXX - these are x86 and may not be applicable to all platforms */
#ifdef ARCH_x86
	console_dev_init(ka);
#endif

	fb_console_dev_init(ka);

	return 0;
}


image_id
dev_load_dev_module(const char *name, const char *dirpath)
{
	image_id image;
	char path[SYS_MAX_PATH_LEN];
	uint32 *api_version = NULL;
	status_t (*init_hardware)(void);
	status_t (*init_driver)(void);
	status_t (*uninit_driver)(void);
	device_hooks *(*find_device)(const char *);
	const char **(*publish_devices)(void);
	const char **devfs_paths;
	bool keep_loaded;
	status_t status;

	sprintf(path, "%s/%s", dirpath, name);

	/* Load the module, return error if unable */
	image = load_kernel_add_on(path);
	if (image < 0)
		return image;

	/* Don't get all the symbols we will require initially. Just get the
	 * ones we need to check we have a valid module.
	 */

	// For a valid device driver the following exports are required
	if (get_image_symbol(image, "publish_devices", B_SYMBOL_TYPE_TEXT, (void **)&publish_devices) != B_OK
		|| get_image_symbol(image, "api_version", B_SYMBOL_TYPE_DATA, (void **)&api_version) != B_OK
		|| get_image_symbol(image, "find_device", B_SYMBOL_TYPE_TEXT, (void **)&find_device) != B_OK) {
		dprintf("DEV: %s: mandatory driver symbol(s) missing! :(\n", name);
		status = EINVAL;
		goto error;
	}

	/* Next, do we have an init_hardware function? If we do run it and
	 * if it fails simply fall through to the exit...
	 */
	if (get_image_symbol(image, "init_hardware", B_SYMBOL_TYPE_TEXT,
			(void **)&init_hardware) == B_OK
		&& init_hardware() != B_OK) {
		dprintf("DEV: %s: init_hardware failed :(\n", name);
		status = ENXIO;
		goto error;
	}

	/* OK, so we now have what appears to be a valid module that has
	 * completed init_hardware and thus thinks it should be used.
	 * ToDo:
	 *  - this is bogus!
	 *  - the driver init routines should be called by devfs and 
	 *       only when the driver is first needed. However, that level
	 *       level of support is not yet in devfs, so we have a hack
	 *       here that calls the init_driver function at this point.
	 *       As a result we will check to see if we actually manage to
	 *       publish the device, and if we do we will keep the module
	 *       loaded.
	 *  - remove this when devfs is fixed!
	 */
	if (get_image_symbol(image, "init_driver", B_SYMBOL_TYPE_TEXT,
			(void **)&init_driver) == B_OK
		&& init_driver() != B_OK) {
		dprintf("DEV: %s: init_driver failed :(\n", name);
		status = ENXIO;
		goto error;
	}

	/* Start of with the keep_loaded as false. If we have a successful
	 * call to devfs_publish_device we will set this to true.
	 */
	keep_loaded = false;
	devfs_paths = publish_devices();
	for (; *devfs_paths; devfs_paths++) {
		device_hooks *hooks = find_device(*devfs_paths);
		if (hooks) {
			dprintf("DEV: %s: publishing %s with hooks %p\n", 
			        name, *devfs_paths, hooks);
			if (devfs_publish_device(*devfs_paths, NULL, hooks) == 0)
				keep_loaded = true;
		}
	}

	/* If we've managed to publish at least one of the device entry
	 * points that the driver wished us to then we MUST keep the driver in
	 * memory or the pointer that devfs has will become invalid.
	 * XXX - This is bogus, see above
	 */
	if (keep_loaded)
		return image;

	/* If the function gets here then the following has happenned...
	 * - the driver has been loaded
	 * - it has appeared valid
	 * - init_hardware has returned saying it should be used
	 * - init_driver has been run OK
	 * - publish_devices return empty paths list or
	 *   devfs_publish_device has for some reason failed on each path.
	 */

	if (get_image_symbol(image, "uninit_driver", B_SYMBOL_TYPE_TEXT,
			(void **)&uninit_driver) == B_OK)
		uninit_driver();

	// ToDo: That might not be the best error code
	status = ENXIO;

error:
	/* If we've gotten here then the driver will be unloaded and an
	 * error code returned.
	 */
	unload_kernel_add_on(image);
	return status;
}


/* This is no longer part of the public kernel API, so we just export the symbol */
status_t load_driver_symbols(const char *driver_name);
status_t
load_driver_symbols(const char *driver_name)
{
	// This will be globally done for the whole kernel via the settings file.
	// We don't have to do anything here.

	return B_OK;
}


