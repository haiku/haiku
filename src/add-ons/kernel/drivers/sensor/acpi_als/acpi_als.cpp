/*
 * Copyright 2020, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT license.
 */


#include <ACPI.h>
#include <condition_variable.h>
#include <Drivers.h>
#include <Errors.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kernel.h>


extern "C" {
#	include "acpi.h"
}


struct als_driver_cookie {
	device_node*				node;
	acpi_device_module_info*	acpi;
	acpi_device					acpi_cookie;
};


struct als_device_cookie {
	als_driver_cookie*		driver_cookie;
	int32					stop_watching;
};


#define ACPI_ALS_DRIVER_NAME "drivers/sensor/acpi_als/driver_v1"
#define ACPI_ALS_DEVICE_NAME "drivers/sensor/acpi_als/device_v1"

/* Base Namespace devices are published to */
#define ACPI_ALS_BASENAME "sensor/acpi_als/%d"

// name of pnp generator of path ids
#define ACPI_ALS_PATHID_GENERATOR "acpi_als/path_id"

#define ACPI_NAME_ALS "ACPI0008"

#define TRACE_ALS
#ifdef TRACE_ALS
#	define TRACE(x...) dprintf("acpi_als: " x)
#else
#	define TRACE(x...)
#endif
#define ERROR(x...) dprintf("acpi_als: " x)


static device_manager_info *sDeviceManager;
static ConditionVariable sALSCondition;


static status_t
acpi_GetInteger(als_driver_cookie *device,
	const char* path, uint64* number)
{
	acpi_data buf;
	acpi_object_type object;
	buf.pointer = &object;
	buf.length = sizeof(acpi_object_type);

	// Assume that what we've been pointed at is an Integer object, or
	// a method that will return an Integer.
	status_t status = device->acpi->evaluate_method(device->acpi_cookie, path,
		NULL, &buf);
	if (status == B_OK) {
		if (object.object_type == ACPI_TYPE_INTEGER)
			*number = object.integer.integer;
		else
			status = B_BAD_VALUE;
	}
	return status;
}


void
als_notify_handler(acpi_handle device, uint32 value, void *context)
{
	TRACE("als_notify_handler event 0x%" B_PRIx32 "\n", value);
	sALSCondition.NotifyAll();
}


//	#pragma mark - device module API


static status_t
acpi_als_init_device(void *driverCookie, void **cookie)
{
	*cookie = driverCookie;
	return B_OK;
}


static void
acpi_als_uninit_device(void *_cookie)
{

}


static status_t
acpi_als_open(void *initCookie, const char *path, int flags, void** cookie)
{
	als_device_cookie *device;
	device = (als_device_cookie*)calloc(1, sizeof(als_device_cookie));
	if (device == NULL)
		return B_NO_MEMORY;

	device->driver_cookie = (als_driver_cookie*)initCookie;
	device->stop_watching = 0;

	*cookie = device;

	return B_OK;
}


static status_t
acpi_als_close(void* cookie)
{
	return B_OK;
}


static status_t
acpi_als_read(void* _cookie, off_t position, void *buffer, size_t* numBytes)
{
	if (*numBytes < 1)
		return B_IO_ERROR;

	als_device_cookie *device = (als_device_cookie*)_cookie;

	if (position == 0) {
		char string[10];
		uint64 luminance = 0;
		status_t status = acpi_GetInteger(device->driver_cookie,
			"_ALI", &luminance);
		if (status != B_OK)
			return B_ERROR;
		snprintf(string, sizeof(string), "%" B_PRIu64 "\n", luminance);
		size_t max_len = user_strlcpy((char*)buffer, string, *numBytes);
		if (max_len < B_OK)
			return B_BAD_ADDRESS;
		*numBytes = max_len;
	} else
		*numBytes = 0;

	return B_OK;
}


static status_t
acpi_als_write(void* cookie, off_t position, const void* buffer,
	size_t* numBytes)
{
	return B_ERROR;
}


static status_t
acpi_als_control(void* _cookie, uint32 op, void* arg, size_t len)
{
	//als_device_cookie* device = (als_device_cookie*)_cookie;

	return B_DEV_INVALID_IOCTL;
}


static status_t
acpi_als_free(void* cookie)
{
	als_device_cookie* device = (als_device_cookie*)cookie;
	free(device);
	return B_OK;
}


//	#pragma mark - driver module API


