/*
 * Copyright 2022, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include "X86PCIController.h"

#include <ioapic.h>

#include <AutoDeleterDrivers.h>
#include <util/AutoLock.h>

#include <string.h>
#include <new>


#define PCI_MECH1_REQ_PORT				0xCF8
#define PCI_MECH1_DATA_PORT 			0xCFC
#define PCI_MECH1_REQ_DATA(bus, device, func, offset) \
	(0x80000000 | (bus << 16) | (device << 11) | (func << 8) | (offset & ~3))

#define PCI_MECH2_ENABLE_PORT			0x0cf8
#define PCI_MECH2_FORWARD_PORT			0x0cfa
#define PCI_MECH2_CONFIG_PORT(dev, offset) \
	(uint16)(0xC00 | (dev << 8) | offset)


//#pragma mark - driver


float
X86PCIController::SupportsDevice(device_node* parent)
{
	const char* bus;
	if (gDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false) < B_OK)
		return -1.0f;

	if (strcmp(bus, "root") == 0)
		return 1.0f;

	return 0.0;
}


status_t
X86PCIController::RegisterDevice(device_node* parent)
{
	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {.string = "X86 PCI Host Controller"} },
		{ B_DEVICE_FIXED_CHILD, B_STRING_TYPE, {.string = "bus_managers/pci/root/driver_v1"} },
		{}
	};

	return gDeviceManager->register_node(parent, PCI_X86_DRIVER_MODULE_NAME, attrs, NULL, NULL);
}


status_t
X86PCIController::InitDriver(device_node* node, X86PCIController*& outDriver)
{
	bool search_mech1 = true;
	bool search_mech2 = true;
	bool search_mechpcie = true;
	void *config = NULL;

	config = load_driver_settings("pci");
	if (config) {
		const char *mech = get_driver_parameter(config, "mechanism", NULL, NULL);
		if (mech) {
			search_mech1 = search_mech2 = search_mechpcie = false;
			if (strcmp(mech, "1") == 0)
				search_mech1 = true;
			else if (strcmp(mech, "2") == 0)
				search_mech2 = true;
			else if (strcmp(mech, "pcie") == 0)
				search_mechpcie = true;
			else
				panic("Unknown pci config mechanism setting %s\n", mech);
		}
		unload_driver_settings(config);
	}

	// PCI configuration mechanism PCIe is the preferred one.
	// If it doesn't work, try mechanism 1.
	// If it doesn't work, try mechanism 2.

	if (search_mechpcie) {
		if (CreateDriver(node, new(std::nothrow) X86PCIControllerMethPcie(), outDriver) >= B_OK)
			return B_OK;
	}
	if (search_mech1) {
		if (CreateDriver(node, new(std::nothrow) X86PCIControllerMeth1(), outDriver) >= B_OK)
			return B_OK;
	}
	if (search_mech2) {
		if (CreateDriver(node, new(std::nothrow) X86PCIControllerMeth2(), outDriver) >= B_OK)
			return B_OK;
	}

	dprintf("PCI: no configuration mechanism found\n");
	return B_ERROR;
}


status_t
X86PCIController::CreateDriver(device_node* node, X86PCIController* driverIn,
	X86PCIController*& driverOut)
{
	ObjectDeleter<X86PCIController> driver(driverIn);
	if (!driver.IsSet())
		return B_NO_MEMORY;

	CHECK_RET(driver->InitDriverInt(node));
	driverOut = driver.Detach();
	return B_OK;
}


status_t
X86PCIController::InitDriverInt(device_node* node)
{
	fNode = node;
	return B_OK;
}


void
X86PCIController::UninitDriver()
{
	delete this;
}


//#pragma mark - PCI controller


status_t
X86PCIController::ReadIrq(uint8 bus, uint8 device, uint8 function,
	uint8 pin, uint8& irq)
{
	return B_UNSUPPORTED;
}


status_t
X86PCIController::WriteIrq(uint8 bus, uint8 device, uint8 function,
	uint8 pin, uint8 irq)
{
	return B_UNSUPPORTED;
}


status_t
X86PCIController::GetRange(uint32 index, pci_resource_range* range)
{
	return B_BAD_INDEX;
}


status_t
X86PCIController::Finalize()
{
	ioapic_init();
	return B_OK;
}


//#pragma mark - X86PCIControllerMeth1


status_t
X86PCIControllerMeth1::InitDriverInt(device_node* node)
{
	CHECK_RET(X86PCIController::InitDriverInt(node));

	// check for mechanism 1
	out32(0x80000000, PCI_MECH1_REQ_PORT);
	if (0x80000000 == in32(PCI_MECH1_REQ_PORT)) {
		dprintf("PCI: mechanism 1 controller found\n");
		return B_OK;
	}
	return B_ERROR;
}


status_t
X86PCIControllerMeth1::ReadConfig(
	uint8 bus, uint8 device, uint8 function,
	uint16 offset, uint8 size, uint32 &value)
{
	if (offset > 0xff)
		return B_BAD_VALUE;

	InterruptsSpinLocker lock(fLock);
	out32(PCI_MECH1_REQ_DATA(bus, device, function, offset), PCI_MECH1_REQ_PORT);
	switch (size) {
		case 1:
			value = in8(PCI_MECH1_DATA_PORT + (offset & 3));
			break;
		case 2:
			value = in16(PCI_MECH1_DATA_PORT + (offset & 3));
			break;
		case 4:
			value = in32(PCI_MECH1_DATA_PORT);
			break;
		default:
			return B_ERROR;
	}

	return B_OK;
}


status_t
X86PCIControllerMeth1::WriteConfig(
	uint8 bus, uint8 device, uint8 function,
	uint16 offset, uint8 size, uint32 value)
{
	if (offset > 0xff)
		return B_BAD_VALUE;

	InterruptsSpinLocker lock(fLock);
	out32(PCI_MECH1_REQ_DATA(bus, device, function, offset), PCI_MECH1_REQ_PORT);
	switch (size) {
		case 1:
			out8(value, PCI_MECH1_DATA_PORT + (offset & 3));
			break;
		case 2:
			out16(value, PCI_MECH1_DATA_PORT + (offset & 3));
			break;
		case 4:
			out32(value, PCI_MECH1_DATA_PORT);
			break;
		default:
			return B_ERROR;
	}

	return B_OK;
}


status_t X86PCIControllerMeth1::GetMaxBusDevices(int32& count)
{
	count = 32;
	return B_OK;
}


//#pragma mark - X86PCIControllerMeth2


status_t
X86PCIControllerMeth2::InitDriverInt(device_node* node)
{
	CHECK_RET(X86PCIController::InitDriverInt(node));

	// check for mechanism 2
	out8(0x00, 0xCFB);
	out8(0x00, 0xCF8);
	out8(0x00, 0xCFA);
	if (in8(0xCF8) == 0x00 && in8(0xCFA) == 0x00) {
		dprintf("PCI: mechanism 2 controller found\n");
		return B_OK;
	}
	return B_ERROR;
}


status_t
X86PCIControllerMeth2::ReadConfig(
	uint8 bus, uint8 device, uint8 function,
	uint16 offset, uint8 size, uint32 &value)
{
	if (offset > 0xff)
		return B_BAD_VALUE;

	InterruptsSpinLocker lock(fLock);
	out8((uint8)(0xf0 | (function << 1)), PCI_MECH2_ENABLE_PORT);
	out8(bus, PCI_MECH2_FORWARD_PORT);
	switch (size) {
		case 1:
			value = in8(PCI_MECH2_CONFIG_PORT(device, offset));
			break;
		case 2:
			value = in16(PCI_MECH2_CONFIG_PORT(device, offset));
			break;
		case 4:
			value = in32(PCI_MECH2_CONFIG_PORT(device, offset));
			break;
		default:
			return B_ERROR;
	}
	out8(0, PCI_MECH2_ENABLE_PORT);

	return B_OK;
}


status_t
X86PCIControllerMeth2::WriteConfig(
	uint8 bus, uint8 device, uint8 function,
	uint16 offset, uint8 size, uint32 value)
{
	if (offset > 0xff)
		return B_BAD_VALUE;

	InterruptsSpinLocker lock(fLock);
	out8((uint8)(0xf0 | (function << 1)), PCI_MECH2_ENABLE_PORT);
	out8(bus, PCI_MECH2_FORWARD_PORT);
	switch (size) {
		case 1:
			out8(value, PCI_MECH2_CONFIG_PORT(device, offset));
			break;
		case 2:
			out16(value, PCI_MECH2_CONFIG_PORT(device, offset));
			break;
		case 4:
			out32(value, PCI_MECH2_CONFIG_PORT(device, offset));
			break;
		default:
			return B_ERROR;
	}
	out8(0, PCI_MECH2_ENABLE_PORT);

	return B_OK;
}


status_t X86PCIControllerMeth2::GetMaxBusDevices(int32& count)
{
	count = 16;
	return B_OK;
}


//#pragma mark - X86PCIControllerMethPcie


status_t
X86PCIControllerMethPcie::InitDriverInt(device_node* node)
{
	status_t status = X86PCIController::InitDriverInt(node);
	if (status != B_OK)
		return status;

	// search ACPI
	device_node *acpiNode = NULL;
	{
		device_node* deviceRoot = gDeviceManager->get_root_node();
		device_attr acpiAttrs[] = {
			{ B_DEVICE_BUS, B_STRING_TYPE, { .string = "acpi" }},
			{ ACPI_DEVICE_HID_ITEM, B_STRING_TYPE, { .string = "PNP0A08" }},
			{ NULL }
		};
		if (gDeviceManager->find_child_node(deviceRoot, acpiAttrs, &acpiNode) != B_OK)
			return ENODEV;
	}

	status = fECAMPCIController.ReadResourceInfo(acpiNode);
	gDeviceManager->put_node(acpiNode);
	return status;
}


status_t
X86PCIControllerMethPcie::ReadConfig(
	uint8 bus, uint8 device, uint8 function,
	uint16 offset, uint8 size, uint32 &value)
{
	// fallback to mechanism 1 for out of range busses
	if (bus < fECAMPCIController.fStartBusNumber || bus > fECAMPCIController.fEndBusNumber) {
		return X86PCIControllerMeth1::ReadConfig(bus, device, function, offset,
			size, value);
	}

	return fECAMPCIController.ReadConfig(bus, device, function, offset, size, value);
}


status_t
X86PCIControllerMethPcie::WriteConfig(
	uint8 bus, uint8 device, uint8 function,
	uint16 offset, uint8 size, uint32 value)
{
	// fallback to mechanism 1 for out of range busses
	if (bus < fECAMPCIController.fStartBusNumber || bus > fECAMPCIController.fEndBusNumber) {
		return X86PCIControllerMeth1::WriteConfig(bus, device, function, offset,
			size, value);
	}

	return fECAMPCIController.WriteConfig(bus, device, function, offset, size, value);
}


status_t
X86PCIControllerMethPcie::GetMaxBusDevices(int32& count)
{
	return fECAMPCIController.GetMaxBusDevices(count);
}


status_t
X86PCIControllerMethPcie::GetRange(uint32 index, pci_resource_range* range)
{
	return fECAMPCIController.GetRange(index, range);
}
