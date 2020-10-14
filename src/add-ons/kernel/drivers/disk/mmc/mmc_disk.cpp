/*
 * Copyright 2018-2020 Haiku, Inc. All rights reserved.
 * Copyright 2020, Viveris Technologies.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		B Krishnan Iyer, krishnaniyer97@gmail.com
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */

#include <new>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "mmc_disk.h"
#include "mmc_icon.h"
#include "mmc.h"

#include <drivers/device_manager.h>
#include <drivers/KernelExport.h>
#include <drivers/Drivers.h>
#include <kernel/OS.h>

// #include <fs/devfs.h>

#define TRACE_MMC_DISK
#ifdef TRACE_MMC_DISK
#	define TRACE(x...) dprintf("\33[33mmmc_disk:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif
#define ERROR(x...)			dprintf("\33[33mmmc_disk:\33[0m " x)
#define CALLED() 			TRACE("CALLED %s\n", __PRETTY_FUNCTION__)

#define MMC_DISK_DRIVER_MODULE_NAME "drivers/disk/mmc/mmc_disk/driver_v1"
#define MMC_DISK_DEVICE_MODULE_NAME "drivers/disk/mmc/mmc_disk/device_v1"
#define MMC_DEVICE_ID_GENERATOR "mmc/device_id"

static device_manager_info* sDeviceManager;


struct mmc_disk_csd {
	uint64 bits[2];

	uint8 structure_version() { return bits[1] >> 60; }
	uint8 read_bl_len() { return (bits[1] >> 8) & 0xF; }
	uint16 c_size()
	{
		return ((bits[0] >> 54) & 0x3FF) | ((bits[1] & 0x3) << 10);
	}
	uint8 c_size_mult() { return (bits[0] >> 39) & 0x7; }
};


static float
mmc_disk_supports_device(device_node* parent)
{
	// Filter all devices that are not on an MMC bus
	const char* bus;
	if (sDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus,
			true) != B_OK)
		return -1;

	if (strcmp(bus, "mmc") != 0)
		return 0.0;

	CALLED();

	// Filter all devices that are not of the known types
	uint8_t deviceType;
	if (sDeviceManager->get_attr_uint8(parent, kMmcTypeAttribute,
			&deviceType, true) != B_OK)
	{
		ERROR("Could not get device type\n");
		return -1;
	}

	if (deviceType == CARD_TYPE_SD)
		TRACE("SD card found, parent: %p\n", parent);
	else if (deviceType == CARD_TYPE_SDHC)
		TRACE("SDHC card found, parent: %p\n", parent);
	else
		return 0.0;

	return 0.8;
}


static status_t
mmc_disk_register_device(device_node* node)
{
	CALLED();

	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, { string: "SD Card" }},
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

	void* unused;
	info->node = node;
	info->parent = sDeviceManager->get_parent_node(info->node);
	sDeviceManager->get_driver(info->parent, (driver_module_info **)&info->mmc,
		&unused);

	TRACE("MMC bus handle: %p %s\n", info->mmc, info->mmc->info.info.name);

	if (sDeviceManager->get_attr_uint16(node, kMmcRcaAttribute, &info->rca,
			true) != B_OK) {
		TRACE("MMC card node has no RCA attribute\n");
		free(info);
		return B_BAD_DATA;
	}

	TRACE("MMC card device initialized for RCA %x\n", info->rca);

	*cookie = info;
	return B_OK;
}


