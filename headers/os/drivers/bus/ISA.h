/*
 * Copyright 2002-2003, Thomas Kurschel. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ISA2_H
#define _ISA2_H


/*!	ISA bus manager

	This is an improper name - this bus manager uses the PnP manager to
	load device drivers, but calling it ISA PnP manager would be wrong as
	ISA PnP information isn't used at all.

	All ISA drivers must be Universal driver (see pnp_manager.h), as they
	are all direct children of the ISA bus node. Having an ISA PnP bus manager
	(which we don't), one node would be created per ISA device and thus you
	could write Specific drivers, but under normal ISA we don't even know
	how many devices are there, therefore the Universal driver trick.

	Apart from the loading, the main change is the resource manager. In
	a driver, you must allocate the resources before registering the node and
	deallocate it when your node is removed and if the driver isn't loaded at
	this time. If it is, you must delay deallocation until the driver gets
	unloaded to make sure no new driver touches the same resources like you
	meanwhile.
*/


#include <device_manager.h>
#include <ISA.h>


// maximum size of one dma transfer
// (in bytes for 8 bit transfer, in words for 16 bit transfer)
#define B_MAX_ISA_DMA_COUNT	0x10000

typedef struct isa2_module_info {
	driver_module_info info;

	uint8 (*read_io_8)(int mapped_io_addr);
	void (*write_io_8)(int mapped_io_addr, uint8 value);
	uint16 (*read_io_16)(int mapped_io_addr);
	void (*write_io_16)(int mapped_io_addr, uint16 value);
	uint32 (*read_io_32)(int mapped_io_addr);
	void (*write_io_32)(int mapped_io_addr, uint32 value);

	// don't know what it's for, remains for compatibility
	void *(*ram_address)(const void *physical_address_in_system_memory);

	// start dma transfer (scattered DMA is not supported as it's EISA specific)
	status_t (*start_isa_dma)(
		long	channel,				// dma channel to use
		void	*buf,					// buffer to transfer
		long	transfer_count,			// # transfers
		uchar	mode,					// mode flags
		uchar	e_mode					// extended mode flags
	);
} isa2_module_info;

// type of isa device
#define ISA_DEVICE_TYPE_NAME "isa/device/v1"
// directory of ISA drivers
// (there is only one device node, so put all drivers under "universal")
#define ISA_DRIVERS_DIR "isa"


#endif	/* _ISA2_H */
