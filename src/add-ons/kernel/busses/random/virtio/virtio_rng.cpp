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
dpc_module_info *gDPC;


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
	VirtioRNGDevice *device =  new(std::nothrow) VirtioRNGDevice(node);
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
virtio_rng_uninit_driver(void *cookie)
{
	VirtioRNGDevice *device = (VirtioRNGDevice*)cookie;
	delete device;
}


static driver_module_info sVirtioRNGDriver = {
	{
		VIRTIO_RNG_DRIVER_MODULE_NAME,
		0,
		NULL
	},
	virtio_rng_supports_device,
	virtio_rng_register_device,
	virtio_rng_init_driver,
	virtio_rng_uninit_driver,
	NULL,
	NULL,	// rescan
	NULL,	// device_removed
};


module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&gDeviceManager },
	{ RANDOM_FOR_CONTROLLER_MODULE_NAME, (module_info **)&gRandom },
	{ B_DPC_MODULE_NAME, (module_info **)&gDPC },
	{}
};


module_info *modules[] = {
	(module_info *)&sVirtioRNGDriver,
	NULL
};
