/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 *		Alexander von Gluck, kallisti5@unixzen.com
 */

#include "acpi_battery.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Drivers.h>
#include <Errors.h>
#include <KernelExport.h>

#include <ACPI.h>
#include <condition_variable.h>
#include <kernel/arch/x86/arch_cpu.h>

#define ACPI_BATTERY_DRIVER_NAME "drivers/power/acpi_battery/driver_v1"
#define ACPI_BATTERY_DEVICE_NAME "drivers/power/acpi_battery/device_v1"

/* Base Namespace devices are published to */
#define ACPI_BATTERY_BASENAME "power/acpi_battery/%d"

// name of pnp generator of path ids
#define ACPI_BATTERY_PATHID_GENERATOR "acpi_battery/path_id"

static device_manager_info *sDeviceManager;
static ConditionVariable sBatteryCondition;


status_t
ReadBatteryStatus(battery_driver_cookie* cookie, acpi_battery_info* batteryStatus)
{
	status_t status = B_ERROR;

	acpi_data buffer;
	buffer.pointer = NULL;
	buffer.length = ACPI_ALLOCATE_BUFFER;

	acpi_object_type* object;
	acpi_object_type* pointer;

	status = cookie->acpi->evaluate_method(cookie->acpi_cookie, "_BST", NULL,
		&buffer);
	if (status != B_OK)
		goto exit;

	object = (acpi_object_type*)buffer.pointer;
	if (object->object_type != ACPI_TYPE_PACKAGE
		|| object->data.package.count < 4) {
		status = B_ERROR;
		goto exit;
	}

	pointer = object->data.package.objects;
	batteryStatus->state = (pointer->object_type == ACPI_TYPE_INTEGER)
		? pointer->data.integer : BATTERY_CRITICAL_STATE;

	pointer++;
	batteryStatus->current_rate = (pointer->object_type == ACPI_TYPE_INTEGER)
		? pointer->data.integer : -1;

	pointer++;
	batteryStatus->capacity = (pointer->object_type == ACPI_TYPE_INTEGER)
		? pointer->data.integer : -1;

	pointer++;
	batteryStatus->voltage = (pointer->object_type == ACPI_TYPE_INTEGER)
		? pointer->data.integer : -1;

	/* If key values are all < 0, it is likely that the battery slot is empty
	 * or the battery is damaged.  Set BATTERY_CRITICAL_STATE
	 */
	if (batteryStatus->voltage < 0
		&& batteryStatus->current_rate < 0
		&& batteryStatus->capacity < 0) {
		batteryStatus->state = BATTERY_CRITICAL_STATE;
	}


exit:
	free(buffer.pointer);
	return status;
}


