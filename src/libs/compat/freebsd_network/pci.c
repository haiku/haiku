/*
 * Copyright 2007, Hugo Santos, hugosantos@gmail.com. All Rights Reserved.
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All Rights Reserved.
 * Copyright 2004, Marcus Overhagen. All Rights Reserved.
 *
 * Distributed under the terms of the MIT License.
 */

#include "device.h"

#include <stdio.h>

#include <KernelExport.h>
#include <image.h>

#include <compat/machine/resource.h>
#include <compat/dev/pci/pcireg.h>
#include <compat/dev/pci/pcivar.h>
#include <compat/sys/bus.h>

#include <compat/dev/mii/miivar.h>

//#define DEBUG_PCI
#ifdef DEBUG_PCI
#	define TRACE_PCI(dev, format, args...) device_printf(dev, format , ##args)
#else
#	define TRACE_PCI(dev, format, args...) do { } while (0)
#endif


uint32_t
pci_read_config(device_t dev, int offset, int size)
{
	pci_info *info = &((struct root_device_softc *)dev->root->softc)->pci_info;

	uint32_t value = gPci->read_pci_config(info->bus, info->device,
		info->function, offset, size);
	TRACE_PCI(dev, "pci_read_config(%i, %i) = 0x%x\n", offset, size, value);
	return value;
}


void
pci_write_config(device_t dev, int offset, uint32_t value, int size)
{
	pci_info *info = &((struct root_device_softc *)dev->root->softc)->pci_info;

	TRACE_PCI(dev, "pci_write_config(%i, 0x%x, %i)\n", offset, value, size);

	gPci->write_pci_config(info->bus, info->device, info->function, offset,
		size, value);
}


uint16_t
pci_get_vendor(device_t dev)
{
	return pci_read_config(dev, PCI_vendor_id, 2);
}


uint16_t
pci_get_device(device_t dev)
{
	return pci_read_config(dev, PCI_device_id, 2);
}


uint16_t
pci_get_subvendor(device_t dev)
{
	return pci_read_config(dev, PCI_subsystem_vendor_id, 2);
}


uint16_t
pci_get_subdevice(device_t dev)
{
	return pci_read_config(dev, PCI_subsystem_id, 2);
}


uint8_t
pci_get_revid(device_t dev)
{
	return pci_read_config(dev, PCI_revision, 1);
}


static void
pci_set_command_bit(device_t dev, uint16_t bit)
{
	uint16_t command = pci_read_config(dev, PCI_command, 2);
	pci_write_config(dev, PCI_command, command | bit, 2);
}


int
pci_enable_busmaster(device_t dev)
{
	pci_set_command_bit(dev, PCI_command_master);
	return 0;
}


int
pci_enable_io(device_t dev, int space)
{
	/* adapted from FreeBSD's pci_enable_io_method */
	int bit = 0;

	switch (space) {
		case SYS_RES_IOPORT:
			bit = PCI_command_io;
			break;
		case SYS_RES_MEMORY:
			bit = PCI_command_memory;
			break;
		default:
			return EINVAL;
	}

	pci_set_command_bit(dev, bit);
	if (pci_read_config(dev, PCI_command, 2) & bit)
		return 0;

	device_printf(dev, "pci_enable_io(%d) failed.\n", space);

	return ENXIO;
}


int
pci_find_extcap(device_t child, int capability, int *_capabilityRegister)
{
	uint8 capabilityPointer;
	uint8 headerType;
	uint16 status;

	status = pci_read_config(child, PCIR_STATUS, 2);
	if ((status & PCIM_STATUS_CAPPRESENT) == 0)
		return ENXIO;

	headerType = pci_read_config(child, PCI_header_type, 1);
	switch (headerType & PCIM_HDRTYPE) {
		case 0:
		case 1:
			capabilityPointer = PCIR_CAP_PTR;
			break;
		case 2:
			capabilityPointer = PCIR_CAP_PTR_2;
			break;
		default:
			return ENXIO;
	}
	capabilityPointer = pci_read_config(child, capabilityPointer, 1);

	while (capabilityPointer != 0) {
		if (pci_read_config(child, capabilityPointer + PCICAP_ID, 1)
				== capability) {
			if (_capabilityRegister != NULL)
				*_capabilityRegister = capabilityPointer;
			return 0;
		}
		capabilityPointer = pci_read_config(child,
			capabilityPointer + PCICAP_NEXTPTR, 1);
	}

	return ENOENT;
}


int
pci_msi_count(device_t dev)
{
	return 0;
}


int
pci_alloc_msi(device_t dev, int *count)
{
    return ENODEV;
}


int
pci_release_msi(device_t dev)
{
    return ENODEV;
}


int
pci_msix_count(device_t dev)
{
	return 0;
}


int
pci_alloc_msix(device_t dev, int *count)
{
    return ENODEV;
}

