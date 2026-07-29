/*
 * Copyright 2013, Jérôme Duval, korli@users.berlios.de.
 *
 * Distributed under the terms of the MIT License.
 */


#include <ACPI.h>

#include <fs/select_sync_pool.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define ACPI_AC_MODULE_NAME "drivers/power/acpi_ac/driver_v1"

#define ACPI_AC_DEVICE_MODULE_NAME "drivers/power/acpi_ac/device_v1"

#define ACPI_NOTIFY_STATUS_CHANGED		0x80

/* Base Namespace devices are published to */
#define ACPI_AC_BASENAME "power/acpi_ac/%d"

// name of pnp generator of path ids
#define ACPI_AC_PATHID_GENERATOR "acpi_ac/path_id"

#define TRACE_AC
#ifdef TRACE_AC
#	define TRACE(x...) dprintf("acpi_ac: " x)
#else
#	define TRACE(x...)
#endif
#define ERROR(x...)	dprintf("acpi_ac: " x)

static device_manager_info *sDeviceManager;


typedef struct acpi_ns_device_info {
	device_node *node;
	acpi_device_module_info *acpi;
	acpi_device acpi_cookie;
	uint8 last_status;
	bool updated;
	select_sync_pool* select_pool;
} acpi_ac_device_info;


static void
acpi_ac_update_status(acpi_ac_device_info* device)
{
	acpi_data buf;
	buf.pointer = NULL;
	buf.length = ACPI_ALLOCATE_BUFFER;

	if (device->acpi->evaluate_method(device->acpi_cookie, "_PSR", NULL, &buf) != B_OK
		|| buf.pointer == NULL
		|| ((acpi_object_type*)buf.pointer)->object_type != ACPI_TYPE_INTEGER) {
		ERROR("couldn't get status\n");
	} else {
		acpi_object_type* object = (acpi_object_type*)buf.pointer;
		device->last_status = object->integer.integer;
		device->updated = true;
		TRACE("status %d\n", device->last_status);
	}
	free(buf.pointer);
}


static void
acpi_ac_notify_handler(acpi_handle _device, uint32 value, void *context)
{
	if (value != ACPI_NOTIFY_STATUS_CHANGED) {
		dprintf("acpi_ac: unknown notification (%d)\n", value);
		return;
	}

	acpi_ac_device_info* device = (acpi_ac_device_info*) context;
	acpi_ac_update_status(device);

	if (device->select_pool != NULL)
		notify_select_event_pool(device->select_pool, B_SELECT_READ);
}


//	#pragma mark - device module API


static status_t
acpi_ac_init_device(void *driverCookie, void **cookie)
{
	*cookie = driverCookie;
	return B_OK;
}


static void
acpi_ac_uninit_device(void *_cookie)
{

}


static status_t
acpi_ac_open(void *_cookie, const char *path, int flags, void** cookie)
{
	acpi_ac_device_info *device = (acpi_ac_device_info *)_cookie;
	*cookie = device;
	return B_OK;
}


static status_t
acpi_ac_read(void* _cookie, off_t position, void *buf, size_t* num_bytes)
{
	acpi_ac_device_info* device = (acpi_ac_device_info*)_cookie;
	if (*num_bytes < 1)
		return B_IO_ERROR;

	if (user_memcpy(buf, &device->last_status, sizeof(uint8)) < B_OK)
		return B_BAD_ADDRESS;

	*num_bytes = 1;
	device->updated = false;
	return B_OK;
}


static status_t
acpi_ac_write(void* cookie, off_t position, const void* buffer, size_t* num_bytes)
{
	return B_ERROR;
}


static status_t
acpi_ac_control(void* _cookie, uint32 op, void* arg, size_t len)
{
	return B_ERROR;
}


static status_t
acpi_ac_select(void *_cookie, uint8 event, selectsync *sync)
{
	acpi_ac_device_info* device = (acpi_ac_device_info*)_cookie;

	if (event != B_SELECT_READ)
		return B_BAD_VALUE;

	// add the event to the pool
	status_t error = add_select_sync_pool_entry(&device->select_pool, sync,
		event);
	if (error != B_OK) {
		ERROR("add_select_sync_pool_entry() failed: %#" B_PRIx32 "\n", error);
		return error;
	}

	if (device->updated)
		notify_select_event(sync, event);

	return B_OK;
}


