/*
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*
	Part of Open SCSI Disk Driver

	Main file.
	
	You'll find das_... all over the place. This stands for
	"Direct Access Storage" which is the official SCSI name for
	normal (floppy/hard/ZIP)-disk drives.
*/


#include "scsi_dsk_int.h"
#include <scsi.h>

#include <string.h>
#include <stdlib.h>


extern block_device_interface das_interface;

scsi_periph_interface *scsi_periph;
device_manager_info *pnp;
block_io_for_driver_interface *gBlockIO;


static status_t
das_read(das_handle_info *handle, const phys_vecs *vecs, off_t pos,
	size_t num_blocks, uint32 block_size, size_t *bytes_transferred)
{
	return scsi_periph->read(handle->scsi_periph_handle, vecs, pos,
		num_blocks, block_size, bytes_transferred, 10);
}


static status_t
das_write(das_handle_info *handle, const phys_vecs *vecs, off_t pos,
	size_t num_blocks, uint32 block_size, size_t *bytes_transferred)
{
	return scsi_periph->write(handle->scsi_periph_handle, vecs, pos,
		num_blocks, block_size, bytes_transferred, 10);
}


static status_t
update_capacity(das_device_info *device)
{
	scsi_ccb *ccb;
	status_t res;

	SHOW_FLOW0(3, "");

	ccb = device->scsi->alloc_ccb(device->scsi_device);
	if (ccb == NULL)
		return B_NO_MEMORY;

	res = scsi_periph->check_capacity(device->scsi_periph_device, ccb);

	device->scsi->free_ccb(ccb);

	return res;	
}


static status_t
get_geometry(das_handle_info *handle, void *buf, size_t len)
{
	das_device_info *device = handle->device;
	device_geometry *geometry = (device_geometry *)buf;
	status_t res;

	SHOW_FLOW0(3, "");

	res = update_capacity(device);

	if (res < B_OK)
		return res;

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

	SHOW_FLOW(3, "%ld, %ld, %ld, %ld, %d, %d, %d, %d",
		geometry->bytes_per_sector,
		geometry->sectors_per_track,
		geometry->cylinder_count,
		geometry->head_count,
		geometry->device_type,
		geometry->removable,
		geometry->read_only,
		geometry->write_once);
	
	SHOW_FLOW0(3, "done");
	
	return B_OK;
}


static status_t
load_eject(das_device_info *device, bool load)
{
	scsi_ccb *ccb;
	err_res res;

	SHOW_FLOW0(0, "");

	ccb = device->scsi->alloc_ccb(device->scsi_device);

	res = scsi_periph->send_start_stop(device->scsi_periph_device, 
		ccb, load, true);

	device->scsi->free_ccb(ccb);

	return res.error_code;
}


static status_t
synchronize_cache(das_device_info *device)
{
	scsi_ccb *ccb;
	err_res res;
	
	SHOW_FLOW0( 0, "" );
		
	ccb = device->scsi->alloc_ccb(device->scsi_device);

	res = scsi_periph->synchronize_cache(device->scsi_periph_device, ccb);
		
	device->scsi->free_ccb(ccb);
	
	return res.error_code;
}

static int
log2(uint32 x)
{
	int y;

	for (y = 31; y >= 0; --y)
		if (x == ((uint32)1 << y))
			break;

	return y;
}


void
das_set_capacity(das_device_info *device, uint64 capacity,
	uint32 block_size)
{
	uint32 ld_block_size;

	SHOW_FLOW(3, "device=%p, capacity=%Ld, block_size=%ld", 
		device, capacity, block_size);

	// get log2, if possible
	ld_block_size = log2(block_size);

	if ((1UL << ld_block_size) != block_size)
		ld_block_size = 0;

	device->capacity = capacity;
	device->block_size = block_size;

	gBlockIO->set_media_params(device->block_io_device, block_size, 
		ld_block_size, capacity);
}


