/*
 * Copyright 2022, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include "DWPCIController.h"
#include <bus/FDT.h>

#include <AutoDeleterDrivers.h>
#include <util/AutoLock.h>

#include <string.h>
#include <new>


static uint32
ReadReg8(addr_t adr)
{
	uint32 ofs = adr % 4;
	adr = adr / 4 * 4;
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
	adr = adr / 4 * 4;
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
	adr = adr / 4 * 4;
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
	adr = adr / 4 * 4;
	union {
		uint32 in;
		uint16 out[2];
	} val{.in = *(vuint32*)adr};
	val.out[ofs] = (uint16)value;
	*(vuint32*)adr = val.in;
}


//#pragma mark - driver


float
DWPCIController::SupportsDevice(device_node* parent)
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

	// Support only a variant used in HiFive Unmatched board.
	// TODO: Support more Synapsis Designware IP core based PCIe host controllers.
	if (strcmp(compatible, "sifive,fu740-pcie") != 0)
		return 0.0f;

	return 1.0f;
}


status_t
DWPCIController::RegisterDevice(device_node* parent)
{
	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {.string = "Designware PCI Host Controller"} },
		{ B_DEVICE_FIXED_CHILD, B_STRING_TYPE, {.string = "bus_managers/pci/root/driver_v1"} },
		{}
	};

	return gDeviceManager->register_node(parent, DESIGNWARE_PCI_DRIVER_MODULE_NAME, attrs, NULL,
		NULL);
}


status_t
DWPCIController::InitDriver(device_node* node, DWPCIController*& outDriver)
{
	ObjectDeleter<DWPCIController> driver(new(std::nothrow) DWPCIController());
	if (!driver.IsSet())
		return B_NO_MEMORY;

	CHECK_RET(driver->InitDriverInt(node));
	outDriver = driver.Detach();
	return B_OK;
}


status_t
DWPCIController::ReadResourceInfo()
{
	DeviceNodePutter<&gDeviceManager> fdtNode(gDeviceManager->get_parent_node(fNode));

	const char* bus;
	CHECK_RET(gDeviceManager->get_attr_string(fdtNode.Get(), B_DEVICE_BUS, &bus, false));
	if (strcmp(bus, "fdt") != 0)
		return B_ERROR;

	fdt_device_module_info *fdtModule;
	fdt_device* fdtDev;
	CHECK_RET(gDeviceManager->get_driver(fdtNode.Get(),
		(driver_module_info**)&fdtModule, (void**)&fdtDev));

	const void* prop;
	int propLen;

	prop = fdtModule->get_prop(fdtDev, "bus-range", &propLen);
	if (prop != NULL && propLen == 8) {
		uint32 busBeg = B_BENDIAN_TO_HOST_INT32(*((uint32*)prop + 0));
		uint32 busEnd = B_BENDIAN_TO_HOST_INT32(*((uint32*)prop + 1));
		dprintf("  bus-range: %" B_PRIu32 " - %" B_PRIu32 "\n", busBeg, busEnd);
	}

	prop = fdtModule->get_prop(fdtDev, "interrupt-map-mask", &propLen);
	if (prop == NULL || propLen != 4 * 4) {
		dprintf("  \"interrupt-map-mask\" property not found or invalid");
		return B_ERROR;
	}
	fInterruptMapMask.childAdr = B_BENDIAN_TO_HOST_INT32(*((uint32*)prop + 0));
	fInterruptMapMask.childIrq = B_BENDIAN_TO_HOST_INT32(*((uint32*)prop + 3));

	prop = fdtModule->get_prop(fdtDev, "interrupt-map", &propLen);
	fInterruptMapLen = (uint32)propLen / (6 * 4);
	fInterruptMap.SetTo(new(std::nothrow) InterruptMap[fInterruptMapLen]);
	if (!fInterruptMap.IsSet())
		return B_NO_MEMORY;

	for (uint32_t *it = (uint32_t*)prop; (uint8_t*)it - (uint8_t*)prop < propLen; it += 6) {
		size_t i = (it - (uint32_t*)prop) / 6;

		fInterruptMap[i].childAdr = B_BENDIAN_TO_HOST_INT32(*(it + 0));
		fInterruptMap[i].childIrq = B_BENDIAN_TO_HOST_INT32(*(it + 3));
		fInterruptMap[i].parentIrqCtrl = B_BENDIAN_TO_HOST_INT32(*(it + 4));
		fInterruptMap[i].parentIrq = B_BENDIAN_TO_HOST_INT32(*(it + 5));
	}

	dprintf("  interrupt-map:\n");
	for (size_t i = 0; i < fInterruptMapLen; i++) {
		dprintf("    ");
		// child unit address
		PciAddress pciAddress{.val = fInterruptMap[i].childAdr};
		dprintf("bus: %" B_PRIu32, pciAddress.bus);
		dprintf(", dev: %" B_PRIu32, pciAddress.device);
		dprintf(", fn: %" B_PRIu32, pciAddress.function);

		dprintf(", childIrq: %" B_PRIu32, fInterruptMap[i].childIrq);
		dprintf(", parentIrq: (%" B_PRIu32, fInterruptMap[i].parentIrqCtrl);
		dprintf(", %" B_PRIu32, fInterruptMap[i].parentIrq);
		dprintf(")\n");
		if (i % 4 == 3 && (i + 1 < fInterruptMapLen))
			dprintf("\n");
	}

	prop = fdtModule->get_prop(fdtDev, "ranges", &propLen);
	if (prop == NULL) {
		dprintf("  \"ranges\" property not found");
		return B_ERROR;
	}
	dprintf("  ranges:\n");
	for (uint32_t *it = (uint32_t*)prop; (uint8_t*)it - (uint8_t*)prop < propLen; it += 7) {
		dprintf("    ");
		uint32_t type      = B_BENDIAN_TO_HOST_INT32(*(it + 0));
		uint64_t childAdr  = B_BENDIAN_TO_HOST_INT64(*(uint64_t*)(it + 1));
		uint64_t parentAdr = B_BENDIAN_TO_HOST_INT64(*(uint64_t*)(it + 3));
		uint64_t len       = B_BENDIAN_TO_HOST_INT64(*(uint64_t*)(it + 5));

		pci_resource_range range = {};
		range.host_address = parentAdr;
		range.pci_address = childAdr;
		range.size = len;

		if ((type & fdtPciRangePrefechable) != 0)
			range.address_type |= PCI_address_prefetchable;

		switch (type & fdtPciRangeTypeMask) {
		case fdtPciRangeIoPort:
			range.type = B_IO_PORT;
			fResourceRanges.Add(range);
			break;
		case fdtPciRangeMmio32Bit:
			range.type = B_IO_MEMORY;
			range.address_type |= PCI_address_type_32;
			fResourceRanges.Add(range);
			break;
		case fdtPciRangeMmio64Bit:
			range.type = B_IO_MEMORY;
			range.address_type |= PCI_address_type_64;
			fResourceRanges.Add(range);
			break;
		}

		switch (type & fdtPciRangeTypeMask) {
		case fdtPciRangeConfig:    dprintf("CONFIG"); break;
		case fdtPciRangeIoPort:    dprintf("IOPORT"); break;
		case fdtPciRangeMmio32Bit: dprintf("MMIO32"); break;
		case fdtPciRangeMmio64Bit: dprintf("MMIO64"); break;
		}

		dprintf(" (0x%08" B_PRIx32 "): ", type);
		dprintf("child: %08" B_PRIx64, childAdr);
		dprintf(", parent: %08" B_PRIx64, parentAdr);
		dprintf(", len: %" B_PRIx64 "\n", len);
	}
	return B_OK;
}


status_t
DWPCIController::InitDriverInt(device_node* node)
{
	fNode = node;
	dprintf("+DWPCIController::InitDriver()\n");

	CHECK_RET(ReadResourceInfo());

	DeviceNodePutter<&gDeviceManager> fdtNode(gDeviceManager->get_parent_node(node));

	fdt_device_module_info *fdtModule;
	fdt_device* fdtDev;
	CHECK_RET(gDeviceManager->get_driver(fdtNode.Get(),
		(driver_module_info**)&fdtModule, (void**)&fdtDev));

	if (!fdtModule->get_reg(fdtDev, 0, &fDbiPhysBase, &fDbiSize))
		return B_ERROR;
	dprintf("  DBI: %08" B_PRIx64 ", %08" B_PRIx64 "\n", fDbiPhysBase, fDbiSize);

	if (!fdtModule->get_reg(fdtDev, 1, &fConfigPhysBase, &fConfigSize))
		return B_ERROR;
	dprintf("  config: %08" B_PRIx64 ", %08" B_PRIx64 "\n", fConfigPhysBase, fConfigSize);

	uint64 msiIrq;
	if (!fdtModule->get_interrupt(fdtDev, 0, NULL, &msiIrq))
		return B_ERROR;

	fDbiArea.SetTo(map_physical_memory("PCI DBI MMIO", fDbiPhysBase, fDbiSize, B_ANY_KERNEL_ADDRESS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, (void**)&fDbiBase));
	CHECK_RET(fDbiArea.Get());

	fConfigArea.SetTo(map_physical_memory("PCI Config MMIO", fConfigPhysBase, fConfigSize,
		B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, (void**)&fConfigBase));
	CHECK_RET(fConfigArea.Get());

	CHECK_RET(fIrqCtrl.Init(GetDbuRegs(), msiIrq));

	AtuDump();

	dprintf("-DWPCIController::InitDriver()\n");
	return B_OK;
}


void
DWPCIController::UninitDriver()
{
	delete this;
}


addr_t
DWPCIController::ConfigAddress(uint8 bus, uint8 device, uint8 function, uint16 offset)
{
	uint32 atuType;
	if (bus == 0) {
		if (device != 0 || function != 0)
			return 0;
		return fDbiBase + offset;
	} else if (bus == 1)
		atuType = kPciAtuTypeCfg0;
	else
		atuType = kPciAtuTypeCfg1;

	uint64 address = (uint64)(PciAddress {
		.function = function,
		.device = device,
		.bus = bus
	}.val) << 8;

	status_t res = AtuMap(1, kPciAtuOutbound, atuType, fConfigPhysBase, address, fConfigSize);
	if (res < B_OK)
		return 0;

	return fConfigBase + offset;
}


//#pragma mark - PCI controller


status_t
DWPCIController::ReadConfig(uint8 bus, uint8 device, uint8 function,
	uint16 offset, uint8 size, uint32& value)
{
	InterruptsSpinLocker lock(fLock);

	addr_t address = ConfigAddress(bus, device, function, offset);
	if (address == 0)
		return B_ERROR;

	switch (size) {
		case 1: value = ReadReg8(address); break;
		case 2: value = ReadReg16(address); break;
		case 4: value = *(vuint32*)address; break;
		default:
			return B_ERROR;
	}

	return B_OK;
}


status_t
DWPCIController::WriteConfig(uint8 bus, uint8 device, uint8 function,
	uint16 offset, uint8 size, uint32 value)
{
	InterruptsSpinLocker lock(fLock);

	addr_t address = ConfigAddress(bus, device, function, offset);
	if (address == 0)
		return B_ERROR;

	switch (size) {
		case 1: WriteReg8(address, value); break;
		case 2: WriteReg16(address, value); break;
		case 4: *(vuint32*)address = value; break;
		default:
			return B_ERROR;
	}

	return B_OK;
}


status_t
DWPCIController::GetMaxBusDevices(int32& count)
{
	count = 32;
	return B_OK;
}


status_t
DWPCIController::ReadIrq(uint8 bus, uint8 device, uint8 function,
	uint8 pin, uint8& irq)
{
	return B_UNSUPPORTED;
}


status_t
DWPCIController::WriteIrq(uint8 bus, uint8 device, uint8 function,
	uint8 pin, uint8 irq)
{
	return B_UNSUPPORTED;
}


status_t
DWPCIController::GetRange(uint32 index, pci_resource_range* range)
{
	if (index >= (uint32)fResourceRanges.Count())
		return B_BAD_INDEX;

	*range = fResourceRanges[index];
	return B_OK;
}


//#pragma mark - DWPCIController


status_t
DWPCIController::AtuMap(uint32 index, uint32 direction, uint32 type, uint64 parentAdr,
	uint64 childAdr, uint32 size)
{
	/*
	dprintf("AtuMap(%" B_PRIu32 ", %" B_PRIu32 ", %#" B_PRIx64 ", %#" B_PRIx64 ", "
		"%#" B_PRIx32 ")\n", index, type, parentAdr, childAdr, size);
	*/
	volatile PciAtuRegs* atu = (PciAtuRegs*)(fDbiBase + kPciAtuOffset
		+ (2 * index + direction) * sizeof(PciAtuRegs));

	atu->baseLo = (uint32)parentAdr;
	atu->baseHi = (uint32)(parentAdr >> 32);
	atu->limit = (uint32)(parentAdr + size - 1);
	atu->targetLo = (uint32)childAdr;
	atu->targetHi = (uint32)(childAdr >> 32);
	atu->ctrl1 = type;
	atu->ctrl2 = kPciAtuEnable;

	for (;;) {
		if ((atu->ctrl2 & kPciAtuEnable) != 0)
			break;
	}

	return B_OK;
}


