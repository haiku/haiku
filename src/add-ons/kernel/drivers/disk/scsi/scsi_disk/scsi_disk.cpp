/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 * Distributed under the terms of the MIT License.
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


//#define TRACE_SCSI_DISK
#ifdef TRACE_SCSI_DISK
#	define TRACE(x...) dprintf("scsi_disk: " x)
#else
#	define TRACE(x...) ;
#endif


static scsi_periph_interface *sSCSIPeripheral;
static device_manager_info *sDeviceManager;
static block_io_for_driver_interface *sBlockIO;


static status_t
update_capacity(das_device_info *device)
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
get_geometry(das_handle_info *handle, device_geometry *geometry)
{
	das_device_info *device = handle->device;

	status_t status = update_capacity(device);
	if (status < B_OK)
		return status;

	geometry->bytes_per_sector = device->block_size;
	geometry->sectors_per_track = 1;
	geometry->cylinder_count = device->capacity;
	geometry->head_count = 1;
	geometry->device_type = B_DISK;
	geometry->removable = device->removable;

	// TBD: for all but CD-ROMs, read mode sense - medium type
	// (bit 7 of block device specific parameter for Optical Memory Block Device)
	// (same for Direct-Access Block Devices)
	// (same for write-once block devices)
	// (same for optical memory block devices)
	geometry->read_only = false;
	geometry->write_once = false;

	TRACE("scsi_disk: get_geometry(): %ld, %ld, %ld, %ld, %d, %d, %d, %d\n",
		geometry->bytes_per_sector, geometry->sectors_per_track,
		geometry->cylinder_count, geometry->head_count, geometry->device_type,
		geometry->removable, geometry->read_only, geometry->write_once);

	return B_OK;
}


static status_t
load_eject(das_device_info *device, bool load)
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
synchronize_cache(das_device_info *device)
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


//	#pragma mark - block_io API


static void
das_set_device(das_device_info *info, block_io_device device)
{
	info->block_io_device = device;

	// and get (initial) capacity
	scsi_ccb *request = info->scsi->alloc_ccb(info->scsi_device);
	if (request == NULL)
		return;

	sSCSIPeripheral->check_capacity(info->scsi_periph_device, request);
	info->scsi->free_ccb(request);
}


static status_t
das_open(das_device_info *device, das_handle_info **_cookie)
{
	TRACE("open()\n");

	das_handle_info *handle = (das_handle_info *)malloc(sizeof(*handle));
	if (handle == NULL)
		return B_NO_MEMORY;

	handle->device = device;

	status_t status = sSCSIPeripheral->handle_open(device->scsi_periph_device,
		(periph_handle_cookie)handle, &handle->scsi_periph_handle);
	if (status < B_OK) {
		free(handle);
		return status;
	}

	*_cookie = handle;
	return B_OK;
}


static status_t
das_close(das_handle_info *handle)
{
	TRACE("close()\n");

	sSCSIPeripheral->handle_close(handle->scsi_periph_handle);
	return B_OK;
}


static status_t
das_free(das_handle_info *handle)
{
	TRACE("free()\n");

	sSCSIPeripheral->handle_free(handle->scsi_periph_handle);
	free(handle);
	return B_OK;
}


static status_t
das_read(das_handle_info *handle, const phys_vecs *vecs, off_t pos,
	size_t num_blocks, uint32 block_size, size_t *bytes_transferred)
{
	return sSCSIPeripheral->read(handle->scsi_periph_handle, vecs, pos,
		num_blocks, block_size, bytes_transferred, 10);
}


static status_t
das_write(das_handle_info *handle, const phys_vecs *vecs, off_t pos,
	size_t num_blocks, uint32 block_size, size_t *bytes_transferred)
{
	return sSCSIPeripheral->write(handle->scsi_periph_handle, vecs, pos,
		num_blocks, block_size, bytes_transferred, 10);
}


