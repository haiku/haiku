/*
 * BeOS audio_module_driver
 *
 * Copyright (c) 2003, Marcus Overhagen <marcus@overhagen.de>
 *
 * All rights reserved.
 * Redistribution only allowed under the terms of the MIT license.
 *
 */


#include <KernelExport.h>
#include <directories.h>
#include <Drivers.h>
#include <Errors.h>
#include <OS.h>

#include <malloc.h>
#include <fcntl.h>
#include <image.h>

#include "audio_module.h"


int32 api_version = B_CUR_DRIVER_API_VERSION;

#define DRIVER_NAME 	"audio_module_driver"

#define MODULE_TEST_PATH "media/audio/ich"


void republish_devices(void);

extern image_id	load_kernel_addon(const char *path);
extern status_t	unload_kernel_addon(image_id imid);


status_t
init_hardware(void)
{
	dprintf("audio_module_driver: init_hardware\n");
	return B_OK;
}


status_t
init_driver(void)
{
	audio_module_info *audio_module;
	image_id id;
	status_t rv;
	void (*print_hello_world)(void);
	
	id = load_kernel_addon(kUserAddonsDirectory "/kernel/media/audio/ich");
	get_image_symbol(id, "print_hello_world", B_SYMBOL_TYPE_TEXT,
		(void **) &print_hello_world);
	print_hello_world();
	unload_kernel_addon(id);
	if (rv != B_OK)
		dprintf("unload_kernel_addon failed\n");
	
/*
	if (B_OK != get_module(MODULE_TEST_PATH, (module_info **)&audio_module)) {
		dprintf("get_module failed\n");
		return B_ERROR;
	}

	dprintf("calling audio_module->print_hello()\n");

	audio_module->print_hello();

	dprintf("done\n");

	if (B_OK != put_module(MODULE_TEST_PATH)) {
		dprintf("put_module failed\n");
		return B_ERROR;
	}
*/
	return B_OK;
}


void
uninit_driver(void)
{
	dprintf("audio_module_driver: uninit_driver\n");
}


static status_t
audio_module_driver_open(const char *name, uint32 flags, void **cookie)
{
	dprintf("audio_module_driver: open %s\n", name);
	return B_OK;
}


static status_t
audio_module_driver_close(void *cookie)
{
	dprintf("audio_module_driver: close\n");
	return B_OK;
}


static status_t
audio_module_driver_free(void *cookie)
{
	dprintf("audio_module_driver: free\n");
	return B_OK;
}


static status_t
audio_module_driver_control(void *cookie, uint32 op, void *arg, size_t len)
{
	dprintf("audio_module_driver: control\n");
	switch (op) {
		default:
			return B_ERROR;
	}
}


static status_t
audio_module_driver_read(void *cookie, off_t position, void *buf,
	size_t *num_bytes)
{
	*num_bytes = 0;
	return B_IO_ERROR;
}


static status_t
audio_module_driver_write(void *cookie, off_t position, const void *buffer,
	size_t *num_bytes)
{
	static int keep_open_fd = -1;
	if (*num_bytes >= 5 && 0 == memcmp("start", buffer, 5)
		&& keep_open_fd == -1) {
		keep_open_fd = open("/dev/audio/audio_module_driver", O_RDWR);
		return B_OK;
	}
	if (*num_bytes >= 4 && 0 == memcmp("stop", buffer, 4)
		&& keep_open_fd != -1) {
		close(keep_open_fd);
		keep_open_fd = -1;
		return B_OK;
	}
	if (*num_bytes >= 6 && 0 == memcmp("rescan", buffer, 6)) {
		republish_devices();
		return B_OK;
	}
	*num_bytes = 0;
	return B_IO_ERROR;
}


static const char *ich_name[] = {
	"audio/" DRIVER_NAME,
	"audio/modules/foobar/1",
	"audio/modules/foobar/2",
	"audio/modules/snafu/1",
	"audio/modules/snafu/2",
	NULL
};


device_hooks audio_module_driver_hooks = {
	audio_module_driver_open,
	audio_module_driver_close,
	audio_module_driver_free,
	audio_module_driver_control,
	audio_module_driver_read,
	audio_module_driver_write,
	NULL,
	NULL,
	NULL,
	NULL
};


const char**
publish_devices(void)
{
	dprintf("audio_module_driver: publish_devices\n");
	return ich_name;
}


device_hooks*
find_device(const char *name)
{
	dprintf("audio_module_driver: find_device %s\n", name);
	return &audio_module_driver_hooks;
}


void republish_devices(void)
{
	int fd = open("/dev", O_WRONLY);
	write(fd, DRIVER_NAME, strlen(DRIVER_NAME));
	close (fd);
}
