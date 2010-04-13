/*
 *	Copyright 2010, Michael Lotz, mmlr@mlotz.ch. All Rights Reserved.
 *	Distributed under the terms of the MIT License.
 */

#include "pci_arch_info.h"

void
pci_read_arch_info(PCIDev *dev)
{
	pci_read_msi_info(dev);
}
