/*
 * Copyright 2025, Jérôme Duval, jerome.duval@gmail.com.
 *
 * Distributed under the terms of the MIT License.
 *
 * AMD Thermal driver.
 */


#include <Drivers.h>
#include <Errors.h>
#include <KernelExport.h>
#include <PCI.h>
#include <bus/PCI.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cpu.h>

#include "amd_thermal.h"


#define AMD_THERMAL_MODULE_NAME "drivers/power/amd_thermal/driver_v1"

#define AMD_THERMAL_DEVICE_MODULE_NAME "drivers/power/amd_thermal/device_v1"

/* Base Namespace devices are published to */
#define AMD_THERMAL_BASENAME "power/amd_thermal/%d"

// name of pnp generator of path ids
#define AMD_THERMAL_PATHID_GENERATOR "amd_thermal/path_id"

static device_manager_info* sDeviceManager;


//#define TRACE_AMD_THERMAL
#ifdef TRACE_AMD_THERMAL
#	define TRACE(x...) dprintf("amd_thermal: " x)
#else
#	define TRACE(x...)
#endif
#define ERROR(x...)	dprintf("amd_thermal: " x)


typedef struct amd_thermal_device_info {
	device_node* node;
	pci_device_module_info* pci;
	pci_device* pci_cookie;

	uint32 ccdOffset;
	uint32 tctlOffset;
	uint32 ccdValid;
} amd_thermal_device_info;


status_t amd_thermal_control(void* _cookie, uint32 op, void* arg, size_t len);


static uint32
ksmn_read_reg(amd_thermal_device_info* device, uint32 address)
{
	device->pci->write_pci_config(device->pci_cookie, AMD_SMN_17H_ADDR, 4, address);
	return device->pci->read_pci_config(device->pci_cookie, AMD_SMN_17H_DATA, 4);
}


static void
ksmn_ccd_init(amd_thermal_device_info* device, int ccpCount)
{
	for (int i = 0; i < ccpCount; i++) {
		uint32 reg = ksmn_read_reg(device, AMD_SMU_17H_CCD_THM(device->ccdOffset, i));
		if ((reg & CURTMP_CCD_VALID) == 0)
			continue;
		device->ccdValid |= (1 << i);
	}
}


static status_t
amd_thermal_open(void* _cookie, const char* path, int flags, void** cookie)
{
	amd_thermal_device_info* device = (amd_thermal_device_info*)_cookie;
	TRACE("amd_thermal_open %p\n", device);
	*cookie = device;
	return B_OK;
}


static status_t
amd_thermal_read(void* _cookie, off_t position, void* buf, size_t* num_bytes)
{
	amd_thermal_device_info* device = (amd_thermal_device_info*)_cookie;
	TRACE("amd_thermal_read %p\n", device);
	amd_thermal_type therm_info;
	if (*num_bytes < 1)
		return B_IO_ERROR;

	if (position == 0) {
		char buffer[128];
		TRACE("amd_thermal: read()\n");
		amd_thermal_control(device, drvOpGetThermalType, &therm_info, 0);

		int32 copied = snprintf(buffer, sizeof(buffer),
			"  Current Temperature: %" B_PRIu32 ".%" B_PRIu32 " C\n",
			(therm_info.current_temp / 10), (therm_info.current_temp % 10));

		copied = min_c((int32)*num_bytes, copied + 1);
		if (user_memcpy((char*)buf, buffer, copied) != B_OK)
			return B_BAD_ADDRESS;
		*num_bytes = copied;
	} else {
		*num_bytes = 0;
	}

	return B_OK;
}


static status_t
amd_thermal_write(void* cookie, off_t position, const void* buffer, size_t* num_bytes)
{
	return B_ERROR;
}


status_t
amd_thermal_control(void* _cookie, uint32 op, void* arg, size_t len)
{
	amd_thermal_device_info* device = (amd_thermal_device_info*)_cookie;
	status_t err = B_ERROR;

	amd_thermal_type* att = NULL;

	switch (op) {
		case drvOpGetThermalType:
		{
			att = (amd_thermal_type*)arg;

			// Read basic temperature thresholds.
			uint32 data = ksmn_read_reg(device, AMD_SMU_17H_THM);
			uint16 raw = GET_CURTMP(data);
			int32 offset = 0;
			if ((data & CURTMP_17H_RANGE_SELECTION) != 0)
				offset -= CURTMP_17H_RANGE_ADJUST;
			offset -= device->tctlOffset;
			offset *= 100000;
			// convert to microCelsius then offset
			uint32 value = raw * 125000 + offset;
			// convert to tenth of Celsius
			att->current_temp = value / 100000;
			err = B_OK;
			break;
		}
	}
	return err;
}


static status_t
amd_thermal_close(void* cookie)
{
	return B_OK;
}


static status_t
amd_thermal_free(void* cookie)
{
	return B_OK;
}


//	#pragma mark - driver module API