status_t
ReadBatteryInfo(battery_driver_cookie* cookie,
	acpi_extended_battery_info* batteryInfo)
{
	status_t status = B_ERROR;

	acpi_data buffer;
	buffer.pointer = NULL;
	buffer.length = ACPI_ALLOCATE_BUFFER;

	acpi_object_type* object;
	acpi_object_type* pointer;

	status = cookie->acpi->evaluate_method(cookie->acpi_cookie, "_BIF", NULL,
		&buffer);
	if (status != B_OK)
		goto exit;

	object = (acpi_object_type*)buffer.pointer;
	if (object->object_type != ACPI_TYPE_PACKAGE
		|| object->data.package.count < 13) {
		status = B_ERROR;
		goto exit;
	}

	pointer = object->data.package.objects;
	batteryInfo->power_unit = (pointer->object_type == ACPI_TYPE_INTEGER)
		? pointer->data.integer : -1;

	pointer++;
	batteryInfo->design_capacity = (pointer->object_type == ACPI_TYPE_INTEGER)
		? pointer->data.integer : -1;

	pointer++;
	batteryInfo->last_full_charge = (pointer->object_type == ACPI_TYPE_INTEGER)
		? pointer->data.integer : -1;

	pointer++;
	batteryInfo->technology = (pointer->object_type == ACPI_TYPE_INTEGER)
		? pointer->data.integer : -1;

	pointer++;
	batteryInfo->design_voltage = (pointer->object_type == ACPI_TYPE_INTEGER)
		? pointer->data.integer : -1;

	pointer++;
	batteryInfo->design_capacity_warning =
		(pointer->object_type == ACPI_TYPE_INTEGER)
		? pointer->data.integer : -1;

	pointer++;
	batteryInfo->design_capacity_low =
		(pointer->object_type == ACPI_TYPE_INTEGER)
		? pointer->data.integer : -1;

	pointer++;
	batteryInfo->capacity_granularity_1 =
		(pointer->object_type == ACPI_TYPE_INTEGER)
		? pointer->data.integer : -1;

	pointer++;
	batteryInfo->capacity_granularity_2 =
		(pointer->object_type == ACPI_TYPE_INTEGER)
		? pointer->data.integer : -1;

	pointer++;
	strlcpy(batteryInfo->model_number,
		(pointer->object_type == ACPI_TYPE_STRING)
		? pointer->data.string.string : "", sizeof(batteryInfo->model_number));

	pointer++;
	strlcpy(batteryInfo->serial_number,
		(pointer->object_type == ACPI_TYPE_STRING)
		? pointer->data.string.string : "", sizeof(batteryInfo->serial_number));

	pointer++;
	strlcpy(batteryInfo->type, (pointer->object_type == ACPI_TYPE_STRING)
		? pointer->data.string.string : "", sizeof(batteryInfo->type));

	pointer++;
	strlcpy(batteryInfo->oem_info, (pointer->object_type == ACPI_TYPE_STRING)
		? pointer->data.string.string : "", sizeof(batteryInfo->oem_info));

exit:
	free(buffer.pointer);
	return status;
}


int
EstimatedRuntime(battery_driver_cookie* cookie, acpi_battery_info* info)
{
	status_t status = B_ERROR;

	acpi_object_type argument;
	argument.object_type = ACPI_TYPE_INTEGER;
	argument.data.integer = info->current_rate;

	acpi_objects arguments;
	arguments.count = 1;
	arguments.pointer = &argument;

	acpi_object_type object;

	acpi_data buffer;
	buffer.pointer = &object;
	buffer.length = sizeof(object);

	acpi_object_type* returnObject;

	status = cookie->acpi->evaluate_method(cookie->acpi_cookie, "_BTM",
		&arguments,	&buffer);
	if (status != B_OK)
		return -1;

	returnObject = (acpi_object_type*)buffer.pointer;

	if (returnObject->object_type != ACPI_TYPE_INTEGER)
		return -1;

	int result = returnObject->data.integer;

	return result;
}


void
battery_notify_handler(acpi_handle device, uint32 value, void *context)
{
	TRACE("battery_notify_handler event 0x%x\n", int(value));
	sBatteryCondition.NotifyAll();
}


void
TraceBatteryInfo(acpi_extended_battery_info* batteryInfo)
{
	TRACE("BIF power unit %i\n", batteryInfo->power_unit);
	TRACE("BIF design capacity %i\n", batteryInfo->design_capacity);
	TRACE("BIF last full charge %i\n", batteryInfo->last_full_charge);
	TRACE("BIF technology %i\n", batteryInfo->technology);
	TRACE("BIF design voltage %i\n", batteryInfo->design_voltage);
	TRACE("BIF design capacity warning %i\n", batteryInfo->design_capacity_warning);
	TRACE("BIF design capacity low %i\n", batteryInfo->design_capacity_low);
	TRACE("BIF capacity granularity 1 %i\n", batteryInfo->capacity_granularity_1);
	TRACE("BIF capacity granularity 2 %i\n", batteryInfo->capacity_granularity_2);
	TRACE("BIF model number %s\n", batteryInfo->model_number);
	TRACE("BIF serial number %s\n", batteryInfo->serial_number);
	TRACE("BIF type %s\n", batteryInfo->type);
	TRACE("BIF oem info %s\n", batteryInfo->oem_info);
}


static status_t
acpi_battery_open(void *initCookie, const char *path, int flags, void** cookie)
{
	battery_device_cookie *device;
	device = (battery_device_cookie*)calloc(1, sizeof(battery_device_cookie));
	if (device == NULL)
		return B_NO_MEMORY;

	device->driver_cookie = (battery_driver_cookie*)initCookie;
	device->stop_watching = 0;

	*cookie = device;

	return B_OK;
}


