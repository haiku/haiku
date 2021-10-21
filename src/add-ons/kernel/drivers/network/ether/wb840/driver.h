/* Copyright (c) 2003-2004 
 * Stefano Ceccherini <burton666@libero.it>. All rights reserved.
 * This file is released under the MIT license
 */
#ifndef __DRIVER_H
#define __DRIVER_H

#include <Drivers.h>
#include <PCI.h>
#include "ether_driver.h"

#define DEVICE_NAME "net/wb840"

extern pci_module_info *gPci;
extern char* gDevNameList[];
extern pci_info *gDevList[];
extern device_hooks gDeviceHooks;



#endif // __WB840_H