static void
das_media_changed(das_device_info *device, scsi_ccb *request)
{
	// do a capacity check
	// TBD: is this a good idea (e.g. if this is an empty CD)?
	scsi_periph->check_capacity(device->scsi_periph_device, request);
}


scsi_periph_callbacks callbacks = {
	(void (*)(periph_device_cookie, uint64, uint32))das_set_capacity,
	(void (*)(periph_device_cookie, scsi_ccb *))das_media_changed
};


static status_t
das_ioctl(das_handle_info *handle, int op, void *buf, size_t len)
{
	das_device_info *device = handle->device;
	status_t res;

	SHOW_FLOW(4, "%d", op);

	switch (op) {
		case B_GET_DEVICE_SIZE:
			res = update_capacity(device);
			if (res == B_OK)
				*(size_t *)buf = device->capacity * device->block_size;
			break;

		case B_GET_GEOMETRY:
			res = get_geometry(handle, buf, len);
			break;

		case B_GET_ICON:
			res = scsi_periph->get_icon(device->removable ? icon_type_floppy : icon_type_disk, 
				(device_icon *)buf);
			break;

		case B_EJECT_DEVICE:
		case B_SCSI_EJECT:
			res = load_eject(device, false);
			break;

		case B_LOAD_MEDIA:
			res = load_eject(device, true);
			break;

	case B_FLUSH_DRIVE_CACHE:
		res = synchronize_cache(device);
		break;

		default:
			res = scsi_periph->ioctl(handle->scsi_periph_handle, op, buf, len);
	}

	SHOW_FLOW(4, "%s", strerror(res));

	return res;
}


static float
das_supports_device(device_node_handle parent, bool *_noConnection)
{
	char *bus;
	uint8 device_type;

	// make sure parent is really the SCSI bus manager
	if (pnp->get_attr_string(parent, B_DRIVER_BUS, &bus, false))
		return B_ERROR;

	if (strcmp(bus, "scsi")) {
		free(bus);
		return 0.0;
	}

	// check whether it's really a Direct Access Device
	if (pnp->get_attr_uint8(parent, SCSI_DEVICE_TYPE_ITEM, &device_type, true) != B_OK
		|| device_type != scsi_dev_direct_access) {
		free(bus);
		return 0.0;
	}	

	free(bus);
	return 0.6;
}


static void
das_get_paths(const char ***_bus, const char ***_device)
{
	static const char *kBus[] = { "scsi", NULL };

	*_bus = kBus;
	*_device = NULL;
}


static status_t
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;

		default:
			return B_ERROR;
	}
}


module_dependency module_dependencies[] = {
	{ SCSI_PERIPH_MODULE_NAME, (module_info **)&scsi_periph },
	{ B_BLOCK_IO_FOR_DRIVER_MODULE_NAME, (module_info **)&gBlockIO },
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&pnp },
	{}
};

block_device_interface scsi_dsk_module = {
	{
		{
			SCSI_DSK_MODULE_NAME,
			0,
			std_ops
		},

		das_supports_device,
		das_device_added,
		das_init_device,
		(status_t (*) (void *))das_uninit_device,
		NULL,	// remove device
		NULL,	// cleanup device
		das_get_paths,
	},

	(status_t (*)(block_device_device_cookie, block_device_handle_cookie *))&das_open,
	(status_t (*)(block_device_handle_cookie))					&das_close,
	(status_t (*)(block_device_handle_cookie))					&das_free,

	(status_t (*)(block_device_handle_cookie, const phys_vecs *,
		off_t, size_t, uint32, size_t *))						&das_read,
	(status_t (*)(block_device_handle_cookie, const phys_vecs *,
		off_t, size_t, uint32, size_t *))						&das_write,

	(status_t (*)(block_device_handle_cookie, int, void *, size_t))&das_ioctl,
};

#if !_BUILDING_kernel && !BOOT
_EXPORT 
module_info *modules[] = {
	(module_info *)&scsi_dsk_module,
	NULL
};
#endif
