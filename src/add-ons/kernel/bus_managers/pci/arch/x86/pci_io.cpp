/*
 * Copyright 2006, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "pci_private.h"
#include "arch_cpu.h"


status_t
pci_io_init()
{
	// nothing to do on x86 hardware
	return B_OK;
}


uint8
pci_read_io_8(int mapped_io_addr)
{
	return in8(mapped_io_addr);
}


void
pci_write_io_8(int mapped_io_addr, uint8 value)
{
	out8(value, mapped_io_addr);
}


uint16
pci_read_io_16(int mapped_io_addr)
{
	return in16(mapped_io_addr);
}


void
pci_write_io_16(int mapped_io_addr, uint16 value)
{
	out16(value, mapped_io_addr);
}


uint32
pci_read_io_32(int mapped_io_addr)
{
	return in32(mapped_io_addr);
}


void
pci_write_io_32(int mapped_io_addr, uint32 value)
{
	out32(value, mapped_io_addr);
}
