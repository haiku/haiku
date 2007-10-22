/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef PCI_BUS_MANAGER_M68K_IO_H
#define PCI_BUS_MANAGER_M68K_IO_H

#include <SupportDefs.h>

#define IOBARRIER() asm volatile("nop") /* nop flushes the instruction pipeline */
/*XXX: sync cache/pmmu?*/

static inline void
m68k_out8(vuint8 *address, uint8 value)
{
	*address = value;
	IOBARRIER();
}


static inline void
m68k_out16(vuint16 *address, uint16 value)
{
	*address = value;
	IOBARRIER();
}

#if _XXX_HAS_REVERSE_IO
static inline void
m68k_out16_reverse(vuint16 *address, uint16 value)
{
	asm volatile("sthbrx %1, 0, %0" : : "r"(address), "r"(value));
	IOBARRIER();
}
#endif


static inline void
m68k_out32(vuint32 *address, uint32 value)
{
	*address = value;
	IOBARRIER();
}


#if _XXX_HAS_REVERSE_IO
static inline void
m68k_out32_reverse(vuint32 *address, uint32 value)
{
	asm volatile("stwbrx %1, 0, %0" : : "r"(address), "r"(value));
	IOBARRIER();
}
#endif

static inline uint8
m68k_in8(const vuint8 *address)
{
	uint8 value = *address;
	IOBARRIER();
	return value;
}


static inline uint16
m68k_in16(const vuint16 *address)
{
	uint16 value = *address;
	IOBARRIER();
	return value;
}


#if _XXX_HAS_REVERSE_IO
static inline uint16
m68k_in16_reverse(const vuint16 *address)
{
	uint16 value;
	asm volatile("lhbrx %0, 0, %1" : "=r"(value) : "r"(address));
	IOBARRIER();
	return value;
}
#endif


static inline uint32
m68k_in32(const vuint32 *address)
{
	uint32 value = *address;
	IOBARRIER();
	return value;
}


#if _XXX_HAS_REVERSE_IO
static inline uint32
m68k_in32_reverse(const vuint32 *address)
{
	uint32 value;
	asm volatile("lwbrx %0, 0, %1" : "=r"(value) : "r"(address));
	IOBARRIER();
	return value;
}
#endif

#endif	// PCI_BUS_MANAGER_M68K_IO_H
