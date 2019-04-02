/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef PCI_BUS_MANAGER_SPARC_IO_H
#define PCI_BUS_MANAGER_SPARC_IO_H

#include <SupportDefs.h>

static inline void
sparc_out8(vuint8 *address, uint8 value)
{
	*address = value;
	asm volatile("MEMBAR #MemIssue");
}


static inline void
sparc_out16(vuint16 *address, uint16 value)
{
	*address = value;
	asm volatile("MEMBAR #MemIssue");
}


static inline void
sparc_out16_reverse(vuint16 *address, uint16 value)
{
	asm volatile("stha %1, [%0] 0x88" : : "r"(address), "r"(value));
	asm volatile("MEMBAR #MemIssue");
}


static inline void
sparc_out32(vuint32 *address, uint32 value)
{
	*address = value;
	asm volatile("MEMBAR #MemIssue");
}


static inline void
sparc_out32_reverse(vuint32 *address, uint32 value)
{
	asm volatile("stwa %1, [%0] 0x88" : : "r"(address), "r"(value));
	asm volatile("MEMBAR #MemIssue");
}


static inline uint8
sparc_in8(const vuint8 *address)
{
	uint8 value = *address;
	asm volatile("MEMBAR #MemIssue");
	return value;
}


static inline uint16
sparc_in16(const vuint16 *address)
{
	uint16 value = *address;
	asm volatile("MEMBAR #MemIssue");
	return value;
}


static inline uint16
sparc_in16_reverse(const vuint16 *address)
{
	uint16 value;
	asm volatile("lha [%1] 0x88,  %0" : "=r"(value) : "r"(address));
	asm volatile("MEMBAR #MemIssue");
	return value;
}


static inline uint32
sparc_in32(const vuint32 *address)
{
	uint32 value = *address;
	asm volatile("MEMBAR #MemIssue");
	return value;
}


static inline uint32
sparc_in32_reverse(const vuint32 *address)
{
	uint32 value;
	asm volatile("ldwa [%1] 0x88, %0" : "=r"(value) : "r"(address));
	asm volatile("MEMBAR #MemIssue");
	return value;
}

#endif	// PCI_BUS_MANAGER_SPARC_IO_H
