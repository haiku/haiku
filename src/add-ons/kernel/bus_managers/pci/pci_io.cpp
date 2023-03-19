/*
 * Copyright 2006, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "pci.h"
#include "pci_private.h"
#include "arch_cpu.h"


//#define TRACE_PCI_IO
#undef TRACE
#ifdef TRACE_PCI_IO
#	define TRACE(x...) dprintf("PCI_IO: " x)
#else
#	define TRACE(x...) ;
#endif


#if defined(__i386__) || defined(__x86_64__)

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

#else

static uint8*
get_io_port_address(int ioPort)
{
	uint8 domain;
	pci_resource_range range;
	uint8 *mappedAdr;

	if (gPCI->LookupRange(kPciRangeIoPort, ioPort, domain, range, &mappedAdr) < B_OK)
		return NULL;

	return mappedAdr + ioPort;
}


uint8
pci_read_io_8(int mapped_io_addr)
{
	TRACE("pci_read_io_8(%d)\n", mapped_io_addr);
	vuint8* ptr = get_io_port_address(mapped_io_addr);
	if (ptr == NULL)
		return 0;

	return *ptr;
}


void
pci_write_io_8(int mapped_io_addr, uint8 value)
{
	TRACE("pci_write_io_8(%d)\n", mapped_io_addr);
	vuint8* ptr = get_io_port_address(mapped_io_addr);
	if (ptr == NULL)
		return;

	*ptr = value;
}


uint16
pci_read_io_16(int mapped_io_addr)
{
	TRACE("pci_read_io_16(%d)\n", mapped_io_addr);
	vuint16* ptr = (uint16*)get_io_port_address(mapped_io_addr);
	if (ptr == NULL)
		return 0;

	return *ptr;
}


void
pci_write_io_16(int mapped_io_addr, uint16 value)
{
	TRACE("pci_write_io_16(%d)\n", mapped_io_addr);
	vuint16* ptr = (uint16*)get_io_port_address(mapped_io_addr);
	if (ptr == NULL)
		return;

	*ptr = value;
}


uint32
pci_read_io_32(int mapped_io_addr)
{
	TRACE("pci_read_io_32(%d)\n", mapped_io_addr);
	vuint32* ptr = (uint32*)get_io_port_address(mapped_io_addr);
	if (ptr == NULL)
		return 0;

	return *ptr;
}


void
pci_write_io_32(int mapped_io_addr, uint32 value)
{
	TRACE("pci_write_io_32(%d)\n", mapped_io_addr);
	vuint32* ptr = (vuint32*)get_io_port_address(mapped_io_addr);
	if (ptr == NULL)
		return;

	*ptr = value;
}

#endif
