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


/** Convert an ACPI address resource descriptor into a pci_resource_range.
 *
 * This is a template because ACPI resources can be encoded using 8, 16, 32 or 64 bit values.
 */
template<typename T> bool
DecodeAddress(const T& resource, pci_resource_range& range)
{
	const auto& acpiRange = resource.address;
	dprintf("PCI: range from ACPI [%lx(%d),%lx(%d)] with length %lx\n",
		(unsigned long)acpiRange.minimum, resource.minAddress_fixed,
		(unsigned long)acpiRange.maximum, resource.maxAddress_fixed,
		(unsigned long)acpiRange.address_length);

	// If address_length isn't set, compute it from minimum and maximum
	// If maximum isn't set, compute it from minimum and length
	auto addressLength = acpiRange.address_length;
	phys_addr_t addressMaximum = acpiRange.maximum;

	if (addressLength == 0 && addressMaximum <= acpiRange.minimum) {
		// There's nothing we can do with that...
		dprintf("PCI: Ignore empty ACPI range\n");
		return false;
	} else if (!resource.maxAddress_fixed) {
		if (addressLength == 0) {
			dprintf("PCI: maxAddress and addressLength are not set, ignore range\n");
			return false;
		}

		dprintf("PCI: maxAddress is not set, compute it\n");
		addressMaximum = acpiRange.minimum + addressLength - 1;
	} else if (addressLength != addressMaximum - acpiRange.minimum + 1) {
		dprintf("PCI: Fixup invalid length from ACPI!\n");
		addressLength = addressMaximum - acpiRange.minimum + 1;
	}

	range.host_address = acpiRange.minimum + acpiRange.translation_offset;
	range.pci_address  = acpiRange.minimum;
	range.size = addressLength;

	return true;
}


acpi_status
ECAMPCIControllerACPI::AcpiCrsScanCallbackInt(acpi_resource *res)
{
	pci_resource_range range = {};

	switch (res->type) {
		case ACPI_RESOURCE_TYPE_ADDRESS16: {
			const auto& address = res->data.address16;
			if (!DecodeAddress(address, range))
				return B_OK;
			break;
		}
		case ACPI_RESOURCE_TYPE_ADDRESS32: {
			const auto& address = res->data.address32;
			if (!DecodeAddress(address, range))
				return B_OK;
			range.address_type |= PCI_address_type_32;
			break;
		}
		case ACPI_RESOURCE_TYPE_ADDRESS64: {
			const auto& address = res->data.address64;
			if (!DecodeAddress(address, range))
				return B_OK;
			range.address_type |= PCI_address_type_64;
			break;
		}

		default:
			return B_OK;
	}

	switch (res->data.address.resource_type) {
		case 0: // ACPI_MEMORY_RANGE
			range.type = B_IO_MEMORY;
			if (res->data.address.info.mem.caching == 3 /*ACPI_PREFETCHABLE_MEMORY*/)
				range.address_type |= PCI_address_prefetchable;
			break;
		case 1: // ACPI_IO_RANGE
			range.type = B_IO_PORT;
			break;

		default:
			return B_OK;
	}

	fResourceRanges.Add(range);
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
