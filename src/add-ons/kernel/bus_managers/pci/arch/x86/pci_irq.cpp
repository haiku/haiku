/*
 * Copyright 2006, Marcus Overhagen. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#include "pci_irq.h"


status_t 
pci_x86_irq_init(void)
{
	return B_OK;
}


status_t 
pci_x86_irq_read(void *cookie,
				 uint8 bus, uint8 device, uint8 function, 
				 uint8 pin, uint8 *irq)
{
	return B_ERROR;
}


status_t 
pci_x86_irq_write(void *cookie,
				  uint8 bus, uint8 device, uint8 function, 
				  uint8 pin, uint8 irq)
{
	return B_ERROR;
}

