/*
 * Copyright 2021 David Sebek, dasebek@gmail.com
 * Copyright 2008-2013 Axel Dörfler, axeld@pinc-software.de
 * Copyright 2002/03 Thomas Kurschel
 * All rights reserved. Distributed under the terms of the MIT License.
 */


/*!	Peripheral driver to handle any kind of SCSI disks,
	i.e. hard disk and floopy disks (ZIP etc.)

	Much work is done by scsi_periph and block_io.

	You'll find das_... all over the place. This stands for
	"Direct Access Storage" which is the official SCSI name for
	normal (floppy/hard/ZIP)-disk drives.
*/


#include "scsi_disk.h"

#include <string.h>
#include <stdlib.h>

#include <AutoDeleter.h>

#include <fs/devfs.h>
#include <util/fs_trim_support.h>

#include "dma_resources.h"
#include "IORequest.h"
#include "IOSchedulerSimple.h"


//#define TRACE_SCSI_DISK
#ifdef TRACE_SCSI_DISK
#	define TRACE(x...) dprintf("scsi_disk: " x)
#else
#	define TRACE(x...) ;
#endif


static const uint8 kDriveIcon[] = {
	0x6e, 0x63, 0x69, 0x66, 0x08, 0x03, 0x01, 0x00, 0x00, 0x02, 0x00, 0x16,
	0x02, 0x3c, 0xc7, 0xee, 0x38, 0x9b, 0xc0, 0xba, 0x16, 0x57, 0x3e, 0x39,
	0xb0, 0x49, 0x77, 0xc8, 0x42, 0xad, 0xc7, 0x00, 0xff, 0xff, 0xd3, 0x02,
	0x00, 0x06, 0x02, 0x3c, 0x96, 0x32, 0x3a, 0x4d, 0x3f, 0xba, 0xfc, 0x01,
	0x3d, 0x5a, 0x97, 0x4b, 0x57, 0xa5, 0x49, 0x84, 0x4d, 0x00, 0x47, 0x47,
	0x47, 0xff, 0xa5, 0xa0, 0xa0, 0x02, 0x00, 0x16, 0x02, 0xbc, 0x59, 0x2f,
	0xbb, 0x29, 0xa7, 0x3c, 0x0c, 0xe4, 0xbd, 0x0b, 0x7c, 0x48, 0x92, 0xc0,
	0x4b, 0x79, 0x66, 0x00, 0x7d, 0xff, 0xd4, 0x02, 0x00, 0x06, 0x02, 0x38,
	0xdb, 0xb4, 0x39, 0x97, 0x33, 0xbc, 0x4a, 0x33, 0x3b, 0xa5, 0x42, 0x48,
	0x6e, 0x66, 0x49, 0xee, 0x7b, 0x00, 0x59, 0x67, 0x56, 0xff, 0xeb, 0xb2,
	0xb2, 0x03, 0xa7, 0xff, 0x00, 0x03, 0xff, 0x00, 0x00, 0x04, 0x01, 0x80,
	0x07, 0x0a, 0x06, 0x22, 0x3c, 0x22, 0x49, 0x44, 0x5b, 0x5a, 0x3e, 0x5a,
	0x31, 0x39, 0x25, 0x0a, 0x04, 0x22, 0x3c, 0x44, 0x4b, 0x5a, 0x31, 0x39,
	0x25, 0x0a, 0x04, 0x44, 0x4b, 0x44, 0x5b, 0x5a, 0x3e, 0x5a, 0x31, 0x0a,
	0x04, 0x22, 0x3c, 0x22, 0x49, 0x44, 0x5b, 0x44, 0x4b, 0x08, 0x02, 0x27,
	0x43, 0xb8, 0x14, 0xc1, 0xf1, 0x08, 0x02, 0x26, 0x43, 0x29, 0x44, 0x0a,
	0x05, 0x44, 0x5d, 0x49, 0x5d, 0x60, 0x3e, 0x5a, 0x3b, 0x5b, 0x3f, 0x08,
	0x0a, 0x07, 0x01, 0x06, 0x00, 0x0a, 0x00, 0x01, 0x00, 0x10, 0x01, 0x17,
	0x84, 0x00, 0x04, 0x0a, 0x01, 0x01, 0x01, 0x00, 0x0a, 0x02, 0x01, 0x02,
	0x00, 0x0a, 0x03, 0x01, 0x03, 0x00, 0x0a, 0x04, 0x01, 0x04, 0x10, 0x01,
	0x17, 0x85, 0x20, 0x04, 0x0a, 0x06, 0x01, 0x05, 0x30, 0x24, 0xb3, 0x99,
	0x01, 0x17, 0x82, 0x00, 0x04, 0x0a, 0x05, 0x01, 0x05, 0x30, 0x20, 0xb2,
	0xe6, 0x01, 0x17, 0x82, 0x00, 0x04
};


