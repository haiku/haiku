/* Contains code to load dev module and bootstrap it */

/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <kernel.h>
#include <stage2.h>
#include <dev.h>
#include <vfs.h>
#include <debug.h>
#include <elf.h>
#include <Errors.h>
#include <devfs.h>
#include <Drivers.h>
#include <memheap.h>

#ifdef ARCH_x86
#include <arch/x86/console_dev.h>
#endif

#include <fb_console.h>

#include <stdio.h>

/* TODO: move this into devfs.c, to add full dynamicly loaded device drivers support
 These are mainly here to allow testing and this needs to be revisited
*/
const char *device_paths[] = {
	"/boot/addons/drivers/dev",
	"/boot/addons/drivers/dev/audio",	
	"/boot/addons/drivers/dev/misc",
	"/boot/addons/drivers/dev/net",
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

image_id dev_load_dev_module(const char *name, const char *dirpath)
{
	image_id id;
	char path[SYS_MAX_PATH_LEN];
	// uint32 * api_version;
	status_t (*init_hardware)(void);
	status_t (*init_driver)(void);
	status_t (*uninit_driver)(void);
	device_hooks *(*find_device)(const char *);
	const char **(*publish_devices)(void);
	const char **devfs_paths;
	bool keep_loaded;
		
	sprintf(path, "%s/%s", dirpath, name);

	id = elf_load_kspace(path, "");
	if(id < 0)
		return id;

	// get device driver API symbols
	// api_version = (uint32 *) elf_lookup_symbol(id, "api_version");
	init_hardware = (void *) elf_lookup_symbol(id, "init_hardware");
	init_driver = (void *) elf_lookup_symbol(id, "init_driver");
	uninit_driver = (void *) elf_lookup_symbol(id, "uninit_driver");
	publish_devices = (void *) elf_lookup_symbol(id, "publish_devices");
	find_device = (void *) elf_lookup_symbol(id, "find_device");

	if (publish_devices == NULL || find_device == NULL) {
		dprintf("DEV: %s: mandatory driver symbol(s) missing! :(\n", name);
		id = EINVAL;
		goto error;
	} 

	if (init_hardware && init_hardware() != 0) {
		dprintf("DEV: %s: init_hardware failed :(\n", name);
		id = ENXIO;
		goto error;
	}

	if (init_driver && init_driver() != 0) {
		dprintf("DEV: %s: init_driver failed :(\n", name);
		id = ENXIO;
		goto error;
		}
		
//	dprintf("DEV: Well alright - %s init'd OK!\n", name);

	/* Should really cycle through these... */
	devfs_paths = publish_devices();
	keep_loaded = false;
	for (; *devfs_paths; devfs_paths++) {
		device_hooks * hooks;

		hooks = find_device(*devfs_paths);
		dprintf("DEV: %s: publishing %s\n", name, *devfs_paths);
		if (hooks) {
			if (devfs_publish_device(*devfs_paths, NULL, hooks) == 0)
				keep_loaded = true;
		}
	}
	
	if (keep_loaded)
		// okay, keep this driver image in memory, as he publish *something*
		return id;
	
	// no need to keep this driver image loaded... 
	if (uninit_driver)
		uninit_driver();
	id = 0;

error:
	elf_unload_kspace(path);
	return id;
}

