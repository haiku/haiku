/*
 * Copyright 2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PCI2_H
#define _PCI2_H


#include <device_manager.h>
#include <PCI.h>


typedef struct pci_device pci_device;

typedef struct pci_device_module_info {
	driver_module_info info;

	uint8	(*read_io_8)(pci_device *device, addr_t mappedIOAddress);
	void	(*write_io_8)(pci_device *device, addr_t mappedIOAddress,
				uint8 value);
	uint16	(*read_io_16)(pci_device *device, addr_t mappedIOAddress);
	void	(*write_io_16)(pci_device *device, addr_t mappedIOAddress,
				uint16 value);
	uint32	(*read_io_32)(pci_device *device, addr_t mappedIOAddress);
	void	(*write_io_32)(pci_device *device, addr_t mappedIOAddress,
				uint32 value);

	phys_addr_t	(*ram_address)(pci_device *device, phys_addr_t physicalAddress);

	uint32	(*read_pci_config)(pci_device *device, uint16 offset,
				uint8 size);
	void	(*write_pci_config)(pci_device *device, uint16 offset,
				uint8 size, uint32 value);
	status_t (*find_pci_capability)(pci_device *device, uint8 capID,
				uint8 *offset);
	void 	(*get_pci_info)(pci_device *device, struct pci_info *info);
	status_t (*find_pci_extended_capability)(pci_device *device, uint16 capID,
				uint16 *offset);
	uint8	(*get_powerstate)(pci_device *device);
	void	(*set_powerstate)(pci_device *device, uint8 state);

	// MSI/MSI-X
	uint8	(*get_msi_count)(pci_device *device);
	status_t (*configure_msi)(pci_device *device,
				uint8 count,
				uint8 *startVector);
	status_t (*unconfigure_msi)(pci_device *device);

	status_t (*enable_msi)(pci_device *device);
	status_t (*disable_msi)(pci_device *device);

	uint8	(*get_msix_count)(pci_device *device);
	status_t (*configure_msix)(pci_device *device,
				uint8 count,
				uint8 *startVector);
	status_t (*enable_msix)(pci_device *device);

} pci_device_module_info;


typedef struct pci_resource_range {
	uint32 type;
	uint8 address_type;
	phys_addr_t host_address;
	phys_addr_t pci_address;
	uint64 size;
} pci_resource_range;


typedef struct pci_controller_module_info {
	driver_module_info info;

	// read PCI config space
	status_t	(*read_pci_config)(void *cookie,
				uint8 bus, uint8 device, uint8 function,
				uint16 offset, uint8 size, uint32 *value);

	// write PCI config space
	status_t	(*write_pci_config)(void *cookie,
				uint8 bus, uint8 device, uint8 function,
				uint16 offset, uint8 size, uint32 value);

	status_t	(*get_max_bus_devices)(void *cookie, int32 *count);

	status_t	(*read_pci_irq)(void *cookie,
				uint8 bus, uint8 device, uint8 function,
				uint8 pin, uint8 *irq);

	status_t	(*write_pci_irq)(void *cookie,
				uint8 bus, uint8 device, uint8 function,
				uint8 pin, uint8 irq);

	status_t	(*get_range)(void *cookie, uint32 index, pci_resource_range *range);

	status_t	(*finalize)(void *cookie);

} pci_controller_module_info;


/* Attributes of PCI device nodes */
#define B_PCI_DEVICE_DOMAIN		"pci/domain"		/* uint32 */
#define B_PCI_DEVICE_BUS		"pci/bus"			/* uint8 */
#define B_PCI_DEVICE_DEVICE		"pci/device"		/* uint8 */
#define B_PCI_DEVICE_FUNCTION	"pci/function"		/* uint8 */

#endif	/* _PCI2_H */
