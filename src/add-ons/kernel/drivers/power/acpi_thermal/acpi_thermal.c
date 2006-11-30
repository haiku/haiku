/* ++++++++++
    ACPI Generic Thermal Zone Driver. 
    Obtains general status of passive devices, monitors / sets critical temperatures
    Controls active devices.
+++++ */
#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>

#include <ACPI.h>
#include <pnp_devfs.h>
#include "acpi_thermal.h"

#define ACPI_THERMAL_MODULE_NAME "drivers/bin/acpi_thermal/acpi_device_v1"

/* Base Namespace devices are published to */
#define ACPI_THERMAL_BASENAME "acpi/thermal/%d"

// name of pnp generator of path ids
#define ACPI_THERMAL_PATHID_GENERATOR "acpi_thermal/path_id"

device_manager_info *gDeviceManager;

typedef struct acpi_ns_device_info {
	device_node_handle node;
	acpi_device_module_info *acpi;
	acpi_device acpi_cookie;
} acpi_thermal_device_info;


status_t acpi_thermal_control(acpi_thermal_device_info* device, uint32 op, void* arg, size_t len);


static status_t
acpi_thermal_open(acpi_thermal_device_info *device, uint32 flags, void** cookie)
{
	*cookie = device;
	return B_OK;
}


static status_t
acpi_thermal_read(acpi_thermal_device_info* device, off_t position, void *buf, size_t* num_bytes)
{
	acpi_thermal_type therm_info;
	if (*num_bytes < 1)
		return B_IO_ERROR;
	
	if (position == 0) {
		size_t max_len = *num_bytes;
		char *str = (char *)buf;
		dprintf("acpi_thermal: read()\n");
		acpi_thermal_control(device, drvOpGetThermalType, &therm_info, 0);
			
		snprintf(str, max_len, "  Critical Temperature: %lu.%lu K\n", 
				(therm_info.critical_temp / 10), (therm_info.critical_temp % 10));
		
		max_len -= strlen(str);
		str += strlen(str);
		snprintf(str, max_len, "  Current Temperature: %lu.%lu K\n", 
				(therm_info.current_temp / 10), (therm_info.current_temp % 10));
		
		if (therm_info.hot_temp > 0) {
			max_len -= strlen(str);
			str += strlen(str);
			snprintf(str, max_len, "  Hot Temperature: %lu.%lu K\n", 
					(therm_info.hot_temp / 10), (therm_info.hot_temp % 10));
		}

		if (therm_info.passive_package) {
/* Incomplete. 
     Needs to obtain acpi global lock.
     acpi_object_type needs Reference entry (with Handle that can be resolved)
     what you see here is _highly_ unreliable.
*/
/*      		if (therm_info.passive_package->data.package.count > 0) {
				sprintf((char *)buf + *num_bytes, "  Passive Devices\n");
				*num_bytes = strlen((char *)buf);
				for (i = 0; i < therm_info.passive_package->data.package.count; i++) {
					sprintf((char *)buf + *num_bytes, "    Processor: %lu\n", therm_info.passive_package->data.package.objects[i].data.processor.cpu_id);
					*num_bytes = strlen((char *)buf);
				}
			}
*/
			free(therm_info.passive_package);
		}
		*num_bytes = strlen((char *)buf);
	} else {
		*num_bytes = 0;
	}
	
	return B_OK;
}


static status_t
acpi_thermal_write (void* cookie, off_t position, const void* buffer, size_t* num_bytes)
{
	return B_ERROR;
}


status_t
acpi_thermal_control (acpi_thermal_device_info* device, uint32 op, void* arg, size_t len)
{
	status_t err = B_ERROR;
	
	acpi_thermal_type *att = NULL;
	
	size_t bufsize = sizeof(acpi_object_type);
	acpi_object_type buf;
	
	switch (op) {
		case drvOpGetThermalType: {
			dprintf("acpi_thermal: GetThermalType()\n");
			att = (acpi_thermal_type *)arg;
			
			// Read basic temperature thresholds.
			err = device->acpi->evaluate_method(device->acpi_cookie, "_CRT", &buf, bufsize, NULL, 0);
			att->critical_temp = buf.data.integer;
			err = device->acpi->evaluate_method(device->acpi_cookie, "_TMP", &buf, bufsize, NULL, 0);
			att->current_temp = buf.data.integer;
			err = device->acpi->evaluate_method(device->acpi_cookie, "_HOT", &buf, bufsize, NULL, 0);
			if (err == B_OK) {
				att->hot_temp = buf.data.integer;
			} else {
				att->hot_temp = 0;
			}
			dprintf("acpi_thermal: GotBasicTemperatures()\n");
			
			// Read Passive Cooling devices
			att->passive_package = NULL;
			err = device->acpi->get_object(device->acpi_cookie, "_PSL", &(att->passive_package));
			
			att->active_count = 0;
			att->active_devices = NULL;
			
			err = B_OK;
			break;
		}
	}
	return err;
}


