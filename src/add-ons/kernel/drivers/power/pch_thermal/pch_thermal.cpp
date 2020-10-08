/*
 * Copyright 2020, Jérôme Duval, jerome.duval@gmail.com.
 *
 * Distributed under the terms of the MIT License.
 *
 * PCH Thermal driver.
 */


#include <AreaKeeper.h>
#include <Drivers.h>
#include <Errors.h>
#include <KernelExport.h>
#include <PCI.h>
#include <bus/PCI.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pch_thermal.h"


#define PCH_THERMAL_MODULE_NAME "drivers/power/pch_thermal/driver_v1"

#define PCH_THERMAL_DEVICE_MODULE_NAME "drivers/power/pch_thermal/device_v1"

/* Base Namespace devices are published to */
#define PCH_THERMAL_BASENAME "power/pch_thermal/%d"

// name of pnp generator of path ids
#define PCH_THERMAL_PATHID_GENERATOR "pch_thermal/path_id"

static device_manager_info *sDeviceManager;


//#define TRACE_PCH_THERMAL
#ifdef TRACE_PCH_THERMAL
#	define TRACE(x...) dprintf("pch_thermal: " x)
#else
#	define TRACE(x...)
#endif
#define ERROR(x...)	dprintf("pch_thermal: " x)


#define write8(address, data) \
	(*((volatile uint8*)(address)) = (data))
#define read8(address) \
	(*((volatile uint8*)(address)))
#define write16(address, data) \
	(*((volatile uint16*)(address)) = (data))
#define read16(address) \
	(*((volatile uint16*)(address)))
#define write32(address, data) \
	(*((volatile uint32*)(address)) = (data))
#define read32(address) \
	(*((volatile uint32*)(address)))


typedef struct pch_thermal_device_info {
	device_node *node;
	pci_device_module_info *pci;
	pci_device *pci_cookie;

	addr_t		registers;
	area_id		registers_area;

	uint32	criticalTemp;
	uint32	hotTemp;
	uint32	passiveTemp;
} pch_thermal_device_info;


status_t pch_thermal_control(void* _cookie, uint32 op, void* arg, size_t len);


static status_t
pch_thermal_open(void *_cookie, const char *path, int flags, void** cookie)
{
	pch_thermal_device_info *device = (pch_thermal_device_info *)_cookie;
	TRACE("pch_thermal_open %p\n", device);
	*cookie = device;
	return B_OK;
}


static status_t
pch_thermal_read(void* _cookie, off_t position, void *buf, size_t* num_bytes)
{
	pch_thermal_device_info* device = (pch_thermal_device_info*)_cookie;
	TRACE("pch_thermal_read %p\n", device);
	pch_thermal_type therm_info;
	if (*num_bytes < 1)
		return B_IO_ERROR;

	if (position == 0) {
		size_t max_len = *num_bytes;
		char *str = (char *)buf;
		TRACE("pch_thermal: read()\n");
		pch_thermal_control(device, drvOpGetThermalType, &therm_info, 0);

		snprintf(str, max_len, "  Critical Temperature: %" B_PRIu32 ".%"
			B_PRIu32 " C\n", (therm_info.critical_temp / 10),
			(therm_info.critical_temp % 10));

		max_len -= strlen(str);
		str += strlen(str);
		snprintf(str, max_len, "  Current Temperature: %" B_PRIu32 ".%"
			B_PRIu32 " C\n", (therm_info.current_temp / 10),
			(therm_info.current_temp % 10));

		if (therm_info.hot_temp > 0) {
			max_len -= strlen(str);
			str += strlen(str);
			snprintf(str, max_len, "  Hot Temperature: %" B_PRIu32 ".%"
				B_PRIu32 " C\n", (therm_info.hot_temp / 10),
				(therm_info.hot_temp % 10));
		}
		*num_bytes = strlen((char *)buf);
	} else {
		*num_bytes = 0;
	}

	return B_OK;
}


