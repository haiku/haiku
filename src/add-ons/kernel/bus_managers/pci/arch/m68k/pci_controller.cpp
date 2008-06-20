/*
 * Copyright 2007, François Revol <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */


#include "pci_controller.h"

#include <arch_platform.h>

#include "pci_private.h"

/*
 * As we don't have any real PCI bus in any of the supported m68k platforms,
 * we fake one, with hardcoded devices.
 * TODO: this doesn't make any sense, as we can't share any drivers, anyway.
 * We should better export dedicated bus managers for Zorro etc. busses on
 * these systems.
 * TODO: actually, there are several PCI boards for the Amiga.
 */

#include "amiga/pci_amiga.h"
#include "apple/pci_apple.h"
#include "atari/pci_atari.h"


status_t
pci_controller_init(void)
{
	switch (M68KPlatform::Default()->PlatformType()) {
/*		case M68K_PLATFORM_AMIGA:
			return m68k_amiga_pci_controller_init();
			break;
		case M68K_PLATFORM_APPLE:
			return m68k_apple_pci_controller_init();
			break;*/
		case M68K_PLATFORM_ATARI:
			return m68k_atari_pci_controller_init();
			break;
		default:
			return EINVAL;
	}
	return B_OK;
}


void *
pci_ram_address(const void *physical_address_in_system_memory)
{
	return (void *)physical_address_in_system_memory;
}
