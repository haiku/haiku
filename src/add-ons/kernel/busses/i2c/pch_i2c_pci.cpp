/*
 * Copyright 2020, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <new>
#include <stdio.h>
#include <string.h>

#include <ACPI.h>
#include <ByteOrder.h>
#include <condition_variable.h>
#include <bus/PCI.h>
#include <PCI_x86.h>


#include "pch_i2c.h"


typedef struct {
	pch_i2c_sim_info info;
	pci_device_module_info* pci;
	pci_device* device;
	pch_i2c_irq_type irq_type;

	pci_info pciinfo;
} pch_i2c_pci_sim_info;


static pci_x86_module_info* sPCIx86Module;


//	#pragma mark -


static status_t
pci_scan_bus(i2c_bus_cookie cookie)
{
	CALLED();
	pch_i2c_pci_sim_info* bus = (pch_i2c_pci_sim_info*)cookie;
	device_node *acpiNode = NULL;

	pci_info *pciInfo = &bus->pciinfo;

	// search ACPI I2C nodes for this device
	{
		device_node* deviceRoot = gDeviceManager->get_root_node();
		uint32 addr = (pciInfo->device << 16) | pciInfo->function;
		device_attr acpiAttrs[] = {
			{ B_DEVICE_BUS, B_STRING_TYPE, { string: "acpi" }},
			{ ACPI_DEVICE_ADDR_ITEM, B_UINT32_TYPE, {ui32: addr}},
			{ NULL }
		};
		if (addr != 0 && gDeviceManager->find_child_node(deviceRoot, acpiAttrs,
				&acpiNode) != B_OK) {
			ERROR("init_bus() acpi device not found\n");
			return B_DEV_CONFIGURATION_ERROR;
		}
	}

	TRACE("init_bus() find_child_node() found %x %x %p\n",
		pciInfo->device, pciInfo->function, acpiNode);
	// TODO eventually check timings on acpi
	acpi_device_module_info *acpi;
	acpi_device	acpiDevice;
	if (gDeviceManager->get_driver(acpiNode, (driver_module_info **)&acpi,
		(void **)&acpiDevice) == B_OK) {
		// find out I2C device nodes
		acpi->walk_namespace(acpiDevice, ACPI_TYPE_DEVICE, 1,
			pch_i2c_scan_bus_callback, NULL, bus, NULL);
	}

	return B_OK;
}


static status_t
register_child_devices(void* cookie)
{
	CALLED();

	pch_i2c_pci_sim_info* bus = (pch_i2c_pci_sim_info*)cookie;
	device_node* node = bus->info.driver_node;

	char prettyName[25];
	sprintf(prettyName, "PCH I2C Controller %" B_PRIu16, 0);

	device_attr attrs[] = {
		// properties of this controller for i2c bus manager
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE,
			{ string: prettyName }},
		{ B_DEVICE_FIXED_CHILD, B_STRING_TYPE,
			{ string: I2C_FOR_CONTROLLER_MODULE_NAME }},

		// private data to identify the device
		{ NULL }
	};

	return gDeviceManager->register_node(node, PCH_I2C_SIM_MODULE_NAME,
		attrs, NULL, NULL);
}


static status_t
init_device(device_node* node, void** device_cookie)
{
	CALLED();
	status_t status = B_OK;

	pch_i2c_pci_sim_info* bus = (pch_i2c_pci_sim_info*)calloc(1,
		sizeof(pch_i2c_pci_sim_info));
	if (bus == NULL)
		return B_NO_MEMORY;

	pci_device_module_info* pci;
	pci_device* device;
	{
		device_node* pciParent = gDeviceManager->get_parent_node(node);
		gDeviceManager->get_driver(pciParent, (driver_module_info**)&pci,
			(void**)&device);
		gDeviceManager->put_node(pciParent);
	}

	bus->pci = pci;
	bus->device = device;
	bus->info.driver_node = node;
	bus->info.scan_bus = pci_scan_bus;

	pci_info *pciInfo = &bus->pciinfo;
	pci->get_pci_info(device, pciInfo);

	bus->info.base_addr = pciInfo->u.h0.base_registers[0];
	if ((pciInfo->u.h0.base_register_flags[0] & PCI_address_type)
			== PCI_address_type_64) {
		bus->info.base_addr |= (uint64)pciInfo->u.h0.base_registers[1] << 32;
	}

	bus->info.map_size = pciInfo->u.h0.base_register_sizes[0];

	// enable power
	pci->set_powerstate(device, PCI_pm_state_d0);

	// enable bus master and memory
	uint16 pcicmd = pci->read_pci_config(device, PCI_command, 2);
	pcicmd |= PCI_command_master | PCI_command_memory;
	pci->write_pci_config(device, PCI_command, 2, pcicmd);

	if (get_module(B_PCI_X86_MODULE_NAME, (module_info**)&sPCIx86Module)
			!= B_OK) {
		sPCIx86Module = NULL;
	}

	if (sPCIx86Module != NULL) {
		// try MSI-X
		uint8 msixCount = sPCIx86Module->get_msix_count(
			pciInfo->bus, pciInfo->device, pciInfo->function);
		if (msixCount >= 1) {
			uint8 vector;
			if (sPCIx86Module->configure_msix(pciInfo->bus, pciInfo->device,
					pciInfo->function, 1, &vector) == B_OK
				&& sPCIx86Module->enable_msix(pciInfo->bus, pciInfo->device,
					pciInfo->function) == B_OK) {
				TRACE_ALWAYS("using MSI-X vector %u\n", vector);
				bus->info.irq = vector;
				bus->irq_type = PCH_I2C_IRQ_MSI_X_SHARED;
			} else {
				ERROR("couldn't use MSI-X SHARED\n");
			}
		} else if (sPCIx86Module->get_msi_count(
			pciInfo->bus, pciInfo->device, pciInfo->function) >= 1) {
			// try MSI
			uint8 vector;
			if (sPCIx86Module->configure_msi(pciInfo->bus, pciInfo->device,
					pciInfo->function, 1, &vector) == B_OK
				&& sPCIx86Module->enable_msi(pciInfo->bus, pciInfo->device,
					pciInfo->function) == B_OK) {
				TRACE_ALWAYS("using MSI vector %u\n", vector);
				bus->info.irq = vector;
				bus->irq_type = PCH_I2C_IRQ_MSI;
			} else {
				ERROR("couldn't use MSI\n");
			}
		}
	}
	if (bus->irq_type == PCH_I2C_IRQ_LEGACY) {
		bus->info.irq = pciInfo->u.h0.interrupt_line;
		TRACE_ALWAYS("using legacy interrupt %u\n", bus->info.irq);
	}
	if (bus->info.irq == 0 || bus->info.irq == 0xff) {
		ERROR("PCI IRQ not assigned\n");
		status = B_ERROR;
		goto err;
	}

	*device_cookie = bus;
	return B_OK;

err:
	free(bus);
	return status;
}


static void
uninit_device(void* device_cookie)
{
	pch_i2c_pci_sim_info* bus = (pch_i2c_pci_sim_info*)device_cookie;
	if (bus->irq_type != PCH_I2C_IRQ_LEGACY) {
		if (sPCIx86Module != NULL) {
			sPCIx86Module->disable_msi(bus->pciinfo.bus,
				bus->pciinfo.device, bus->pciinfo.function);
			sPCIx86Module->unconfigure_msi(bus->pciinfo.bus,
				bus->pciinfo.device, bus->pciinfo.function);
		}
	}
	if (sPCIx86Module != NULL) {
		put_module(B_PCI_X86_MODULE_NAME);
		sPCIx86Module = NULL;
	}
	free(bus);
}


static status_t
register_device(device_node* parent)
{
	device_attr attrs[] = {
		{B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {string: "PCH I2C PCI"}},
		{}
	};

	return gDeviceManager->register_node(parent,
		PCH_I2C_PCI_DEVICE_MODULE_NAME, attrs, NULL, NULL);
}


static float
supports_device(device_node* parent)
{
	CALLED();
	const char* bus;
	uint16 vendorID, deviceID;

	// make sure parent is a PCH I2C PCI device node
	if (gDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false)
		< B_OK || gDeviceManager->get_attr_uint16(parent, B_DEVICE_VENDOR_ID,
				&vendorID, false) < B_OK
		|| gDeviceManager->get_attr_uint16(parent, B_DEVICE_ID, &deviceID,
				false) < B_OK) {
		return -1;
	}

	if (strcmp(bus, "pci") != 0)
		return 0.0f;

	if (vendorID == 0x8086) {
		switch (deviceID) {
			case 0x02c5:
			case 0x02c6:
			case 0x02e8:
			case 0x02e9:
			case 0x02ea:
			case 0x02eb:
			case 0x06e8:
			case 0x06e9:
			case 0x06ea:
			case 0x06eb:
			case 0x0aac:
			case 0x0aae:
			case 0x0ab0:
			case 0x0ab2:
			case 0x0ab4:
			case 0x0ab6:
			case 0x0ab8:
			case 0x0aba:
			case 0x1aac:
			case 0x1aae:

			case 0x1ab0:
			case 0x1ab2:
			case 0x1ab4:
			case 0x1ab6:
			case 0x1ab8:
			case 0x1aba:

			case 0x31ac:
			case 0x31ae:
			case 0x31b0:
			case 0x31b2:
			case 0x31b4:
			case 0x31b6:
			case 0x31b8:
			case 0x31ba:

			case 0x34c5:
			case 0x34c6:
			case 0x34e8:
			case 0x34e9:
			case 0x34ea:
			case 0x34eb:

			case 0x4b44:
			case 0x4b45:
			case 0x4b4b:
			case 0x4b4c:
			case 0x4b78:
			case 0x4b79:
			case 0x4b7a:
			case 0x4b7b:

			case 0x4dc5:
			case 0x4dc6:
			case 0x4de8:
			case 0x4de9:
			case 0x4dea:
			case 0x4deb:

			case 0x5aac:
			case 0x5aae:
			case 0x5ab0:
			case 0x5ab2:
			case 0x5ab4:
			case 0x5ab6:
			case 0x5ab8:
			case 0x5aba:

			case 0x9d60:
			case 0x9d61:
			case 0x9d62:
			case 0x9d63:
			case 0x9d64:
			case 0x9d65:

			case 0x9dc5:
			case 0x9dc6:
			case 0x9de8:
			case 0x9de9:
			case 0x9dea:
			case 0x9deb:

			case 0xa0c5:
			case 0xa0c6:
			case 0xa0d8:
			case 0xa0d9:
			case 0xa0e8:
			case 0xa0e9:
			case 0xa0ea:
			case 0xa0eb:

			case 0xa160:
			case 0xa161:
			case 0xa162:

			case 0xa2e0:
			case 0xa2e1:
			case 0xa2e2:
			case 0xa2e3:

			case 0xa368:
			case 0xa369:
			case 0xa36a:
			case 0xa36b:

			case 0xa3e0:
			case 0xa3e1:
			case 0xa3e2:
			case 0xa3e3:

			case 0x43ad:
			case 0x43ae:
			case 0x43d8:

			case 0x43e8:
			case 0x43e9:
			case 0x43ea:
			case 0x43eb:
				break;
			default:
				return 0.0f;
		}
		pci_device_module_info* pci;
		pci_device* device;
		gDeviceManager->get_driver(parent, (driver_module_info**)&pci,
			(void**)&device);
		uint8 pciSubDeviceId = pci->read_pci_config(device, PCI_revision,
			1);

		TRACE("PCH I2C device found! vendor 0x%04x, device 0x%04x\n", vendorID,
			deviceID);
		return 0.8f;
	}

	return 0.0f;
}


//	#pragma mark -


driver_module_info gPchI2cPciDevice = {
	{
		PCH_I2C_PCI_DEVICE_MODULE_NAME,
		0,
		NULL
	},

	supports_device,
	register_device,
	init_device,
	uninit_device,
	register_child_devices,
	NULL,	// rescan
	NULL,	// device removed
};

