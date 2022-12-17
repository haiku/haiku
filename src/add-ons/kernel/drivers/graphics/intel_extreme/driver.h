/*
 * Copyright 2006-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef DRIVER_H
#define DRIVER_H


#include <KernelExport.h>
#include <PCI.h>

#include <kernel/lock.h>

#include "intel_extreme_private.h"


extern char* gDeviceNames[];
extern intel_info* gDeviceInfo[];
extern pci_module_info* gPCI;
extern agp_gart_module_info* gGART;
extern mutex gLock;


static inline uint32
get_pci_config(pci_info* info, uint8 offset, uint8 size)
{
	return gPCI->read_pci_config(info->bus, info->device, info->function,
		offset, size);
}


static inline void
set_pci_config(pci_info* info, uint8 offset, uint8 size, uint32 value)
{
	gPCI->write_pci_config(info->bus, info->device, info->function, offset,
		size, value);
}


static inline uint16
read16(intel_info &info, uint32 encodedRegister)
{
	return *(volatile uint16*)(info.registers
		+ info.shared_info->register_blocks[REGISTER_BLOCK(encodedRegister)]
		+ REGISTER_REGISTER(encodedRegister));
}


static inline uint32
read32(intel_info &info, uint32 encodedRegister)
{
	return *(volatile uint32*)(info.registers
		+ info.shared_info->register_blocks[REGISTER_BLOCK(encodedRegister)]
		+ REGISTER_REGISTER(encodedRegister));
}


static inline void
write16(intel_info &info, uint32 encodedRegister, uint16 value)
{
	*(volatile uint16*)(info.registers
		+ info.shared_info->register_blocks[REGISTER_BLOCK(encodedRegister)]
		+ REGISTER_REGISTER(encodedRegister)) = value;
}


static inline void
write32(intel_info &info, uint32 encodedRegister, uint32 value)
{
	*(volatile uint32*)(info.registers
		+ info.shared_info->register_blocks[REGISTER_BLOCK(encodedRegister)]
		+ REGISTER_REGISTER(encodedRegister)) = value;
}

#endif  /* DRIVER_H */
