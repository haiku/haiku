/*
 * Copyright 2013, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2005, Nathan Whitehorn.
 *
 * Distributed under the terms of the MIT License.
 *
 * ACPI Button Driver, used to get info on power buttons, etc.
 */


#include <ACPI.h>

#include <fs/select_sync_pool.h>

#include <stdlib.h>
#include <string.h>


#define ACPI_BUTTON_MODULE_NAME "drivers/power/acpi_button/driver_v1"

#define ACPI_BUTTON_DEVICE_MODULE_NAME "drivers/power/acpi_button/device_v1"

#define ACPI_NOTIFY_BUTTON_SLEEP		0x80
#define ACPI_NOTIFY_BUTTON_WAKEUP		0x2

//#define TRACE_BUTTON
#ifdef TRACE_BUTTON
#	define TRACE(x...) dprintf("acpi_button: " x)
#else
#	define TRACE(x...)
#endif
#define ERROR(x...)	dprintf("acpi_button: " x)

static device_manager_info *sDeviceManager;
static struct acpi_module_info *sAcpi;


typedef struct acpi_ns_device_info {
	device_node *node;
	acpi_device_module_info *acpi;
	acpi_device acpi_cookie;
	uint32 type;
	bool fixed;
	uint8 last_status;
	select_sync_pool* select_pool;
} acpi_button_device_info;


static void
acpi_button_notify_handler(acpi_handle _device, uint32 value, void *context)
{
	acpi_button_device_info *device = (acpi_button_device_info *)context;
	if (value == ACPI_NOTIFY_BUTTON_SLEEP) {
		TRACE("sleep\n");
		device->last_status = 1;
		if (device->select_pool != NULL)
			notify_select_event_pool(device->select_pool, B_SELECT_READ);
	} else if (value == ACPI_NOTIFY_BUTTON_WAKEUP) {
		TRACE("wakeup\n");
	} else {
		ERROR("unknown notification\n");
	}

}


static uint32
acpi_button_fixed_handler(void *context)
{
	acpi_button_device_info *device = (acpi_button_device_info *)context;
	TRACE("sleep\n");
	device->last_status = 1;
	if (device->select_pool != NULL)
		notify_select_event_pool(device->select_pool, B_SELECT_READ);
	return B_OK;
}


//	#pragma mark - device module API


static status_t
acpi_button_init_device(void *_cookie, void **cookie)
{
	device_node *node = (device_node *)_cookie;
	acpi_button_device_info *device;
	device_node *parent;

	device = (acpi_button_device_info *)calloc(1, sizeof(*device));
	if (device == NULL)
		return B_NO_MEMORY;

	device->node = node;

	parent = sDeviceManager->get_parent_node(node);
	sDeviceManager->get_driver(parent, (driver_module_info **)&device->acpi,
		(void **)&device->acpi_cookie);

	const char *hid;
	if (sDeviceManager->get_attr_string(parent, ACPI_DEVICE_HID_ITEM, &hid,
		false) != B_OK) {
		sDeviceManager->put_node(parent);
		return B_ERROR;
	}

	sDeviceManager->put_node(parent);

	device->fixed = strcmp(hid, "PNP0C0C") != 0 && strcmp(hid, "PNP0C0E") != 0;
	TRACE("Device found, hid: %s, fixed: %d\n", hid, device->fixed);
	if (strcmp(hid, "PNP0C0C") == 0 || strcmp(hid, "ACPI_FPB") == 0)
		device->type = ACPI_EVENT_POWER_BUTTON;
	else if (strcmp(hid, "PNP0C0E") == 0 || strcmp(hid, "ACPI_FSB") == 0)
		device->type = ACPI_EVENT_SLEEP_BUTTON;
	else
		return B_ERROR;
	device->last_status = 0;
	device->select_pool = NULL;

	if (device->fixed) {
		sAcpi->reset_fixed_event(device->type);
		TRACE("Installing fixed handler for type %" B_PRIu32 "\n",
			device->type);
		if (sAcpi->install_fixed_event_handler(device->type,
			acpi_button_fixed_handler, device) != B_OK) {
			ERROR("can't install fixed handler\n");
		}
	} else {
		TRACE("Installing notify handler for type %" B_PRIu32 "\n",
			device->type);
		if (device->acpi->install_notify_handler(device->acpi_cookie,
			ACPI_DEVICE_NOTIFY, acpi_button_notify_handler, device) != B_OK) {
			ERROR("can't install notify handler\n");
		}
	}


	*cookie = device;
	return B_OK;
}


static void
acpi_button_uninit_device(void *_cookie)
{
	acpi_button_device_info *device = (acpi_button_device_info *)_cookie;
	if (device->fixed) {
		sAcpi->remove_fixed_event_handler(device->type,
			acpi_button_fixed_handler);
	} else {
		device->acpi->remove_notify_handler(device->acpi_cookie,
			ACPI_DEVICE_NOTIFY, acpi_button_notify_handler);
	}
	free(device);
}


static status_t
acpi_button_open(void *_cookie, const char *path, int flags, void** cookie)
{
	acpi_button_device_info *device = (acpi_button_device_info *)_cookie;

	if (device->fixed)
		sAcpi->enable_fixed_event(device->type);

	*cookie = device;
	return B_OK;
}