static scsi_periph_interface* sSCSIPeripheral;
static device_manager_info* sDeviceManager;


static status_t
update_capacity(das_driver_info* device)
{
	TRACE("update_capacity()\n");

	scsi_ccb *ccb = device->scsi->alloc_ccb(device->scsi_device);
	if (ccb == NULL)
		return B_NO_MEMORY;

	status_t status = sSCSIPeripheral->check_capacity(
		device->scsi_periph_device, ccb);

	device->scsi->free_ccb(ccb);

	return status;
}


static status_t
get_geometry(das_handle* handle, device_geometry* geometry)
{
	das_driver_info* info = handle->info;

	status_t status = update_capacity(info);
	if (status != B_OK)
		return status;

	devfs_compute_geometry_size(geometry, info->capacity, info->block_size);

	geometry->device_type = B_DISK;
	geometry->removable = info->removable;

	// TBD: for all but CD-ROMs, read mode sense - medium type
	// (bit 7 of block device specific parameter for Optical Memory Block Device)
	// (same for Direct-Access Block Devices)
	// (same for write-once block devices)
	// (same for optical memory block devices)
	geometry->read_only = false;
	geometry->write_once = false;

	TRACE("scsi_disk: get_geometry(): %" B_PRId32 ", %" B_PRId32 ", %" B_PRId32
		", %" B_PRId32 ", %d, %d, %d, %d\n", geometry->bytes_per_sector,
		geometry->sectors_per_track, geometry->cylinder_count,
		geometry->head_count, geometry->device_type,
		geometry->removable, geometry->read_only, geometry->write_once);

	return B_OK;
}


static status_t
load_eject(das_driver_info *device, bool load)
{
	TRACE("load_eject()\n");

	scsi_ccb *ccb = device->scsi->alloc_ccb(device->scsi_device);
	if (ccb == NULL)
		return B_NO_MEMORY;

	err_res result = sSCSIPeripheral->send_start_stop(
		device->scsi_periph_device, ccb, load, true);

	device->scsi->free_ccb(ccb);

	return result.error_code;
}


static status_t
synchronize_cache(das_driver_info *device)
{
	TRACE("synchronize_cache()\n");

	scsi_ccb *ccb = device->scsi->alloc_ccb(device->scsi_device);
	if (ccb == NULL)
		return B_NO_MEMORY;

	err_res result = sSCSIPeripheral->synchronize_cache(
		device->scsi_periph_device, ccb);

	device->scsi->free_ccb(ccb);

	return result.error_code;
}


static status_t
trim_device(das_driver_info* device, fs_trim_data* trimData)
{
	TRACE("trim_device()\n");

	trimData->trimmed_size = 0;

	scsi_ccb* request = device->scsi->alloc_ccb(device->scsi_device);
	if (request == NULL)
		return B_NO_MEMORY;

	scsi_block_range* blockRanges = (scsi_block_range*)
		malloc(trimData->range_count * sizeof(*blockRanges));
	if (blockRanges == NULL)
		return B_NO_MEMORY;

	MemoryDeleter deleter(blockRanges);

	for (uint32 i = 0; i < trimData->range_count; i++) {
		uint64 startBytes = trimData->ranges[i].offset;
		uint64 sizeBytes = trimData->ranges[i].size;
		uint32 blockSize = device->block_size;

		// Align to a block boundary so we don't discard blocks
		// that could also contain some other data
		uint64 blockOffset = startBytes % blockSize;
		if (blockOffset == 0) {
			blockRanges[i].lba = startBytes / blockSize;
			blockRanges[i].size = sizeBytes / blockSize;
		} else {
			blockRanges[i].lba = startBytes / blockSize + 1;
			blockRanges[i].size = (sizeBytes - (blockSize - blockOffset))
				/ blockSize;
		}
	}

	// Check ranges against device capacity and make them fit
	for (uint32 i = 0; i < trimData->range_count; i++) {
		if (blockRanges[i].lba >= device->capacity) {
			dprintf("trim_device(): range offset (LBA) %" B_PRIu64
				" exceeds device capacity %" B_PRIu64 "\n",
				blockRanges[i].lba, device->capacity);
			return B_BAD_VALUE;
		}
		uint64 maxSize = device->capacity - blockRanges[i].lba;
		blockRanges[i].size = min_c(blockRanges[i].size, maxSize);
	}

	uint64 trimmedBlocks;
	status_t status = sSCSIPeripheral->trim_device(device->scsi_periph_device,
		request, blockRanges, trimData->range_count, &trimmedBlocks);

	device->scsi->free_ccb(request);
	// Some blocks may have been trimmed even if trim_device returns a failure
	trimData->trimmed_size = trimmedBlocks * device->block_size;

	return status;
}


