/*
 * Copyright 2005-2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2016, Jessica Hamilton, jessica.l.hamilton@gmail.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef DRIVER_H
#define DRIVER_H


#include <ISA.h>
#include <KernelExport.h>

#include <lock.h>

#include "framebuffer_private.h"


extern char* gDeviceNames[];
extern framebuffer_info* gDeviceInfo[];
extern mutex gLock;

#endif  /* DRIVER_H */
