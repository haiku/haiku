/*
 * Copyright 2018, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include "VirtioBalloonPrivate.h"

#include <new>
#include <stdlib.h>
#include <string.h>


#define VIRTIO_BALLOON_CONTROLLER_PRETTY_NAME "Virtio Balloon Device"

#define VIRTIO_BALLOON_DRIVER_MODULE_NAME "drivers/misc/virtio_balloon/driver_v1"
#define VIRTIO_BALLOON_DEVICE_MODULE_NAME "drivers/misc/virtio_balloon/device_v1"


extern device_manager_info *gDeviceManager;


//	#pragma mark - Device module interface


static status_t
virtio_balloon_init_device(device_node *node, void **_cookie)
{
	CALLED();

	VirtioBalloonDevice *device =  new(std::nothrow)
		VirtioBalloonDevice(node);
	if (device == NULL)
		return B_NO_MEMORY;
	status_t status = device->InitCheck();
	if (status < B_OK) {
		delete device;
		return status;
	}

	*_cookie = device;
	return B_OK;
}


static void
virtio_balloon_uninit_device(void *cookie)
{
	CALLED();
	VirtioBalloonDevice *device = (VirtioBalloonDevice*)cookie;

	delete device;
}


//	#pragma mark -	Driver module interface


static float
virtio_balloon_supports_device(device_node *parent)
{
	const char *bus;
	uint16 deviceType;

	// make sure parent is really the Virtio bus manager
	if (gDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false))
		return -1;

	if (strcmp(bus, "virtio"))
		return 0.0;

	// check whether it's really a Virtio Entropy Device
	if (gDeviceManager->get_attr_uint16(parent, VIRTIO_DEVICE_TYPE_ITEM,
			&deviceType, true) != B_OK || deviceType != VIRTIO_DEVICE_ID_BALLOON)
		return 0.0;

	TRACE("Virtio Balloon device found!\n");

	return 0.6f;
}


static status_t
virtio_balloon_register_device(device_node *parent)
{
	CALLED();

	device_attr attrs[] = {
		{ NULL }
	};

	return gDeviceManager->register_node(parent, VIRTIO_BALLOON_DRIVER_MODULE_NAME,
		attrs, NULL, NULL);
}


static status_t
virtio_balloon_init_driver(device_node *node, void **_cookie)
{
	CALLED();
	*_cookie = node;
	return B_OK;
}


static status_t
virtio_balloon_register_child_devices(void *cookie)
{
	CALLED();
	device_node *node = (device_node *)cookie;

	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE,
			{ string: VIRTIO_BALLOON_CONTROLLER_PRETTY_NAME }},
		{ NULL }
	};

	return gDeviceManager->register_node(node,
		VIRTIO_BALLOON_DEVICE_MODULE_NAME, attrs, NULL, NULL);
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


driver_module_info sVirtioBalloonDeviceInterface = {
	{
		VIRTIO_BALLOON_DEVICE_MODULE_NAME,
		0,
		std_ops
	},
	NULL,	// supported devices
	NULL,	// register node
	virtio_balloon_init_device,
	virtio_balloon_uninit_device,
	NULL,	// register child devices
	NULL,	// rescan
	NULL	// bus_removed
};


driver_module_info sVirtioBalloonDriver = {
	{
		VIRTIO_BALLOON_DRIVER_MODULE_NAME,
		0,
		std_ops
	},
	virtio_balloon_supports_device,
	virtio_balloon_register_device,
	virtio_balloon_init_driver,
	NULL,	// uninit_driver,
	virtio_balloon_register_child_devices,
	NULL,	// rescan
	NULL,	// device_removed
};

