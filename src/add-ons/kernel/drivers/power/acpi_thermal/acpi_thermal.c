/*
 * Copyright 2006-2008, Haiku, Inc. All Rights Reserved.
 *
 * Distributed under the terms of the MIT License.
 *
 * ACPI Generic Thermal Zone Driver.
 * Obtains general status of passive devices, monitors / sets critical temperatures
 * Controls active devices.
 */

#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>

#include <ACPI.h>

#include <util/convertutf.h>

#include "acpi_thermal.h"

#define ACPI_THERMAL_MODULE_NAME "drivers/power/acpi_thermal/driver_v1"

#define ACPI_THERMAL_DEVICE_MODULE_NAME "drivers/power/acpi_thermal/device_v1"

/* Base Namespace devices are published to */
#define ACPI_THERMAL_BASENAME "power/acpi_thermal/%d"

// name of pnp generator of path ids
#define ACPI_THERMAL_PATHID_GENERATOR "acpi_thermal/path_id"

static device_manager_info* sDeviceManager;

typedef struct acpi_ns_device_info {
	device_node* node;
	acpi_device_module_info* acpi;
	acpi_device acpi_cookie;
	uint kelvin_offset;	// Initialized on first acpi_thermal_open() call.
} acpi_thermal_device_info;


status_t acpi_thermal_control(void* _cookie, uint32 op, void* arg, size_t len);

static void guess_kelvin_offset(acpi_thermal_device_info* device);


/*
 * Exact value for Kelvin->Celsius conversion is 273.15, but as ACPI uses deci-Kelvins
 * that means we only get one digit for the decimal part. Some implementations use 2731,
 * while others use 2732. We try to guess which one here (as Linux's acpi/thermal.c driver
 * does) by checking whether the "critical temp" value is set to something reasonable
 * (> 0 °C), and if it is a multiple of 0.5 °C.
 */
static void
guess_kelvin_offset(acpi_thermal_device_info* device)
{
	acpi_thermal_type therm_info;

	acpi_thermal_control(device, drvOpGetThermalType, &therm_info, 0);

	device->kelvin_offset = 2732;
	if (therm_info.critical_temp > 2732 && (therm_info.critical_temp % 5) == 1)
		device->kelvin_offset = 2731;
}


static status_t
acpi_thermal_open(void* _cookie, const char* path, int flags, void** cookie)
{
	acpi_thermal_device_info* device = (acpi_thermal_device_info*)_cookie;
	*cookie = device;

	if (device->kelvin_offset == 0)
		guess_kelvin_offset(device);

	return B_OK;
}


