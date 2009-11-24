/*
 * Copyright 2005-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DRIVER_H
#define DRIVER_H


#include <ISA.h>
#include <KernelExport.h>

#include <lock.h>

#include "vesa_private.h"


extern char* gDeviceNames[];
extern vesa_info* gDeviceInfo[];
extern isa_module_info* gISA;
extern mutex gLock;

#endif  /* DRIVER_H */
