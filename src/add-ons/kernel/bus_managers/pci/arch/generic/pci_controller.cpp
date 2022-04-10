/*
 * Copyright 2009-2022, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */


#include "pci_controller.h"

#include <kernel/debug.h>
#include <kernel/int.h>
#include <util/Vector.h>

#include <AutoDeleterDrivers.h>

#include "pci_private.h"

#include <ACPI.h> // module
#include <acpi.h>

#include "acpi_irq_routing_table.h"


addr_t gPCIeBase;
uint8 gStartBusNumber;
uint8 gEndBusNumber;
addr_t gPCIioBase;

extern pci_controller pci_controller_ecam;


enum RangeType
{
	RANGE_IO,
	RANGE_MEM
};


struct PciRange
{
	RangeType type;
	phys_addr_t hostAddr;
	phys_addr_t pciAddr;
	size_t length;
};


static Vector<PciRange> *sRanges;


template<typename T>
acpi_address64_attribute AcpiCopyAddressAttr(const T &src)
{
	acpi_address64_attribute dst;

	dst.granularity = src.granularity;
	dst.minimum = src.minimum;
	dst.maximum = src.maximum;
	dst.translation_offset = src.translation_offset;
	dst.address_length = src.address_length;

	return dst;
}


static acpi_status
AcpiCrsScanCallback(acpi_resource *res, void *context)
{
	Vector<PciRange> &ranges = *(Vector<PciRange>*)context;

	acpi_address64_attribute address;
	if (res->type == ACPI_RESOURCE_TYPE_ADDRESS16)
		address = AcpiCopyAddressAttr(res->data.address16.address);
	else if (res->type == ACPI_RESOURCE_TYPE_ADDRESS32)
		address = AcpiCopyAddressAttr(res->data.address32.address);
	else if (res->type == ACPI_RESOURCE_TYPE_ADDRESS64)
		address = AcpiCopyAddressAttr(res->data.address64.address);
	else
		return B_OK;

	acpi_resource_address &common = res->data.address;

	if (common.resource_type != 0 && common.resource_type != 1)
		return B_OK;

	ASSERT(address.minimum + address.address_length - 1 == address.maximum);

	PciRange range;
	range.type = (common.resource_type == 0 ? RANGE_MEM : RANGE_IO);
	range.hostAddr = address.minimum + address.translation_offset;
	range.pciAddr = address.minimum;
	range.length = address.address_length;
	ranges.PushBack(range);

	return B_OK;
}


static bool
is_interrupt_available(int32 gsi)
{
	return true;
}


status_t
pci_controller_init(void)
{
	if (gPCIRootNode == NULL)
		return B_DEV_NOT_READY;

	dprintf("PCI: pci_controller_init\n");

	DeviceNodePutter<&gDeviceManager>
		parent(gDeviceManager->get_parent_node(gPCIRootNode));

	if (parent.Get() == NULL)
		return B_ERROR;

	const char *bus;
	if (gDeviceManager->get_attr_string(parent.Get(), B_DEVICE_BUS, &bus, false) < B_OK)
		return B_ERROR;

	if (strcmp(bus, "acpi") == 0) {
		dprintf("initialize PCI controller from ACPI\n");

		status_t res;
		acpi_module_info *acpiModule;
		acpi_device_module_info *acpiDeviceModule;
		acpi_device acpiDevice;

		res = get_module(B_ACPI_MODULE_NAME, (module_info**)&acpiModule);
		if (res != B_OK)
			return B_ERROR;

		acpi_mcfg *mcfg;
		res = acpiModule->get_table(ACPI_MCFG_SIGNATURE, 0, (void**)&mcfg);
		if (res != B_OK)
			return B_ERROR;

		res = gDeviceManager->get_driver(parent.Get(),
			(driver_module_info**)&acpiDeviceModule, (void**)&acpiDevice);
		if (res != B_OK)
			return B_ERROR;

		sRanges = new Vector<PciRange>();

		acpi_status acpi_res = acpiDeviceModule->walk_resources(acpiDevice,
			(char *)"_CRS", AcpiCrsScanCallback, sRanges);

		if (acpi_res != 0)
			return B_ERROR;

		for (Vector<PciRange>::Iterator it = sRanges->Begin(); it != sRanges->End(); it++) {
			if (it->type != RANGE_IO)
				continue;

			if (gPCIioBase != 0) {
				dprintf("PCI: multiple io ranges not supported!");
				continue;
			}

			area_id area = map_physical_memory("pci io",
				it->hostAddr, it->length, B_ANY_KERNEL_ADDRESS,
				B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, (void **)&gPCIioBase);

			if (area < 0)
				return B_ERROR;
		}

		acpi_mcfg_allocation *end = (acpi_mcfg_allocation *) ((char*)mcfg + mcfg->header.length);
		acpi_mcfg_allocation *alloc = (acpi_mcfg_allocation *) (mcfg + 1);

		if (alloc + 1 != end)
			dprintf("PCI: multiple host bridges not supported!");

		for (; alloc < end; alloc++) {
			dprintf("PCI: mechanism addr: %" B_PRIx64 ", seg: %x, start: %x, end: %x\n",
				alloc->address, alloc->pci_segment, alloc->start_bus_number, alloc->end_bus_number);

			if (alloc->pci_segment != 0) {
				dprintf("PCI: multiple segments not supported!");
				continue;
			}

			gStartBusNumber = alloc->start_bus_number;
			gEndBusNumber = alloc->end_bus_number;

			area_id area = map_physical_memory("pci config",
				alloc->address, (gEndBusNumber - gStartBusNumber + 1) << 20, B_ANY_KERNEL_ADDRESS,
				B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, (void **)&gPCIeBase);

			if (area < 0)
				break;

			dprintf("PCI: ecam controller found\n");
			return pci_controller_add(&pci_controller_ecam, NULL);
		}
	}

	return B_ERROR;
}


status_t
pci_controller_finalize(void)
{
	status_t res;

	acpi_module_info *acpiModule;
	res = get_module(B_ACPI_MODULE_NAME, (module_info**)&acpiModule);
	if (res != B_OK)
		return B_ERROR;

	IRQRoutingTable table;
	res = prepare_irq_routing(acpiModule, table, &is_interrupt_available);
	if (res != B_OK) {
		dprintf("PCI: irq routing preparation failed\n");
		return B_ERROR;
	}

	for (Vector<irq_routing_entry>::Iterator it = table.Begin(); it != table.End(); it++)
		reserve_io_interrupt_vectors(1, it->irq, INTERRUPT_TYPE_IRQ);

	res = enable_irq_routing(acpiModule, table);
	if (res != B_OK) {
		dprintf("PCI: irq routing failed\n");
		return B_ERROR;
	}

	print_irq_routing_table(table);

	return B_OK;
}


phys_addr_t
pci_ram_address(phys_addr_t addr)
{
	for (Vector<PciRange>::Iterator it = sRanges->Begin(); it != sRanges->End(); it++) {
		if (addr >= it->pciAddr && addr < it->pciAddr + it->length) {
			phys_addr_t result = addr - it->pciAddr;
			if (it->type != RANGE_IO)
				result += it->hostAddr;
			return result;
		}
	}

	dprintf("PCI: requested translation of invalid address %" B_PRIxPHYSADDR "\n", addr);
	return 0;
}