static status_t
acpi_thermal_read(void* _cookie, off_t position, void* buf, size_t* num_bytes)
{
	acpi_thermal_device_info* device = (acpi_thermal_device_info*)_cookie;
	acpi_thermal_type therm_info;

	if (*num_bytes < 1)
		return B_IO_ERROR;

	if (position == 0) {
		char string[128];
		char* str = string;
		size_t max_len = sizeof(string);

		uint kelvinOffset = device->kelvin_offset;

		acpi_thermal_control(device, drvOpGetThermalType, &therm_info, 0);

		snprintf(str, max_len, "  Critical Temperature: %" B_PRIu32 ".%" B_PRIu32 " °C\n",
				((therm_info.critical_temp - kelvinOffset) / 10),
				((therm_info.critical_temp - kelvinOffset) % 10));
		max_len -= strlen(str);
		str += strlen(str);

		snprintf(str, max_len, "  Current Temperature: %" B_PRIu32 ".%" B_PRIu32 " °C\n",
				((therm_info.current_temp - kelvinOffset) / 10),
				((therm_info.current_temp - kelvinOffset) % 10));
		max_len -= strlen(str);
		str += strlen(str);

		if (therm_info.hot_temp > 0) {
			snprintf(str, max_len, "  Hot Temperature: %" B_PRIu32 ".%" B_PRIu32 " °C\n",
					((therm_info.hot_temp - kelvinOffset) / 10),
					((therm_info.hot_temp - kelvinOffset) % 10));
			max_len -= strlen(str);
			str += strlen(str);
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

		max_len = user_strlcpy((char*)buf, string, *num_bytes);
		if (max_len < B_OK)
			return B_BAD_ADDRESS;
		*num_bytes = max_len;
	} else {
		*num_bytes = 0;
	}

	return B_OK;
}


static status_t
acpi_thermal_write(void* cookie, off_t position, const void* buffer, size_t* num_bytes)
{
	return B_ERROR;
}


status_t
acpi_thermal_control(void* _cookie, uint32 op, void* arg, size_t len)
{
	acpi_thermal_device_info* device = (acpi_thermal_device_info*)_cookie;
	status_t err = B_DEV_INVALID_IOCTL;

	switch (op) {
		case B_GET_DEVICE_NAME: {
			acpi_data buffer;

			buffer.pointer = NULL;
			buffer.length = ACPI_ALLOCATE_BUFFER;

			err = device->acpi->evaluate_method (device->acpi_cookie, "_STR",
				NULL, &buffer);

			if (err != B_OK) {
				dprintf("acpi_thermal: could not get zone name: %s\n", strerror(err));
				free(buffer.pointer);
				break;
			}

			acpi_object_type* object = (acpi_object_type*)buffer.pointer;
			if (object->object_type == ACPI_TYPE_STRING) {
				err = user_strlcpy((char*)arg, object->string.string, min(object->string.len, len));
				free(buffer.pointer);
			} else if (object->object_type == ACPI_TYPE_BUFFER) {
				char utf8[256];
				ssize_t bytes = utf16le_to_utf8(object->buffer.buffer, object->buffer.length / 2,
					utf8, sizeof(utf8));
				free(buffer.pointer);
				if (bytes < 0) {
					err = bytes;
					break;
				}

				err = user_strlcpy((char*)arg, utf8, min((size_t)bytes, len));
			}

			if (err > 0)
				err = B_OK;
			break;
		}
		case drvOpGetThermalType: {
			acpi_object_type object;
			acpi_thermal_type* att = NULL;

			acpi_data buffer;
			buffer.pointer = &object;
			buffer.length = sizeof(object);

			att = (acpi_thermal_type*)arg;

			// Read basic temperature thresholds.
			err = device->acpi->evaluate_method (device->acpi_cookie, "_CRT",
				NULL, &buffer);

			att->critical_temp =
				(err == B_OK && object.object_type == ACPI_TYPE_INTEGER)
				? object.integer.integer : 0;

			err = device->acpi->evaluate_method (device->acpi_cookie, "_TMP",
				NULL, &buffer);

			att->current_temp =
				(err == B_OK && object.object_type == ACPI_TYPE_INTEGER)
				? object.integer.integer : 0;

			err = device->acpi->evaluate_method(device->acpi_cookie, "_HOT",
				NULL, &buffer);

			att->hot_temp =
				(err == B_OK && object.object_type == ACPI_TYPE_INTEGER)
				? object.integer.integer : 0;

			// Read Passive Cooling devices
			att->passive_package = NULL;
			//err = device->acpi->get_object(device->acpi_cookie, "_PSL", &(att->passive_package));

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


//	#pragma mark - driver module API


static float
acpi_thermal_support(device_node *parent)
{
	const char* bus;
	uint32 device_type;

	// make sure parent is really the ACPI bus manager
	if (sDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false))
		return -1;

	if (strcmp(bus, "acpi"))
		return 0.0;

	// check whether it's really a thermal Device
	if (sDeviceManager->get_attr_uint32(parent, ACPI_DEVICE_TYPE_ITEM,
			&device_type, false) != B_OK || device_type != ACPI_TYPE_THERMAL) {
		return 0.0;
	}

	// TODO check there are _CRT and _TMP ?

	return 0.6;
}


static status_t
acpi_thermal_register_device(device_node *node)
{
	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, { .string = "ACPI Thermal" }},
		{ NULL }
	};

	return sDeviceManager->register_node(node, ACPI_THERMAL_MODULE_NAME, attrs,
		NULL, NULL);
}


static status_t
acpi_thermal_init_driver(device_node* node, void** _driverCookie)
{
	*_driverCookie = node;
	return B_OK;
}


static void
acpi_thermal_uninit_driver(void* driverCookie)
{
}


static status_t
acpi_thermal_register_child_devices(void* _cookie)
{
	device_node* node = _cookie;
	int path_id;
	char name[128];

	path_id = sDeviceManager->create_id(ACPI_THERMAL_PATHID_GENERATOR);
	if (path_id < 0) {
		dprintf("acpi_thermal_register_child_devices: couldn't create a path_id\n");
		return B_ERROR;
	}

	snprintf(name, sizeof(name), ACPI_THERMAL_BASENAME, path_id);

	return sDeviceManager->publish_device(node, name,
		ACPI_THERMAL_DEVICE_MODULE_NAME);
}


static status_t
acpi_thermal_init_device(void* _cookie, void** cookie)
{
	device_node* node = (device_node*)_cookie;
	acpi_thermal_device_info* device;
	device_node* parent;

	device = (acpi_thermal_device_info*)calloc(1, sizeof(*device));
	if (device == NULL)
		return B_NO_MEMORY;

	device->node = node;

	parent = sDeviceManager->get_parent_node(node);
	sDeviceManager->get_driver(parent, (driver_module_info**)&device->acpi,
		(void**)&device->acpi_cookie);
	sDeviceManager->put_node(parent);

	*cookie = device;
	return B_OK;
}


static void
acpi_thermal_uninit_device(void* _cookie)
{
	acpi_thermal_device_info* device = (acpi_thermal_device_info*)_cookie;
	free(device);
}



module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&sDeviceManager },
	{}
};


driver_module_info acpi_thermal_driver_module = {
	{
		ACPI_THERMAL_MODULE_NAME,
		0,
		NULL
	},

	acpi_thermal_support,
	acpi_thermal_register_device,
	acpi_thermal_init_driver,
	acpi_thermal_uninit_driver,
	acpi_thermal_register_child_devices,
	NULL,	// rescan
	NULL,	// removed
};


struct device_module_info acpi_thermal_device_module = {
	{
		ACPI_THERMAL_DEVICE_MODULE_NAME,
		0,
		NULL
	},

	acpi_thermal_init_device,
	acpi_thermal_uninit_device,
	NULL,

	acpi_thermal_open,
	acpi_thermal_close,
	acpi_thermal_free,
	acpi_thermal_read,
	acpi_thermal_write,
	NULL,
	acpi_thermal_control,

	NULL,
	NULL
};

module_info* modules[] = {
	(module_info*)&acpi_thermal_driver_module,
	(module_info*)&acpi_thermal_device_module,
	NULL
};
