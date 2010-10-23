/*
 * Copyright 2008, Dustin Howett, dustin.howett@gmail.com. All rights reserved.
 * Copyright 2007, Michael Lotz, mmlr@mlotz.ch. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_x86_ARCH_ACPI_H
#define _KERNEL_ARCH_x86_ARCH_ACPI_H

#define ACPI_RSDP_SIGNATURE		"RSD PTR "
#define ACPI_RSDT_SIGNATURE		"RSDT"
#define ACPI_MADT_SIGNATURE		"APIC"

#define ACPI_LOCAL_APIC_ENABLED	0x01

typedef struct acpi_rsdp {
	char	signature[8];			/* "RSD PTR " including blank */
	uint8	checksum;				/* checksum of bytes 0-19 (per ACPI 1.0) */
	char	oem_id[6];				/* not null terminated */
	uint8	revision;				/* 0 = ACPI 1.0, 2 = ACPI 3.0 */
	uint32	rsdt_address;			/* physical memory address of RSDT */
	uint32	rsdt_length;			/* length in bytes including header */
	uint64	xsdt_address;			/* 64bit physical memory address of XSDT */
	uint8	extended_checksum;		/* including entire table */
	uint8	reserved[3];
} _PACKED acpi_rsdp;

typedef struct acpi_descriptor_header {
	char	signature[4];			/* table identifier as ASCII string */
	uint32	length;					/* length in bytes of the entire table */
	uint8	revision;
	uint8	checksum;				/* checksum of entire table */
	char	oem_id[6];				/* not null terminated */
	char	oem_table_id[8];		/* oem supplied table identifier */
	uint32	oem_revision;			/* oem supplied revision number */
	char	creator_id[4];			/* creator / asl compiler id */
	uint32	creator_revision;		/* compiler revision */
} _PACKED acpi_descriptor_header;

typedef struct acpi_madt {
	acpi_descriptor_header	header;		/* "APIC" signature */
	uint32	local_apic_address;		/* physical address for local CPUs APICs */
	uint32	flags;
} _PACKED acpi_madt;

enum {
	ACPI_MADT_LOCAL_APIC = 0,
	ACPI_MADT_IO_APIC = 1,
	ACPI_MADT_INTERRUPT_SOURCE_OVERRIDE = 2,
	ACPI_MADT_NMI_SOURCE = 3,
	ACPI_MADT_LOCAL_APIC_NMI = 4,
	ACPI_MADT_LOCAL_APIC_ADDRESS_OVERRIDE = 5,
	ACPI_MADT_IO_SAPIC = 6,
	ACPI_MADT_LOCAL_SAPIC = 7,
	ACPI_MADT_PLATFORM_INTERRUPT_SOURCES = 8,
	ACPI_MADT_PROCESSOR_LOCAL_X2_APIC_NMI = 9,
	ACPI_MADT_LOCAL_X2_APIC_NMI = 0XA
};

typedef struct acpi_apic {
	uint8	type;
	uint8	length;
} _PACKED acpi_apic;

typedef struct acpi_local_apic {
	uint8	type;					/* 0 = processor local APIC */
	uint8	length;					/* 8 bytes */
	uint8	acpi_processor_id;
	uint8	apic_id;				/* the id of this APIC */
	uint32	flags;					/* 1 = enabled */
} _PACKED acpi_local_apic;

typedef struct acpi_io_apic {
	uint8	type;					/* 1 = I/O APIC */
	uint8	length;					/* 12 bytes */
	uint8	io_apic_id;				/* the id of this APIC */
	uint8	reserved;
	uint32	io_apic_address;		/* phyisical address of I/O APIC */
	uint32	interrupt_base;			/* global system interrupt base */
} _PACKED acpi_io_apic;

#endif	/* _KERNEL_ARCH_x86_ARCH_ACPI_H */
