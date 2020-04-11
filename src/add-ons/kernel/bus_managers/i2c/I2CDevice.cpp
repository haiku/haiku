/*
 * Copyright 2020, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include "I2CPrivate.h"


I2CDevice::I2CDevice(device_node *node, I2CBus* bus, i2c_addr slaveAddress)
	:
	fNode(node),
	fBus(bus),
	fSlaveAddress(slaveAddress)
{
	CALLED();
}


I2CDevice::~I2CDevice()
{
}


status_t
I2CDevice::InitCheck()
{
	return B_OK;
}


status_t
I2CDevice::ExecCommand(i2c_op op, const void *cmdBuffer,
	size_t cmdLength, void* dataBuffer, size_t dataLength)
{
	CALLED();
	return fBus->ExecCommand(op, fSlaveAddress, cmdBuffer, cmdLength,
		dataBuffer, dataLength);
}


status_t
I2CDevice::AcquireBus()
{
	CALLED();
	return fBus->AcquireBus();
}


void
I2CDevice::ReleaseBus()
{
	CALLED();
	fBus->ReleaseBus();
}


static status_t
i2c_init_device(device_node *node, void **_device)
{
	CALLED();
	I2CBus* bus;

	{
		device_node *parent = gDeviceManager->get_parent_node(node);
		gDeviceManager->get_driver(parent, NULL, (void **)&bus);
		gDeviceManager->put_node(parent);
	}

	uint16 slaveAddress;
	if (gDeviceManager->get_attr_uint16(node, I2C_DEVICE_SLAVE_ADDR_ITEM,
		&slaveAddress, false) != B_OK) {
		return B_ERROR;
	}


	I2CDevice *device = new(std::nothrow) I2CDevice(node, bus, slaveAddress);
	if (device == NULL)
		return B_NO_MEMORY;

	status_t result = device->InitCheck();
	if (result != B_OK) {
		ERROR("failed to set up i2c device object\n");
		return result;
	}

	*_device = device;

	return B_OK;
}


static void
i2c_uninit_device(void *_device)
{
	CALLED();
	I2CDevice *device = (I2CDevice *)_device;
	delete device;
}


static void
i2c_device_removed(void *_device)
{
	CALLED();
	//I2CDevice *device = (I2CDevice *)_device;
}


static status_t
i2c_exec_command(i2c_device _device, i2c_op op, const void *cmdBuffer,
	size_t cmdLength, void* dataBuffer, size_t dataLength)
{
	I2CDevice *device = (I2CDevice *)_device;
	return device->ExecCommand(op, cmdBuffer, cmdLength, dataBuffer, dataLength);
}


static status_t
i2c_acquire_bus(i2c_device _device)
{
	I2CDevice *device = (I2CDevice *)_device;
	return device->AcquireBus();
}


static void
i2c_release_bus(i2c_device _device)
{
	I2CDevice *device = (I2CDevice *)_device;
	return device->ReleaseBus();
}


static status_t
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		{
			// Link to I2C bus.
			// I2C device driver must have I2C bus loaded, but it calls its
			// functions directly instead via official interface, so this
			// pointer is never read.
			module_info *dummy;
			return get_module(I2C_BUS_MODULE_NAME, &dummy);
		}
		case B_MODULE_UNINIT:
			return put_module(I2C_BUS_MODULE_NAME);

		default:
			return B_ERROR;
	}
}


i2c_device_interface gI2CDeviceModule = {
	{
		{
			I2C_DEVICE_MODULE_NAME,
			0,
			std_ops
		},

		NULL,	// supported devices
		NULL,	// register node
		i2c_init_device,
		(void (*)(void *)) i2c_uninit_device,
		NULL,	// register child devices
		NULL,	// rescan
		(void (*)(void *)) i2c_device_removed
	},
	i2c_exec_command,
	i2c_acquire_bus,
	i2c_release_bus,
};
