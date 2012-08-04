/*
 * Copyright 2012, Haiku, Inc. All Rights Reserved.
 *
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Yongcong Du <ycdu.vmcore@gmail.com>
 */

#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>
#include <smp.h>
#include <cpu.h>
#include <arch_system_info.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ACPI.h>

#include "x86_cpuidle.h"

#define ACPI_CPUIDLE_MODULE_NAME "drivers/power/x86_cpuidle/acpi/driver_v1"

struct acpi_cpuidle_driver_info {
	device_node *node;
	acpi_device_module_info *acpi;
	acpi_device acpi_cookie;
};

static device_manager_info *sDeviceManager;
static acpi_cpuidle_driver_info *acpi_processor[B_MAX_CPU_COUNT];


static status_t
acpi_cpuidle_init(void)
{
	dprintf("acpi_cpuidle_init\n");

	return B_ERROR;
}


static status_t
acpi_processor_init(acpi_cpuidle_driver_info *device)
{
	status_t status = B_ERROR;

	acpi_data buffer;
	buffer.pointer = NULL;
	buffer.length = ACPI_ALLOCATE_BUFFER;
	dprintf("get acpi processor @%p\n", device->acpi_cookie);
	status = device->acpi->evaluate_method(device->acpi_cookie, NULL, NULL,
		&buffer);
	if (status != B_OK) {
		dprintf("failed to get processor obj\n");
		return B_IO_ERROR;
	}
	acpi_object_type *object = (acpi_object_type *)buffer.pointer;
	dprintf("acpi cpu%"B_PRId32": P_BLK at %#x/%lu\n",
			object->data.processor.cpu_id,
			object->data.processor.pblk_address,
			object->data.processor.pblk_length);
	int32 cpuid = object->data.processor.cpu_id;
	free(buffer.pointer);
	if (cpuid > smp_get_num_cpus())
		return B_ERROR;

	if (smp_get_num_cpus() == 1)
		cpuid = 1;

	acpi_processor[cpuid-1] = device;

	if (cpuid == 1) {
		if (intel_cpuidle_init() != B_OK)
			return acpi_cpuidle_init();
	}

	return B_OK;
}


static float
acpi_cpuidle_support(device_node *parent)
{
	const char *bus;
	uint32 device_type;

	dprintf("acpi_cpuidle_support\n");
	// make sure parent is really the ACPI bus manager
	if (sDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false))
		return -1;

	if (strcmp(bus, "acpi"))
		return 0.0;

	// check whether it's really a cpu Device
	if (sDeviceManager->get_attr_uint32(parent, ACPI_DEVICE_TYPE_ITEM, &device_type, false) != B_OK
		|| device_type != ACPI_TYPE_PROCESSOR) {
		return 0.0;
	}

	return 0.6;
}


static status_t
acpi_cpuidle_register_device(device_node *node)
{
	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, { string: "ACPI CPU IDLE" }},
		{ NULL }
	};

	dprintf("acpi_cpuidle_register_device\n");
	return sDeviceManager->register_node(node, ACPI_CPUIDLE_MODULE_NAME, attrs,
		NULL, NULL);
}


static status_t
acpi_cpuidle_init_driver(device_node *node, void **driverCookie)
{
	dprintf("acpi_cpuidle_init_driver\n");
	acpi_cpuidle_driver_info *device;
	device = (acpi_cpuidle_driver_info *)calloc(1, sizeof(*device));
	if (device == NULL)
		return B_NO_MEMORY;

	*driverCookie = device;

	device->node = node;

	device_node *parent;
	parent = sDeviceManager->get_parent_node(node);
	sDeviceManager->get_driver(parent, (driver_module_info **)&device->acpi,
		(void **)&device->acpi_cookie);
	sDeviceManager->put_node(parent);

	return acpi_processor_init(device);
}


static void
acpi_cpuidle_uninit_driver(void *driverCookie)
{
	dprintf("acpi_cpuidle_uninit_driver");
	acpi_cpuidle_driver_info *device = (acpi_cpuidle_driver_info *)driverCookie;
	free(device);
}


module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&sDeviceManager },
	{}
};


static driver_module_info sAcpiidleModule = {
	{
		ACPI_CPUIDLE_MODULE_NAME,
		0,
		NULL
	},

	acpi_cpuidle_support,
	acpi_cpuidle_register_device,
	acpi_cpuidle_init_driver,
	acpi_cpuidle_uninit_driver,
	NULL,
	NULL,	// rescan
	NULL,	// removed
};


module_info *modules[] = {
	(module_info *)&sAcpiidleModule,
	NULL
};
