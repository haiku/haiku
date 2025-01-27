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


#include "pch_i2c.h"


struct {
	uint16 id;
	pch_version version;
} pch_pci_devices [] = {

	/* Unknown */
	{0x0aac, PCH_NONE},
	{0x0aae, PCH_NONE},
	{0x0ab0, PCH_NONE},
	{0x0ab2, PCH_NONE},
	{0x0ab4, PCH_NONE},
	{0x0ab6, PCH_NONE},
	{0x0ab8, PCH_NONE},
	{0x0aba, PCH_NONE},

	{0x1aac, PCH_NONE},
	{0x1aae, PCH_NONE},

	{0x1ab0, PCH_NONE},
	{0x1ab2, PCH_NONE},
	{0x1ab4, PCH_NONE},
	{0x1ab6, PCH_NONE},
	{0x1ab8, PCH_NONE},
	{0x1aba, PCH_NONE},

	{0x4b44, PCH_NONE},
	{0x4b45, PCH_NONE},
	{0x4b4b, PCH_NONE},
	{0x4b4c, PCH_NONE},
	{0x4b78, PCH_NONE},
	{0x4b79, PCH_NONE},
	{0x4b7a, PCH_NONE},
	{0x4b7b, PCH_NONE},

	{0x4dc5, PCH_NONE},
	{0x4dc6, PCH_NONE},
	{0x4de8, PCH_NONE},
	{0x4de9, PCH_NONE},
	{0x4dea, PCH_NONE},
	{0x4deb, PCH_NONE},

	{0x7a4c, PCH_NONE},
	{0x7a4d, PCH_NONE},
	{0x7a4e, PCH_NONE},
	{0x7a4f, PCH_NONE},
	{0x7a7c, PCH_NONE},
	{0x7a7d, PCH_NONE},
	
	{0x98c5, PCH_NONE},
	{0x98c6, PCH_NONE},
	{0x98e8, PCH_NONE},
	{0x98e9, PCH_NONE},
	{0x98ea, PCH_NONE},
	{0x98eb, PCH_NONE},

	{0xa2e0, PCH_NONE},
	{0xa2e1, PCH_NONE},
	{0xa2e2, PCH_NONE},
	{0xa2e3, PCH_NONE},

	/* Baytrail */
	{0x0f41, PCH_ATOM},
	{0x0f42, PCH_ATOM},
	{0x0f43, PCH_ATOM},
	{0x0f44, PCH_ATOM},
	{0x0f45, PCH_ATOM},
	{0x0f46, PCH_ATOM},
	{0x0f47, PCH_ATOM},

	/* Haswell */
	{0x9c61, PCH_HASWELL},
	{0x9c62, PCH_HASWELL},

	/* Braswell */
	{0x22c1, PCH_ATOM},
	{0x22c2, PCH_ATOM},
	{0x22c3, PCH_ATOM},
	{0x22c4, PCH_ATOM},
	{0x22c5, PCH_ATOM},
	{0x22c6, PCH_ATOM},
	{0x22c7, PCH_ATOM},

	/* Skylake */
	{0x9d60, PCH_SKYLAKE},
	{0x9d61, PCH_SKYLAKE},
	{0x9d62, PCH_SKYLAKE},
	{0x9d63, PCH_SKYLAKE},
	{0x9d64, PCH_SKYLAKE},
	{0x9d65, PCH_SKYLAKE},

	/* Kaby Lake */
	{0xa160, PCH_SKYLAKE},
	{0xa161, PCH_SKYLAKE},
	{0xa162, PCH_SKYLAKE}, // ?

	/* Apolo Lake */
	{0x5aac, PCH_APL},
	{0x5aae, PCH_APL},
	{0x5ab0, PCH_APL},
	{0x5ab2, PCH_APL},
	{0x5ab4, PCH_APL},
	{0x5ab6, PCH_APL},
	{0x5ab8, PCH_APL},
	{0x5aba, PCH_APL},

	/* Cannon Lake */
	{0x9dc5, PCH_CANNONLAKE},
	{0x9dc6, PCH_CANNONLAKE},
	{0x9de8, PCH_CANNONLAKE},
	{0x9de9, PCH_CANNONLAKE},
	{0x9dea, PCH_CANNONLAKE},
	{0x9deb, PCH_CANNONLAKE},
	{0xa368, PCH_CANNONLAKE},
	{0xa369, PCH_CANNONLAKE},
	{0xa36a, PCH_CANNONLAKE},
	{0xa36b, PCH_CANNONLAKE},

	/* Comet Lake */
	{0x02e8, PCH_CANNONLAKE},
	{0x02e9, PCH_CANNONLAKE},
	{0x02ea, PCH_CANNONLAKE},
	{0x02eb, PCH_CANNONLAKE},
	{0x02c5, PCH_CANNONLAKE},
	{0x02c6, PCH_CANNONLAKE},
	{0x06e8, PCH_CANNONLAKE},
	{0x06e9, PCH_CANNONLAKE},
	{0x06ea, PCH_CANNONLAKE},
	{0x06eb, PCH_CANNONLAKE},
	{0xa3e0, PCH_CANNONLAKE},
	{0xa3e1, PCH_CANNONLAKE},
	{0xa3e2, PCH_CANNONLAKE},
	{0xa3e3, PCH_CANNONLAKE},

	/* Ice Lake */
	{0x34e8, PCH_TIGERLAKE},
	{0x34e9, PCH_TIGERLAKE},
	{0x34ea, PCH_TIGERLAKE},
	{0x34eb, PCH_TIGERLAKE},
	{0x34c5, PCH_TIGERLAKE},
	{0x34c6, PCH_TIGERLAKE},

	/* Tiger Lake */
	{0x43d8, PCH_TIGERLAKE},
	{0x43e8, PCH_TIGERLAKE},
	{0x43e9, PCH_TIGERLAKE},
	{0x43ea, PCH_TIGERLAKE},
	{0x43eb, PCH_TIGERLAKE},
	{0x43ad, PCH_TIGERLAKE},
	{0x43ae, PCH_TIGERLAKE},
	{0xa0c5, PCH_SKYLAKE},
	{0xa0c6, PCH_SKYLAKE},
	{0xa0d8, PCH_SKYLAKE},
	{0xa0d9, PCH_SKYLAKE},
	{0xa0e8, PCH_SKYLAKE},
	{0xa0e9, PCH_SKYLAKE},
	{0xa0ea, PCH_SKYLAKE},
	{0xa0eb, PCH_SKYLAKE},

	/* Gemini Lake */
	{0x31ac, PCH_GEMINILAKE},
	{0x31ae, PCH_GEMINILAKE},
	{0x31b0, PCH_GEMINILAKE},
	{0x31b2, PCH_GEMINILAKE},
	{0x31b4, PCH_GEMINILAKE},
	{0x31b6, PCH_GEMINILAKE},
	{0x31b8, PCH_GEMINILAKE},
	{0x31ba, PCH_GEMINILAKE},

	/* Alder Lake */
	{0x51e8, PCH_TIGERLAKE},
	{0x51e9, PCH_TIGERLAKE},
	{0x51ea, PCH_TIGERLAKE},
	{0x51eb, PCH_TIGERLAKE},
	{0x51c5, PCH_TIGERLAKE},
	{0x51c6, PCH_TIGERLAKE},
	{0x51d8, PCH_TIGERLAKE},
	{0x51d9, PCH_TIGERLAKE},
	{0x7acc, PCH_TIGERLAKE},
	{0x7acd, PCH_TIGERLAKE},
	{0x7ace, PCH_TIGERLAKE},
	{0x7acf, PCH_TIGERLAKE},
	{0x7afc, PCH_TIGERLAKE},
	{0x7afd, PCH_TIGERLAKE},
	{0x54e8, PCH_TIGERLAKE},
	{0x54e9, PCH_TIGERLAKE},
	{0x54ea, PCH_TIGERLAKE},
	{0x54eb, PCH_TIGERLAKE},
	{0x54c5, PCH_TIGERLAKE},
	{0x54c6, PCH_TIGERLAKE},

	/* Meteor Lake */
	{0x7e50, PCH_TIGERLAKE},
	{0x7e51, PCH_TIGERLAKE},
	{0x7e78, PCH_TIGERLAKE},
	{0x7e79, PCH_TIGERLAKE},
	{0x7e7a, PCH_TIGERLAKE},
	{0x7e7b, PCH_TIGERLAKE},

	{0, PCH_NONE}
};

