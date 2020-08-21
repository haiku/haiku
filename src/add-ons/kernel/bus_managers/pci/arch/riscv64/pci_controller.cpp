/*
 * Copyright 2009-2020, Haiku Inc., All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "pci_controller.h"

//#include <arch_platform.h>

#include "pci_private.h"


status_t
pci_controller_init(void)
{
	/* no support yet */
#warning RISCV64: WRITEME
	return B_OK;
}


phys_addr_t
pci_ram_address(phys_addr_t physical_address_in_system_memory)
{
	return physical_address_in_system_memory;
}
