/*
 * Copyright 2013, Jérôme Duval, korli@users.berlios.de.
 * Distributed under the terms of the MIT License.
 */


#include <virtio.h>

#include "virtio_blk.h"


class DMAResource;
class IOScheduler;


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


#define VIRTIO_BLOCK_DRIVER_MODULE_NAME "drivers/disk/virtual/virtio_block/driver_v1"
#define VIRTIO_BLOCK_DEVICE_MODULE_NAME "drivers/disk/virtual/virtio_block/device_v1"
#define VIRTIO_BLOCK_DEVICE_ID_GENERATOR	"virtio_block/device_id"


typedef struct {
	device_node*			node;
	::virtio_device			virtio_device;
	virtio_device_interface*	virtio;
	::virtio_queue			virtio_queue;
	IOScheduler*			io_scheduler;
	DMAResource*			dma_resource;

	struct virtio_blk_config	config;

	uint32 					features;
	uint64					capacity;
	uint32					block_size;
	status_t				media_status;

	sem_id 	sem_cb;
} virtio_block_driver_info;


typedef struct {
	virtio_block_driver_info*		info;
} virtio_block_handle;


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <fs/devfs.h>

#include "dma_resources.h"
#include "IORequest.h"
#include "IOSchedulerSimple.h"


//#define TRACE_VIRTIO_BLOCK
#ifdef TRACE_VIRTIO_BLOCK
#	define TRACE(x...) dprintf("virtio_block: " x)
#else
#	define TRACE(x...) ;
#endif
#define ERROR(x...)			dprintf("\33[33mvirtio_block:\33[0m " x)
#define CALLED() 			TRACE("CALLED %s\n", __PRETTY_FUNCTION__)


static device_manager_info* sDeviceManager;


void virtio_block_set_capacity(virtio_block_driver_info* info, uint64 capacity,
	uint32 blockSize);


const char *
get_feature_name(uint32 feature)
{
	switch (feature) {
		case VIRTIO_BLK_F_BARRIER:
			return "host barrier";
		case VIRTIO_BLK_F_SIZE_MAX:
			return "maximum segment size";
		case VIRTIO_BLK_F_SEG_MAX:
			return "maximum segment count";
		case VIRTIO_BLK_F_GEOMETRY:
			return "disk geometry";
		case VIRTIO_BLK_F_RO:
			return "read only";
		case VIRTIO_BLK_F_BLK_SIZE:
			return "block size";
		case VIRTIO_BLK_F_SCSI:
			return "scsi commands";
		case VIRTIO_BLK_F_FLUSH:
			return "flush command";
		case VIRTIO_BLK_F_TOPOLOGY:
			return "topology";
	}
	return NULL;
}