static float
acpi_als_support(device_node *parent)
{
	// make sure parent is really the ACPI bus manager
	const char *bus;
	if (sDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false))
		return -1;

	if (strcmp(bus, "acpi"))
		return 0.0;

	// check whether it's really a device
	uint32 device_type;
	if (sDeviceManager->get_attr_uint32(parent, ACPI_DEVICE_TYPE_ITEM,
			&device_type, false) != B_OK
		|| device_type != ACPI_TYPE_DEVICE) {
		return 0.0;
	}

	// check whether it's a als device
	const char *name;
	if (sDeviceManager->get_attr_string(parent, ACPI_DEVICE_HID_ITEM, &name,
		false) != B_OK || strcmp(name, ACPI_NAME_ALS)) {
		return 0.0;
	}

	return 0.6;
}


static status_t
acpi_als_register_device(device_node *node)
{
	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, { string: "ACPI ALS" }},
		{ NULL }
	};

	return sDeviceManager->register_node(node, ACPI_ALS_DRIVER_NAME, attrs,
		NULL, NULL);
}


static status_t
acpi_als_init_driver(device_node *node, void **driverCookie)
{
	als_driver_cookie *device;
	device = (als_driver_cookie *)calloc(1, sizeof(als_driver_cookie));
	if (device == NULL)
		return B_NO_MEMORY;

	*driverCookie = device;

	device->node = node;

	device_node *parent;
	parent = sDeviceManager->get_parent_node(node);
	sDeviceManager->get_driver(parent, (driver_module_info **)&device->acpi,
		(void **)&device->acpi_cookie);

#ifdef TRACE_ALS
	const char* device_path;
	if (sDeviceManager->get_attr_string(parent, ACPI_DEVICE_PATH_ITEM,
		&device_path, false) == B_OK) {
		TRACE("acpi_als_init_driver %s\n", device_path);
	}
#endif

	sDeviceManager->put_node(parent);

	uint64 sta;
	status_t status = acpi_GetInteger(device, "_STA", &sta);
	uint64 mask = ACPI_STA_DEVICE_PRESENT | ACPI_STA_DEVICE_ENABLED
		| ACPI_STA_DEVICE_FUNCTIONING;
	if (status == B_OK && (sta & mask) != mask) {
		ERROR("acpi_als_init_driver device disabled\n");
		return B_ERROR;
	}

	uint64 luminance;
	status = acpi_GetInteger(device, "_ALI", &luminance);
	if (status != B_OK) {
		ERROR("acpi_als_init_driver error when calling _ALI\n");
		return B_ERROR;
	}

	// install notify handler
	device->acpi->install_notify_handler(device->acpi_cookie,
		ACPI_ALL_NOTIFY, als_notify_handler, device);

	return B_OK;
}


static void
acpi_als_uninit_driver(void *driverCookie)
{
	TRACE("acpi_als_uninit_driver\n");
	als_driver_cookie *device = (als_driver_cookie*)driverCookie;

	device->acpi->remove_notify_handler(device->acpi_cookie,
		ACPI_ALL_NOTIFY, als_notify_handler);

	free(device);
}


static status_t
acpi_als_register_child_devices(void *cookie)
{
	als_driver_cookie *device = (als_driver_cookie*)cookie;

	int pathID = sDeviceManager->create_id(ACPI_ALS_PATHID_GENERATOR);
	if (pathID < 0) {
		ERROR("register_child_devices: couldn't create a path_id\n");
		return B_ERROR;
	}

	char name[128];
	snprintf(name, sizeof(name), ACPI_ALS_BASENAME, pathID);

	return sDeviceManager->publish_device(device->node, name,
		ACPI_ALS_DEVICE_NAME);
}






module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&sDeviceManager },
	{}
};


driver_module_info acpi_als_driver_module = {
	{
		ACPI_ALS_DRIVER_NAME,
		0,
		NULL
	},

	acpi_als_support,
	acpi_als_register_device,
	acpi_als_init_driver,
	acpi_als_uninit_driver,
	acpi_als_register_child_devices,
	NULL,	// rescan
	NULL,	// removed
};


struct device_module_info acpi_als_device_module = {
	{
		ACPI_ALS_DEVICE_NAME,
		0,
		NULL
	},

	acpi_als_init_device,
	acpi_als_uninit_device,
	NULL,

	acpi_als_open,
	acpi_als_close,
	acpi_als_free,
	acpi_als_read,
	acpi_als_write,
	NULL,
	acpi_als_control,

	NULL,
	NULL
};

module_info *modules[] = {
	(module_info *)&acpi_als_driver_module,
	(module_info *)&acpi_als_device_module,
	NULL
};