static int
log2(uint32 x)
{
	int y;

	for (y = 31; y >= 0; --y) {
		if (x == ((uint32)1 << y))
			break;
	}

	return y;
}


static status_t
do_io(void* cookie, IOOperation* operation)
{
	das_driver_info* info = (das_driver_info*)cookie;

	// TODO: this can go away as soon as we pushed the IOOperation to the upper
	// layers - we can then set scsi_periph::io() as callback for the scheduler
	size_t bytesTransferred;
	status_t status = sSCSIPeripheral->io(info->scsi_periph_device, operation,
		&bytesTransferred);

	info->io_scheduler->OperationCompleted(operation, status, bytesTransferred);
	return status;
}


//	#pragma mark - device module API


static status_t
das_init_device(void* _info, void** _cookie)
{
	das_driver_info* info = (das_driver_info*)_info;

	// and get (initial) capacity
	scsi_ccb *request = info->scsi->alloc_ccb(info->scsi_device);
	if (request == NULL)
		return B_NO_MEMORY;

	sSCSIPeripheral->check_capacity(info->scsi_periph_device, request);
	info->scsi->free_ccb(request);

	*_cookie = info;
	return B_OK;
}


static void
das_uninit_device(void* _cookie)
{
	das_driver_info* info = (das_driver_info*)_cookie;

	delete info->io_scheduler;
}


static status_t
das_open(void* _info, const char* path, int openMode, void** _cookie)
{
	das_driver_info* info = (das_driver_info*)_info;

	das_handle* handle = (das_handle*)malloc(sizeof(das_handle));
	if (handle == NULL)
		return B_NO_MEMORY;

	handle->info = info;

	status_t status = sSCSIPeripheral->handle_open(info->scsi_periph_device,
		(periph_handle_cookie)handle, &handle->scsi_periph_handle);
	if (status < B_OK) {
		free(handle);
		return status;
	}

	*_cookie = handle;
	return B_OK;
}


static status_t
das_close(void* cookie)
{
	das_handle* handle = (das_handle*)cookie;
	TRACE("close()\n");

	sSCSIPeripheral->handle_close(handle->scsi_periph_handle);
	return B_OK;
}


static status_t
das_free(void* cookie)
{
	das_handle* handle = (das_handle*)cookie;
	TRACE("free()\n");

	sSCSIPeripheral->handle_free(handle->scsi_periph_handle);
	free(handle);
	return B_OK;
}


static status_t
das_read(void* cookie, off_t pos, void* buffer, size_t* _length)
{
	das_handle* handle = (das_handle*)cookie;
	size_t length = *_length;

	IORequest request;
	status_t status = request.Init(pos, (addr_t)buffer, length, false, 0);
	if (status != B_OK)
		return status;

	status = handle->info->io_scheduler->ScheduleRequest(&request);
	if (status != B_OK)
		return status;

	status = request.Wait(0, 0);
	if (status == B_OK)
		*_length = length;
	else
		dprintf("das_read(): request.Wait() returned: %s\n", strerror(status));

	return status;
}


static status_t
das_write(void* cookie, off_t pos, const void* buffer, size_t* _length)
{
	das_handle* handle = (das_handle*)cookie;
	size_t length = *_length;

	IORequest request;
	status_t status = request.Init(pos, (addr_t)buffer, length, true, 0);
	if (status != B_OK)
		return status;

	status = handle->info->io_scheduler->ScheduleRequest(&request);
	if (status != B_OK)
		return status;

	status = request.Wait(0, 0);
	if (status == B_OK)
		*_length = length;
	else
		dprintf("das_write(): request.Wait() returned: %s\n", strerror(status));

	return status;
}


static status_t
das_io(void *cookie, io_request *request)
{
	das_handle* handle = (das_handle*)cookie;

	return handle->info->io_scheduler->ScheduleRequest(request);
}


