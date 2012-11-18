/*
 * Copyright 2006, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <KernelExport.h>
#include <driver_settings.h>
#include <string.h>
#include "pci_irq.h"
#include "pci_bios.h"
#include "pci_private.h"
#include "pci_controller.h"
#include "arch_cpu.h"

#define PCI_MECH1_REQ_PORT				0xCF8
#define PCI_MECH1_DATA_PORT 			0xCFC
#define PCI_MECH1_REQ_DATA(bus, device, func, offset) \
	(0x80000000 | (bus << 16) | (device << 11) | (func << 8) | (offset & ~3))

#define PCI_MECH2_ENABLE_PORT			0x0cf8
#define PCI_MECH2_FORWARD_PORT			0x0cfa
#define PCI_MECH2_CONFIG_PORT(dev, offset) \
	(uint16)(0xC00 | (dev << 8) | offset)

#define PCI_LOCK_CONFIG(cpu_status)			\
{											\
       cpu_status = disable_interrupts();	\
       acquire_spinlock(&sConfigLock);		\
}

#define PCI_UNLOCK_CONFIG(cpu_status)		\
{											\
       release_spinlock(&sConfigLock);		\
       restore_interrupts(cpu_status);		\
}

spinlock sConfigLock = B_SPINLOCK_INITIALIZER;

static status_t
pci_mech1_read_config(void *cookie, uint8 bus, uint8 device, uint8 function,
					  uint8 offset, uint8 size, uint32 *value)
{
	cpu_status cpu;
	status_t status = B_OK;

	PCI_LOCK_CONFIG(cpu);
	out32(PCI_MECH1_REQ_DATA(bus, device, function, offset), PCI_MECH1_REQ_PORT);
	switch (size) {
		case 1:
			*value = in8(PCI_MECH1_DATA_PORT + (offset & 3));
			break;
		case 2:
			*value = in16(PCI_MECH1_DATA_PORT + (offset & 3));
			break;
		case 4:
			*value = in32(PCI_MECH1_DATA_PORT);
			break;
		default:
			status = B_ERROR;
			break;
	}
	PCI_UNLOCK_CONFIG(cpu);

	return status;
}


static status_t
pci_mech1_write_config(void *cookie, uint8 bus, uint8 device, uint8 function,
					   uint8 offset, uint8 size, uint32 value)
{
	cpu_status cpu;
	status_t status = B_OK;

	PCI_LOCK_CONFIG(cpu);
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
			status = B_ERROR;
			break;
	}
	PCI_UNLOCK_CONFIG(cpu);

	return status;
}


static status_t
pci_mech1_get_max_bus_devices(void *cookie, int32 *count)
{
	*count = 32;
	return B_OK;
}


static status_t
pci_mech2_read_config(void *cookie, uint8 bus, uint8 device, uint8 function,
					  uint8 offset, uint8 size, uint32 *value)
{
	cpu_status cpu;
	status_t status = B_OK;

	PCI_LOCK_CONFIG(cpu);
	out8((uint8)(0xf0 | (function << 1)), PCI_MECH2_ENABLE_PORT);
	out8(bus, PCI_MECH2_FORWARD_PORT);
	switch (size) {
		case 1:
			*value = in8(PCI_MECH2_CONFIG_PORT(device, offset));
			break;
		case 2:
			*value = in16(PCI_MECH2_CONFIG_PORT(device, offset));
			break;
		case 4:
			*value = in32(PCI_MECH2_CONFIG_PORT(device, offset));
			break;
		default:
			status = B_ERROR;
			break;
	}
	out8(0, PCI_MECH2_ENABLE_PORT);
	PCI_UNLOCK_CONFIG(cpu);

	return status;
}


static status_t
pci_mech2_write_config(void *cookie, uint8 bus, uint8 device, uint8 function,
					   uint8 offset, uint8 size, uint32 value)
{
	cpu_status cpu;
	status_t status = B_OK;

	PCI_LOCK_CONFIG(cpu);
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
			status = B_ERROR;
			break;
	}
	out8(0, PCI_MECH2_ENABLE_PORT);
	PCI_UNLOCK_CONFIG(cpu);

	return status;
}


static status_t
pci_mech2_get_max_bus_devices(void *cookie, int32 *count)
{
	*count = 16;
	return B_OK;
}


void *
pci_ram_address(const void *physical_address_in_system_memory)
{
	return (void *)physical_address_in_system_memory;
}


pci_controller pci_controller_x86_mech1 =
{
	pci_mech1_read_config,
	pci_mech1_write_config,
	pci_mech1_get_max_bus_devices,
	pci_x86_irq_read,
	pci_x86_irq_write,
};

pci_controller pci_controller_x86_mech2 =
{
	pci_mech2_read_config,
	pci_mech2_write_config,
	pci_mech2_get_max_bus_devices,
	pci_x86_irq_read,
	pci_x86_irq_write,
};

pci_controller pci_controller_x86_bios =
{
	pci_bios_read_config,
	pci_bios_write_config,
	pci_bios_get_max_bus_devices,
	pci_x86_irq_read,
	pci_x86_irq_write,
};


status_t
pci_controller_init(void)
{
	bool search_mech1 = true;
	bool search_mech2 = true;
	bool search_bios = true;
	void *config = NULL;
	status_t status;

	status = pci_x86_irq_init();
	if (status != B_OK)
		return status;

	config = load_driver_settings("pci");
	if (config) {
		const char *mech = get_driver_parameter(config, "mechanism",
			NULL, NULL);
		if (mech) {
			search_mech1 = search_mech2 = search_bios = false;
			if (strcmp(mech, "1") == 0)
				search_mech1 = true;
			else if (strcmp(mech, "2") == 0)
				search_mech2 = true;
			else if (strcmp(mech, "bios") == 0)
				search_bios = true;
			else
				panic("Unknown pci config mechanism setting %s\n", mech);
		}
		unload_driver_settings(config);
	}

	// TODO: check safemode "don't call the BIOS" setting and unset search_bios!

	// PCI configuration mechanism 1 is the preferred one.
	// If it doesn't work, try mechanism 2.
	// Finally, try to fallback to PCI BIOS

	if (search_mech1) {
		// check for mechanism 1
		out32(0x80000000, PCI_MECH1_REQ_PORT);
		if (0x80000000 == in32(PCI_MECH1_REQ_PORT)) {
			dprintf("PCI: mechanism 1 controller found\n");
			return pci_controller_add(&pci_controller_x86_mech1, NULL);
		}
	}

	if (search_mech2) {
		// check for mechanism 2
		out8(0x00, 0xCFB);
		out8(0x00, 0xCF8);
		out8(0x00, 0xCFA);
		if (in8(0xCF8) == 0x00 && in8(0xCFA) == 0x00) {
			dprintf("PCI: mechanism 2 controller found\n");
			return pci_controller_add(&pci_controller_x86_mech2, NULL);
		}
	}

	if (search_bios) {
		// check for PCI BIOS
		if (pci_bios_init() == B_OK) {
			dprintf("PCI: BIOS support found\n");
			return pci_controller_add(&pci_controller_x86_bios, NULL);
		}
	}

	dprintf("PCI: no configuration mechanism found\n");
	return B_ERROR;
}


