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
		} legacy;
		struct {
			uint8		bus;
			uint8		slot;
			uint8		function;
		} pci;
	} bus;
	union {
		struct {
			bool		master;
		} ata;
		struct {
			bool		master;
			uint8		logical_unit;
		} atapi;
		struct {
			uint8		logical_unit;
		} scsi;
		struct {
			uint8		tbd;
		} usb;
		struct {
			uint64		guid;
		} firewire;
		struct {
			uint64		wwd;
		} fibre;
		struct {
			off_t		size;
			struct {
				off_t	offset;
				uint32	sum;
			} check_sums[NUM_DISK_CHECK_SUMS];
		} unknown;
	} device;
} disk_identifier;

#endif	/* KERNEL_BOOT_DISK_IDENTIFIER_H */