static status_t
acpi_battery_close(void* cookie)
{
	battery_device_cookie* device = (battery_device_cookie*)cookie;
	free(device);

	return B_OK;
}


static status_t
acpi_battery_read(void* _cookie, off_t position, void *buffer, size_t* numBytes)
{
	if (*numBytes < 1)
		return B_IO_ERROR;

	battery_device_cookie *device = (battery_device_cookie*)_cookie;

	acpi_battery_info batteryStatus;
	ReadBatteryStatus(device->driver_cookie, &batteryStatus);

	acpi_extended_battery_info batteryInfo;
	ReadBatteryInfo(device->driver_cookie, &batteryInfo);

	if (position == 0) {
		size_t max_len = *numBytes;
		char *str = (char *)buffer;

		snprintf(str, max_len, "Battery Status:\n");
		max_len-= strlen(str);
		str += strlen(str);

		snprintf(str, max_len, " State %i, Current Rate %i, Capacity %i, "
			"Voltage %i\n", batteryStatus.state, batteryStatus.current_rate,
			batteryStatus.capacity,	batteryStatus.voltage);
		max_len-= strlen(str);
		str += strlen(str);

		snprintf(str, max_len, "\nBattery Info:\n");
		max_len-= strlen(str);
		str += strlen(str);

		snprintf(str, max_len, " Power Unit %i, Design Capacity %i, "
			"Last Full Charge %i, Technology %i\n", batteryInfo.power_unit,
			batteryInfo.design_capacity, batteryInfo.last_full_charge,
			batteryInfo.technology);
		max_len-= strlen(str);
		str += strlen(str);
		snprintf(str, max_len, " Design Voltage %i, Design Capacity Warning %i, "
			"Design Capacity Low %i, Capacity Granularity1 %i, "
			"Capacity Granularity1 %i\n", batteryInfo.design_voltage,
			batteryInfo.design_capacity_warning, batteryInfo.design_capacity_low,
			batteryInfo.capacity_granularity_1, batteryInfo.capacity_granularity_1);
		max_len-= strlen(str);
		str += strlen(str);
		snprintf(str, max_len, " Model Number %s, Serial Number %s, "
			"Type %s, OEM Info %s\n", batteryInfo.model_number,
			batteryInfo.serial_number, batteryInfo.type, batteryInfo.oem_info);
		max_len-= strlen(str);
		str += strlen(str);

		*numBytes = strlen((char *)buffer);
	} else
		*numBytes = 0;

	return B_OK;
}


static status_t
acpi_battery_write(void* cookie, off_t position, const void* buffer, size_t* numBytes)
{
	return B_ERROR;
}


status_t
acpi_battery_control(void* _cookie, uint32 op, void* arg, size_t len)
{
	battery_device_cookie* device = (battery_device_cookie*)_cookie;
	status_t err;

	switch (op) {
		case IDENTIFY_DEVICE: {
			if (len < sizeof(uint32))
				return B_BAD_VALUE;

			uint32 magicId = kMagicACPIBatteryID;
			return user_memcpy(arg, &magicId, sizeof(magicId));
		}

		case GET_BATTERY_INFO: {
			if (len < sizeof(acpi_battery_info))
				return B_BAD_VALUE;

			acpi_battery_info batteryInfo;
			err = ReadBatteryStatus(device->driver_cookie, &batteryInfo);
			if (err != B_OK)
				return err;
			return user_memcpy(arg, &batteryInfo, sizeof(batteryInfo));
		}

		case GET_EXTENDED_BATTERY_INFO: {
			if (len < sizeof(acpi_extended_battery_info))
				return B_BAD_VALUE;

			acpi_extended_battery_info extBatteryInfo;
			err = ReadBatteryInfo(device->driver_cookie, &extBatteryInfo);
			if (err != B_OK)
				return err;
			return user_memcpy(arg, &extBatteryInfo, sizeof(extBatteryInfo));
		}

		case WATCH_BATTERY:
			sBatteryCondition.Wait();
			if (atomic_get(&(device->stop_watching))) {
				atomic_set(&(device->stop_watching), 0);
				return B_ERROR;
			}
			return B_OK;

		case STOP_WATCHING_BATTERY:
			atomic_set(&(device->stop_watching), 1);
			sBatteryCondition.NotifyAll();
			return B_OK;
	}

	return B_DEV_INVALID_IOCTL;
}


