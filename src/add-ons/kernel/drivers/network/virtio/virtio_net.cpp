/*
 * Copyright 2013, Jérôme Duval, korli@users.berlios.de.
 * Distributed under the terms of the MIT License.
 */


#include <ethernet.h>
#include <virtio.h>

#define ETHER_ADDR_LEN	ETHER_ADDRESS_LENGTH
#include "virtio_net.h"



#define VIRTIO_NET_DRIVER_MODULE_NAME "drivers/network/virtio_net/driver_v1"
#define VIRTIO_NET_DEVICE_MODULE_NAME "drivers/network/virtio_net/device_v1"
#define VIRTIO_NET_DEVICE_ID_GENERATOR	"virtio_net/device_id"


typedef struct {
	device_node*			node;
	::virtio_device			virtio_device;
	virtio_device_interface*	virtio;

	uint32 					features;

} virtio_net_driver_info;


typedef struct {
	virtio_net_driver_info*		info;
} virtio_net_handle;


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <fs/devfs.h>


#define TRACE_VIRTIO_NET
#ifdef TRACE_VIRTIO_NET
#	define TRACE(x...) dprintf("virtio_net: " x)
#else
#	define TRACE(x...) ;
#endif
#define ERROR(x...)			dprintf("\33[33mvirtio_net:\33[0m " x)
#define CALLED() 			TRACE("CALLED %s\n", __PRETTY_FUNCTION__)


static device_manager_info* sDeviceManager;


const char *
get_feature_name(uint32 feature)
{
	switch (feature) {
	}
	return NULL;
}


//	#pragma mark - device module API


static status_t
virtio_net_init_device(void* _info, void** _cookie)
{
	CALLED();
	virtio_net_driver_info* info = (virtio_net_driver_info*)_info;

	device_node* parent = sDeviceManager->get_parent_node(info->node);
	sDeviceManager->get_driver(parent, (driver_module_info **)&info->virtio,
		(void **)&info->virtio_device);
	sDeviceManager->put_node(parent);

	info->virtio->negociate_features(info->virtio_device,
		0, &info->features, &get_feature_name);

	// TODO read config
	// TODO alloc queues and setup interrupts

	*_cookie = info;
	return B_OK;
}


static void
virtio_net_uninit_device(void* _cookie)
{
	CALLED();
	virtio_net_driver_info* info = (virtio_net_driver_info*)_cookie;
}


static status_t
virtio_net_open(void* _info, const char* path, int openMode, void** _cookie)
{
	CALLED();
	virtio_net_driver_info* info = (virtio_net_driver_info*)_info;

	virtio_net_handle* handle = (virtio_net_handle*)malloc(
		sizeof(virtio_net_handle));
	if (handle == NULL)
		return B_NO_MEMORY;

	handle->info = info;

	*_cookie = handle;
	return B_OK;
}


static status_t
virtio_net_close(void* cookie)
{
	//virtio_net_handle* handle = (virtio_net_handle*)cookie;
	CALLED();

	return B_OK;
}


static status_t
virtio_net_free(void* cookie)
{
	CALLED();
	virtio_net_handle* handle = (virtio_net_handle*)cookie;

	free(handle);
	return B_OK;
}


static status_t
virtio_net_read(void* cookie, off_t pos, void* buffer, size_t* _length)
{
	CALLED();
	virtio_net_handle* handle = (virtio_net_handle*)cookie;
	// TODO implement
	return B_ERROR;
}


static status_t
virtio_net_write(void* cookie, off_t pos, const void* buffer,
	size_t* _length)
{
	CALLED();
	virtio_net_handle* handle = (virtio_net_handle*)cookie;
	// TODO implement
	return B_ERROR;
}


static status_t
virtio_net_ioctl(void* cookie, uint32 op, void* buffer, size_t length)
{
	CALLED();
	virtio_net_handle* handle = (virtio_net_handle*)cookie;
	virtio_net_driver_info* info = handle->info;

	TRACE("ioctl(op = %ld)\n", op);

	switch (op) {
	}

	return B_DEV_INVALID_IOCTL;
}


//	#pragma mark - driver module API


static float
virtio_net_supports_device(device_node *parent)
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
			&deviceType, true) != B_OK || deviceType != VIRTIO_DEVICE_ID_NETWORK)
		return 0.0;

	TRACE("Virtio network device found!\n");

	return 0.6;
}


static status_t
virtio_net_register_device(device_node *node)
{
	CALLED();

	// ready to register
	device_attr attrs[] = {
		{ NULL }
	};

	return sDeviceManager->register_node(node, VIRTIO_NET_DRIVER_MODULE_NAME,
		attrs, NULL, NULL);
}


static status_t
virtio_net_init_driver(device_node *node, void **cookie)
{
	CALLED();

	virtio_net_driver_info* info = (virtio_net_driver_info*)malloc(
		sizeof(virtio_net_driver_info));
	if (info == NULL)
		return B_NO_MEMORY;

	memset(info, 0, sizeof(*info));

	info->node = node;

	*cookie = info;
	return B_OK;
}


static void
virtio_net_uninit_driver(void *_cookie)
{
	CALLED();
	virtio_net_driver_info* info = (virtio_net_driver_info*)_cookie;
	free(info);
}


static status_t
virtio_net_register_child_devices(void* _cookie)
{
	CALLED();
	virtio_net_driver_info* info = (virtio_net_driver_info*)_cookie;
	status_t status;

	int32 id = sDeviceManager->create_id(VIRTIO_NET_DEVICE_ID_GENERATOR);
	if (id < 0)
		return id;

	char name[64];
	snprintf(name, sizeof(name), "net/virtio/%" B_PRId32,
		id);

	status = sDeviceManager->publish_device(info->node, name,
		VIRTIO_NET_DEVICE_MODULE_NAME);

	return status;
}


//	#pragma mark -


module_dependency module_dependencies[] = {
	{B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&sDeviceManager},
	{}
};

struct device_module_info sVirtioNetDevice = {
	{
		VIRTIO_NET_DEVICE_MODULE_NAME,
		0,
		NULL
	},

	virtio_net_init_device,
	virtio_net_uninit_device,
	NULL, // remove,

	virtio_net_open,
	virtio_net_close,
	virtio_net_free,
	virtio_net_read,
	virtio_net_write,
	NULL,	// io
	virtio_net_ioctl,

	NULL,	// select
	NULL,	// deselect
};

struct driver_module_info sVirtioNetDriver = {
	{
		VIRTIO_NET_DRIVER_MODULE_NAME,
		0,
		NULL
	},

	virtio_net_supports_device,
	virtio_net_register_device,
	virtio_net_init_driver,
	virtio_net_uninit_driver,
	virtio_net_register_child_devices,
	NULL,	// rescan
	NULL,	// removed
};

module_info* modules[] = {
	(module_info*)&sVirtioNetDriver,
	(module_info*)&sVirtioNetDevice,
	NULL
};
