/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef _KERNEL_ARCH_RISCV64_VM_TRANSLATION_MAP_H
#define _KERNEL_ARCH_RISCV64_VM_TRANSLATION_MAP_H

#include <arch/vm_translation_map.h>


//gVirtFromPhysOffset = virtAdr - physAdr;
extern ssize_t gVirtFromPhysOffset;


static inline void*
VirtFromPhys(phys_addr_t physAdr)
{
	return (void*)(physAdr + gVirtFromPhysOffset);
}


static inline phys_addr_t
PhysFromVirt(void* virtAdr)
{
	return (phys_addr_t)virtAdr - gVirtFromPhysOffset;
}


#endif /* _KERNEL_ARCH_RISCV64_VM_TRANSLATION_MAP_H */
