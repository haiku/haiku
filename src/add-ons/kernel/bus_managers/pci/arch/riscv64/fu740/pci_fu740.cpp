/*
 * Copyright 2021, Haiku Inc., All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 	x512 <danger_mail@list.ru>
 * 	Alexander von Gluck IV <kallisti5@unixzen.com>
 */


#include "pci_fu740.h"


extern ArchPCIController* gArchPCI;


static int32
msi_interrupt_handler(void* arg)
{
	if (gArchPCI == NULL) {
		dprintf("  irq on unconfigured PCI bus!\n");
		return B_ERROR;
	}

	return gArchPCI->HandleMSIIrq(arg);
}


int32
PCIFU740::HandleMSIIrq(void* arg)
{
//      dprintf("MsiInterruptHandler()\n");
	uint32 status = GetDbuRegs()->msiIntr[0].status;
	for (int i = 0; i < 32; i++) {
		if (((1 << i) & status) != 0) {
//                      dprintf("MSI IRQ: %d (%ld)\n", i, gStartMsiIrq + i);
			int_io_interrupt_handler(fMsiStartIrq + i, false);
			GetDbuRegs()->msiIntr[0].status = (1 << i);
		}
	}
	return B_HANDLED_INTERRUPT;
}


status_t
PCIFU740::InitMSI(int32 msiIrq)
{
	dprintf("InitPciMsi()\n");
	dprintf("  msiIrq: %" B_PRId32 "\n", msiIrq);

	physical_entry pe;
	status_t result = get_memory_map(&fMsiData, sizeof(fMsiData), &pe, 1);
	if (result != B_OK) {
		dprintf("  unable to get MSI Memory map!\n");
		return result;
	}

	fMsiPhysAddr = pe.address;
        dprintf("  fMsiPhysAddr: %#" B_PRIxADDR "\n", fMsiPhysAddr);
        GetDbuRegs()->msiAddrLo = (uint32)fMsiPhysAddr;
        GetDbuRegs()->msiAddrHi = (uint32)(fMsiPhysAddr >> 32);

        GetDbuRegs()->msiIntr[0].enable = 0xffffffff;
        GetDbuRegs()->msiIntr[0].mask = 0xffffffff;

	result = install_io_interrupt_handler(msiIrq, msi_interrupt_handler, NULL, 0);
	if (result != B_OK) {
		dprintf("  unable to attach MSI irq handler!\n");
		return result;
	}
	result = allocate_io_interrupt_vectors(32, &fMsiStartIrq, INTERRUPT_TYPE_IRQ);

	if (result != B_OK) {
		dprintf("  unable to attach MSI irq handler!\n");
		return result;
	}

	dprintf("  fMsiStartIrq: %ld\n", fMsiStartIrq);

	return B_OK;
}


int32
PCIFU740::AllocateMSIIrq()
{
	for (int i = 0; i < 32; i++) {
		if (((1 << i) & fAllocatedMsiIrqs[0]) == 0) {
			fAllocatedMsiIrqs[0] |= (1 << i);
			GetDbuRegs()->msiIntr[0].mask &= ~(1 << i);
			return i;
		}
	}
	return B_ERROR;
}


void
PCIFU740::FreeMSIIrq(int32 irq)
{
	if (irq >= 0 && irq < 32 && ((1 << irq) & fAllocatedMsiIrqs[0]) != 0) {
		GetDbuRegs()->msiIntr[0].mask |= (1 << (uint32)irq);
		fAllocatedMsiIrqs[0] &= ~(1 << (uint32)irq);
	}
}