static status_t
pch_thermal_write(void* cookie, off_t position, const void* buffer,
	size_t* num_bytes)
{
	return B_ERROR;
}


status_t
pch_thermal_control(void* _cookie, uint32 op, void* arg, size_t len)
{
	pch_thermal_device_info* device = (pch_thermal_device_info*)_cookie;
	status_t err = B_ERROR;

	pch_thermal_type *att = NULL;

	switch (op) {
		case drvOpGetThermalType: {
			att = (pch_thermal_type *)arg;

			// Read basic temperature thresholds.
			att->critical_temp = device->criticalTemp;
			att->hot_temp = device->hotTemp;

			uint16 temp = read16(device->registers + PCH_THERMAL_TEMP);
			temp = (temp >> PCH_THERMAL_TEMP_TSR_SHIFT)
				& PCH_THERMAL_TEMP_TSR_MASK;
			att->current_temp = (uint32)temp * 10 / 2 - 500;

			err = B_OK;
			break;
		}
	}
	return err;
}


static status_t
pch_thermal_close (void* cookie)
{
	return B_OK;
}


static status_t
pch_thermal_free (void* cookie)
{
	return B_OK;
}


//	#pragma mark - driver module API


static float
pch_thermal_support(device_node *parent)
{
	const char *bus;

	// make sure parent is really the PCI bus manager
	if (sDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false))
		return -1;

	if (strcmp(bus, "pci"))
		return 0.0;

	uint16 vendorID;
	uint16 deviceID;

	// get vendor and device ID
	if (sDeviceManager->get_attr_uint16(parent, B_DEVICE_VENDOR_ID, &vendorID,
			false) != B_OK
		|| sDeviceManager->get_attr_uint16(parent, B_DEVICE_ID, &deviceID,
			false) != B_OK) {
		return -1;
	}

	// check whether it's really a PCH thermal device
	if (vendorID != 0x8086)
		return 0.0;

	const uint16 devices[] = { 0x9c24, 0x8c24, 0x9ca4, 0x9d31, 0xa131, 0x9df9,
		0xa379, 0x06f9, 0x02f9, 0 };
	for (const uint16* device = devices; *device != 0; device++) {
		if (*device == deviceID)
			return 0.6;
	}

	return 0.0;
}


static status_t
pch_thermal_register_device(device_node *node)
{
	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, { string: "PCH Thermal" }},
		{ NULL }
	};

	return sDeviceManager->register_node(node, PCH_THERMAL_MODULE_NAME, attrs,
		NULL, NULL);
}


static status_t
pch_thermal_init_driver(device_node *node, void **_driverCookie)
{
	*_driverCookie = node;

	return B_OK;
}


static void
pch_thermal_uninit_driver(void *driverCookie)
{
}


static status_t
pch_thermal_register_child_devices(void *_cookie)
{
	device_node *node = (device_node*)_cookie;
	int path_id;
	char name[128];

	path_id = sDeviceManager->create_id(PCH_THERMAL_PATHID_GENERATOR);
	if (path_id < 0) {
		ERROR("pch_thermal_register_child_devices: couldn't create a path_id"
			"\n");
		return B_ERROR;
	}

	snprintf(name, sizeof(name), PCH_THERMAL_BASENAME, path_id);

	return sDeviceManager->publish_device(node, name,
		PCH_THERMAL_DEVICE_MODULE_NAME);
}


