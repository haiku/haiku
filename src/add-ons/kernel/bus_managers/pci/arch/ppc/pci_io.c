/*
 * Copyright 2006, Ingo Weinhold. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "pci_io.h"
#include "pci_private.h"


status_t
pci_io_init()
{
	return B_OK;
}


uint8
pci_read_io_8(int mapped_io_addr)
{
	return ppc_in8((vuint8*)mapped_io_addr);
}


void
pci_write_io_8(int mapped_io_addr, uint8 value)
{
	ppc_out8((vuint8*)mapped_io_addr, value);
}


uint16
pci_read_io_16(int mapped_io_addr)
{
	return ppc_in16((vuint16*)mapped_io_addr);
}


void
pci_write_io_16(int mapped_io_addr, uint16 value)
{
	ppc_out16((vuint16*)mapped_io_addr, value);
}


uint32
pci_read_io_32(int mapped_io_addr)
{
	return ppc_in32((vuint32*)mapped_io_addr);
}


void
pci_write_io_32(int mapped_io_addr, uint32 value)
{
	ppc_out32((vuint32*)mapped_io_addr, value);
}
