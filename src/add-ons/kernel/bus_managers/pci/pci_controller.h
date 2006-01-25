/*
 * Copyright 2006, Marcus Overhagen. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef __PCI_CONTROLLER_H
#define __PCI_CONTROLLER_H

#include <SupportDefs.h>

typedef struct pci_controller
{
	// read PCI config space
	status_t	(*read_pci_config)(void *cookie, 
				uint8 bus, uint8 device, uint8 function,
				uint8 offset, uint8 size, uint32 *value);

	// write PCI config space
	status_t	(*write_pci_config)(void *cookie, 
				uint8 bus, uint8 device, uint8 function,
				uint8 offset, uint8 size, uint32 value);
				
	status_t	(*get_max_bus_devices)(void *cookie, int32 *count);
	
	status_t	(*read_pci_irq)(void *cookie,
				uint8 bus, uint8 device, uint8 function, 
				uint8 pin, uint8 *irq);

	status_t	(*write_pci_irq)(void *cookie,
				uint8 bus, uint8 device, uint8 function, 
				uint8 pin, uint8 irq);

} pci_controller;


#ifdef __cplusplus
extern "C" {
#endif

status_t	pci_controller_init(void);
status_t	pci_controller_add(pci_controller *controller, void *cookie);

#ifdef __cplusplus
}
#endif

#endif