static status_t
acpi_battery_free(void* cookie)
{
	return B_OK;
}


//	#pragma mark - driver module API


static float
acpi_battery_support(device_node *parent)
{
	// make sure parent is really the ACPI bus manager
	const char *bus;
	uint32 device_type;
	const char *name;

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

	// check whether it's a battery device
	if (sDeviceManager->get_attr_string(parent, ACPI_DEVICE_HID_ITEM, &name,
		false) != B_OK || strcmp(name, ACPI_NAME_BATTERY))
		return 0.0;

	return 0.6;
}


static status_t
acpi_battery_register_device(device_node *node)
{
	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE,
			{ string: "ACPI Battery" }},
		{ NULL }
	};

	return sDeviceManager->register_node(node, ACPI_BATTERY_DRIVER_NAME, attrs,
		NULL, NULL);
}


static status_t
acpi_battery_init_driver(device_node *node, void **driverCookie)
{
	battery_driver_cookie *device;
	device = (battery_driver_cookie *)calloc(1, sizeof(battery_driver_cookie));
	if (device == NULL)
		return B_NO_MEMORY;

	*driverCookie = device;

	device->node = node;

	device_node *parent;
	parent = sDeviceManager->get_parent_node(node);
	sDeviceManager->get_driver(parent, (driver_module_info **)&device->acpi,
		(void **)&device->acpi_cookie);
	sDeviceManager->put_node(parent);

	// install notify handler
	device->acpi->install_notify_handler(device->acpi_cookie,
    	ACPI_ALL_NOTIFY, battery_notify_handler, device);

	return B_OK;
}


static void
acpi_battery_uninit_driver(void *driverCookie)
{
	TRACE("acpi_battery_uninit_driver\n");
	battery_driver_cookie *device = (battery_driver_cookie*)driverCookie;

	device->acpi->remove_notify_handler(device->acpi_cookie,
    	ACPI_ALL_NOTIFY, battery_notify_handler);

	free(device);
}


static status_t
acpi_battery_register_child_devices(void *cookie)
{
	battery_driver_cookie *device = (battery_driver_cookie*)cookie;

	int pathID = sDeviceManager->create_id(ACPI_BATTERY_PATHID_GENERATOR);
	if (pathID < 0) {
		TRACE("register_child_devices: couldn't create a path_id\n");
		return B_ERROR;
	}

	char name[128];
	snprintf(name, sizeof(name), ACPI_BATTERY_BASENAME, pathID);

	return sDeviceManager->publish_device(device->node, name,
		ACPI_BATTERY_DEVICE_NAME);
}


static status_t
acpi_battery_init_device(void *driverCookie, void **cookie)
{
	*cookie = driverCookie;
	return B_OK;
}


static void
acpi_battery_uninit_device(void *_cookie)
{

}



module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&sDeviceManager },
	{}
};


driver_module_info acpi_battery_driver_module = {
	{
		ACPI_BATTERY_DRIVER_NAME,
		0,
		NULL
	},

	acpi_battery_support,
	acpi_battery_register_device,
	acpi_battery_init_driver,
	acpi_battery_uninit_driver,
	acpi_battery_register_child_devices,
	NULL,	// rescan
	NULL,	// removed
};


struct device_module_info acpi_battery_device_module = {
	{
		ACPI_BATTERY_DEVICE_NAME,
		0,
		NULL
	},

	acpi_battery_init_device,
	acpi_battery_uninit_device,
	NULL,

	acpi_battery_open,
	acpi_battery_close,
	acpi_battery_free,
	acpi_battery_read,
	acpi_battery_write,
	NULL,
	acpi_battery_control,

	NULL,
	NULL
};

module_info *modules[] = {
	(module_info *)&acpi_battery_driver_module,
	(module_info *)&acpi_battery_device_module,
	NULL
};
