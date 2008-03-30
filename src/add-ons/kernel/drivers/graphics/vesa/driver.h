/*
 * Copyright 2005-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef DRIVER_H
#define DRIVER_H


#include <KernelExport.h>
#include <ISA.h>

#include "vesa_private.h"
#include "lock.h"


extern char *gDeviceNames[];
extern vesa_info *gDeviceInfo[];
extern isa_module_info *gISA;
extern lock gLock;

#endif  /* DRIVER_H */
