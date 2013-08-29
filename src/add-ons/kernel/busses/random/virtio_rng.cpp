/*
 * Copyright 2013, Jérôme Duval, korli@users.berlios.de.
 * Distributed under the terms of the MIT License.
 */


#include "VirtioRNGPrivate.h"

#include <new>
#include <stdlib.h>
#include <string.h>


#define VIRTIO_RNG_CONTROLLER_PRETTY_NAME "Virtio RNG Device"

#define VIRTIO_RNG_DRIVER_MODULE_NAME "busses/random/virtio_rng/driver_v1"
#define VIRTIO_RNG_DEVICE_MODULE_NAME "busses/random/virtio_rng/device_v1"


device_manager_info *gDeviceManager;
random_for_controller_interface *gRandom;


//	#pragma mark - Random module interface


static status_t
sim_init_bus(device_node *node, void **_cookie)
{
	CALLED();

	VirtioRNGDevice *device =  new(std::nothrow)
		VirtioRNGDevice(node);
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
sim_uninit_bus(void *cookie)
{
	CALLED();
	VirtioRNGDevice *device = (VirtioRNGDevice*)cookie;

	delete device;
}


static status_t
sim_read(void* cookie, void *_buffer, size_t *_numBytes)
{
	VirtioRNGDevice *device = (VirtioRNGDevice*)cookie;
	return device->Read(_buffer, _numBytes);
}


//	#pragma mark -	Driver module interface


static float
virtio_rng_supports_device(device_node *parent)
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
			&deviceType, true) != B_OK || deviceType != VIRTIO_DEVICE_ID_ENTROPY)
		return 0.0;

	TRACE("Virtio RNG device found!\n");

	return 0.6f;
}


static status_t
virtio_rng_register_device(device_node *parent)
{
	CALLED();

	device_attr attrs[] = {
		{ NULL }
	};

	return gDeviceManager->register_node(parent, VIRTIO_RNG_DRIVER_MODULE_NAME,
		attrs, NULL, NULL);
}


static status_t
virtio_rng_init_driver(device_node *node, void **_cookie)
{
	CALLED();
	*_cookie = node;
	return B_OK;
}


static status_t
virtio_rng_register_child_devices(void *cookie)
{
	CALLED();
	device_node *node = (device_node *)cookie;

	device_attr attrs[] = {
		{ B_DEVICE_FIXED_CHILD, B_STRING_TYPE,
			{ string: RANDOM_FOR_CONTROLLER_MODULE_NAME }},
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE,
			{ string: VIRTIO_RNG_CONTROLLER_PRETTY_NAME }},
		{ NULL }
	};

	return gDeviceManager->register_node(node,
		VIRTIO_RNG_DEVICE_MODULE_NAME, attrs, NULL, NULL);
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


static random_module_info sVirtioRNGDeviceInterface = {
	{
		{
			VIRTIO_RNG_DEVICE_MODULE_NAME,
			0,
			std_ops
		},
		NULL,	// supported devices
		NULL,	// register node
		sim_init_bus,
		sim_uninit_bus,
		NULL,	// register child devices
		NULL,	// rescan
		NULL	// bus_removed
	},

	sim_read,
	NULL	// write

};


static driver_module_info sVirtioRNGDriver = {
	{
		VIRTIO_RNG_DRIVER_MODULE_NAME,
		0,
		std_ops
	},
	virtio_rng_supports_device,
	virtio_rng_register_device,
	virtio_rng_init_driver,
	NULL,	// uninit_driver,
	virtio_rng_register_child_devices,
	NULL,	// rescan
	NULL,	// device_removed
};


module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&gDeviceManager },
	{ RANDOM_FOR_CONTROLLER_MODULE_NAME, (module_info **)&gRandom },
	{}
};


module_info *modules[] = {
	(module_info *)&sVirtioRNGDriver,
	(module_info *)&sVirtioRNGDeviceInterface,
	NULL
};
