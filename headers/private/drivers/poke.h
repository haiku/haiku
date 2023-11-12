/*
 * Copyright 2005, Oscar Lesta. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _POKE_DRIVER_H_
#define _POKE_DRIVER_H_

#include <Drivers.h>
#include <ISA.h>
#include <PCI.h>


#define POKE_DEVICE_NAME		"poke"
#define POKE_DEVICE_FULLNAME	"/dev/misc/poke"
#define POKE_SIGNATURE			'wltp'	// "We Like To Poke"


enum {
	POKE_PORT_READ = B_DEVICE_OP_CODES_END + 1,
	POKE_PORT_WRITE,
	POKE_PORT_INDEXED_READ,
	POKE_PORT_INDEXED_WRITE,
	POKE_PCI_READ_CONFIG,
	POKE_PCI_WRITE_CONFIG,
	POKE_GET_NTH_PCI_INFO,
	POKE_GET_PHYSICAL_ADDRESS,
	POKE_MAP_MEMORY,
	POKE_UNMAP_MEMORY
};


typedef struct {
	uint32		signature;
	uint8		index;
	pci_info*	info;
	status_t	status;
} pci_info_args;


typedef struct {
	uint32	signature;
	uint16	port;
	uint8	size;		// == index for POKE_PORT_INDEXED_*
	uint32	value;
} port_io_args;


typedef struct {
	uint32	signature;
	uint8	bus;
	uint8	device;
	uint8	function;
	uint8	size;
	uint8	offset;
	uint32	value;
} pci_io_args;


typedef struct {
	uint32		signature;
	area_id		area;
	const char*	name;
	phys_addr_t	physical_address;
	size_t		size;
	uint32		flags;
	uint32		protection;
	void*		address;
} mem_map_args;


#endif	// _POKE_DRIVER_H_
