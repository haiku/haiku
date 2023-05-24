/*
 * Copyright 2022, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include "ECAMPCIController.h"
#include <acpi.h>

#include <AutoDeleterDrivers.h>

#include "acpi_irq_routing_table.h"

#include <string.h>


status_t
ECAMPCIControllerACPI::ReadResourceInfo()
{
	DeviceNodePutter<&gDeviceManager> parent(gDeviceManager->get_parent_node(fNode));
	return ReadResourceInfo(parent.Get());
}


status_t
ECAMPCIControllerACPI::ReadResourceInfo(device_node* parent)
{
	dprintf("initialize PCI controller from ACPI\n");

	acpi_module_info* acpiModule;
	acpi_device_module_info* acpiDeviceModule;
	acpi_device acpiDevice;

	CHECK_RET(get_module(B_ACPI_MODULE_NAME, (module_info**)&acpiModule));

	acpi_mcfg *mcfg;
	CHECK_RET(acpiModule->get_table(ACPI_MCFG_SIGNATURE, 0, (void**)&mcfg));

	CHECK_RET(gDeviceManager->get_driver(parent, (driver_module_info**)&acpiDeviceModule,
		(void**)&acpiDevice));

	acpi_status acpi_res = acpiDeviceModule->walk_resources(acpiDevice, (char *)"_CRS",
		AcpiCrsScanCallback, this);

	if (acpi_res != 0)
		return B_ERROR;

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

		fStartBusNumber = alloc->start_bus_number;
		fEndBusNumber = alloc->end_bus_number;

		fRegsLen = (fEndBusNumber - fStartBusNumber + 1) << 20;
		fRegsArea.SetTo(map_physical_memory("PCI Config MMIO",
			alloc->address, fRegsLen, B_ANY_KERNEL_ADDRESS,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, (void **)&fRegs));
		CHECK_RET(fRegsArea.Get());

		return B_OK;
	}

	return B_ERROR;
}


acpi_status
ECAMPCIControllerACPI::AcpiCrsScanCallback(acpi_resource *res, void *context)
{
	return static_cast<ECAMPCIControllerACPI*>(context)->AcpiCrsScanCallbackInt(res);
}


acpi_status
ECAMPCIControllerACPI::AcpiCrsScanCallbackInt(acpi_resource *res)
{
	uint32 outType;

	switch (res->data.address.resource_type) {
		case 0: // ACPI_MEMORY_RANGE
			outType = kPciRangeMmio;
			if (res->type == ACPI_RESOURCE_TYPE_ADDRESS64)
				outType += kPciRangeMmio64Bit;
			if (res->data.address.info.mem.caching == 3 /*ACPI_PREFETCHABLE_MEMORY*/)
				outType += kPciRangeMmioPrefetch;
			break;
		case 1: // ACPI_IO_RANGE
			outType = kPciRangeIoPort;
			break;
		default:
			return B_OK;
	}

	pci_resource_range& range = fResourceRanges[outType];
	range.type = outType;

	switch (res->type) {
		case ACPI_RESOURCE_TYPE_ADDRESS16: {
			const auto &address = res->data.address16.address;
			// If address_length isn't set, compute it from minimum and maximum
			auto address_length = address.address_length;
			if (address_length == 0)
				address_length = address.maximum - address.minimum + 1;
			ASSERT(address.minimum + address_length - 1 == address.maximum);
			range.host_addr = address.minimum + address.translation_offset;
			range.pci_addr  = address.minimum;
			range.size = address_length;
			break;
		}
		case ACPI_RESOURCE_TYPE_ADDRESS32: {
			const auto &address = res->data.address32.address;
			// If address_length isn't set, compute it from minimum and maximum
			auto address_length = address.address_length;
			if (address_length == 0)
				address_length = address.maximum - address.minimum + 1;
			ASSERT(address.minimum + address_length - 1 == address.maximum);
			range.host_addr = address.minimum + address.translation_offset;
			range.pci_addr  = address.minimum;
			range.size = address_length;
			break;
		}
		case ACPI_RESOURCE_TYPE_ADDRESS64: {
			const auto &address = res->data.address64.address;
			// If address_length isn't set, compute it from minimum and maximum
			auto address_length = address.address_length;
			if (address_length == 0)
				address_length = address.maximum - address.minimum + 1;
			ASSERT(address.minimum + address_length - 1 == address.maximum);
			range.host_addr = address.minimum + address.translation_offset;
			range.pci_addr  = address.minimum;
			range.size = address_length;
			break;
		}
	}

	return B_OK;
}


status_t
ECAMPCIControllerACPI::Finalize()
{
	dprintf("finalize PCI controller from ACPI\n");

	acpi_module_info *acpiModule;
	CHECK_RET(get_module(B_ACPI_MODULE_NAME, (module_info**)&acpiModule));

	IRQRoutingTable table;
	CHECK_RET(prepare_irq_routing(acpiModule, table, [](int32 gsi) {return true;}));

	CHECK_RET(enable_irq_routing(acpiModule, table));

	print_irq_routing_table(table);

	return B_OK;
}
