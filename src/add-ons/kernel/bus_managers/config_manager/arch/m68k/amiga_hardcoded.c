/*
 * Copyright 2007 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * arch-specific config manager
 *
 * Authors (in chronological order):
 *              François Revol (revol@free.fr)
 */

#include "config_manager_arch.h"

#if 0

#define DIS sizeof(struct device_info)
#define DIF (B_DEVICE_INFO_ENABLED|B_DEVICE_INFO_CONFIGURED)
#define DID 'm', '6', '8',

static struct hardcoded_device gHardcodedDevices[] = {
/* video */
	{{DIS, DIS, B_ISA_BUS, {PCI_display, PCI_display_other, 0xff}, {DID, 0}, DIF, B_OK },
			.isa = {}},
/* ide */
	{{DIS, DIS, B_ISA_BUS, {PCI_mass_storage, PCI_ide, 0xff}, {DID, 1}, DIF, B_OK },
			.isa = {}}
/* scsi */
	{{DIS, DIS, B_ISA_BUS, {PCI_mass_storage, PCI_scsi, 0xff}, {DID, 2}, DIF, B_OK },
			.isa = {}}
/* audio */
	{{DIS, DIS, B_ISA_BUS, {PCI_multimedia, 0xff, 0xff}, {DID, 3}, DIF, B_OK },
			.isa = {}}
};

#endif

status_t
amiga_hardcoded(struct device_info **info, int32 *count)
{
	return B_OK;
}


