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
#include <memheap.h>

#ifdef ARCH_x86
#	include <arch/x86/console_dev.h>
#endif

#include <fb_console.h>

#include <stdio.h>

/* XXX - move this into the core
 * These are mainly here to allow testing and this needs to be revisited
 */
const char *device_paths[] = {
	"/boot/drivers/dev",
	"/boot/drivers/dev/audio",	
	"/boot/drivers/dev/misc",
	"/boot/drivers/dev/net",
	NULL
};

int dev_init(kernel_args *ka)
{
	int fd;
	const char **ptr;
	
	dprintf("dev_init: entry\n");

	for (ptr = device_paths; (*ptr); ptr++) {
		fd = sys_open_dir(*ptr);
		if (fd >= 0) {
			ssize_t len;
			char buf[SYS_MAX_NAME_LEN + sizeof(struct dirent) + 1];
			struct dirent *dirent = (struct dirent *)buf;

			while ((len = sys_read_dir(fd, dirent, sizeof(buf), 1)) > 0) {
				dprintf("loading '%s' dev module\n", dirent->d_name);
				dev_load_dev_module(dirent->d_name, *ptr);
				dprintf("loaded dev module!\n");
			}
			sys_close(fd);
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
	image_id id;
	char path[SYS_MAX_PATH_LEN];
	uint32 *api_version;
	status_t (*init_hardware)(void);
	status_t (*init_driver)(void);
	status_t (*uninit_driver)(void);
	device_hooks *(*find_device)(const char *);
	const char **(*publish_devices)(void);
	const char **devfs_paths;
	bool keep_loaded;
		
	sprintf(path, "%s/%s", dirpath, name);

	/* Load the module, return error if unable */
	id = elf_load_kspace(path, "");
	if (id < 0)
		return id;

	/* Don't get all the symbols we will require initially. Just get the
	 * ones we need to check we have a valid module.
	 * elf_find_symbol is an extra call and so we don't make it more often
	 * than we have to.
	 */
	/* For a valid device driver we MUST have the publish_devices and 
	 * find_device functions available
	 */
	/* XXX - very few drivers have the api_version symbol defined yet.
	 *       Add api_version to the rest of the drivers!
	 */
	api_version = (uint32 *) elf_lookup_symbol(id, "api_version");
	publish_devices = (void *) elf_lookup_symbol(id, "publish_devices");
	find_device = (void *) elf_lookup_symbol(id, "find_device");

	if (publish_devices == NULL || find_device == NULL) {
		dprintf("DEV: %s: mandatory driver symbol(s) missing! :(\n", name);
		id = EINVAL;
		goto error;
	}
	/* ToDo: we should fail if the driver doesn't export the api_version,
	 * but right now, there are some drivers in our tree that doesn't...
	 */
	if (api_version == NULL)
		dprintf("driver \"%s\" has no api_version set!\n", name);

	/* Next, do we have an init_hardware function? If we do run it and
	 * if it fails simply fall through to the exit...
	 */
	init_hardware = (void *) elf_lookup_symbol(id, "init_hardware");
	if (init_hardware && init_hardware() != 0) {
		dprintf("DEV: %s: init_hardware failed :(\n", name);
		id = ENXIO;
		goto error;
	}

	/* OK, so we now have what appears to be a valid module that has
	 * completed init_hardware and thus thinks it should be used.
	 * XXX - this is bogus!
	 * XXX - the driver init routines should be called by devfs and 
	 *       only when the driver is first needed. However, that level
	 *       level of support is not yet in devfs, so we have a hack
	 *       here that calls the init_driver function at this point.
	 *       As a result we will check to see if we actually manage to
	 *       publish the device, and if we do we will keep the module
	 *       loaded.
	 * XXX - remove this when devfs is fixed!
	 */
	init_driver = (void *) elf_lookup_symbol(id, "init_driver");
	if (init_driver && init_driver() != 0) {
		dprintf("DEV: %s: init_driver failed :(\n", name);
		id = ENXIO;
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
		return id;

	/* If the function gets here then the following has happenned...
	 * - the driver has been loaded
	 * - it has appeared valid
	 * - init_hardware has returned saying it should be used
	 * - init_driver has been run OK
	 * - publish_devices return empty paths list or
	 *   devfs_publish_device has for some reason failed on each path.
	 */

	uninit_driver = (void *) elf_lookup_symbol(id, "uninit_driver");
	if (uninit_driver)
		uninit_driver();

	// ToDo: That might not be the best error code
	id = ENXIO;

error:
	/* If we've gotten here then the driver will be unloaded and an
	 * error code returned.
	 */
	elf_unload_kspace(path);
	return id;
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


