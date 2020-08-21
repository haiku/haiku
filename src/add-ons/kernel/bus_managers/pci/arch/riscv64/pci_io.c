/*
 * Copyright 2009-2020, Haiku Inc., All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "pci_io.h"
#include "pci_private.h"
#warning RISCV64: WRITEME


status_t
pci_io_init()
{
	return B_OK;
}


uint8
pci_read_io_8(int mapped_io_addr)
{
	/* NOT IMPLEMENTED */
	return 0;
}


void
pci_write_io_8(int mapped_io_addr, uint8 value)
{
	/* NOT IMPLEMENTED */
}


uint16
pci_read_io_16(int mapped_io_addr)
{
	/* NOT IMPLEMENTED */
	return 0;
}


void
pci_write_io_16(int mapped_io_addr, uint16 value)
{
	/* NOT IMPLEMENTED */
}


uint32
pci_read_io_32(int mapped_io_addr)
{
	/* NOT IMPLEMENTED */
	return 0;
}


void
pci_write_io_32(int mapped_io_addr, uint32 value)
{
	/* NOT IMPLEMENTED */
}
