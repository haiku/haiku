/* Realtek RTL8169 Family Driver
 * Copyright (C) 2004 Marcus Overhagen <marcus@overhagen.de>. All rights reserved.
 */
#ifndef __DRIVER_H
#define __DRIVER_H

#include <Drivers.h>
#include <PCI.h>
#include "ether_driver.h"

#define MAX_CARDS 8

extern pci_module_info *gPci;

extern char* gDevNameList[];
extern pci_info *gDevList[];

#endif
