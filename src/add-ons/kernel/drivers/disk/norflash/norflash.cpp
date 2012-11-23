/*
 * Copyright 2012, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ithamar R. Adema <ithamar@upgrade-android.com>
 */


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include <drivers/device_manager.h>
#include <drivers/KernelExport.h>
#include <drivers/Drivers.h>
#include <kernel/OS.h>


//#define TRACE_NORFLASH
#ifdef TRACE_NORFLASH
#define TRACE(x...)	dprintf("nor: " x)
#else
#define TRACE(x...)
#endif


#define NORFLASH_DEVICE_MODULE_NAME	"drivers/disk/norflash/device_v1"
#define NORFLASH_DRIVER_MODULE_NAME	"drivers/disk/norflash/driver_v1"


#define NORFLASH_ADDR	0x00000000


struct nor_driver_info {
	device_node *node;
	size_t blocksize;
	size_t totalsize;

	area_id id;
	void *mapped;
};


static device_manager_info *sDeviceManager;


static status_t
nor_init_device(void *_info, void **_cookie)
{
	TRACE("init_device\n");
	nor_driver_info *info = (nor_driver_info*)_info;

	info->mapped = NULL;
	info->blocksize = 128 * 1024;
	info->totalsize = info->blocksize * 256;

	info->id = map_physical_memory("NORFlash", NORFLASH_ADDR, info->totalsize, B_ANY_KERNEL_ADDRESS, B_READ_AREA, &info->mapped);
	if (info->id < 0)
		return info->id;

	*_cookie = info;
	return B_OK;
}


static void
nor_uninit_device(void *_cookie)
{
	TRACE("uninit_device\n");
	nor_driver_info *info = (nor_driver_info*)_cookie;
	if (info)
		delete_area(info->id);	
}


static status_t
nor_open(void *deviceCookie, const char *path, int openMode,
	void **_cookie)
{
	TRACE("open(%s)\n", path);
	*_cookie = deviceCookie;
	return B_OK;
}


static status_t
nor_close(void *_cookie)
{
	TRACE("close()\n");
	return B_OK;
}


static status_t
nor_free(void *_cookie)
{
	TRACE("free()\n");
	return B_OK;
}


static status_t
nor_ioctl(void *cookie, uint32 op, void *buffer, size_t length)
{
	nor_driver_info *info = (nor_driver_info*)cookie;
	TRACE("ioctl(%ld,%lu)\n", op, length);

	switch (op) {
		case B_GET_GEOMETRY:
		{
			device_geometry *deviceGeometry = (device_geometry*)buffer;
			deviceGeometry->removable = false;
			deviceGeometry->bytes_per_sector = info->blocksize;
			deviceGeometry->sectors_per_track = info->totalsize / info->blocksize;
			deviceGeometry->cylinder_count = 1;
			deviceGeometry->head_count = 1;
			deviceGeometry->device_type = B_DISK;
			deviceGeometry->removable = false;
			deviceGeometry->read_only = true;
			deviceGeometry->write_once = false;
			return B_OK;
		}
		break;

		case B_GET_DEVICE_NAME:
			strlcpy((char*)buffer, "NORFlash", length);
			break;
	}

	return B_ERROR;
}


static status_t
nor_read(void *_cookie, off_t position, void *data, size_t *numbytes)
{
	nor_driver_info *info = (nor_driver_info*)_cookie;
	TRACE("read(%Ld,%lu)\n", position, *numbytes);

	if (position + *numbytes > info->totalsize)
		*numbytes = info->totalsize - (position + *numbytes);

	memcpy(data, info->mapped + position, *numbytes);

	return B_OK;
}


static status_t
nor_write(void *_cookie, off_t position, const void *data, size_t *numbytes)
{
	TRACE("write(%Ld,%lu)\n", position, *numbytes);
	*numbytes = 0;
	return B_ERROR;
}


static float
nor_supports_device(device_node *parent)
{
	TRACE("supports_device\n");
	return 0.6;
}


static status_t
nor_register_device(device_node *node)
{
	TRACE("register_device\n");
	// ready to register
	device_attr attrs[] = {
		{ NULL }
	};

	return sDeviceManager->register_node(node, NORFLASH_DRIVER_MODULE_NAME,
		attrs, NULL, NULL);
}


static status_t
nor_init_driver(device_node *node, void **cookie)
{
	TRACE("init_driver\n");

	nor_driver_info *info = (nor_driver_info*)malloc(sizeof(nor_driver_info));
	if (info == NULL)
		return B_NO_MEMORY;

	memset(info, 0, sizeof(*info));

	info->node = node;

	*cookie = info;
	return B_OK;
}


static void
nor_uninit_driver(void *_cookie)
{
	TRACE("uninit_driver\n");
	nor_driver_info *info = (nor_driver_info*)_cookie;
	free(info);
}


static status_t
nor_register_child_devices(void *_cookie)
{
	TRACE("register_child_devices\n");
	nor_driver_info *info = (nor_driver_info*)_cookie;
	status_t status;

	status = sDeviceManager->publish_device(info->node, "disk/nor/0/raw",
		NORFLASH_DEVICE_MODULE_NAME);

	return status;
}


struct device_module_info sNORFlashDiskDevice = {
	{
		NORFLASH_DEVICE_MODULE_NAME,
		0,
		NULL
	},

	nor_init_device,
	nor_uninit_device,
	NULL, //nor_remove,

	nor_open,
	nor_close,
	nor_free,
	nor_read,
	nor_write,
	NULL,	// nor_io,
	nor_ioctl,

	NULL,	// select
	NULL,	// deselect
};



struct driver_module_info sNORFlashDiskDriver = {
	{
		NORFLASH_DRIVER_MODULE_NAME,
		0,
		NULL
	},

	nor_supports_device,
	nor_register_device,
	nor_init_driver,
	nor_uninit_driver,
	nor_register_child_devices,
	NULL,	// rescan
	NULL,	// removed
};


module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&sDeviceManager },
	{ }
};


module_info *modules[] = {
	(module_info*)&sNORFlashDiskDriver,
	(module_info*)&sNORFlashDiskDevice,
	NULL
};
