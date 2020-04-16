/*
 * Copyright 2020, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include "I2CPrivate.h"


device_manager_info *gDeviceManager = NULL;


//	#pragma mark -


status_t
i2c_added_device(device_node *parent)
{
	CALLED();

	int32 pathID = gDeviceManager->create_id(I2C_PATHID_GENERATOR);
	if (pathID < 0) {
		ERROR("cannot register i2c controller - out of path IDs\n");
		return B_ERROR;
	}

	device_attr attributes[] = {
		// info about device
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, { string: "I2C bus" }},
		{ B_DEVICE_BUS, B_STRING_TYPE, { string: "i2c" }},
		{ I2C_BUS_PATH_ID_ITEM, B_UINT8_TYPE, { ui8: (uint8)pathID }},
		{ NULL }
	};

	TRACE("i2c_added_device parent %p\n", parent);

	return gDeviceManager->register_node(parent, I2C_BUS_MODULE_NAME,
		attributes, NULL, NULL);
}


status_t
i2c_register_device(i2c_bus _bus, i2c_addr slaveAddress, char* hid,
	char** cid, acpi_handle acpiHandle)
{
	CALLED();
	I2CBus* bus = (I2CBus*)_bus;
	return bus->RegisterDevice(slaveAddress, hid, cid, acpiHandle);
}


static status_t
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;

		default:
			break;
	}

	return B_ERROR;
}


//	#pragma mark -


i2c_for_controller_interface gI2CForControllerModule = {
	{
		{
			I2C_FOR_CONTROLLER_MODULE_NAME,
			0,
			&std_ops
		},

		NULL, // supported devices
		i2c_added_device,
		NULL,
		NULL,
		NULL
	},

	i2c_register_device,
};


module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&gDeviceManager },
	{}
};


module_info *modules[] = {
	(module_info *)&gI2CForControllerModule,
	(module_info *)&gI2CBusModule,
	(module_info *)&gI2CDeviceModule,
	(module_info *)&gI2CBusRawModule,
	NULL
};