static status_t
das_ioctl(void* cookie, uint32 op, void* buffer, size_t length)
{
	das_handle* handle = (das_handle*)cookie;
	das_driver_info* info = handle->info;

	TRACE("ioctl(op = %" B_PRIu32 ")\n", op);

	switch (op) {
		case B_GET_DEVICE_SIZE:
		{
			status_t status = update_capacity(info);
			if (status != B_OK)
				return status;

			size_t size = info->capacity * info->block_size;
			return user_memcpy(buffer, &size, sizeof(size_t));
		}

		case B_GET_GEOMETRY:
		{
			if (buffer == NULL /*|| length != sizeof(device_geometry)*/)
				return B_BAD_VALUE;

		 	device_geometry geometry;
			status_t status = get_geometry(handle, &geometry);
			if (status != B_OK)
				return status;

			return user_memcpy(buffer, &geometry, sizeof(device_geometry));
		}

		case B_GET_ICON_NAME:
			// TODO: take device type into account!
			return user_strlcpy((char*)buffer, info->removable
				? "devices/drive-removable-media" : "devices/drive-harddisk",
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

		case B_EJECT_DEVICE:
		case B_SCSI_EJECT:
			return load_eject(info, false);

		case B_LOAD_MEDIA:
			return load_eject(info, true);

		case B_FLUSH_DRIVE_CACHE:
			return synchronize_cache(info);

		case B_TRIM_DEVICE:
		{
			// We know the buffer is kernel-side because it has been
			// preprocessed in devfs
			ASSERT(IS_KERNEL_ADDRESS(buffer));
			return trim_device(info, (fs_trim_data*)buffer);
		}

		default:
			return sSCSIPeripheral->ioctl(handle->scsi_periph_handle, op,
				buffer, length);
	}
}


//	#pragma mark - scsi_periph callbacks


static void
das_set_capacity(das_driver_info* info, uint64 capacity, uint32 blockSize)
{
	TRACE("das_set_capacity(device = %p, capacity = %" B_PRIu64
		", blockSize = %" B_PRIu32 ")\n", info, capacity, blockSize);

	// get log2, if possible
	uint32 blockShift = log2(blockSize);

	if ((1UL << blockShift) != blockSize)
		blockShift = 0;

	info->capacity = capacity;

	if (info->block_size != blockSize) {
		if (info->block_size != 0) {
			dprintf("old %" B_PRId32 ", new %" B_PRId32 "\n", info->block_size,
				blockSize);
			panic("updating DMAResource not yet implemented...");
		}

		// TODO: we need to replace the DMAResource in our IOScheduler
		status_t status = info->dma_resource->Init(info->node, blockSize, 1024,
			32);
		if (status != B_OK)
			panic("initializing DMAResource failed: %s", strerror(status));

		info->io_scheduler = new(std::nothrow) IOSchedulerSimple(
			info->dma_resource);
		if (info->io_scheduler == NULL)
			panic("allocating IOScheduler failed.");

		// TODO: use whole device name here
		status = info->io_scheduler->Init("scsi");
		if (status != B_OK)
			panic("initializing IOScheduler failed: %s", strerror(status));

		info->io_scheduler->SetCallback(do_io, info);
	}

	info->block_size = blockSize;
}


static void
das_media_changed(das_driver_info *device, scsi_ccb *request)
{
	// do a capacity check
	// TODO: is this a good idea (e.g. if this is an empty CD)?
	sSCSIPeripheral->check_capacity(device->scsi_periph_device, request);
}


scsi_periph_callbacks callbacks = {
	(void (*)(periph_device_cookie, uint64, uint32))das_set_capacity,
	(void (*)(periph_device_cookie, scsi_ccb *))das_media_changed
};


//	#pragma mark - driver module API


static float
das_supports_device(device_node *parent)
{
	const char *bus;
	uint8 deviceType;

	// make sure parent is really the SCSI bus manager
	if (sDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false))
		return -1;

	if (strcmp(bus, "scsi"))
		return 0.0;

	// check whether it's really a Direct Access Device
	if (sDeviceManager->get_attr_uint8(parent, SCSI_DEVICE_TYPE_ITEM,
			&deviceType, true) != B_OK || deviceType != scsi_dev_direct_access)
		return 0.0;

	return 0.6;
}


