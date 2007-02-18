/*
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*
	PCI bus manager
*/

#ifndef _PCI2_H
#define _PCI2_H

#include <device_manager.h>
#include <PCI.h>

// currently, this structure is disabled to avoid collision with R5 header
#if 0

typedef struct pci_info {
	ushort	vendor_id;				/* vendor id */
	ushort	device_id;				/* device id */
	uchar	bus;					/* bus number */
	uchar	device;					/* device number on bus */
	uchar	function;				/* function number in device */
	uchar	revision;				/* revision id */
	uchar	class_api;				/* specific register interface type */
	uchar	class_sub;				/* specific device function */
	uchar	class_base;				/* device type (display vs network, etc) */
	uchar	line_size;				/* cache line size in 32 bit words */
	uchar	latency;				/* latency timer */
	uchar	header_type;			/* header type */
	uchar	bist;					/* built-in self-test */
	uchar	reserved;				/* filler, for alignment */
	union {
		struct {
			ulong	cardbus_cis;			/* CardBus CIS pointer */
			ushort	subsystem_id;			/* subsystem (add-in card) id */
			ushort	subsystem_vendor_id;	/* subsystem (add-in card) vendor id */
			ulong	rom_base;				/* rom base address, viewed from host */
			ulong	rom_base_pci;			/* rom base addr, viewed from pci */
			ulong	rom_size;				/* rom size */
			ulong	base_registers[6];		/* base registers, viewed from host */
			ulong	base_registers_pci[6];	/* base registers, viewed from pci */
			ulong	base_register_sizes[6];	/* size of what base regs point to */
			uchar	base_register_flags[6];	/* flags from base address fields */
			uchar	interrupt_line;			/* interrupt line */
			uchar	interrupt_pin;			/* interrupt pin */
			uchar	min_grant;				/* burst period @ 33 Mhz */
			uchar	max_latency;			/* how often PCI access needed */
		} h0;
		struct {
			ulong	base_registers[2];		/* base registers, viewed from host */
			ulong	base_registers_pci[2];	/* base registers, viewed from pci */
			ulong	base_register_sizes[2];	/* size of what base regs point to */
			uchar	base_register_flags[2];	/* flags from base address fields */
			uchar	primary_bus;
			uchar	secondary_bus;
			uchar	subordinate_bus;
			uchar	secondary_latency;
			uchar	io_base;
			uchar	io_limit;
			ushort	secondary_status;
			ushort	memory_base;
			ushort	memory_limit;
			ushort  prefetchable_memory_base;
			ushort  prefetchable_memory_limit;
			ulong	prefetchable_memory_base_upper32;
			ulong	prefetchable_memory_limit_upper32;
			ushort	io_base_upper16;
			ushort	io_limit_upper16;
			ulong	rom_base;				/* rom base address, viewed from host */
			ulong	rom_base_pci;			/* rom base addr, viewed from pci */
			uchar	interrupt_line;			/* interrupt line */
			uchar	interrupt_pin;			/* interrupt pin */
			ushort	bridge_control;		
		} h1; 
	} u;
} pci_info;

#endif


typedef struct pci_device_info *pci_device;

//	Interface to one PCI device.
//	Actually, this is a _function_ of a device only, but 
//	pci_function_module_info would be a bit non-intuitive
typedef struct pci_device_module_info {
	driver_module_info info;

	uint8	(*read_io_8)(pci_device device, int mapped_io_addr);
	void	(*write_io_8)(pci_device device, int mapped_io_addr, uint8 value);
	uint16	(*read_io_16)(pci_device device, int mapped_io_addr);
	void	(*write_io_16)(pci_device device, int mapped_io_addr, uint16 value);
	uint32	(*read_io_32)(pci_device device, int mapped_io_addr);
	void	(*write_io_32)(pci_device device, int mapped_io_addr, uint32 value);

	uint32	(*read_pci_config)(pci_device device,
				uchar	offset,		/* offset in configuration space */
				uchar	size);		/* # bytes to read (1, 2 or 4) */
	void	(*write_pci_config)(pci_device device, 
				uchar	offset,		/* offset in configuration space */
				uchar	size,		/* # bytes to write (1, 2 or 4) */
				uint32	value);		/* value to write */

	void *(*ram_address)(pci_device device, const void *physical_address_in_system_memory);
	
/*	status_t (*allocate_iomem)( void *base, size_t len, const char *name );
	status_t (*release_iomem)( void *base, size_t len );
	
	status_t (*allocate_ioports)( uint16 ioport_base, size_t len, const char *name );
	status_t (*release_ioports)( uint16 ioport_base, size_t len );*/

	status_t (*get_pci_info)(pci_device device, struct pci_info *info);
	
	status_t (*find_pci_capability)(pci_device device,
				uchar	cap_id,
				uchar	*offset);
} pci_device_module_info;


// directory of PCI drivers
#define PCI_DRIVERS_DIR "pci"

// attributes of PCI device nodes
// bus idx (uint8)
#define PCI_DEVICE_BUS_ITEM "pci/bus"
// device idx (uint8)
#define PCI_DEVICE_DEVICE_ITEM "pci/device"
// function idx (uint8)
#define PCI_DEVICE_FUNCTION_ITEM "pci/function"

// vendor id (uint16)
#define PCI_DEVICE_VENDOR_ID_ITEM "pci/vendor_id"
// device id (uint16)
#define PCI_DEVICE_DEVICE_ID_ITEM "pci/device_id"
// subsystem id (uint16)
#define PCI_DEVICE_SUBSYSTEM_ID_ITEM "pci/subsystem_id"
// subvendor id (uint16)
#define PCI_DEVICE_SUBVENDOR_ID_ITEM "pci/subvendor_id"

// device base class (uint8)
#define PCI_DEVICE_BASE_CLASS_ID_ITEM "pci/class/base_id"
// device subclass (uint8)
#define PCI_DEVICE_SUB_CLASS_ID_ITEM "pci/class/sub_id"
// device api (uint8)
#define PCI_DEVICE_API_ID_ITEM "pci/class/api_id"


// dynamic consumer patterns for PCI devices
#define PCI_DEVICE_DYNAMIC_CONSUMER_0 \
	PCI_DRIVERS_DIR "/" \
	"vendor %" PCI_DEVICE_VENDOR_ID_ITEM "%|" \
	", device %" PCI_DEVICE_DEVICE_ID_ITEM "%|" \
	", subsystem %" PCI_DEVICE_SUBSYSTEM_ID_ITEM "%|" \
	", subvendor %" PCI_DEVICE_SUBVENDOR_ID_ITEM "%"
	
#define PCI_DEVICE_DYNAMIC_CONSUMER_1 \
	PCI_DRIVERS_DIR "/" \
	"base_class %" PCI_DEVICE_BASE_CLASS_ID_ITEM "%|" \
	", sub_class %" PCI_DEVICE_SUB_CLASS_ID_ITEM "%|" \
	", api %" PCI_DEVICE_API_ID_ITEM "%"

#endif
