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
#include <drivers.h>
#include <memheap.h>

#include <stdio.h>

const char *device_paths[] = {
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
dprintf("DEV: looking at directory %s\n", *ptr);
		fd = sys_open(*ptr, STREAM_TYPE_DIR, 0);
		if(fd >= 0) {
			ssize_t len;
			char buf[SYS_MAX_NAME_LEN];

			while((len = sys_read(fd, buf, 0, sizeof(buf))) > 0) {
				dprintf("loading '%s' dev module\n", buf);
				dev_load_dev_module(buf, *ptr);
			}
			sys_close(fd);
		}
	}
	
	return 0;
}

image_id dev_load_dev_module(const char *name, const char *dirpath)
{
	image_id id;
	status_t (*init_hardware)(void);
	const char **(*publish_devices)(void);
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
//		dprintf("DEV: found init_hardware in %s\n", name);
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
		for (; *devfs_paths; devfs_paths++) {
			dprintf("DEV: %s: wants to be known as %s\n", name, *devfs_paths);
		}
		find_device = (void*)elf_lookup_symbol(id, "find_device");
		if (!find_device) {
			elf_unload_kspace(path);
			return EFTYPE;
		}
		/* Should really cycle through these... */
		devfs_paths = publish_devices();
		for (; *devfs_paths; devfs_paths++) {
			hooks = find_device(*devfs_paths);
			if (hooks)
				devfs_publish_device(*devfs_paths, NULL, hooks);
		}
	} else
		return EFTYPE;
		
	return id;
}