/*!	Called whenever a new device was added to system;
	if we really support it, we create a new node that gets
	server by the block_io module
*/
static status_t
das_register_device(device_node *node)
{
	const scsi_res_inquiry *deviceInquiry = NULL;
	size_t inquiryLength;
	uint32 maxBlocks;

	// get inquiry data
	if (sDeviceManager->get_attr_raw(node, SCSI_DEVICE_INQUIRY_ITEM,
			(const void **)&deviceInquiry, &inquiryLength, true) != B_OK
		|| inquiryLength < sizeof(scsi_res_inquiry))
		return B_ERROR;

	// get block limit of underlying hardware to lower it (if necessary)
	if (sDeviceManager->get_attr_uint32(node, B_DMA_MAX_TRANSFER_BLOCKS,
			&maxBlocks, true) != B_OK)
		maxBlocks = INT_MAX;

	// using 10 byte commands, at most 0xffff blocks can be transmitted at once
	// (sadly, we cannot update this value later on if only 6 byte commands
	//  are supported, but the block_io module can live with that)
	maxBlocks = min_c(maxBlocks, 0xffff);

	// ready to register
	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, { string: "SCSI Disk" }},
		// tell block_io whether the device is removable
		{"removable", B_UINT8_TYPE, {ui8: deviceInquiry->removable_medium}},
		// impose own max block restriction
		{B_DMA_MAX_TRANSFER_BLOCKS, B_UINT32_TYPE, {ui32: maxBlocks}},
		{ NULL }
	};

	return sDeviceManager->register_node(node, SCSI_DISK_DRIVER_MODULE_NAME,
		attrs, NULL, NULL);
}


static status_t
das_init_driver(device_node *node, void **cookie)
{
	TRACE("das_init_driver");

	uint8 removable;
	status_t status = sDeviceManager->get_attr_uint8(node, "removable",
		&removable, false);
	if (status != B_OK)
		return status;

	das_driver_info* info = (das_driver_info*)malloc(sizeof(das_driver_info));
	if (info == NULL)
		return B_NO_MEMORY;

	memset(info, 0, sizeof(*info));

	info->dma_resource = new(std::nothrow) DMAResource;
	if (info->dma_resource == NULL) {
		free(info);
		return B_NO_MEMORY;
	}

	info->node = node;
	info->removable = removable;

	device_node* parent = sDeviceManager->get_parent_node(node);
	sDeviceManager->get_driver(parent, (driver_module_info **)&info->scsi,
		(void **)&info->scsi_device);
	sDeviceManager->put_node(parent);

	status = sSCSIPeripheral->register_device((periph_device_cookie)info,
		&callbacks, info->scsi_device, info->scsi, info->node,
		info->removable, 10, &info->scsi_periph_device);
	if (status != B_OK) {
		delete info->dma_resource;
		free(info);
		return status;
	}

	*cookie = info;
	return B_OK;
}


static void
das_uninit_driver(void *_cookie)
{
	das_driver_info* info = (das_driver_info*)_cookie;

	sSCSIPeripheral->unregister_device(info->scsi_periph_device);
	delete info->dma_resource;
	free(info);
}


static status_t
das_register_child_devices(void* _cookie)
{
	das_driver_info* info = (das_driver_info*)_cookie;
	status_t status;

	char* name = sSCSIPeripheral->compose_device_name(info->node,
		"disk/scsi");
	if (name == NULL)
		return B_ERROR;

	status = sDeviceManager->publish_device(info->node, name,
		SCSI_DISK_DEVICE_MODULE_NAME);

	free(name);
	return status;
}


static status_t
das_rescan_child_devices(void* _cookie)
{
	das_driver_info* info = (das_driver_info*)_cookie;
	uint64 capacity = info->capacity;
	update_capacity(info);
	if (info->capacity != capacity)
		sSCSIPeripheral->media_changed(info->scsi_periph_device);
	return B_OK;
}



module_dependency module_dependencies[] = {
	{SCSI_PERIPH_MODULE_NAME, (module_info**)&sSCSIPeripheral},
	{B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&sDeviceManager},
	{}
};

struct device_module_info sSCSIDiskDevice = {
	{
		SCSI_DISK_DEVICE_MODULE_NAME,
		0,
		NULL
	},

	das_init_device,
	das_uninit_device,
	NULL, //das_remove,

	das_open,
	das_close,
	das_free,
	das_read,
	das_write,
	das_io,
	das_ioctl,

	NULL,	// select
	NULL,	// deselect
};

struct driver_module_info sSCSIDiskDriver = {
	{
		SCSI_DISK_DRIVER_MODULE_NAME,
		0,
		NULL
	},

	das_supports_device,
	das_register_device,
	das_init_driver,
	das_uninit_driver,
	das_register_child_devices,
	das_rescan_child_devices,
	NULL,	// removed
};

module_info* modules[] = {
	(module_info*)&sSCSIDiskDriver,
	(module_info*)&sSCSIDiskDevice,
	NULL
};