static void
mmc_disk_uninit_driver(void* _cookie)
{
	CALLED();
	mmc_disk_driver_info* info = (mmc_disk_driver_info*)_cookie;
	sDeviceManager->put_node(info->parent);
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


//	#pragma mark - device module API


static status_t
mmc_block_init_device(void* _info, void** _cookie)
{
	CALLED();

	// No additional context, so just reuse the same data as the disk device
	mmc_disk_driver_info* info = (mmc_disk_driver_info*)_info;
	*_cookie = info;

	return B_OK;
}


static void
mmc_block_uninit_device(void* _cookie)
{
	CALLED();
	//mmc_disk_driver_info* info = (mmc_disk_driver_info*)_cookie;

	// TODO cleanup whatever is relevant
}


static status_t
mmc_block_open(void* _info, const char* path, int openMode, void** _cookie)
{
	CALLED();
	mmc_disk_driver_info* info = (mmc_disk_driver_info*)_info;

	// allocate cookie
	mmc_disk_handle* handle = new(std::nothrow) mmc_disk_handle;
	*_cookie = handle;
	if (handle == NULL) {
		return B_NO_MEMORY;
	}
	handle->info = info;

	return B_OK;
}


static status_t
mmc_block_close(void* cookie)
{
	mmc_disk_handle* handle = (mmc_disk_handle*)cookie;
	CALLED();

	return B_OK;
}


static status_t
mmc_block_free(void* cookie)
{
	CALLED();
	mmc_disk_handle* handle = (mmc_disk_handle*)cookie;

	delete handle;
	return B_OK;
}


static status_t 
mmc_block_read(void* cookie, off_t pos, void* buffer, size_t* _length)
{
	CALLED();
	mmc_disk_handle* handle = (mmc_disk_handle*)cookie;
	TRACE("Ready to execute %p\n", handle->info->mmc->read_naive);
	return handle->info->mmc->read_naive(handle->info->parent, handle->info->rca, pos, buffer, _length);
}


static status_t
mmc_block_write(void* cookie, off_t position, const void* buffer,
	size_t* length)
{
	CALLED();
	mmc_disk_handle* handle = (mmc_disk_handle*)cookie;

	return B_NOT_SUPPORTED;
}


static status_t
mmc_block_io(void* cookie, io_request* request)
{
	CALLED();
	mmc_disk_handle* handle = (mmc_disk_handle*)cookie;

	return B_NOT_SUPPORTED;
}


static status_t
mmc_block_get_geometry(mmc_disk_handle* handle, device_geometry* geometry)
{
	struct mmc_disk_csd csd;
	TRACE("Ready to execute %p\n", handle->info->mmc->execute_command);
	handle->info->mmc->execute_command(handle->info->parent, SD_SEND_CSD,
		handle->info->rca << 16, (uint32_t*)&csd);

	TRACE("CSD: %lx %lx\n", csd.bits[0], csd.bits[1]);

	if (csd.structure_version() == 0) {
		geometry->bytes_per_sector = 1 << csd.read_bl_len();
		geometry->sectors_per_track = csd.c_size() + 1;
		geometry->cylinder_count = 1 << (csd.c_size_mult() + 2);
		geometry->head_count = 1;
		geometry->device_type = B_DISK;
		geometry->removable = true; // TODO detect eMMC which isn't
		geometry->read_only = true; // TODO add write support
		geometry->write_once = false;
		return B_OK;
	}

	TRACE("unknown CSD version %d\n", csd.structure_version());
	return B_NOT_SUPPORTED;
}


static status_t
mmc_block_ioctl(void* cookie, uint32 op, void* buffer, size_t length)
{
	CALLED();
	mmc_disk_handle* handle = (mmc_disk_handle*)cookie;
	mmc_disk_driver_info* info = handle->info;

	TRACE("ioctl(op = %" B_PRId32 ")\n", op);

	switch (op) {
		case B_GET_MEDIA_STATUS:
		{
			if (buffer == NULL || length < sizeof(status_t))
				return B_BAD_VALUE;

			*(status_t *)buffer = B_OK;
			TRACE("B_GET_MEDIA_STATUS: 0x%08" B_PRIx32 "\n",
				*(status_t *)buffer);
			return B_OK;
			break;
		}

		case B_GET_DEVICE_SIZE:
		{
			//size_t size = info->capacity * info->block_size;
			//return user_memcpy(buffer, &size, sizeof(size_t));
			return B_NOT_SUPPORTED;
		}

		case B_GET_GEOMETRY:
		{
			if (buffer == NULL || length < sizeof(device_geometry))
				return B_BAD_VALUE;

		 	device_geometry geometry;
			status_t status = mmc_block_get_geometry(handle, &geometry);
			if (status != B_OK)
				return status;

			return user_memcpy(buffer, &geometry, sizeof(device_geometry));
		}

		case B_GET_ICON_NAME:
			return user_strlcpy((char*)buffer, "devices/drive-harddisk",
				B_FILE_NAME_LENGTH);

		case B_GET_VECTOR_ICON:
		{
			// TODO: take device type into account!
			device_icon iconData;
			if (length != sizeof(device_icon))
				return B_BAD_VALUE;
			if (user_memcpy(&iconData, buffer, sizeof(device_icon)) != B_OK)
				return B_BAD_ADDRESS;

			if (iconData.icon_size >= (int32)sizeof(kDriveIcon)) {
				if (user_memcpy(iconData.icon_data, kDriveIcon,
						sizeof(kDriveIcon)) != B_OK)
					return B_BAD_ADDRESS;
			}

			iconData.icon_size = sizeof(kDriveIcon);
			return user_memcpy(buffer, &iconData, sizeof(device_icon));
		}

		/*case B_FLUSH_DRIVE_CACHE:
			return synchronize_cache(info);*/
	}

	return B_DEV_INVALID_IOCTL;
}


module_dependency module_dependencies[] = {
	{B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&sDeviceManager},
	{}
};


// The "block device" associated with the device file. It can be open()
// multiple times, eash allocating an mmc_disk_handle. It does not interact
// with the hardware directly, instead it forwards all IO requests to the
// disk driver through the IO scheduler.
struct device_module_info sMMCBlockDevice = {
	{
		MMC_DISK_DEVICE_MODULE_NAME,
		0,
		NULL
	},

	mmc_block_init_device,
	mmc_block_uninit_device,
	NULL, // remove,

	mmc_block_open,
	mmc_block_close,
	mmc_block_free,
	mmc_block_read,
	mmc_block_write,
	mmc_block_io,
	mmc_block_ioctl,

	NULL,	// select
	NULL,	// deselect
};


// Driver for the disk devices itself. This is paired with an
// mmc_disk_driver_info instanciated once per device. Handles the actual disk
// I/O operations
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
	(module_info*)&sMMCBlockDevice,
	NULL
};
