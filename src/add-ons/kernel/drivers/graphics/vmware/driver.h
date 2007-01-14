/*
 * Copyright 2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Eric Petit <eric.petit@lapsus.org>
 */
#ifndef DRIVER_H
#define DRIVER_H


#define TRACE(a...) dprintf("VMware: " a)

#include "DriverInterface.h"

typedef struct {
	char		*names[2];
	Benaphore	kernel;
	uint32		isOpen;
	area_id		sharedArea;
	SharedInfo	*si;
	pci_info	pcii;
} DeviceData;


/*--------------------------------------------------------------------*/
/* Global variables */

extern DeviceData *gPd;
extern pci_module_info *gPciBus;
extern device_hooks gGraphicsDeviceHooks;


/*--------------------------------------------------------------------*/
/* Access to 32 bit registers in the IO address space. */

static inline uint32
inl(uint16 port)
{
	uint32 ret;
	__asm__ __volatile__("inl %w1, %0" : "=a" (ret) : "Nd" (port));
	return ret;
}

static inline void
outl(uint16 port, uint32 value)
{
	__asm__ __volatile__("outl %1, %w0" :: "Nd" (port), "a" (value));
}

static inline uint32
ReadReg(uint32 index)
{
	outl(gPd->si->indexPort, index);
	return inl(gPd->si->valuePort);
}

static inline void
WriteReg(uint32 index, uint32 value)
{
	outl(gPd->si->indexPort, index);
	outl(gPd->si->valuePort, value);
}

#endif	// DRIVER_H