status_t
PCIFU740::AtuMap(uint32 index, uint32 direction, uint32 type, uint64 parentAdr, uint64 childAdr,
	uint32 size)
{
	/*
	dprintf("AtuMap(%" B_PRIu32 ", %" B_PRIu32 ", %#" B_PRIx64 ", %#" B_PRIx64 ", "
		"%#" B_PRIx32 ")\n", index, type, parentAdr, childAdr, size);
	*/
	volatile PciAtuRegs* atu = (PciAtuRegs*)(fDbiBase + kPciAtuOffset + (2*index + direction)*sizeof(PciAtuRegs));
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
PCIFU740::AtuDump()
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




status_t
PCIFU740::Init(device_node* pciRootNode)
{
	fdt_device* parentDev;
	fdt_device_module_info* parentModule;

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
	uint64 dbiRegs = 0;
	uint64 dbiRegsLen = 0;
	if (!parentModule->get_reg(parentDev, 0, &dbiRegs, &dbiRegsLen)
		|| !parentModule->get_reg(parentDev, 1, &configRegs, &configRegsLen)) {
				dprintf("  no regs\n");
				return B_ERROR;
	}
/*
			// !!!
			configRegs = 0x60070000;
			configRegsLen = 0x10000;
*/

	dprintf("  configRegs: (0x%" B_PRIx64 ", 0x%" B_PRIx64 ")\n",
		configRegs, configRegsLen);
	dprintf("  dbiRegs: (0x%" B_PRIx64 ", 0x%" B_PRIx64 ")\n",
		dbiRegs, dbiRegsLen);

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
	fConfigArea.SetTo(map_physical_memory("PCI Config MMIO", configRegs,
		fConfigSize, B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
		(void**)&fConfigBase));

	if (dbiRegs != 0) {
		fDbiPhysBase = dbiRegs;
		fDbiSize = dbiRegsLen;
		fDbiArea.SetTo(map_physical_memory("PCI DBI MMIO", dbiRegs, fDbiSize,
			B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
			(void**)&fDbiBase));
	}

	fIoArea.SetTo(map_physical_memory("PCI IO", fRegisterRanges[kRegIo].parentBase,
		fRegisterRanges[kRegIo].size, B_ANY_KERNEL_ADDRESS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, (void**)&fIoBase));

	if (!fConfigArea.IsSet()) {
		dprintf("  can't map Config MMIO\n");
		return fConfigArea.Get();
	}

	if (!fIoArea.IsSet()) {
		dprintf("  can't map IO\n");
		return fIoArea.Get();
	}

	AtuDump();

	// TODO: Get from MSI!
	InitMSI(0x38);
	AllocRegs();

	AtuDump();

	return B_OK;
}


addr_t
PCIFU740::ConfigAddress(uint8 bus, uint8 device, uint8 function, uint16 offset)
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

	status_t res = AtuMap(1, kPciAtuOutbound, atuType, fConfigPhysBase, EncodePciAddress(bus, device, function) << 8, fConfigSize);
	if (res < B_OK)
		return 0;

	return fConfigBase + offset;
}


void
PCIFU740::InitDeviceMSI(uint8 bus, uint8 device, uint8 function)
{
	uint32 status;
	uint32 capPtr;
	uint32 capId;

	ReadConfig(NULL, bus, device, function, PCI_status, 2, &status);
	if ((status & PCI_status_capabilities) == 0)
		return;

	uint32 headerType;
	ReadConfig(NULL, bus, device, function, PCI_header_type, 1, &headerType);

	switch (headerType & PCI_header_type_mask) {
		case PCI_header_type_generic:
		case PCI_header_type_PCI_to_PCI_bridge:
			ReadConfig(NULL, bus, device, function, PCI_capabilities_ptr, 1, &capPtr);
			break;
		case PCI_header_type_cardbus:
			ReadConfig(NULL, bus, device, function, PCI_capabilities_ptr_2, 1, &capPtr);
			break;
		default:
			return;
	}

	capPtr &= ~3;
	if (capPtr == 0)
		return;

	for (int i = 0; i < 48; i++) {
		ReadConfig(NULL, bus, device, function, capPtr + 0, 1, &capId);

		if (capId == PCI_cap_id_msi)
			break;

		ReadConfig(NULL, bus, device, function, capPtr + 1, 1, &capPtr);
		capPtr &= ~3;
		if (capPtr == 0)
			return;
	}

	dprintf("  MSI offset: %#x\n", capPtr);

	int32 msiIrq = AllocateMSIIrq();
	dprintf("  msiIrq: %" B_PRId32 "\n", msiIrq);

	uint32 control;
	ReadConfig(NULL, bus, device, function, capPtr + PCI_msi_control, 2, &control);

	WriteConfig(NULL, bus, device, function, capPtr + PCI_msi_address, 4, (uint32)fMsiPhysAddr);
	if ((control & PCI_msi_control_64bit) != 0) {
		WriteConfig(NULL, bus, device, function, capPtr + PCI_msi_address_high, 4, (uint32)(fMsiPhysAddr >> 32));
		WriteConfig(NULL, bus, device, function, capPtr + PCI_msi_data_64bit, 2, msiIrq);
	} else
		WriteConfig(NULL, bus, device, function, capPtr + PCI_msi_data, 2, msiIrq);

	control &= ~PCI_msi_control_mme_mask;
	control |= (ffs(1) - 1) << 4;
	control |= PCI_msi_control_enable;
	WriteConfig(NULL, bus, device, function, capPtr + PCI_msi_control, 2, control);
	WriteConfig(NULL, bus, device, function, PCI_interrupt_line, 1, fMsiStartIrq + msiIrq);
}
