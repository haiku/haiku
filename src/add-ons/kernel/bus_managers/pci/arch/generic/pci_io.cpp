/*
 * Copyright 2022, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */


#include <SupportDefs.h>

#include "pci_private.h"

//#define TRACE_PCI_IO
#ifdef TRACE_PCI_IO
#	define TRACE(x...) dprintf("PCI_IO: " x)
#else
#	define TRACE(x...) ;
#endif


extern addr_t gPCIioBase;

status_t
pci_io_init()
{
	TRACE("pci_io_init()\n");
	return B_OK;
}


uint8
pci_read_io_8(int mapped_io_addr)
{
	TRACE("pci_read_io_8(%d)\n", mapped_io_addr);
	volatile uint8* ptr = (uint8*)(gPCIioBase + mapped_io_addr);
	return *ptr;
}


void
pci_write_io_8(int mapped_io_addr, uint8 value)
{
	TRACE("pci_write_io_8(%d)\n", mapped_io_addr);
	volatile uint8* ptr = (uint8*)(gPCIioBase + mapped_io_addr);
	*ptr = value;
}


uint16
pci_read_io_16(int mapped_io_addr)
{
	TRACE("pci_read_io_16(%d)\n", mapped_io_addr);
	volatile uint16* ptr = (uint16*)(gPCIioBase + mapped_io_addr);
	return *ptr;
}


void
pci_write_io_16(int mapped_io_addr, uint16 value)
{
	TRACE("pci_write_io_16(%d)\n", mapped_io_addr);
	volatile uint16* ptr = (uint16*)(gPCIioBase + mapped_io_addr);
	*ptr = value;
}


uint32
pci_read_io_32(int mapped_io_addr)
{
	TRACE("pci_read_io_32(%d)\n", mapped_io_addr);
	volatile uint32* ptr = (uint32*)(gPCIioBase + mapped_io_addr);
	return *ptr;
}


void
pci_write_io_32(int mapped_io_addr, uint32 value)
{
	TRACE("pci_write_io_32(%d)\n", mapped_io_addr);
	volatile uint32* ptr = (uint32*)(gPCIioBase + mapped_io_addr);
	*ptr = value;
}
