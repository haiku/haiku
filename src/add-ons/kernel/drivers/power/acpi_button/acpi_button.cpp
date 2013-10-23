/*
 * Copyright 2013, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2005, Nathan Whitehorn.
 *
 * Distributed under the terms of the MIT License.
 *
 * ACPI Button Driver, used to get info on power buttons, etc.
 */


#include <ACPI.h>

#include <stdlib.h>
#include <string.h>


#define ACPI_BUTTON_MODULE_NAME "drivers/power/acpi_button/driver_v1"

#define ACPI_BUTTON_DEVICE_MODULE_NAME "drivers/power/acpi_button/device_v1"


static device_manager_info *sDeviceManager;
static struct acpi_module_info *sAcpi;


typedef struct acpi_ns_device_info {
	device_node *node;
	acpi_device_module_info *acpi;
	acpi_device acpi_cookie;
	uint32 type;
} acpi_button_device_info;


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
	sDeviceManager->put_node(parent);

	*cookie = device;
	return B_OK;
}


static void
acpi_button_uninit_device(void *_cookie)
{
	acpi_button_device_info *device = (acpi_button_device_info *)_cookie;
	free(device);
}


static status_t
acpi_button_open(void *_cookie, const char *path, int flags, void** cookie)
{
	acpi_button_device_info *device = (acpi_button_device_info *)_cookie;
	if (strcmp(path,"power/button/power") == 0)
		device->type = ACPI_EVENT_POWER_BUTTON;
	else if (strcmp(path,"power/button/sleep") == 0)
		device->type = ACPI_EVENT_SLEEP_BUTTON;
	else
		return B_ERROR;

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

	*((uint8 *)(buf)) = sAcpi->fixed_event_status(device->type) ? 1 : 0;

	sAcpi->reset_fixed_event(device->type);

	*num_bytes = 1;
	return B_OK;
}


static status_t
acpi_button_write(void* cookie, off_t position, const void* buffer, size_t* num_bytes)
{
	return B_ERROR;
}


static status_t
acpi_button_control(void* _cookie, uint32 op, void* arg, size_t len)
{
	return B_ERROR;
}


static status_t
acpi_button_close (void* cookie)
{
	acpi_button_device_info* device = (acpi_button_device_info*)cookie;
	sAcpi->disable_fixed_event(device->type);
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

	dprintf("acpi_button_support\n");

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

	// check whether it's a lid device
	if (sDeviceManager->get_attr_string(parent, ACPI_DEVICE_HID_ITEM, &hid,
		false) != B_OK || (strcmp(hid, "PNP0C0C") && strcmp(hid, "ACPI_FPB")
			&& strcmp(hid, "PNP0C0E") && strcmp(hid, "ACPI_FSB"))) {
		return 0.0;
	}

	dprintf("acpi_button_support button device found\n");

	return 0.6;
}


static status_t
acpi_button_register_device(device_node *node)
{
	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, { string: "ACPI Button" }},
		{ NULL }
	};

	return sDeviceManager->register_node(node, ACPI_BUTTON_MODULE_NAME, attrs, NULL, NULL);
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

	dprintf("acpi_button_register_child_devices\n");

	status_t status = sDeviceManager->publish_device(node,
		"power/button/power", ACPI_BUTTON_DEVICE_MODULE_NAME);
	if (status != B_OK)
		return status;

	return sDeviceManager->publish_device(node, "power/button/sleep",
		ACPI_BUTTON_DEVICE_MODULE_NAME);
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

	NULL,
	NULL
};

module_info *modules[] = {
	(module_info *)&acpi_button_driver_module,
	(module_info *)&acpi_button_device_module,
	NULL
};
