/*
 * Copyright 2009, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */


#include "pci_controller.h"

//#include <arch_platform.h>

#include "pci_private.h"


status_t
pci_controller_init(void)
{
	/* no support yet */
#warning ARM:WRITEME
	return B_OK;
}


void *
pci_ram_address(const void *physical_address_in_system_memory)
{
	return (void *)physical_address_in_system_memory;
}
