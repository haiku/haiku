#include "display_adapter.h"


typedef struct acpi_ns_device_info {
	device_node *node;
	acpi_handle acpi_device;
} displayadapter_device_info;


device_manager_info *gDeviceManager = NULL;
acpi_module_info *gAcpi = NULL;


/*

TODO: ACPI Spec 5 Appendix B: Implement:
_DOS Method to control display output switching
(  _DOD Method to retrieve information about child output devices
	- You can already do this by listing child devices )
_ROM Method to retrieve the ROM image for this device
_GPD Method for determining which VGA device will post
_SPD Method for controlling which VGA device will post
_VPO Method for determining the post options

Display cycling notifications

*/


//	#pragma mark - device module API

	
static status_t
displayadapter_init_device(void *_cookie, void **cookie)
{
	device_node *node = (device_node *)_cookie;
	displayadapter_device_info *device;
//	device_node *parent;

//	acpi_objects arguments;
//	acpi_object_type argument;

	const char *path;
	dprintf("%s: start.\n", __func__);


	device = (displayadapter_device_info *)calloc(1, sizeof(*device));
	if (device == NULL)
		return B_NO_MEMORY;

	device->node = node;
	if (gDeviceManager->get_attr_string(node, ACPI_DEVICE_PATH_ITEM, &path,
			false) != B_OK
		|| gAcpi->get_handle(NULL, path, &device->acpi_device) != B_OK) {
		dprintf("%s: failed to get acpi node.\n", __func__);
		free(device);
		return B_ERROR;
	}
/*
	argument.object_type = ACPI_TYPE_INTEGER;
	argument.integer.integer = BIOS_DISPLAY_SWITCH | BIOS_BRIGHTNESS_CONTROL;
	arguments.count = 1;
	arguments.pointer = &argument;
	if (gAcpi->evaluate_object(&device->acpi_device, "_DOS", &arguments, NULL,
			0) != B_OK)
		dprintf("%s: failed to set _DOS %s\n", __func__, path);

	dprintf("%s: done.\n", __func__);
*/
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
displayadapter_write(void* cookie, off_t position, const void* buffer,
	size_t* num_bytes)
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
	if (gDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false))
		return -1;

	if (strcmp(bus, "acpi"))
		return 0.0;

	if (gDeviceManager->get_attr_string(parent, ACPI_DEVICE_PATH_ITEM, &path,
			false) != B_OK)
		return 0.0;

	// check whether it's really a device
	if (gDeviceManager->get_attr_uint32(parent, ACPI_DEVICE_TYPE_ITEM,
			&device_type, false) != B_OK
		|| device_type != ACPI_TYPE_DEVICE) {
		return 0.0;
	}


	if (gAcpi->get_handle(NULL, path, &handle) != B_OK)
		return 0.0;

	if (gAcpi->get_handle(handle, "_DOD", &method) != B_OK ||
		gAcpi->get_handle(handle, "_DOS", &method) != B_OK) {// ||
//		sAcpi->get_type(method, &dosType) != B_OK ||
//		dosType != ACPI_TYPE_METHOD) {
		return 0.0;
	}

	dprintf("%s: found at bus: %s path: %s\n", __func__, bus, path);
	return 0.6;
}


static status_t
register_displays(const char *parentName, device_node *node)
{
	acpi_handle acpiHandle;
	const char *path;
	device_node *parent = gDeviceManager->get_parent_node(node);
	if (gDeviceManager->get_attr_string(parent, ACPI_DEVICE_PATH_ITEM, &path,
			false) != B_OK
		|| gAcpi->get_handle(NULL, path, &acpiHandle) != B_OK) {
		dprintf("%s: failed to get acpi node.\n", __func__);
		gDeviceManager->put_node(parent);
		return B_ERROR;
	}

	//get list of ids from _DOD
	acpi_object_type *pkgData = (acpi_object_type *)malloc(128);
	if (pkgData == NULL)
		return B_ERROR;

	status_t status = gAcpi->evaluate_object(acpiHandle, "_DOD", NULL, pkgData,
		128);
	if (status != B_OK || pkgData->object_type != ACPI_TYPE_PACKAGE) {
		dprintf("%s: fail. %ld %lu\n", __func__, status, pkgData->object_type);
		free(pkgData);
		return status;	
	}

	acpi_object_type *displayIDs = pkgData->package.objects;
	for (uint32 i = 0; i < pkgData->package.count; i++) {
		dprintf("Display ID = %lld\n", displayIDs[i].integer.integer);
	}
    
	acpi_object_type result;
	acpi_handle child = NULL;
	while (gAcpi->get_next_object(ACPI_TYPE_DEVICE, acpiHandle, &child)
		== B_OK) {
		char name[5] = {0};
		//TODO: HARDCODED type.
		if(gAcpi->get_name(child, 1, name, 5) == B_OK) 
			dprintf("name: %s\n", name);
		if (gAcpi->evaluate_object(child, "_ADR", NULL, &result, sizeof(result))
			!= B_OK)
			continue;
		
		dprintf("Child _adr %llu\n", result.integer.integer);
		uint32 i;
		for (i = 0; i < pkgData->package.count; i++)
			if (displayIDs[i].integer.integer == result.integer.integer) break;
		
		if (i == pkgData->package.count) continue;
		
		device_attr attrs[] = {
			{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, { string: name }},
			{ B_DEVICE_FLAGS, B_UINT32_TYPE, { ui32: B_KEEP_DRIVER_LOADED }},
			{ NULL }
		
		};
		
		device_node* deviceNode;
		gDeviceManager->register_node(node, DISPLAY_DEVICE_MODULE_NAME, attrs,
				NULL, &deviceNode);

		char deviceName[128];
		snprintf(deviceName, sizeof(deviceName), "%s/%s", parentName, name);
		gDeviceManager->publish_device(parent, deviceName,
			DISPLAY_DEVICE_MODULE_NAME);
		
	}
	gDeviceManager->put_node(parent);
	free(pkgData);
	return B_OK;
}


static status_t
displayadapter_register_device(device_node *node)
{
	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, { string: "Display Adapter" }},
		{ B_DEVICE_FLAGS, B_UINT32_TYPE, {
			ui32: B_KEEP_DRIVER_LOADED | B_FIND_MULTIPLE_CHILDREN }},
		{ NULL }
	};

	return gDeviceManager->register_node(node, DISPLAYADAPTER_MODULE_NAME,
		attrs, NULL, NULL);
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

	int path_id = gDeviceManager->create_id(DISPLAYADAPTER_PATHID_GENERATOR);
	if (path_id < 0) {
		dprintf("displayadapter_register_child_devices: error creating path\n");
		return B_ERROR;
	}

	char name[128];
	snprintf(name, sizeof(name), DISPLAYADAPTER_BASENAME, path_id);
	status_t status = gDeviceManager->publish_device(node, name,
		DISPLAYADAPTER_DEVICE_MODULE_NAME);

	if (status == B_OK)
		register_displays(name, node);

	return status;
}


module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&gDeviceManager },
	{ B_ACPI_MODULE_NAME, (module_info **)&gAcpi},
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


device_module_info displayadapter_device_module = {
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
	(module_info *)&display_device_module,
	(module_info *)&displayadapter_device_module,
	(module_info *)&displayadapter_driver_module,
	NULL
};
