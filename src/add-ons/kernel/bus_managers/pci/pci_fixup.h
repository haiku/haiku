/*
 * Copyright 2007, Marcus Overhagen. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _PCI_FIXUP_H
#define _PCI_FIXUP_H

class PCI;

void
pci_fixup_device(PCI *pci, int domain, uint8 bus, uint8 device, uint8 function);

#endif