static status_t
acpi_thermal_close (void* cookie)
{
	return B_OK;
}


static status_t
acpi_thermal_free (void* cookie)
{	
	return B_OK;
}


static float
acpi_thermal_support(device_node_handle parent, bool *_noConnection)
{
	char *bus;
	uint32 device_type;

	// make sure parent is really the ACPI bus manager
	if (gDeviceManager->get_attr_string(parent, B_DRIVER_BUS, &bus, false) != B_OK) {
		dprintf("no bus\n");
		return 0.0;
	}
	
	if (strcmp(bus, "acpi")) {
		dprintf("bad bus\n");
		goto err;
	}

	// check whether it's really a thermal Device
	if (gDeviceManager->get_attr_uint32(parent, ACPI_DEVICE_TYPE_ITEM, &device_type, false) != B_OK
		|| device_type != ACPI_TYPE_THERMAL) {
		dprintf("bad type\n");
		goto err;
	}

	// TODO check there are _CRT and _TMP ?

	free(bus);
	return 0.6;
err:
	free(bus);
	return 0.0;
}


static status_t
acpi_thermal_init_device(device_node_handle node, void *user_cookie, void **cookie)
{
	acpi_thermal_device_info *device;
	status_t res;

	device = (acpi_thermal_device_info *)calloc(1, sizeof(*device));
	if (device == NULL)
		return B_NO_MEMORY;

	device->node = node;

	// register it everywhere
	res = gDeviceManager->init_driver(gDeviceManager->get_parent(node), NULL, 
			(driver_module_info **)&device->acpi, (void**) &device->acpi_cookie);
	if (res != B_OK)
		goto err;

	*cookie = device;
	return B_OK;

err:
	free(device);
	return res;
}


static status_t
acpi_thermal_uninit_device(acpi_thermal_device_info *device)
{
	gDeviceManager->uninit_driver(gDeviceManager->get_parent(device->node));
	free(device);

	return B_OK;
}


static status_t
acpi_thermal_added(device_node_handle node)
{
	int path_id;
	char name[128];
		
	path_id = gDeviceManager->create_id(ACPI_THERMAL_PATHID_GENERATOR);
	if (path_id < 0) {
		return B_ERROR;
	}
	
	snprintf(name, sizeof(name), ACPI_THERMAL_BASENAME, path_id);
	{
		device_attr attrs[] = {
			{ B_DRIVER_MODULE, B_STRING_TYPE, { string: ACPI_THERMAL_MODULE_NAME }},
		
			{ PNP_DRIVER_CONNECTION, B_STRING_TYPE, { string: "acpi_thermal" }},
			// we want devfs on top of us (who wouldn't?)
			{ B_DRIVER_FIXED_CHILD, B_STRING_TYPE, { string: PNP_DEVFS_MODULE_NAME }},

			{ PNP_MANAGER_ID_GENERATOR, B_STRING_TYPE, { string: ACPI_THERMAL_PATHID_GENERATOR }},
			{ PNP_MANAGER_AUTO_ID, B_UINT32_TYPE, { ui32: path_id }},

			// tell which name we want to have in devfs
			{ PNP_DEVFS_FILENAME, B_STRING_TYPE, { string: name }},
			{ NULL }
		};

		return gDeviceManager->register_device(node, attrs, NULL, &node);
	}
	
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

module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&gDeviceManager },
	{}
};


pnp_devfs_driver_info acpi_thermal_dump_module = {
	{
		{
			ACPI_THERMAL_MODULE_NAME,
			0,			
			std_ops
		},

		acpi_thermal_support,
		acpi_thermal_added,
		acpi_thermal_init_device,
		(status_t (*) (void *))acpi_thermal_uninit_device,
		NULL
	},

	(pnp_device_open_hook) 	&acpi_thermal_open,
	acpi_thermal_close,
	acpi_thermal_free,
	(device_control_hook)	&acpi_thermal_control,

	(device_read_hook) acpi_thermal_read,
	acpi_thermal_write,

	NULL,
	NULL,

	NULL,
	NULL
};

module_info *modules[] = {
	(module_info *)&acpi_thermal_dump_module,
	NULL
};
