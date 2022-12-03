/*
 * Copyright 2022, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include "ocores_i2c.h"
#include <bus/FDT.h>

#include <AutoDeleterDrivers.h>

#include <string.h>
#include <new>


int32
OcoresI2c::InterruptReceived(void* arg)
{
	return static_cast<OcoresI2c*>(arg)->InterruptReceivedInt();
}


int32
OcoresI2c::InterruptReceivedInt()
{
	// TODO: implement interrupt handling, use polling for now
	return B_HANDLED_INTERRUPT;
}


status_t
OcoresI2c::WaitCompletion()
{
	while (!fRegs->status.interrupt) {}
	return B_OK;
}


status_t
OcoresI2c::WriteByte(OcoresI2cRegsCommand cmd, uint8 val)
{
	cmd.intAck = true;
	cmd.write = true;
	//dprintf("OcoresI2c::WriteByte(cmd: %#02x, val: %#02x)\n", cmd.val, val);
	fRegs->data = val;
	fRegs->command.val = cmd.val;
	CHECK_RET(WaitCompletion());
	return B_OK;
}


status_t
OcoresI2c::ReadByte(OcoresI2cRegsCommand cmd, uint8& val)
{
	cmd.intAck = true;
	cmd.read = true;
	cmd.nack = cmd.stop;
	fRegs->command.val = cmd.val;
	CHECK_RET(WaitCompletion());
	val = fRegs->data;
	//dprintf("OcoresI2c::ReadByte(cmd: %#02x, val: %#02x)\n", cmd.val, val);
	return B_OK;
}


status_t
OcoresI2c::WriteAddress(i2c_addr adr, bool isRead)
{
	// TODO: 10 bit address support
	//dprintf("OcoresI2c::WriteAddress(adr: %#04x, isRead: %d)\n", adr, isRead);
	uint8 val = OcoresI2cRegsAddress7{.read = isRead, .address = (uint8)adr}.val;
	CHECK_RET(WriteByte({.start = true}, val));
	return B_OK;
}


//#pragma mark - driver

float
OcoresI2c::SupportsDevice(device_node* parent)
{
	const char* bus;
	status_t status = gDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false);
	if (status < B_OK)
		return -1.0f;

	if (strcmp(bus, "fdt") != 0)
		return 0.0f;

	const char* compatible;
	status = gDeviceManager->get_attr_string(parent, "fdt/compatible", &compatible, false);
	if (status < B_OK)
		return -1.0f;

	if (strcmp(compatible, "opencores,i2c-ocores") != 0)
		return 0.0f;

	return 1.0f;
}


status_t
OcoresI2c::RegisterDevice(device_node* parent)
{
	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {string: "Opencores I2C Controller"} },
		{ B_DEVICE_FIXED_CHILD, B_STRING_TYPE, {string: I2C_FOR_CONTROLLER_MODULE_NAME} },
		{}
	};

	return gDeviceManager->register_node(parent, OCORES_I2C_DRIVER_MODULE_NAME, attrs, NULL, NULL);
}


status_t
OcoresI2c::InitDriver(device_node* node, OcoresI2c*& outDriver)
{
	ObjectDeleter<OcoresI2c> driver(new(std::nothrow) OcoresI2c());
	if (!driver.IsSet())
		return B_NO_MEMORY;

	CHECK_RET(driver->InitDriverInt(node));
	outDriver = driver.Detach();
	return B_OK;
}


status_t
OcoresI2c::InitDriverInt(device_node* node)
{
	fNode = node;
	dprintf("+OcoresI2c::InitDriver()\n");

	DeviceNodePutter<&gDeviceManager> parent(gDeviceManager->get_parent_node(node));

	const char* bus;
	CHECK_RET(gDeviceManager->get_attr_string(parent.Get(), B_DEVICE_BUS, &bus, false));
	if (strcmp(bus, "fdt") != 0)
		return B_ERROR;

	fdt_device_module_info *parentModule;
	fdt_device* parentDev;
	CHECK_RET(gDeviceManager->get_driver(parent.Get(),
		(driver_module_info**)&parentModule, (void**)&parentDev));

	uint64 regs = 0;
	uint64 regsLen = 0;
	if (!parentModule->get_reg(parentDev, 0, &regs, &regsLen))
		return B_ERROR;

	fRegsArea.SetTo(map_physical_memory("Ocores i2c MMIO", regs, regsLen, B_ANY_KERNEL_ADDRESS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, (void**)&fRegs));
	if (!fRegsArea.IsSet())
		return fRegsArea.Get();

	uint64 irq;
	if (!parentModule->get_interrupt(parentDev, 0, NULL, &irq))
		return B_ERROR;
	fIrqVector = irq; // TODO: take interrupt controller into account

	// TODO: enable when implement interrupt handling
	if (false)
		install_io_interrupt_handler(fIrqVector, InterruptReceived, this, 0);

	dprintf("-OcoresI2c::InitDriver()\n");
	return B_OK;
}


void
OcoresI2c::UninitDriver()
{
	if (false)
		remove_io_interrupt_handler(fIrqVector, InterruptReceived, this);
	delete this;
}


//#pragma mark - i2c controller

void
OcoresI2c::SetI2cBus(i2c_bus bus)
{
	dprintf("OcoresI2c::SetI2cBus()\n");
	fBus = bus;
}


status_t
OcoresI2c::ExecCommand(i2c_op op,
	i2c_addr slaveAddress, const uint8 *cmdBuffer, size_t cmdLength,
	uint8* dataBuffer, size_t dataLength)
{
	//dprintf("OcoresI2c::ExecCommand()\n");
	(void)op;
	if (cmdLength > 0) {
		CHECK_RET(WriteAddress(slaveAddress, false));
		do {
			if (fRegs->status.nackReceived) {
				fRegs->command.val = OcoresI2cRegsCommand{
					.intAck = true,
					.stop = true
				}.val;
				return B_ERROR;
			}
			cmdLength--;
			CHECK_RET(WriteByte({.stop = cmdLength == 0 && dataLength == 0}, *cmdBuffer++));
		} while (cmdLength > 0);
	}
	if (dataLength > 0) {
		CHECK_RET(WriteAddress(slaveAddress, true));
		do {
			dataLength--;
			CHECK_RET(ReadByte({.stop = dataLength == 0}, *dataBuffer++));
		} while (dataLength > 0);
	}
	return B_OK;
}


status_t
OcoresI2c::AcquireBus()
{
	return mutex_lock(&fLock);
}


void
OcoresI2c::ReleaseBus()
{
	mutex_unlock(&fLock);
}
