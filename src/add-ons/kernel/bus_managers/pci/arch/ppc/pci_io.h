/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef PCI_BUS_MANAGER_PPC_IO_H
#define PCI_BUS_MANAGER_PPC_IO_H

#include <SupportDefs.h>

static inline void
ppc_out8(vuint8 *address, uint8 value)
{
	*address = value;
	asm volatile("eieio; sync");
}


static inline void
ppc_out16(vuint16 *address, uint16 value)
{
	*address = value;
	asm volatile("eieio; sync");
}


static inline void
ppc_out16_reverse(vuint16 *address, uint16 value)
{
	asm volatile("sthbrx %1, 0, %0" : : "r"(address), "r"(value));
	asm volatile("eieio; sync");
}


static inline void
ppc_out32(vuint32 *address, uint32 value)
{
	*address = value;
	asm volatile("eieio; sync");
}


static inline void
ppc_out32_reverse(vuint32 *address, uint32 value)
{
	asm volatile("stwbrx %1, 0, %0" : : "r"(address), "r"(value));
	asm volatile("eieio; sync");
}


static inline uint8
ppc_in8(const vuint8 *address)
{
	uint8 value = *address;
	asm volatile("eieio; sync");
	return value;
}


static inline uint16
ppc_in16(const vuint16 *address)
{
	uint16 value = *address;
	asm volatile("eieio; sync");
	return value;
}


static inline uint16
ppc_in16_reverse(const vuint16 *address)
{
	uint16 value;
	asm volatile("lhbrx %0, 0, %1" : "=r"(value) : "r"(address));
	asm volatile("eieio; sync");
	return value;
}


static inline uint32
ppc_in32(const vuint32 *address)
{
	uint32 value = *address;
	asm volatile("eieio; sync");
	return value;
}


static inline uint32
ppc_in32_reverse(const vuint32 *address)
{
	uint32 value;
	asm volatile("lwbrx %0, 0, %1" : "=r"(value) : "r"(address));
	asm volatile("eieio; sync");
	return value;
}

#endif	// PCI_BUS_MANAGER_PPC_IO_H