void
DWPCIController::AtuDump()
{
	dprintf("ATU:\n");
	for (uint32 direction = 0; direction < 2; direction++) {
		switch (direction) {
			case kPciAtuOutbound:
				dprintf("  outbound:\n");
				break;
			case kPciAtuInbound:
				dprintf("  inbound:\n");
				break;
		}

		for (uint32 index = 0; index < 8; index++) {
			volatile PciAtuRegs* atu = (PciAtuRegs*)(fDbiBase
				+ kPciAtuOffset + (2 * index + direction) * sizeof(PciAtuRegs));

			dprintf("    %" B_PRIu32 ": ", index);
			dprintf("base: %#08" B_PRIx64, atu->baseLo + ((uint64)atu->baseHi << 32));
			dprintf(", limit: %#08" B_PRIx32, atu->limit);
			dprintf(", target: %#08" B_PRIx64, atu->targetLo
				+ ((uint64)atu->targetHi << 32));
			dprintf(", ctrl1: ");
			uint32 ctrl1 = atu->ctrl1;
			switch (ctrl1) {
				case kPciAtuTypeMem:
					dprintf("mem");
					break;
				case kPciAtuTypeIo:
					dprintf("io");
					break;
				case kPciAtuTypeCfg0:
					dprintf("cfg0");
					break;
				case kPciAtuTypeCfg1:
					dprintf("cfg1");
					break;
				default:
					dprintf("? (%#" B_PRIx32 ")", ctrl1);
			}
			dprintf(", ctrl2: {");
			uint32 ctrl2 = atu->ctrl2;
			bool first = true;
			for (uint32 i = 0; i < 32; i++) {
				if (((1 << i) & ctrl2) != 0) {
					if (first)
						first = false;
					else
						dprintf(", ");
					switch (i) {
						case 30:
							dprintf("barModeEnable");
							break;
						case 31:
							dprintf("enable");
							break;
						default:
							dprintf("? (%" B_PRIu32 ")", i);
							break;
					}
				}
			}
			dprintf("}\n");
		}
	}
}