static status_t
das_ioctl(das_handle_info *handle, int op, void *buffer, size_t length)
{
	das_device_info *device = handle->device;

	TRACE("ioctl(op = %d)\n", op);

	switch (op) {
		case B_GET_DEVICE_SIZE:
		{
			status_t status = update_capacity(device);
			if (status != B_OK)
				return status;

			size_t size = device->capacity * device->block_size;
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

		case B_GET_ICON:
			return sSCSIPeripheral->get_icon(device->removable
				? icon_type_floppy : icon_type_disk, (device_icon *)buffer);

		case B_EJECT_DEVICE:
		case B_SCSI_EJECT:
			return load_eject(device, false);

		case B_LOAD_MEDIA:
			return load_eject(device, true);

		case B_FLUSH_DRIVE_CACHE:
			return synchronize_cache(device);

		default:
			return sSCSIPeripheral->ioctl(handle->scsi_periph_handle, op,
				buffer, length);
	}
}


//	#pragma mark - scsi_periph callbacks


static void
das_set_capacity(das_device_info *device, uint64 capacity, uint32 blockSize)
{
	TRACE("das_set_capacity(device = %p, capacity = %Ld, blockSize = %ld)\n",
		device, capacity, blockSize);

	// get log2, if possible
	uint32 blockShift = log2(blockSize);

	if ((1UL << blockShift) != blockSize)
		blockShift = 0;

	device->capacity = capacity;
	device->block_size = blockSize;

	sBlockIO->set_media_params(device->block_io_device, blockSize,
		blockShift, capacity);
}


static void
das_media_changed(das_device_info *device, scsi_ccb *request)
{
	// do a capacity check
	// TBD: is this a good idea (e.g. if this is an empty CD)?
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
	uint8 deviceType;
	size_t inquiryLength;
	uint32 maxBlocks;

	// check whether it's really a Direct Access Device
	if (sDeviceManager->get_attr_uint8(node, SCSI_DEVICE_TYPE_ITEM,
			&deviceType, true) != B_OK
		|| deviceType != scsi_dev_direct_access)
		return B_ERROR;

	// get inquiry data
	if (sDeviceManager->get_attr_raw(node, SCSI_DEVICE_INQUIRY_ITEM,
			(const void **)&deviceInquiry, &inquiryLength, true) != B_OK
		|| inquiryLength < sizeof(deviceInquiry))
		return B_ERROR;

	// get block limit of underlying hardware to lower it (if necessary)
	if (sDeviceManager->get_attr_uint32(node, B_BLOCK_DEVICE_MAX_BLOCKS_ITEM,
			&maxBlocks, true) != B_OK)
		maxBlocks = INT_MAX;

	// using 10 byte commands, at most 0xffff blocks can be transmitted at once
	// (sadly, we cannot update this value later on if only 6 byte commands
	//  are supported, but the block_io module can live with that)
	maxBlocks = min_c(maxBlocks, 0xffff);

	// ready to register
	device_attr attrs[] = {
		// tell block_io whether the device is removable
		{"removable", B_UINT8_TYPE, {ui8: deviceInquiry->removable_medium}},
		// impose own max block restriction
		{B_BLOCK_DEVICE_MAX_BLOCKS_ITEM, B_UINT32_TYPE, {ui32: maxBlocks}},
		// in general, any disk can be a BIOS drive (even ZIP-disks)
		{B_BLOCK_DEVICE_IS_BIOS_DRIVE, B_UINT8_TYPE, {ui8: 1}},
		{ NULL }
	};

	return sDeviceManager->register_node(node, SCSI_DISK_MODULE_NAME, attrs,
		NULL, NULL);
}


static status_t
das_init_driver(device_node *node, void **cookie)
{
	das_device_info *device;
	status_t status;
	uint8 removable;

	TRACE("das_init_driver");

	status = sDeviceManager->get_attr_uint8(node, "removable",
		&removable, false);
	if (status != B_OK)
		return status;

	device = (das_device_info *)malloc(sizeof(*device));
	if (device == NULL)
		return B_NO_MEMORY;

	memset(device, 0, sizeof(*device));

	device->node = node;
	device->removable = removable;

	{
		device_node *parent = sDeviceManager->get_parent_node(node);
		sDeviceManager->get_driver(parent, (driver_module_info **)&device->scsi,
			(void **)&device->scsi_device);
		sDeviceManager->put_node(parent);
	}

	status = sSCSIPeripheral->register_device((periph_device_cookie)device,
		&callbacks, device->scsi_device, device->scsi, device->node,
		device->removable, &device->scsi_periph_device);
	if (status != B_OK) {
		free(device);
		return status;
	}

	*cookie = device;
	return B_OK;
}


static void
das_uninit_driver(void *_cookie)
{
	das_device_info *device = (das_device_info *)_cookie;

	sSCSIPeripheral->unregister_device(device->scsi_periph_device);
	free(device);
}


static status_t
das_register_child_devices(void *_cookie)
{
	das_device_info *device = (das_device_info *)_cookie;
	status_t status;
	char *name;

	name = sSCSIPeripheral->compose_device_name(device->node, "disk/scsi");
	if (name == NULL)
		return B_ERROR;

	status = sDeviceManager->publish_device(device->node, name,
		B_BLOCK_IO_DEVICE_MODULE_NAME);

	free(name);
	return status;
}


module_dependency module_dependencies[] = {
	{SCSI_PERIPH_MODULE_NAME, (module_info **)&sSCSIPeripheral},
	{B_BLOCK_IO_FOR_DRIVER_MODULE_NAME, (module_info **)&sBlockIO},
	{B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&sDeviceManager},
	{}
};

block_device_interface sSCSIDiskModule = {
	{
		{
			SCSI_DISK_MODULE_NAME,
			0,
			NULL
		},

		das_supports_device,
		das_register_device,
		das_init_driver,
		das_uninit_driver,
		das_register_child_devices,
		NULL,	// rescan
		NULL,	// removed
	},

	(void (*)(block_device_cookie *, block_io_device))	&das_set_device,
	(status_t (*)(block_device_cookie *, block_device_handle_cookie **))&das_open,
	(status_t (*)(block_device_handle_cookie *))			&das_close,
	(status_t (*)(block_device_handle_cookie *))			&das_free,

	(status_t (*)(block_device_handle_cookie *, const phys_vecs *,
		off_t, size_t, uint32, size_t *))				&das_read,
	(status_t (*)(block_device_handle_cookie *, const phys_vecs *,
		off_t, size_t, uint32, size_t *))				&das_write,

	(status_t (*)(block_device_handle_cookie *, int, void *, size_t))&das_ioctl,
};

module_info *modules[] = {
	(module_info *)&sSCSIDiskModule,
	NULL
};
