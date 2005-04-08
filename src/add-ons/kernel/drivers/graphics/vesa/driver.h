/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef DRIVER_H
#define DRIVER_H


#include <KernelExport.h>
#include <PCI.h>

#include "vesa_info.h"
#include "lock.h"


extern char *gDeviceNames[];
extern vesa_info *gDeviceInfo[];
extern lock gLock;

#endif  /* DRIVER_H */