static status_t
pch_thermal_init_device(void *_cookie, void **cookie)
{
	device_node *node = (device_node *)_cookie;
	pch_thermal_device_info *device;
	device_node *parent;
	status_t status = B_NO_INIT;

	device = (pch_thermal_device_info *)calloc(1, sizeof(*device));
	if (device == NULL)
		return B_NO_MEMORY;

	device->node = node;

	parent = sDeviceManager->get_parent_node(node);
	sDeviceManager->get_driver(parent, (driver_module_info **)&device->pci,
		(void **)&device->pci_cookie);
	sDeviceManager->put_node(parent);

	struct pci_info info;
	device->pci->get_pci_info(device->pci_cookie, &info);

	// map the registers (low + high for 64-bit when requested)
	phys_addr_t physicalAddress = info.u.h0.base_registers[0];
	if ((info.u.h0.base_register_flags[0] & PCI_address_type)
			== PCI_address_type_64) {
		physicalAddress |= (uint64)info.u.h0.base_registers[1] << 32;
	}

	size_t mapSize = info.u.h0.base_register_sizes[0];

	AreaKeeper mmioMapper;
	device->registers_area = mmioMapper.Map("intel PCH thermal mmio",
		physicalAddress, mapSize, B_ANY_KERNEL_ADDRESS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, (void**)&device->registers);

	if (mmioMapper.InitCheck() < B_OK) {
		ERROR("could not map memory I/O!\n");
		status = device->registers_area;
		goto err;
	}

	{
		bool enabled = false;
		uint8 tsel = read8(device->registers + PCH_THERMAL_TSEL);
		if ((tsel & PCH_THERMAL_TSEL_ETS) != 0)
			enabled = true;
		if (!enabled) {
			if ((tsel & PCH_THERMAL_TSEL_PLDB) != 0) {
				status = B_DEVICE_NOT_FOUND;
				goto err;
			}

			write8(device->registers + PCH_THERMAL_TSEL,
				tsel | PCH_THERMAL_TSEL_ETS);

			if ((tsel & PCH_THERMAL_TSEL_ETS) == 0) {
				status = B_DEVICE_NOT_FOUND;
				goto err;
			}
		}

		// read config temperatures
		uint16 ctt = read16(device->registers + PCH_THERMAL_CTT);
		ctt = (ctt >> PCH_THERMAL_CTT_CTRIP_SHIFT) & PCH_THERMAL_CTT_CTRIP_MASK;
		device->criticalTemp = (ctt != 0) ? (uint32)ctt * 10 / 2 - 500 : 0;

		uint16 phl = read16(device->registers + PCH_THERMAL_PHL);
		phl = (phl >> PCH_THERMAL_PHL_PHLL_SHIFT) & PCH_THERMAL_PHL_PHLL_MASK;
		device->hotTemp = (phl != 0) ? (uint32)phl * 10 / 2 - 500 : 0;
	}
	TRACE("pch_thermal_init_device %p\n", device);

	mmioMapper.Detach();
	*cookie = device;
	return B_OK;

err:
	free(device);
	return status;
}


static void
pch_thermal_uninit_device(void *_cookie)
{
	pch_thermal_device_info *device = (pch_thermal_device_info *)_cookie;
	TRACE("pch_thermal_uninit_device %p\n", device);
	delete_area(device->registers_area);
	free(device);
}



module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&sDeviceManager },
	{}
};


driver_module_info pch_thermal_driver_module = {
	{
		PCH_THERMAL_MODULE_NAME,
		0,
		NULL
	},

	pch_thermal_support,
	pch_thermal_register_device,
	pch_thermal_init_driver,
	pch_thermal_uninit_driver,
	pch_thermal_register_child_devices,
	NULL,	// rescan
	NULL,	// removed
};


struct device_module_info pch_thermal_device_module = {
	{
		PCH_THERMAL_DEVICE_MODULE_NAME,
		0,
		NULL
	},

	pch_thermal_init_device,
	pch_thermal_uninit_device,
	NULL,

	pch_thermal_open,
	pch_thermal_close,
	pch_thermal_free,
	pch_thermal_read,
	pch_thermal_write,
	NULL,
	pch_thermal_control,

	NULL,
	NULL
};

module_info *modules[] = {
	(module_info *)&pch_thermal_driver_module,
	(module_info *)&pch_thermal_device_module,
	NULL
};