typedef struct {
	pch_i2c_sim_info info;
	pci_device_module_info* pci;
	pci_device* device;
	pch_i2c_irq_type irq_type;

	pci_info pciinfo;
} pch_i2c_pci_sim_info;


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
			{ B_DEVICE_BUS, B_STRING_TYPE, { .string = "acpi" }},
			{ ACPI_DEVICE_ADDR_ITEM, B_UINT32_TYPE, {.ui32 = addr}},
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
			{ .string = prettyName }},
		{ B_DEVICE_FIXED_CHILD, B_STRING_TYPE,
			{ .string = I2C_FOR_CONTROLLER_MODULE_NAME }},

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

	size_t dev = 0;
	while (pch_pci_devices[dev].id != 0) {
		if (pch_pci_devices[dev].id == pciInfo->device_id) {
			bus->info.version = pch_pci_devices[dev].version;
			break;
		}
		dev++;
	}

	bus->info.base_addr = pciInfo->u.h0.base_registers[0];
	bus->info.map_size = pciInfo->u.h0.base_register_sizes[0];
	if ((pciInfo->u.h0.base_register_flags[0] & PCI_address_type)
			== PCI_address_type_64) {
		bus->info.base_addr |= (uint64)pciInfo->u.h0.base_registers[1] << 32;
		bus->info.map_size |= (uint64)pciInfo->u.h0.base_register_sizes[1] << 32;
	}

	if (bus->info.base_addr == 0) {
		ERROR("PCI BAR not assigned\n");
		free(bus);
		return B_ERROR;
	}

	// enable power
	pci->set_powerstate(device, PCI_pm_state_d0);

	// enable bus master and memory
	uint16 pcicmd = pci->read_pci_config(device, PCI_command, 2);
	pcicmd |= PCI_command_master | PCI_command_memory;
	pci->write_pci_config(device, PCI_command, 2, pcicmd);

	// try MSI-X
	if (pci->get_msix_count(device) >= 1) {
		uint32 vector;
		if (pci->configure_msix(device, 1, &vector) == B_OK
			&& pci->enable_msix(device) == B_OK) {
			TRACE_ALWAYS("using MSI-X vector %" B_PRIu32 "\n", vector);
			bus->info.irq = vector;
			bus->irq_type = PCH_I2C_IRQ_MSI_X_SHARED;
		} else {
			ERROR("couldn't use MSI-X SHARED\n");
		}
	} else if (pci->get_msi_count(device) >= 1) {
		// try MSI
		uint32 vector;
		if (pci->configure_msi(device, 1, &vector) == B_OK
			&& pci->enable_msi(device) == B_OK) {
			TRACE_ALWAYS("using MSI vector %" B_PRIu32 "\n", vector);
			bus->info.irq = vector;
			bus->irq_type = PCH_I2C_IRQ_MSI;
		} else {
			ERROR("couldn't use MSI\n");
		}
	}
	if (bus->irq_type == PCH_I2C_IRQ_LEGACY) {
		bus->info.irq = pciInfo->u.h0.interrupt_line;
		if (bus->info.irq == 0xff)
			bus->info.irq = 0;

		TRACE_ALWAYS("using legacy interrupt %" B_PRIu32 "\n", bus->info.irq);
	}
	if (bus->info.irq == 0) {
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
		bus->pci->disable_msi(bus->device);
		bus->pci->unconfigure_msi(bus->device);
	}
	free(bus);
}


static status_t
register_device(device_node* parent)
{
	device_attr attrs[] = {
		{B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {.string = "PCH I2C PCI"}},
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
		size_t dev = 0;
		bool found = false;

		while (pch_pci_devices[dev].id != 0) {
			if (pch_pci_devices[dev].id == deviceID) {
				found = true;
				break;
			}
			dev++;
		}

		if (!found)
			return 0.0f;

		pci_device_module_info* pci;
		pci_device* device;
		gDeviceManager->get_driver(parent, (driver_module_info**)&pci,
			(void**)&device);
#ifdef TRACE_PCH_I2C
		uint8 pciSubDeviceId = pci->read_pci_config(device, PCI_revision,
			1);

		TRACE("PCH I2C device found! vendor 0x%04x, device 0x%04x, subdevice 0x%02x\n", vendorID,
			deviceID, pciSubDeviceId);
#endif
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

