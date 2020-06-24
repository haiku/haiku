/*
 * Copyright 2020, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <new>
#include <stdio.h>
#include <string.h>

#include <ACPI.h>
#include <ByteOrder.h>
#include <condition_variable.h>

#include "pch_i2c.h"


typedef struct {
	pch_i2c_sim_info info;
	acpi_device_module_info* acpi;
	acpi_device device;

} pch_i2c_acpi_sim_info;


static status_t
pch_i2c_acpi_set_powerstate(pch_i2c_acpi_sim_info* info, uint8 power)
{
	status_t status = info->acpi->evaluate_method(info->device,
		power == 1 ? "_PS0" : "_PS3", NULL, NULL);
	return status;
}



static acpi_status
pch_i2c_scan_parse_callback(ACPI_RESOURCE *res, void *context)
{
	struct pch_i2c_crs* crs = (struct pch_i2c_crs*)context;

	if (res->Type == ACPI_RESOURCE_TYPE_IRQ) {
		crs->irq = res->Data.Irq.Interrupts[0];
		crs->irq_triggering = res->Data.Irq.Triggering;
		crs->irq_polarity = res->Data.Irq.Polarity;
		crs->irq_sharable = res->Data.Irq.Sharable;
	} else if (res->Type == ACPI_RESOURCE_TYPE_EXTENDED_IRQ) {
		crs->irq = res->Data.ExtendedIrq.Interrupts[0];
		crs->irq_triggering = res->Data.ExtendedIrq.Triggering;
		crs->irq_polarity = res->Data.ExtendedIrq.Polarity;
		crs->irq_sharable = res->Data.ExtendedIrq.Sharable;
	} else if (res->Type == ACPI_RESOURCE_TYPE_FIXED_MEMORY32) {
		crs->addr_bas = res->Data.FixedMemory32.Address;
		crs->addr_len = res->Data.FixedMemory32.AddressLength;
	}

	return B_OK;
}


//	#pragma mark -


static status_t
acpi_scan_bus(i2c_bus_cookie cookie)
{
	CALLED();
	pch_i2c_acpi_sim_info* bus = (pch_i2c_acpi_sim_info*)cookie;

	bus->acpi->walk_namespace(bus->device, ACPI_TYPE_DEVICE, 1,
		pch_i2c_scan_bus_callback, NULL, bus, NULL);

	return B_OK;
}


static status_t
register_child_devices(void* cookie)
{
	CALLED();

	pch_i2c_acpi_sim_info* bus = (pch_i2c_acpi_sim_info*)cookie;
	device_node* node = bus->info.driver_node;

	char prettyName[25];
	sprintf(prettyName, "PCH I2C Controller");

	device_attr attrs[] = {
		// properties of this controller for i2c bus manager
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE,
			{ string: prettyName }},
		{ B_DEVICE_FIXED_CHILD, B_STRING_TYPE,
			{ string: I2C_FOR_CONTROLLER_MODULE_NAME }},

		// private data to identify the device
		{ NULL }
	};

	return gDeviceManager->register_node(node, PCH_I2C_SIM_MODULE_NAME,
		attrs, NULL, NULL);
}


static status_t
init_device(device_node* node, void** device_cookie)
{
	CALLED();
	status_t status = B_OK;

	pch_i2c_acpi_sim_info* bus = (pch_i2c_acpi_sim_info*)calloc(1,
		sizeof(pch_i2c_acpi_sim_info));
	if (bus == NULL)
		return B_NO_MEMORY;

	acpi_device_module_info* acpi;
	acpi_device device;
	{
		device_node* acpiParent = gDeviceManager->get_parent_node(node);
		gDeviceManager->get_driver(acpiParent, (driver_module_info**)&acpi,
			(void**)&device);
		gDeviceManager->put_node(acpiParent);
	}

	bus->acpi = acpi;
	bus->device = device;
	bus->info.driver_node = node;
	bus->info.scan_bus = acpi_scan_bus;

	// Attach devices for I2C resources
	struct pch_i2c_crs crs;
	status = acpi->walk_resources(device, (ACPI_STRING)"_CRS",
		pch_i2c_scan_parse_callback, &crs);
	if (status != B_OK) {
		ERROR("Error while getting I2C devices\n");
		free(bus);
		return status;
	}
	if (crs.addr_bas == 0 || crs.addr_len == 0) {
		TRACE("skipping non configured I2C devices\n");
		free(bus);
		return B_BAD_VALUE;
	}

	bus->info.base_addr = crs.addr_bas;
	bus->info.map_size = crs.addr_len;
	bus->info.irq = crs.irq;

	pch_i2c_acpi_set_powerstate(bus, 1);

	*device_cookie = bus;
	return B_OK;
}


static void
uninit_device(void* device_cookie)
{
	pch_i2c_acpi_sim_info* bus = (pch_i2c_acpi_sim_info*)device_cookie;
	free(bus);
}


static status_t
register_device(device_node* parent)
{
	device_attr attrs[] = {
		{B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {string: "PCH I2C ACPI"}},
		{}
	};

	return gDeviceManager->register_node(parent,
		PCH_I2C_ACPI_DEVICE_MODULE_NAME, attrs, NULL, NULL);
}


static float
supports_device(device_node* parent)
{
	CALLED();
	const char* bus;

	// make sure parent is a PCH I2C ACPI device node
	if (gDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false)
		< B_OK) {
		return -1;
	}

	if (strcmp(bus, "acpi") != 0)
		return 0.0f;

	TRACE("found an acpi node\n");

	// check whether it's really a device
	uint32 device_type;
	if (gDeviceManager->get_attr_uint32(parent, ACPI_DEVICE_TYPE_ITEM,
			&device_type, false) != B_OK
		|| device_type != ACPI_TYPE_DEVICE) {
		return 0.0;
	}
	TRACE("found an acpi device\n");

	// check whether it's a PCH I2C device
	const char *name;
	if (gDeviceManager->get_attr_string(parent, ACPI_DEVICE_HID_ITEM, &name,
		false) != B_OK) {
		return 0.0;
	}
	TRACE("found an acpi device hid %s\n", name);

	if (strcmp(name, "INT33C2") == 0
		|| strcmp(name, "INT33C3") == 0
		|| strcmp(name, "INT3432") == 0
		|| strcmp(name, "INT3433") == 0
		|| strcmp(name, "INT3442") == 0
		|| strcmp(name, "INT3443") == 0
		|| strcmp(name, "INT3444") == 0
		|| strcmp(name, "INT3445") == 0
		|| strcmp(name, "INT3446") == 0
		|| strcmp(name, "INT3447") == 0
		|| strcmp(name, "80860AAC") == 0
		|| strcmp(name, "80865AAC") == 0
		|| strcmp(name, "80860F41") == 0
		|| strcmp(name, "808622C1") == 0) {
		TRACE("PCH I2C device found! name %s\n", name);
		return 0.6f;
	}

	return 0.0f;
}


//	#pragma mark -


driver_module_info gPchI2cAcpiDevice = {
	{
		PCH_I2C_ACPI_DEVICE_MODULE_NAME,
		0,
		NULL
	},

	supports_device,
	register_device,
	init_device,
	uninit_device,
	register_child_devices,
	NULL,	// rescan
	NULL,	// device removed
};