static status_t
acpi_ac_deselect(void *_cookie, uint8 event, selectsync *sync)
{
	acpi_ac_device_info* device = (acpi_ac_device_info*)_cookie;

	if (event != B_SELECT_READ)
		return B_BAD_VALUE;

	return remove_select_sync_pool_entry(&device->select_pool, sync, event);
}

static status_t
acpi_ac_close (void* cookie)
{
	return B_OK;
}


static status_t
acpi_ac_free (void* cookie)
{
	return B_OK;
}


//	#pragma mark - driver module API


static float
acpi_ac_support(device_node *parent)
{
	const char *bus;
	uint32 device_type;
	const char *hid;

	// make sure parent is really the ACPI bus manager
	if (sDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false))
		return -1;

	if (strcmp(bus, "acpi"))
		return 0.0;

	// check whether it's really a device
	if (sDeviceManager->get_attr_uint32(parent, ACPI_DEVICE_TYPE_ITEM,
			&device_type, false) != B_OK
		|| device_type != ACPI_TYPE_DEVICE) {
		return 0.0;
	}

	// check whether it's an ac device
	if (sDeviceManager->get_attr_string(parent, ACPI_DEVICE_HID_ITEM, &hid,
		false) != B_OK || strcmp(hid, "ACPI0003")) {
		return 0.0;
	}

	dprintf("acpi_ac_support ac device found\n");

	return 0.6;
}


static status_t
acpi_ac_register_device(device_node *node)
{
	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, { .string = "ACPI AC" }},
		{ NULL }
	};

	return sDeviceManager->register_node(node, ACPI_AC_MODULE_NAME, attrs,
		NULL, NULL);
}


static status_t
acpi_ac_init_driver(device_node *node, void **_driverCookie)
{
	acpi_ac_device_info *device;
	device_node *parent;
	status_t status;

	device = (acpi_ac_device_info *)calloc(1, sizeof(*device));
	if (device == NULL)
		return B_NO_MEMORY;

	device->node = node;

	parent = sDeviceManager->get_parent_node(node);
	sDeviceManager->get_driver(parent, (driver_module_info **)&device->acpi,
		(void **)&device->acpi_cookie);
	sDeviceManager->put_node(parent);

	status = device->acpi->install_notify_handler(device->acpi_cookie,
		ACPI_DEVICE_NOTIFY, acpi_ac_notify_handler, device);
	if (status != B_OK) {
		ERROR("can't install notify handler\n");
	}

	device->last_status = 0;
	device->updated = false;
	device->select_pool = NULL;

	*_driverCookie = device;
	return B_OK;
}


static void
acpi_ac_uninit_driver(void *driverCookie)
{
	acpi_ac_device_info *device = (acpi_ac_device_info *)driverCookie;

	device->acpi->remove_notify_handler(device->acpi_cookie,
		ACPI_DEVICE_NOTIFY, acpi_ac_notify_handler);

	free(device);
}


static status_t
acpi_ac_register_child_devices(void *driverCookie)
{
	acpi_ac_device_info *device = (acpi_ac_device_info *)driverCookie;
	int path_id;
	char name[B_DEV_NAME_LENGTH];

	path_id = sDeviceManager->create_id(ACPI_AC_PATHID_GENERATOR);
	if (path_id < 0) {
		ERROR("register_child_devices: couldn't create a path_id\n");
		return B_ERROR;
	}

	snprintf(name, sizeof(name), ACPI_AC_BASENAME, path_id);

	return sDeviceManager->publish_device(device->node, name,
		ACPI_AC_DEVICE_MODULE_NAME);
}


module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&sDeviceManager },
	{}
};


driver_module_info acpi_ac_driver_module = {
	{
		ACPI_AC_MODULE_NAME,
		0,
		NULL
	},

	acpi_ac_support,
	acpi_ac_register_device,
	acpi_ac_init_driver,
	acpi_ac_uninit_driver,
	acpi_ac_register_child_devices,
	NULL,	// rescan
	NULL,	// removed
};


struct device_module_info acpi_ac_device_module = {
	{
		ACPI_AC_DEVICE_MODULE_NAME,
		0,
		NULL
	},

	acpi_ac_init_device,
	acpi_ac_uninit_device,
	NULL,

	acpi_ac_open,
	acpi_ac_close,
	acpi_ac_free,
	acpi_ac_read,
	acpi_ac_write,
	NULL,
	acpi_ac_control,
	acpi_ac_select,
	acpi_ac_deselect
};

module_info *modules[] = {
	(module_info *)&acpi_ac_driver_module,
	(module_info *)&acpi_ac_device_module,
	NULL
};