static float
amd_thermal_support(device_node* parent)
{
	const char* bus;

	// make sure parent is really the PCI bus manager
	if (sDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false))
		return -1;

	if (strcmp(bus, "pci"))
		return 0.0;

	uint16 vendorID;
	uint16 deviceID;

	// get vendor and device ID
	if (sDeviceManager->get_attr_uint16(parent, B_DEVICE_VENDOR_ID, &vendorID, false) != B_OK
		|| sDeviceManager->get_attr_uint16(parent, B_DEVICE_ID, &deviceID, false) != B_OK) {
		return -1;
	}

	// check whether it's really an AMD thermal device
	if (vendorID != 0x1022)
		return 0.0;

	const uint16 devices[]
		= {0x1450, 0x1480, 0x15d0, 0x1630, 0x14a4, 0x14b5, 0x14d8, 0x14e8, 0x1507, 0};
	for (const uint16* device = devices; *device != 0; device++) {
		if (*device == deviceID)
			return 0.6;
	}

	return 0.0;
}


static status_t
amd_thermal_register_device(device_node* node)
{
	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, { .string = "AMD Thermal" }},
		{ NULL }
	};

	return sDeviceManager->register_node(node, AMD_THERMAL_MODULE_NAME, attrs, NULL, NULL);
}


static status_t
amd_thermal_init_driver(device_node* node, void** _driverCookie)
{
	*_driverCookie = node;

	return B_OK;
}


static void
amd_thermal_uninit_driver(void* driverCookie)
{
}


static status_t
amd_thermal_register_child_devices(void* _cookie)
{
	device_node* node = (device_node*)_cookie;
	int path_id;
	char name[128];

	path_id = sDeviceManager->create_id(AMD_THERMAL_PATHID_GENERATOR);
	if (path_id < 0) {
		ERROR("amd_thermal_register_child_devices: couldn't create a path_id\n");
		return B_ERROR;
	}

	snprintf(name, sizeof(name), AMD_THERMAL_BASENAME, path_id);

	return sDeviceManager->publish_device(node, name, AMD_THERMAL_DEVICE_MODULE_NAME);
}


static status_t
amd_thermal_init_device(void* _cookie, void** cookie)
{
	device_node* node = (device_node*)_cookie;
	amd_thermal_device_info* device;

	device = (amd_thermal_device_info*)calloc(1, sizeof(*device));
	if (device == NULL)
		return B_NO_MEMORY;

	device->node = node;
	device->tctlOffset = 0;
	device->ccdValid = 0;

	{
		device_node* parent = sDeviceManager->get_parent_node(node);
		sDeviceManager->get_driver(parent, (driver_module_info**)&device->pci,
			(void**)&device->pci_cookie);
		sDeviceManager->put_node(parent);
	}

	if (gCPU[0].arch.family == 0x17 || gCPU[0].arch.family == 0x18) {
		switch (gCPU[0].arch.model) {
			case 0x1:
			case 0x8:
			case 0x11:
			case 0x18:
				device->ccdOffset = 0x154;
				ksmn_ccd_init(device, 4);
				break;
			case 0x31:
			case 0x60:
			case 0x68:
			case 0x71:
				device->ccdOffset = 0x154;
				ksmn_ccd_init(device, 8);
				break;
			case 0xa0 ... 0xaf:
				device->ccdOffset = 0x300;
				ksmn_ccd_init(device, 8);
				break;
		}
	} else if (gCPU[0].arch.family == 0x19) {
		switch (gCPU[0].arch.model) {
			case 0x0 ... 0x1:
			case 0x8:
			case 0x21:
			case 0x50 ... 0x5f:
				device->ccdOffset = 0x154;
				ksmn_ccd_init(device, 8);
				break;
			case 0x60 ... 0x6f:
			case 0x70 ... 0x7f:
				device->ccdOffset = 0x308;
				ksmn_ccd_init(device, 8);
				break;
			case 0x10 ... 0x1f:
			case 0xa0 ... 0xaf:
				device->ccdOffset = 0x300;
				ksmn_ccd_init(device, 12);
				break;
		}
	}
	TRACE("amd_thermal_init_device %p\n", device);

	*cookie = device;
	return B_OK;
}


static void
amd_thermal_uninit_device(void* _cookie)
{
	amd_thermal_device_info* device = (amd_thermal_device_info*)_cookie;
	TRACE("amd_thermal_uninit_device %p\n", device);
	free(device);
}



module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&sDeviceManager },
	{}
};


driver_module_info amd_thermal_driver_module = {
	{
		AMD_THERMAL_MODULE_NAME,
		0,
		NULL
	},

	amd_thermal_support,
	amd_thermal_register_device,
	amd_thermal_init_driver,
	amd_thermal_uninit_driver,
	amd_thermal_register_child_devices,
	NULL,	// rescan
	NULL,	// removed
};


struct device_module_info amd_thermal_device_module = {
	{
		AMD_THERMAL_DEVICE_MODULE_NAME,
		0,
		NULL
	},

	amd_thermal_init_device,
	amd_thermal_uninit_device,
	NULL,

	amd_thermal_open,
	amd_thermal_close,
	amd_thermal_free,
	amd_thermal_read,
	amd_thermal_write,
	NULL,
	amd_thermal_control,

	NULL,
	NULL
};

module_info *modules[] = {
	(module_info *)&amd_thermal_driver_module,
	(module_info *)&amd_thermal_device_module,
	NULL
};
