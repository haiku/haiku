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

/* These are mainly here to allow testing and this needs to be revisited */
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
	status_t (*init_hardware)(void);
	const char **(*publish_devices)(void) = NULL;
	device_hooks *(*find_device)(const char *);
	void (*bootstrap)();
	char path[SYS_MAX_PATH_LEN];
	const char **devfs_paths;
	device_hooks *hooks;
		
	sprintf(path, "%s/%s", dirpath, name);

	id = elf_load_kspace(path, "");
	if(id < 0)
		return id;
	
	init_hardware = (void*)elf_lookup_symbol(id, "init_hardware");
	if (init_hardware) {
		dprintf("DEV: found init_hardware in %s\n", name);
		if (init_hardware() != 0) {
			dprintf("DEV: %s: init_hardware failed :(\n", name);
			elf_unload_kspace(path);
			return ENXIO;
		}
//		dprintf("DEV: Well alright - %s init'd OK!\n", name);
		publish_devices = (void*)elf_lookup_symbol(id, "publish_devices");
//		dprintf("DEV: %s: found publish_devices\n", name);	
		devfs_paths = publish_devices();
//		dprintf("DEV: %s: publish_devices = %p\n", name, (char*)devfs_paths);
//		for (; *devfs_paths; devfs_paths++) {
//			dprintf("DEV: %s: wants to be known as %s\n", name, *devfs_paths);
//		}
		find_device = (void*)elf_lookup_symbol(id, "find_device");
		if (!find_device) {
			elf_unload_kspace(path);
			dprintf("failed to find the find_device symbol!\n");
			return EINVAL; /* should be EFTYPE */
		}
		/* Should really cycle through these... */
		devfs_paths = publish_devices();
		for (; *devfs_paths; devfs_paths++) {
			hooks = find_device(*devfs_paths);
			dprintf("publishing %s\n", *devfs_paths);
			if (hooks)
				devfs_publish_device(*devfs_paths, NULL, hooks);
		}
	} else
		return EINVAL; /* should be FTYPE */
		
	return id;
}
