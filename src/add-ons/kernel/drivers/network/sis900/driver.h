/*
 * Copyright (c) 2001-2007 pinc Software. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef DRIVER_H
#define DRIVER_H


#include <PCI.h>


// PCI Communications

#define IO_PORT_PCI_ACCESS true
//#define MEMORY_MAPPED_PCI_ACCESS true

#if IO_PORT_PCI_ACCESS
#	define write8(address,value)		(*pci->write_io_8)((address),(value))
#	define write16(address,value)		(*pci->write_io_16)((address),(value))
#	define write32(address,value)		(*pci->write_io_32)((address),(value))
#	define read8(address)				((*pci->read_io_8)(address))
#	define read16(address)				((*pci->read_io_16)(address))
#	define read32(address)				((*pci->read_io_32)(address))
#else	/* MEMORY_MAPPED_PCI_ACCESS */
#	define read8(address)   			(*((volatile uint8*)(address)))
#	define read16(address)  			(*((volatile uint16*)(address)))
#	define read32(address) 				(*((volatile uint32*)(address)))
#	define write8(address,data)  		(*((volatile uint8 *)(address))  = data)
#	define write16(address,data) 		(*((volatile uint16 *)(address)) = (data))
#	define write32(address,data) 		(*((volatile uint32 *)(address)) = (data))
#endif

extern char *gDeviceNames[];
extern pci_info *pciInfo[];
extern pci_module_info *pci;

#endif  /* DRIVER_H */