static status_t
acpi_button_read(void* _cookie, off_t position, void *buf, size_t* num_bytes)
{
	acpi_button_device_info* device = (acpi_button_device_info*)_cookie;
	if (*num_bytes < 1)
		return B_IO_ERROR;

	*((uint8 *)(buf)) = device->last_status;
	device->last_status = 0;

	*num_bytes = 1;
	return B_OK;
}


static status_t
acpi_button_write(void* cookie, off_t position, const void* buffer,
	size_t* num_bytes)
{
	return B_ERROR;
}


static status_t
acpi_button_control(void* _cookie, uint32 op, void* arg, size_t len)
{
	return B_ERROR;
}


static status_t
acpi_button_select(void *_cookie, uint8 event, selectsync *sync)
{
	acpi_button_device_info* device = (acpi_button_device_info*)_cookie;

	if (event != B_SELECT_READ)
		return B_BAD_VALUE;

	// add the event to the pool
	status_t error = add_select_sync_pool_entry(&device->select_pool, sync,
		event);
	if (error != B_OK) {
		ERROR("add_select_sync_pool_entry() failed: %#lx\n", error);
		return error;
	}

	if (device->last_status != 0)
		notify_select_event(sync, event);

	return B_OK;
}


static status_t
acpi_button_deselect(void *_cookie, uint8 event, selectsync *sync)
{
	acpi_button_device_info* device = (acpi_button_device_info*)_cookie;

	if (event != B_SELECT_READ)
		return B_BAD_VALUE;

	return remove_select_sync_pool_entry(&device->select_pool, sync, event);
}


static status_t
acpi_button_close (void* cookie)
{
	return B_OK;
}


static status_t
acpi_button_free (void* cookie)
{
	return B_OK;
}


//	#pragma mark - driver module API


static float
acpi_button_support(device_node *parent)
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
		&device_type, false) != B_OK || device_type != ACPI_TYPE_DEVICE) {
		return 0.0;
	}

	// check whether it's a button device
	if (sDeviceManager->get_attr_string(parent, ACPI_DEVICE_HID_ITEM, &hid,
		false) != B_OK || (strcmp(hid, "PNP0C0C") != 0
			&& strcmp(hid, "ACPI_FPB") != 0 && strcmp(hid, "PNP0C0E") != 0
			&& strcmp(hid, "ACPI_FSB") != 0)) {
		return 0.0;
	}

	TRACE("acpi_button_support button device found: %s\n", hid);

	return 0.6;
}


static status_t
acpi_button_register_device(device_node *node)
{
	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, { string: "ACPI Button" }},
		{ NULL }
	};

	return sDeviceManager->register_node(node, ACPI_BUTTON_MODULE_NAME, attrs,
		NULL, NULL);
}


static status_t
acpi_button_init_driver(device_node *node, void **_driverCookie)
{
	*_driverCookie = node;
	return B_OK;
}


static void
acpi_button_uninit_driver(void *driverCookie)
{
}


static status_t
acpi_button_register_child_devices(void *_cookie)
{
	device_node *node = (device_node*)_cookie;
	device_node *parent = sDeviceManager->get_parent_node(node);
	const char *hid;
	if (sDeviceManager->get_attr_string(parent, ACPI_DEVICE_HID_ITEM, &hid,
		false) != B_OK) {
		sDeviceManager->put_node(parent);
		return B_ERROR;
	}

	sDeviceManager->put_node(parent);

	status_t status = B_ERROR;
	if (strcmp(hid, "PNP0C0C") == 0) {
		status = sDeviceManager->publish_device(node,
			"power/button/power", ACPI_BUTTON_DEVICE_MODULE_NAME);
	} else if (strcmp(hid, "ACPI_FPB") == 0) {
		status = sDeviceManager->publish_device(node,
			"power/button/power_fixed", ACPI_BUTTON_DEVICE_MODULE_NAME);
	} else if (strcmp(hid, "PNP0C0E") == 0) {
		status = sDeviceManager->publish_device(node, "power/button/sleep",
			ACPI_BUTTON_DEVICE_MODULE_NAME);
	} else if ( strcmp(hid, "ACPI_FSB") == 0) {
		status = sDeviceManager->publish_device(node,
			"power/button/sleep_fixed", ACPI_BUTTON_DEVICE_MODULE_NAME);
	}

	return status;
}


module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&sDeviceManager },
	{ B_ACPI_MODULE_NAME, (module_info **)&sAcpi },
	{}
};


driver_module_info acpi_button_driver_module = {
	{
		ACPI_BUTTON_MODULE_NAME,
		0,
		NULL
	},

	acpi_button_support,
	acpi_button_register_device,
	acpi_button_init_driver,
	acpi_button_uninit_driver,
	acpi_button_register_child_devices,
	NULL,	// rescan
	NULL,	// removed
};


struct device_module_info acpi_button_device_module = {
	{
		ACPI_BUTTON_DEVICE_MODULE_NAME,
		0,
		NULL
	},

	acpi_button_init_device,
	acpi_button_uninit_device,
	NULL,

	acpi_button_open,
	acpi_button_close,
	acpi_button_free,
	acpi_button_read,
	acpi_button_write,
	NULL,
	acpi_button_control,
	acpi_button_select,
	acpi_button_deselect
};

module_info *modules[] = {
	(module_info *)&acpi_button_driver_module,
	(module_info *)&acpi_button_device_module,
	NULL
};
