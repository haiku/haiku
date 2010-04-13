/*
 *	Copyright 2010, Michael Lotz, mmlr@mlotz.ch. All Rights Reserved.
 *	Distributed under the terms of the MIT License.
 */
#ifndef _PCI_ARCH_INFO_H
#define _PCI_ARCH_INFO_H

#include "pci_msi.h"

typedef struct pci_arch_info {
	msi_info	msi;
} pci_arch_info;


void pci_read_arch_info(PCIDev *device);

#endif // _PCI_ARCH_INFO_H
