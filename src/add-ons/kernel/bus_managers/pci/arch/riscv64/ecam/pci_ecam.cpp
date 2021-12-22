/*
 * Copyright 2021, Haiku Inc., All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 	x512 <danger_mail@list.ru>
 * 	Alexander von Gluck IV <kallisti5@unixzen.com>
 */


#include "pci_ecam.h"


status_t
PCIEcam::InitMSI(int32 irq)
{
	// No MSI on Ecam?
	return B_ERROR;
}


int32
PCIEcam::AllocateMSIIrq()
{
	return B_ERROR;
}


void
PCIEcam::FreeMSIIrq(int32 irq)
{
}


int32
PCIEcam::HandleMSIIrq(void* arg)
{
	return B_UNHANDLED_INTERRUPT;
}


status_t
PCIEcam::Init(device_node* pciRootNode)
{
	fdt_device_module_info* parentModule;
	fdt_device* parentDev;

	DeviceNodePutter<&gDeviceManager> parent(
		gDeviceManager->get_parent_node(gPCIRootNode));

	if (gDeviceManager->get_driver(parent.Get(),
		(driver_module_info**)&parentModule, (void**)&parentDev))
		panic("can't get parent node driver");

	uint64 regs;
	uint64 regsLen;

	for (uint32 i = 0; parentModule->get_reg(parentDev, i, &regs, &regsLen);
		i++) {
		dprintf("  reg[%" B_PRIu32 "]: (0x%" B_PRIx64 ", 0x%" B_PRIx64 ")\n",
			i, regs, regsLen);
	}

	uint64 configRegs = 0;
	uint64 configRegsLen = 0;

	if (!parentModule->get_reg(parentDev, 0, &configRegs, &configRegsLen)) {
		dprintf("  no regs\n");
		return B_ERROR;
	}

	dprintf("  configRegs: (0x%" B_PRIx64 ", 0x%" B_PRIx64 ")\n",
		configRegs, configRegsLen);

	int intMapLen;
	const void* intMapAdr = parentModule->get_prop(parentDev, "interrupt-map",
		&intMapLen);
	if (intMapAdr == NULL) {
		dprintf("  \"interrupt-map\" property not found");
		return B_ERROR;
	} else {
		int intMapMaskLen;
		const void* intMapMask = parentModule->get_prop(parentDev, "interrupt-map-mask",
			&intMapMaskLen);

		if (intMapMask == NULL || intMapMaskLen != 4 * 4) {
			dprintf("  \"interrupt-map-mask\" property not found or invalid");
			return B_ERROR;
		}

		fInterruptMapMask.childAdr = B_BENDIAN_TO_HOST_INT32(*((uint32*)intMapMask + 0));
		fInterruptMapMask.childIrq = B_BENDIAN_TO_HOST_INT32(*((uint32*)intMapMask + 3));

		fInterruptMapLen = (uint32)intMapLen / (6 * 4);
		fInterruptMap.SetTo(new(std::nothrow) InterruptMap[fInterruptMapLen]);
		if (!fInterruptMap.IsSet())
			return B_NO_MEMORY;

		for (uint32_t *it = (uint32_t*)intMapAdr;
			(uint8_t*)it - (uint8_t*)intMapAdr < intMapLen; it += 6) {
			size_t i = (it - (uint32_t*)intMapAdr) / 6;

			fInterruptMap[i].childAdr = B_BENDIAN_TO_HOST_INT32(*(it + 0));
			fInterruptMap[i].childIrq = B_BENDIAN_TO_HOST_INT32(*(it + 3));
			fInterruptMap[i].parentIrqCtrl = B_BENDIAN_TO_HOST_INT32(*(it + 4));
			fInterruptMap[i].parentIrq = B_BENDIAN_TO_HOST_INT32(*(it + 5));
		}

		dprintf("  interrupt-map:\n");
		for (size_t i = 0; i < fInterruptMapLen; i++) {
			dprintf("    ");
			// child unit address
			uint8 bus, device, function;
			DecodePciAddress(fInterruptMap[i].childAdr, bus, device, function);
			dprintf("bus: %" B_PRIu32, bus);
			dprintf(", dev: %" B_PRIu32, device);
			dprintf(", fn: %" B_PRIu32, function);

			dprintf(", childIrq: %" B_PRIu32,
				fInterruptMap[i].childIrq);
			dprintf(", parentIrq: (%" B_PRIu32,
				fInterruptMap[i].parentIrqCtrl);
			dprintf(", %" B_PRIu32, fInterruptMap[i].parentIrq);
			dprintf(")\n");
			if (i % 4 == 3 && (i + 1 < fInterruptMapLen))
				dprintf("\n");
		}
	}

	memset(fRegisterRanges, 0, sizeof(fRegisterRanges));
	int rangesLen;
	const void* rangesAdr = parentModule->get_prop(parentDev, "ranges",
		&rangesLen);
	if (rangesAdr == NULL) {
		dprintf("  \"ranges\" property not found");
	} else {
		dprintf("  ranges:\n");
		for (uint32_t *it = (uint32_t*)rangesAdr;
			(uint8_t*)it - (uint8_t*)rangesAdr < rangesLen; it += 7) {
			dprintf("    ");
			uint32_t kind = B_BENDIAN_TO_HOST_INT32(*(it + 0));
			uint64_t childAdr = B_BENDIAN_TO_HOST_INT64(*(uint64_t*)(it + 1));
			uint64_t parentAdr = B_BENDIAN_TO_HOST_INT64(*(uint64_t*)(it + 3));
			uint64_t len = B_BENDIAN_TO_HOST_INT64(*(uint64_t*)(it + 5));

			switch (kind & 0x03000000) {
			case 0x01000000:
				SetRegisterRange(kRegIo, parentAdr, childAdr, len);
				break;
			case 0x02000000:
				SetRegisterRange(kRegMmio32, parentAdr, childAdr, len);
				break;
			case 0x03000000:
				SetRegisterRange(kRegMmio64, parentAdr, childAdr, len);
				break;
			}

			switch (kind & 0x03000000) {
			case 0x00000000: dprintf("CONFIG"); break;
			case 0x01000000: dprintf("IOPORT"); break;
			case 0x02000000: dprintf("MMIO32"); break;
			case 0x03000000: dprintf("MMIO64"); break;
			}

			dprintf(" (0x%08" B_PRIx32 "): ", kind);
			dprintf("child: %08" B_PRIx64, childAdr);
			dprintf(", parent: %08" B_PRIx64, parentAdr);
			dprintf(", len: %" B_PRIx64 "\n", len);
		}
	}

	fConfigPhysBase = configRegs;
	fConfigSize = configRegsLen;
	fConfigArea.SetTo(map_physical_memory(
		"PCI Config MMIO",
		configRegs, fConfigSize, B_ANY_KERNEL_ADDRESS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
		(void**)&fConfigBase
	));

	fIoArea.SetTo(map_physical_memory(
		"PCI IO",
		fRegisterRanges[kRegIo].parentBase,
		fRegisterRanges[kRegIo].size,
		B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
		(void**)&fIoBase
	));

	if (!fConfigArea.IsSet()) {
		dprintf("  can't map Config MMIO\n");
		return fConfigArea.Get();
	}

	if (!fIoArea.IsSet()) {
		dprintf("  can't map IO\n");
		return fIoArea.Get();
	}

	InitMSI(-1);

	AllocRegs();

	return B_OK;
}


addr_t
PCIEcam::ConfigAddress(uint8 bus, uint8 device, uint8 function, uint16 offset)
{
	addr_t address = fConfigBase + EncodePciAddress(bus, device, function) * (1 << 4) + offset;

	if (address < fConfigBase || address /*+ size*/ > fConfigBase + fConfigSize)
		return 0;

	return address;
}


void
PCIEcam::InitDeviceMSI(uint8 bus, uint8 device, uint8 function)
{
}
