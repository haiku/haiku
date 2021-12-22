/*
 * Copyright 2009-2020, Haiku Inc., All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "arch_pci_controller.h"
#include "pci_io.h"
#include "pci_private.h"

#include <AutoDeleterOS.h>


#undef TRACE

//#define TRACE_PCI_IO
#ifdef TRACE_PCI_IO
#	define TRACE(x...) fprintf("PCI_IO: " x)
#else
#	define TRACE(x...) ;
#endif


extern ArchPCIController* gArchPCI;
addr_t gPCIeIoBase;


status_t
pci_io_init()
{
	TRACE("pci_io_init()\n");
	if (gArchPCI == NULL)
		return B_ERROR;

	gPCIeIoBase = gArchPCI->GetIoRegs();
	return B_OK;
}


uint8
pci_read_io_8(int mapped_io_addr)
{
	TRACE("pci_read_io_8(%d)\n", mapped_io_addr);
	volatile uint8* ptr = (uint8*)(gPCIeIoBase + mapped_io_addr);
	return *ptr;
}


void
pci_write_io_8(int mapped_io_addr, uint8 value)
{
	TRACE("pci_write_io_8(%d)\n", mapped_io_addr);
	volatile uint8* ptr = (uint8*)(gPCIeIoBase + mapped_io_addr);
	*ptr = value;
}


uint16
pci_read_io_16(int mapped_io_addr)
{
	TRACE("pci_read_io_16(%d)\n", mapped_io_addr);
	volatile uint16* ptr = (uint16*)(gPCIeIoBase + mapped_io_addr);
	return *ptr;
}


void
pci_write_io_16(int mapped_io_addr, uint16 value)
{
	TRACE("pci_write_io_16(%d)\n", mapped_io_addr);
	volatile uint16* ptr = (uint16*)(gPCIeIoBase + mapped_io_addr);
	*ptr = value;
}


uint32
pci_read_io_32(int mapped_io_addr)
{
	TRACE("pci_read_io_32(%d)\n", mapped_io_addr);
	volatile uint32* ptr = (uint32*)(gPCIeIoBase + mapped_io_addr);
	return *ptr;
}


void
pci_write_io_32(int mapped_io_addr, uint32 value)
{
	TRACE("pci_write_io_32(%d)\n", mapped_io_addr);
	volatile uint32* ptr = (uint32*)(gPCIeIoBase + mapped_io_addr);
	*ptr = value;
}
