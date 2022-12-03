/*
 * Copyright 2022, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include "ocores_i2c.h"


device_manager_info* gDeviceManager;
i2c_for_controller_interface* gI2c;


i2c_sim_interface gOcoresI2cDriver = {
	.info = {
		.info = {
			.name = OCORES_I2C_DRIVER_MODULE_NAME,
		},
		.supports_device = [](device_node* parent) {
			return OcoresI2c::SupportsDevice(parent);
		},
		.register_device = [](device_node* parent) {
			return OcoresI2c::RegisterDevice(parent);
		},
		.init_driver = [](device_node* node, void** driverCookie) {
			return OcoresI2c::InitDriver(node, *(OcoresI2c**)driverCookie);
		},
		.uninit_driver = [](void* driverCookie) {
			return static_cast<OcoresI2c*>(driverCookie)->UninitDriver();
		},
	},
	.set_i2c_bus = [](i2c_bus_cookie cookie, i2c_bus bus) {
		static_cast<OcoresI2c*>(cookie)->SetI2cBus(bus);
	},
	.exec_command = [](i2c_bus_cookie cookie, i2c_op op,
		i2c_addr slaveAddress, const void *cmdBuffer, size_t cmdLength,
		void* dataBuffer, size_t dataLength) {
		return static_cast<OcoresI2c*>(cookie)->ExecCommand(op, slaveAddress,
			(const uint8*)cmdBuffer, cmdLength, (uint8*)dataBuffer, dataLength);
	},
	.acquire_bus = [](i2c_bus_cookie cookie) {
		return static_cast<OcoresI2c*>(cookie)->AcquireBus();
	},
	.release_bus = [](i2c_bus_cookie cookie) {
		static_cast<OcoresI2c*>(cookie)->ReleaseBus();
	},
};


_EXPORT module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&gDeviceManager },
	{ I2C_FOR_CONTROLLER_MODULE_NAME, (module_info**)&gI2c },
	{}
};

_EXPORT module_info *modules[] = {
	(module_info *)&gOcoresI2cDriver,
	NULL
};
