/*
 * Copyright 2004-2006, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_BOOT_DISK_IDENTIFIER_H
#define KERNEL_BOOT_DISK_IDENTIFIER_H


#include <SupportDefs.h>


enum bus_types {
	UNKNOWN_BUS,
	LEGACY_BUS,
	PCI_BUS,
};

enum device_types {
	UNKNOWN_DEVICE,
	ATA_DEVICE,
	ATAPI_DEVICE,
	SCSI_DEVICE,
	USB_DEVICE,
	FIREWIRE_DEVICE,
	FIBRE_DEVICE,
};

#define NUM_DISK_CHECK_SUMS 5

typedef struct disk_identifier {
	int32				bus_type;
	int32				device_type;

	union {
		struct {
			uint16		base_address;
		} _PACKED legacy;
		struct {
			uint8		bus;
			uint8		slot;
			uint8		function;
		} _PACKED pci;
	} bus;
	union {
		struct {
			bool		master;
		} _PACKED ata;
		struct {
			bool		master;
			uint8		logical_unit;
		} _PACKED atapi;
		struct {
			uint8		logical_unit;
		} _PACKED scsi;
		struct {
			uint8		tbd;
		} _PACKED usb;
		struct {
			uint64		guid;
		} _PACKED firewire;
		struct {
			uint64		wwd;
		} _PACKED fibre;
		struct {
			off_t		size;
			struct {
				off_t	offset;
				uint32	sum;
			} _PACKED check_sums[NUM_DISK_CHECK_SUMS];
		} _PACKED unknown;
	} device;
} _PACKED disk_identifier;

#endif	/* KERNEL_BOOT_DISK_IDENTIFIER_H */