static status_t
get_geometry(virtio_block_handle* handle, device_geometry* geometry)
{
	virtio_block_driver_info* info = handle->info;

	devfs_compute_geometry_size(geometry, info->capacity, info->block_size);

	geometry->device_type = B_DISK;
	geometry->removable = false;

	geometry->read_only = ((info->features & VIRTIO_BLK_F_RO) != 0);
	geometry->write_once = false;

	TRACE("virtio_block: get_geometry(): %ld, %ld, %ld, %ld, %d, %d, %d, %d\n",
		geometry->bytes_per_sector, geometry->sectors_per_track,
		geometry->cylinder_count, geometry->head_count, geometry->device_type,
		geometry->removable, geometry->read_only, geometry->write_once);

	return B_OK;
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


static void
virtio_block_config_callback(void* driverCookie)
{
	virtio_block_driver_info* info = (virtio_block_driver_info*)driverCookie;

	status_t status = info->virtio->read_device_config(info->virtio_device, 0,
		&info->config, sizeof(struct virtio_blk_config));
	if (status != B_OK)
		return;

	uint32 block_size = 512;
	if ((info->features & VIRTIO_BLK_F_BLK_SIZE) != 0)
		block_size = info->config.blk_size;
	uint64 capacity = info->config.capacity * 512 / block_size;

	if (block_size != info->block_size || capacity != info->capacity) {
		virtio_block_set_capacity(info, capacity, block_size);
		info->media_status = B_DEV_MEDIA_CHANGED;
	}

}


static void
virtio_block_callback(void* driverCookie, void* cookie)
{
	virtio_block_driver_info* info = (virtio_block_driver_info*)cookie;

	// consume all queued elements
	while (info->virtio->queue_dequeue(info->virtio_queue, NULL, NULL))
		;

	release_sem_etc(info->sem_cb, 1, B_DO_NOT_RESCHEDULE);
}


static status_t
do_io(void* cookie, IOOperation* operation)
{
	virtio_block_driver_info* info = (virtio_block_driver_info*)cookie;

	size_t bytesTransferred = 0;
	status_t status = B_OK;

	physical_entry entries[operation->VecCount() + 2];

	void *buffer = malloc(sizeof(struct virtio_blk_outhdr) + sizeof(uint8));
	struct virtio_blk_outhdr *header = (struct virtio_blk_outhdr*)buffer;
	header->type = operation->IsWrite() ? VIRTIO_BLK_T_OUT : VIRTIO_BLK_T_IN;
	header->sector = operation->Offset() / 512;
	header->ioprio = 1;

	uint8* ack = (uint8*)buffer + sizeof(struct virtio_blk_outhdr);
	*ack = 0xff;

	get_memory_map(buffer, sizeof(struct virtio_blk_outhdr) + sizeof(uint8),
		&entries[0], 1);
	entries[operation->VecCount() + 1].address = entries[0].address
		+ sizeof(struct virtio_blk_outhdr);
	entries[operation->VecCount() + 1].size = sizeof(uint8);
	entries[0].size = sizeof(struct virtio_blk_outhdr);

	memcpy(entries + 1, operation->Vecs(), operation->VecCount()
		* sizeof(physical_entry));

	info->virtio->queue_request_v(info->virtio_queue, entries,
		1 + (operation->IsWrite() ? operation->VecCount() : 0 ),
		1 + (operation->IsWrite() ? 0 : operation->VecCount()),
		info);

	acquire_sem(info->sem_cb);

	switch (*ack) {
		case VIRTIO_BLK_S_OK:
			status = B_OK;
			bytesTransferred = operation->Length();
			break;
		case VIRTIO_BLK_S_UNSUPP:
			status = ENOTSUP;
			break;
		default:
			status = EIO;
			break;
	}
	free(buffer);

	info->io_scheduler->OperationCompleted(operation, status,
		bytesTransferred);
	return status;
}


//	#pragma mark - device module API


static status_t
virtio_block_init_device(void* _info, void** _cookie)
{
	CALLED();
	virtio_block_driver_info* info = (virtio_block_driver_info*)_info;

	device_node* parent = sDeviceManager->get_parent_node(info->node);
	sDeviceManager->get_driver(parent, (driver_module_info **)&info->virtio,
		(void **)&info->virtio_device);
	sDeviceManager->put_node(parent);

	info->virtio->negotiate_features(info->virtio_device,
		VIRTIO_BLK_F_BARRIER | VIRTIO_BLK_F_SIZE_MAX
			| VIRTIO_BLK_F_SEG_MAX | VIRTIO_BLK_F_GEOMETRY
			| VIRTIO_BLK_F_RO | VIRTIO_BLK_F_BLK_SIZE
			| VIRTIO_BLK_F_FLUSH | VIRTIO_FEATURE_RING_INDIRECT_DESC,
		&info->features, &get_feature_name);

	status_t status = info->virtio->read_device_config(
		info->virtio_device, 0, &info->config,
		sizeof(struct virtio_blk_config));
	if (status != B_OK)
		return status;

	// and get (initial) capacity
	uint32 block_size = 512;
	if ((info->features & VIRTIO_BLK_F_BLK_SIZE) != 0)
		block_size = info->config.blk_size;
	uint64 capacity = info->config.capacity * 512 / block_size;

	virtio_block_set_capacity(info, capacity, block_size);

	TRACE("virtio_block: capacity: %" B_PRIu64 ", block_size %" B_PRIu32 "\n",
		info->capacity, info->block_size);

	status = info->virtio->alloc_queues(info->virtio_device, 1,
		&info->virtio_queue);
	if (status != B_OK) {
		ERROR("queue allocation failed (%s)\n", strerror(status));
		return status;
	}
	status = info->virtio->setup_interrupt(info->virtio_device,
		virtio_block_config_callback, info);

	if (status == B_OK) {
		status = info->virtio->queue_setup_interrupt(info->virtio_queue,
			virtio_block_callback, info);
	}

	*_cookie = info;
	return status;
}


static void
virtio_block_uninit_device(void* _cookie)
{
	CALLED();
	virtio_block_driver_info* info = (virtio_block_driver_info*)_cookie;

	delete info->io_scheduler;
	delete info->dma_resource;
}


static status_t
virtio_block_open(void* _info, const char* path, int openMode, void** _cookie)
{
	CALLED();
	virtio_block_driver_info* info = (virtio_block_driver_info*)_info;

	virtio_block_handle* handle = (virtio_block_handle*)malloc(
		sizeof(virtio_block_handle));
	if (handle == NULL)
		return B_NO_MEMORY;

	handle->info = info;

	*_cookie = handle;
	return B_OK;
}


static status_t
virtio_block_close(void* cookie)
{
	//virtio_block_handle* handle = (virtio_block_handle*)cookie;
	CALLED();

	return B_OK;
}


static status_t
virtio_block_free(void* cookie)
{
	CALLED();
	virtio_block_handle* handle = (virtio_block_handle*)cookie;

	free(handle);
	return B_OK;
}


static status_t
virtio_block_read(void* cookie, off_t pos, void* buffer, size_t* _length)
{
	CALLED();
	virtio_block_handle* handle = (virtio_block_handle*)cookie;
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
		dprintf("read(): request.Wait() returned: %s\n", strerror(status));

	return status;
}


static status_t
virtio_block_write(void* cookie, off_t pos, const void* buffer,
	size_t* _length)
{
	CALLED();
	virtio_block_handle* handle = (virtio_block_handle*)cookie;
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
		dprintf("write(): request.Wait() returned: %s\n", strerror(status));

	return status;
}


static status_t
virtio_block_io(void *cookie, io_request *request)
{
	CALLED();
	virtio_block_handle* handle = (virtio_block_handle*)cookie;

	return handle->info->io_scheduler->ScheduleRequest(request);
}


static status_t
virtio_block_ioctl(void* cookie, uint32 op, void* buffer, size_t length)
{
	CALLED();
	virtio_block_handle* handle = (virtio_block_handle*)cookie;
	virtio_block_driver_info* info = handle->info;

	TRACE("ioctl(op = %ld)\n", op);

	switch (op) {
		case B_GET_MEDIA_STATUS:
		{
			*(status_t *)buffer = info->media_status;
			info->media_status = B_OK;
			TRACE("B_GET_MEDIA_STATUS: 0x%08lx\n", *(status_t *)buffer);
			return B_OK;
			break;
		}

		case B_GET_DEVICE_SIZE:
		{
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


void
virtio_block_set_capacity(virtio_block_driver_info* info, uint64 capacity,
	uint32 blockSize)
{
	TRACE("set_capacity(device = %p, capacity = %Ld, blockSize = %ld)\n",
		info, capacity, blockSize);

	// get log2, if possible
	uint32 blockShift = log2(blockSize);

	if ((1UL << blockShift) != blockSize)
		blockShift = 0;

	info->capacity = capacity;

	if (info->block_size != blockSize) {
		if (info->block_size != 0) {
			ERROR("old %" B_PRId32 ", new %" B_PRId32 "\n", info->block_size,
				blockSize);
			panic("updating DMAResource not yet implemented...");
		}

		dma_restrictions restrictions;
		memset(&restrictions, 0, sizeof(restrictions));
		if ((info->features & VIRTIO_BLK_F_SIZE_MAX) != 0)
			restrictions.max_segment_size = info->config.size_max;
		if ((info->features & VIRTIO_BLK_F_SEG_MAX) != 0)
			restrictions.max_segment_count = info->config.seg_max;

		// TODO: we need to replace the DMAResource in our IOScheduler
		status_t status = info->dma_resource->Init(restrictions, blockSize,
			1024, 32);
		if (status != B_OK)
			panic("initializing DMAResource failed: %s", strerror(status));

		info->io_scheduler = new(std::nothrow) IOSchedulerSimple(
			info->dma_resource);
		if (info->io_scheduler == NULL)
			panic("allocating IOScheduler failed.");

		// TODO: use whole device name here
		status = info->io_scheduler->Init("virtio");
		if (status != B_OK)
			panic("initializing IOScheduler failed: %s", strerror(status));

		info->io_scheduler->SetCallback(do_io, info);
	}

	info->block_size = blockSize;
}


//	#pragma mark - driver module API


static float
virtio_block_supports_device(device_node *parent)
{
	CALLED();
	const char *bus;
	uint16 deviceType;

	// make sure parent is really the Virtio bus manager
	if (sDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false))
		return -1;

	if (strcmp(bus, "virtio"))
		return 0.0;

	// check whether it's really a Direct Access Device
	if (sDeviceManager->get_attr_uint16(parent, VIRTIO_DEVICE_TYPE_ITEM,
			&deviceType, true) != B_OK || deviceType != VIRTIO_DEVICE_ID_BLOCK)
		return 0.0;

	TRACE("Virtio block device found!\n");

	return 0.6;
}


static status_t
virtio_block_register_device(device_node *node)
{
	CALLED();

	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {string: "Virtio Block"} },
		{ NULL }
	};

	return sDeviceManager->register_node(node, VIRTIO_BLOCK_DRIVER_MODULE_NAME,
		attrs, NULL, NULL);
}


static status_t
virtio_block_init_driver(device_node *node, void **cookie)
{
	CALLED();

	virtio_block_driver_info* info = (virtio_block_driver_info*)malloc(
		sizeof(virtio_block_driver_info));
	if (info == NULL)
		return B_NO_MEMORY;

	memset(info, 0, sizeof(*info));

	info->media_status = B_OK;
	info->dma_resource = new(std::nothrow) DMAResource;
	if (info->dma_resource == NULL) {
		free(info);
		return B_NO_MEMORY;
	}

	info->sem_cb = create_sem(0, "virtio_block_cb");
	if (info->sem_cb < 0) {
		delete info->dma_resource;
		status_t status = info->sem_cb;
		free(info);
		return status;
	}
	info->node = node;

	*cookie = info;
	return B_OK;
}


static void
virtio_block_uninit_driver(void *_cookie)
{
	CALLED();
	virtio_block_driver_info* info = (virtio_block_driver_info*)_cookie;
	delete_sem(info->sem_cb);
	free(info);
}


static status_t
virtio_block_register_child_devices(void* _cookie)
{
	CALLED();
	virtio_block_driver_info* info = (virtio_block_driver_info*)_cookie;
	status_t status;

	int32 id = sDeviceManager->create_id(VIRTIO_BLOCK_DEVICE_ID_GENERATOR);
	if (id < 0)
		return id;

	char name[64];
	snprintf(name, sizeof(name), "disk/virtual/virtio_block/%" B_PRId32 "/raw",
		id);

	status = sDeviceManager->publish_device(info->node, name,
		VIRTIO_BLOCK_DEVICE_MODULE_NAME);

	return status;
}


//	#pragma mark -


module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&sDeviceManager },
	{ NULL }
};

struct device_module_info sVirtioBlockDevice = {
	{
		VIRTIO_BLOCK_DEVICE_MODULE_NAME,
		0,
		NULL
	},

	virtio_block_init_device,
	virtio_block_uninit_device,
	NULL, // remove,

	virtio_block_open,
	virtio_block_close,
	virtio_block_free,
	virtio_block_read,
	virtio_block_write,
	virtio_block_io,
	virtio_block_ioctl,

	NULL,	// select
	NULL,	// deselect
};

struct driver_module_info sVirtioBlockDriver = {
	{
		VIRTIO_BLOCK_DRIVER_MODULE_NAME,
		0,
		NULL
	},

	virtio_block_supports_device,
	virtio_block_register_device,
	virtio_block_init_driver,
	virtio_block_uninit_driver,
	virtio_block_register_child_devices,
	NULL,	// rescan
	NULL,	// removed
};

module_info* modules[] = {
	(module_info*)&sVirtioBlockDriver,
	(module_info*)&sVirtioBlockDevice,
	NULL
};
