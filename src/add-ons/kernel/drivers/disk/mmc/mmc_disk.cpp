/*
 * Copyright 2018 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		B Krishnan Iyer, krishnaniyer97@gmail.com
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "mmc_disk.h"

#include <drivers/device_manager.h>
#include <drivers/KernelExport.h>
#include <drivers/Drivers.h>
#include <kernel/OS.h>

// #include <fs/devfs.h>

#define TRACE_MMC_DISK
#ifdef TRACE_MMC_DISK
#	define TRACE(x...) dprintf("mmc_disk: " x)
#else
#	define TRACE(x...) ;
#endif
#define ERROR(x...)			dprintf("\33[33mmc_disk:\33[0m " x)
#define CALLED() 			TRACE("CALLED %s\n", __PRETTY_FUNCTION__)

#define MMC_DISK_DRIVER_MODULE_NAME "drivers/disk/mmc/mmc_disk/driver_v1"
#define MMC_DISK_DEVICE_MODULE_NAME "drivers/disk/mmc/mmc_disk/device_v1"
#define MMC_DEVICE_ID_GENERATOR "mmc/device_id"

static device_manager_info* sDeviceManager;


static float
mmc_disk_supports_device(device_node* parent)
{
	CALLED();
	const char* bus;
	// uint16 deviceType;

	if (sDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus,
			true) != B_OK)
		return -1;

	if (strcmp(bus, "mmc") != 0) {
		TRACE("bus value %s, parent: %p\n",bus, parent);
		return 0.0;
	}
	// Used to check the device type but later we have attached an
	// attribute(mmc) to the bus. Not a compulsion to use this condition
	#if 0
	if (sDeviceManager->get_attr_uint16(parent, SDHCI_DEVICE_TYPE_ITEM,
			&deviceType, true) != B_OK)
	{
		TRACE(" Inavlid device type, bus found: %s and attr val %d\n",
				bus, deviceType);
		return 0.0;
	}
	#endif
	TRACE("sdhci device found, parent: %p\n", parent);

	return 0.8;
}


static status_t
mmc_disk_register_device(device_node* node)
{
	CALLED();

	device_attr attrs[] = {
		{ NULL }
	};

	return sDeviceManager->register_node(node
		, MMC_DISK_DRIVER_MODULE_NAME, attrs, NULL, NULL);
}


static status_t
mmc_disk_init_driver(device_node* node, void** cookie)
{
	CALLED();
	mmc_disk_driver_info* info = (mmc_disk_driver_info*)malloc(
		sizeof(mmc_disk_driver_info));

	if (info == NULL)
		return B_NO_MEMORY;

	memset(info, 0, sizeof(*info));

	info->node = node;

	*cookie = info;
	return B_OK;
}


static void
mmc_disk_uninit_driver(void* _cookie)
{
	CALLED();
	mmc_disk_driver_info* info = (mmc_disk_driver_info*)_cookie;
	free(info);
}


static status_t
mmc_disk_register_child_devices(void* _cookie)
{
	CALLED();
	mmc_disk_driver_info* info = (mmc_disk_driver_info*)_cookie;
	status_t status;

	int32 id = sDeviceManager->create_id(MMC_DEVICE_ID_GENERATOR);
	if (id < 0)
		return id;

	char name[64];
	snprintf(name, sizeof(name), "disk/mmc/%" B_PRId32 "/raw", id);

	status = sDeviceManager->publish_device(info->node, name,
		MMC_DISK_DEVICE_MODULE_NAME);

	return status;
}


module_dependency module_dependencies[] = {
	{B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&sDeviceManager},
	{}
};


struct driver_module_info sMMCDiskDriver = {
	{
		MMC_DISK_DRIVER_MODULE_NAME,
		0,
		NULL
	},
	mmc_disk_supports_device,
	mmc_disk_register_device,
	mmc_disk_init_driver,
	mmc_disk_uninit_driver,
	mmc_disk_register_child_devices,
	NULL, // mmc_disk_rescan_child_devices,
	NULL,
};


module_info* modules[] = {
	(module_info*)&sMMCDiskDriver,
	NULL
};
