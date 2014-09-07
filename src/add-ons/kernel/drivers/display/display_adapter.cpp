#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>

#include <ACPI.h>


#define DISPLAYADAPTER_MODULE_NAME "drivers/display/display_adapter/driver_v1"

#define DISPLAYADAPTER_DEVICE_MODULE_NAME "drivers/display/display_adapter/device_v1"

/* Base Namespace devices are published to */
#define DISPLAYADAPTER_BASENAME "display/display_adapter/%d"

// name of pnp generator of path ids
#define DISPLAYADAPTER_PATHID_GENERATOR "display_adapter/path_id"


#define OS_DISPLAY_SWITCH 0
#define BIOS_DISPLAY_SWITCH 1
#define LOCK_DISPLAY_SWITCH 2
#define NOTIFY_DISPLAY_SWITCH 3

#define OS_BRIGHTNESS_CONTROL (1 << 2)
#define BIOS_BRIGHTNESS_CONTROL (0 << 2)

static device_manager_info *sDeviceManager;
static acpi_module_info *sAcpi;


typedef struct acpi_ns_device_info {
	device_node *node;
	acpi_handle acpi_device;
} displayadapter_device_info;


//	#pragma mark - device module API


static status_t
displayadapter_init_device(void *_cookie, void **cookie)
{
	device_node *node = (device_node *)_cookie;
	displayadapter_device_info *device;
//	device_node *parent;

	acpi_objects arguments;
	acpi_object_type argument;

	const char *path;
	dprintf("%s: start.\n", __func__);


	device = (displayadapter_device_info *)calloc(1, sizeof(*device));
	if (device == NULL)
		return B_NO_MEMORY;

	device->node = node;
	if (sDeviceManager->get_attr_string(node, ACPI_DEVICE_PATH_ITEM, &path, false)
			!= B_OK || sAcpi->get_handle(NULL, path, &device->acpi_device) != B_OK) {
		dprintf("%s: failed to get acpi node.\n", __func__);
		return B_ERROR;
	}

	argument.object_type = ACPI_TYPE_INTEGER;
	argument.integer.integer = BIOS_DISPLAY_SWITCH | BIOS_BRIGHTNESS_CONTROL;
	arguments.count = 1;
	arguments.pointer = &argument;
	if (sAcpi->evaluate_object(&device->acpi_device, "_DOS", &arguments, NULL, 0)
			!= B_OK)
		dprintf("%s: failed to set _DOS %s\n", __func__, path);

	dprintf("%s: done.\n", __func__);
	*cookie = device;
	return B_OK;
}


static void
displayadapter_uninit_device(void *_cookie)
{
	displayadapter_device_info *device = (displayadapter_device_info *)_cookie;
	free(device);
}


static status_t
displayadapter_open(void *_cookie, const char *path, int flags, void** cookie)
{
	displayadapter_device_info *device = (displayadapter_device_info *)_cookie;
	*cookie = device;
	return B_OK;
}


static status_t
displayadapter_read(void* _cookie, off_t position, void *buf, size_t* num_bytes)
{
	return B_ERROR;
}


static status_t
displayadapter_write(void* cookie, off_t position, const void* buffer, size_t* num_bytes)
{
	return B_ERROR;
}


static status_t
displayadapter_control(void* _cookie, uint32 op, void* arg, size_t len)
{
//	displayadapter_device_info* device = (displayadapter_device_info*)_cookie;

	return B_ERROR;
}


static status_t
displayadapter_close(void* cookie)
{
	return B_OK;
}


static status_t
displayadapter_free(void* cookie)
{
	return B_OK;
}


//	#pragma mark - driver module API


static float
displayadapter_support(device_node *parent)
{
	acpi_handle handle, method;
//	acpi_object_type dosType;

	const char *bus;
	const char *path;
	uint32 device_type;

	// make sure parent is really the ACPI bus manager
	if (sDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false))
		return -1;

	if (strcmp(bus, "acpi"))
		return 0.0;

	if (sDeviceManager->get_attr_string(parent, ACPI_DEVICE_PATH_ITEM, &path, false) != B_OK)
		return 0.0;

	// check whether it's really a device
	if (sDeviceManager->get_attr_uint32(parent, ACPI_DEVICE_TYPE_ITEM, &device_type, false) != B_OK
		|| device_type != ACPI_TYPE_DEVICE) {
		return 0.0;
	}


	if (sAcpi->get_handle(NULL, path, &handle) != B_OK)
		return 0.0;

	if (sAcpi->get_handle(handle, "_DOD", &method) != B_OK ||
		sAcpi->get_handle(handle, "_DOS", &method) != B_OK) {// ||
//		sAcpi->get_type(method, &dosType) != B_OK ||
//		dosType != ACPI_TYPE_METHOD) {
		return 0.0;
	}

	dprintf("%s: found at bus: %s path: %s\n", __func__, bus, path);
	return 0.6;
}


static status_t
displayadapter_register_device(device_node *node)
{
	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, { string: "Display Controls" }},
		{ B_DEVICE_FLAGS, B_UINT32_TYPE, { ui32: B_KEEP_DRIVER_LOADED }},
		{ NULL }
	};

	return sDeviceManager->register_node(node, DISPLAYADAPTER_MODULE_NAME, attrs, NULL, NULL);
}


static status_t
displayadapter_init_driver(device_node *node, void **_driverCookie)
{
	*_driverCookie = node;
	return B_OK;
}


static void
displayadapter_uninit_driver(void *driverCookie)
{
}


static status_t
displayadapter_register_child_devices(void *_cookie)
{
	device_node *node = (device_node*)_cookie;
	int path_id;
	char name[128];

	path_id = sDeviceManager->create_id(DISPLAYADAPTER_PATHID_GENERATOR);
	if (path_id < 0) {
		dprintf("displayadapter_register_child_devices: couldn't create a path_id\n");
		return B_ERROR;
	}

	snprintf(name, sizeof(name), DISPLAYADAPTER_BASENAME, path_id);

	return sDeviceManager->publish_device(node, name, DISPLAYADAPTER_DEVICE_MODULE_NAME);
}


module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&sDeviceManager },
	{ B_ACPI_MODULE_NAME, (module_info **)&sAcpi},
	{}
};


driver_module_info displayadapter_driver_module = {
	{
		DISPLAYADAPTER_MODULE_NAME,
		0,
		NULL
	},

	displayadapter_support,
	displayadapter_register_device,
	displayadapter_init_driver,
	displayadapter_uninit_driver,
	displayadapter_register_child_devices,
	NULL,	// rescan
	NULL,	// removed
};


struct device_module_info displayadapter_device_module = {
	{
		DISPLAYADAPTER_DEVICE_MODULE_NAME,
		0,
		NULL
	},

	displayadapter_init_device,
	displayadapter_uninit_device,
	NULL,

	displayadapter_open,
	displayadapter_close,
	displayadapter_free,
	displayadapter_read,
	displayadapter_write,
	NULL,
	displayadapter_control,

	NULL,
	NULL
};

module_info *modules[] = {
	(module_info *)&displayadapter_driver_module,
	(module_info *)&displayadapter_device_module,
	NULL
};
