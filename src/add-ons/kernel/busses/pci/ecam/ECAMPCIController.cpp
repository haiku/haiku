/*
 * Copyright 2022, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include "ECAMPCIController.h"
#include <acpi.h>

#include <AutoDeleterDrivers.h>

#include "acpi_irq_routing_table.h"

#include <string.h>
#include <new>


static uint32
ReadReg8(addr_t adr)
{
	uint32 ofs = adr % 4;
	adr = ROUNDDOWN(adr, 4);
	union {
		uint32 in;
		uint8 out[4];
	} val{.in = *(vuint32*)adr};
	return val.out[ofs];
}


static uint32
ReadReg16(addr_t adr)
{
	uint32 ofs = adr / 2 % 2;
	adr = ROUNDDOWN(adr, 4);
	union {
		uint32 in;
		uint16 out[2];
	} val{.in = *(vuint32*)adr};
	return val.out[ofs];
}


static void
WriteReg8(addr_t adr, uint32 value)
{
	uint32 ofs = adr % 4;
	adr = ROUNDDOWN(adr, 4);
	union {
		uint32 in;
		uint8 out[4];
	} val{.in = *(vuint32*)adr};
	val.out[ofs] = (uint8)value;
	*(vuint32*)adr = val.in;
}


static void
WriteReg16(addr_t adr, uint32 value)
{
	uint32 ofs = adr / 2 % 2;
	adr = ROUNDDOWN(adr, 4);
	union {
		uint32 in;
		uint16 out[2];
	} val{.in = *(vuint32*)adr};
	val.out[ofs] = (uint16)value;
	*(vuint32*)adr = val.in;
}


//#pragma mark - driver


float
ECAMPCIController::SupportsDevice(device_node* parent)
{
	const char* bus;
	status_t status = gDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false);
	if (status < B_OK)
		return -1.0f;

	if (strcmp(bus, "fdt") == 0) {
		const char* compatible;
		status = gDeviceManager->get_attr_string(parent, "fdt/compatible", &compatible, false);
		if (status < B_OK)
			return -1.0f;

		if (strcmp(compatible, "pci-host-ecam-generic") != 0)
			return 0.0f;

		return 1.0f;
	}

	if (strcmp(bus, "acpi") == 0) {
		const char* hid;
		if (gDeviceManager->get_attr_string(parent, ACPI_DEVICE_HID_ITEM, &hid, false) < B_OK)
			return -1.0f;

		if (strcmp(hid, "PNP0A03") != 0 && strcmp(hid, "PNP0A08") != 0)
			return 0.0f;

		return 1.0f;
	}

	return 0.0f;
}


status_t
ECAMPCIController::RegisterDevice(device_node* parent)
{
	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {.string = "ECAM PCI Host Controller"} },
		{ B_DEVICE_FIXED_CHILD, B_STRING_TYPE, {.string = "bus_managers/pci/root/driver_v1"} },
		{}
	};

	return gDeviceManager->register_node(parent, ECAM_PCI_DRIVER_MODULE_NAME, attrs, NULL, NULL);
}


#if !defined(ECAM_PCI_CONTROLLER_NO_INIT)
status_t
ECAMPCIController::InitDriver(device_node* node, ECAMPCIController*& outDriver)
{
	dprintf("+ECAMPCIController::InitDriver()\n");
	DeviceNodePutter<&gDeviceManager> parentNode(gDeviceManager->get_parent_node(node));

	ObjectDeleter<ECAMPCIController> driver;

	const char* bus;
	CHECK_RET(gDeviceManager->get_attr_string(parentNode.Get(), B_DEVICE_BUS, &bus, false));
	if (strcmp(bus, "fdt") == 0)
		driver.SetTo(new(std::nothrow) ECAMPCIControllerFDT());
	else if (strcmp(bus, "acpi") == 0)
		driver.SetTo(new(std::nothrow) ECAMPCIControllerACPI());
	else
		return B_ERROR;

	if (!driver.IsSet())
		return B_NO_MEMORY;

	driver->fNode = node;

	CHECK_RET(driver->ReadResourceInfo());
	outDriver = driver.Detach();

	dprintf("-ECAMPCIController::InitDriver()\n");
	return B_OK;
}


void
ECAMPCIController::UninitDriver()
{
	delete this;
}
#endif


/** Compute the virtual address for accessing a PCI ECAM register.
 *
 * \returns NULL if the address is out of bounds.
 */
addr_t
ECAMPCIController::ConfigAddress(uint8 bus, uint8 device, uint8 function, uint16 offset)
{
	PciAddressEcam address {
		.offset = offset,
		.function = function,
		.device = device,
		.bus = bus
	};
	if ((ROUNDDOWN(address.val, 4) + 4) > fRegsLen)
		return 0;

	return (addr_t)fRegs + address.val;
}


//#pragma mark - PCI controller


status_t
ECAMPCIController::ReadConfig(uint8 bus, uint8 device, uint8 function,
	uint16 offset, uint8 size, uint32& value)
{
	addr_t address = ConfigAddress(bus, device, function, offset);
	if (address == 0)
		return ERANGE;

	switch (size) {
		case 1: value = ReadReg8(address); break;
		case 2: value = ReadReg16(address); break;
		case 4: value = *(vuint32*)address; break;
		default:
			return B_BAD_VALUE;
	}

	return B_OK;
}


status_t
ECAMPCIController::WriteConfig(uint8 bus, uint8 device, uint8 function,
	uint16 offset, uint8 size, uint32 value)
{
	addr_t address = ConfigAddress(bus, device, function, offset);
	if (address == 0)
		return ERANGE;

	switch (size) {
		case 1: WriteReg8(address, value); break;
		case 2: WriteReg16(address, value); break;
		case 4: *(vuint32*)address = value; break;
		default:
			return B_BAD_VALUE;
	}

	return B_OK;
}


status_t
ECAMPCIController::GetMaxBusDevices(int32& count)
{
	count = 32;
	return B_OK;
}


status_t
ECAMPCIController::ReadIrq(uint8 bus, uint8 device, uint8 function,
	uint8 pin, uint8& irq)
{
	return B_UNSUPPORTED;
}


status_t
ECAMPCIController::WriteIrq(uint8 bus, uint8 device, uint8 function,
	uint8 pin, uint8 irq)
{
	return B_UNSUPPORTED;
}


status_t
ECAMPCIController::GetRange(uint32 index, pci_resource_range* range)
{
	if (index >= (uint32)fResourceRanges.Count())
		return B_BAD_INDEX;

	*range = fResourceRanges[index];
	return B_OK;
}
