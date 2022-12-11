/*
 * Copyright 2022, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <new>
#include <stdio.h>
#include <string.h>

#include <ACPI.h>
#include <ByteOrder.h>
#include <condition_variable.h>

extern "C" {
#	include "acpi.h"
}


#define DRIVER_NAME "ccp_rng_acpi"
#include "ccp.h"


typedef struct {
	ccp_device_info info;
	acpi_device_module_info* acpi;
	acpi_device device;

} ccp_acpi_sim_info;


struct ccp_crs {
	uint32	addr_bas;
	uint32	addr_len;
};


static acpi_status
ccp_scan_parse_callback(ACPI_RESOURCE *res, void *context)
{
	struct ccp_crs* crs = (struct ccp_crs*)context;

	if (res->Type == ACPI_RESOURCE_TYPE_FIXED_MEMORY32) {
		crs->addr_bas = res->Data.FixedMemory32.Address;
		crs->addr_len = res->Data.FixedMemory32.AddressLength;
	}

	return B_OK;
}


//	#pragma mark -


static status_t
init_device(device_node* node, void** device_cookie)
{
	CALLED();
	status_t status = B_OK;

	ccp_acpi_sim_info* bus = (ccp_acpi_sim_info*)calloc(1,
		sizeof(ccp_acpi_sim_info));
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

	struct ccp_crs crs;
	status = acpi->walk_resources(device, (ACPI_STRING)"_CRS",
		ccp_scan_parse_callback, &crs);
	if (status != B_OK) {
		ERROR("Error while getting resouces\n");
		free(bus);
		return status;
	}
	if (crs.addr_bas == 0 || crs.addr_len == 0) {
		TRACE("skipping non configured CCP devices\n");
		free(bus);
		return B_BAD_VALUE;
	}

	bus->info.base_addr = crs.addr_bas;
	bus->info.map_size = crs.addr_len;

	*device_cookie = bus;
	return B_OK;
}


static void
uninit_device(void* device_cookie)
{
	ccp_acpi_sim_info* bus = (ccp_acpi_sim_info*)device_cookie;
	free(bus);
}


static status_t
register_device(device_node* parent)
{
	device_attr attrs[] = {
		{B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {.string = "CCP ACPI"}},
		{B_DEVICE_BUS, B_STRING_TYPE, {.string = "CCP"}},
		{B_DEVICE_FIXED_CHILD, B_STRING_TYPE, {.string = CCP_DEVICE_MODULE_NAME}},
		{}
	};

	return gDeviceManager->register_node(parent,
		CCP_ACPI_DEVICE_MODULE_NAME, attrs, NULL, NULL);
}


static float
supports_device(device_node* parent)
{
	const char* bus;

	// make sure parent is a CCP ACPI device node
	if (gDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false)
		< B_OK) {
		return -1;
	}

	if (strcmp(bus, "acpi") != 0)
		return 0.0f;

	// check whether it's really a device
	uint32 device_type;
	if (gDeviceManager->get_attr_uint32(parent, ACPI_DEVICE_TYPE_ITEM,
			&device_type, false) != B_OK
		|| device_type != ACPI_TYPE_DEVICE) {
		return 0.0;
	}

	// check whether it's a CCP device
	const char *name;
	if (gDeviceManager->get_attr_string(parent, ACPI_DEVICE_HID_ITEM, &name,
		false) != B_OK) {
		return 0.0;
	}

	if (strcmp(name, "AMDI0C00") == 0) {
		TRACE("CCP device found! name %s\n", name);
		return 0.6f;
	}

	return 0.0f;
}


//	#pragma mark -


driver_module_info gCcpAcpiDevice = {
	{
		CCP_ACPI_DEVICE_MODULE_NAME,
		0,
		NULL
	},

	supports_device,
	register_device,
	init_device,
	uninit_device,
	NULL,	// register_child_devices,
	NULL,	// rescan
	NULL,	// device removed
};


