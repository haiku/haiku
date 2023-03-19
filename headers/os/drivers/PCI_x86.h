/*
 *	Copyright 2010, Haiku Inc. All Rights Reserved.
 *	Distributed under the terms of the MIT License.
 */
#ifndef PCI_X86_H
#define PCI_X86_H

// DEPRECATED. Use `pci_module_info` or `pci_device_module_info` instead.

#include <SupportDefs.h>
#include <module.h>

typedef struct pci_x86_module_info {
	module_info		info;

	uint8			(*get_msi_count)(
						uint8 bus,				/* bus number */
						uint8 device,			/* device # on bus */
						uint8 function);		/* function # in device */

	status_t		(*configure_msi)(
						uint8 bus,				/* bus number */
						uint8 device,			/* device # on bus */
						uint8 function,			/* function # in device */
						uint8 count,			/* count of vectors desired */
						uint8 *startVector);	/* first configured vector */
	status_t		(*unconfigure_msi)(
						uint8 bus,				/* bus number */
						uint8 device,			/* device # on bus */
						uint8 function);		/* function # in device */

	status_t		(*enable_msi)(
						uint8 bus,				/* bus number */
						uint8 device,			/* device # on bus */
						uint8 function);		/* function # in device */
	status_t		(*disable_msi)(
						uint8 bus,				/* bus number */
						uint8 device,			/* device # on bus */
						uint8 function);		/* function # in device */

	uint8			(*get_msix_count)(
						uint8 bus,				/* bus number */
						uint8 device,			/* device # on bus */
						uint8 function);		/* function # in device */

	status_t		(*configure_msix)(
						uint8 bus,				/* bus number */
						uint8 device,			/* device # on bus */
						uint8 function,			/* function # in device */
						uint8 count,			/* count of vectors desired */
						uint8 *startVector);	/* first configured vector */
	status_t		(*enable_msix)(
						uint8 bus,				/* bus number */
						uint8 device,			/* device # on bus */
						uint8 function);		/* function # in device */

} pci_x86_module_info;

#define B_PCI_X86_MODULE_NAME	"bus_managers/pci/x86/v1"

#endif // PCI_X86_H
