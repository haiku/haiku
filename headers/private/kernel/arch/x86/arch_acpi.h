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
	uint32	xsdt_length;			/* length in bytes including header */
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
	ACPI_MADT_PLATFORM_INTERRUPT_SOURCE = 8,
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
	uint32	io_apic_address;		/* physical address of I/O APIC */
	uint32	interrupt_base;			/* global system interrupt base */
} _PACKED acpi_io_apic;

typedef struct acpi_int_source_override {
	uint8	type;					/* 2 = Interrupt source override */
	uint8	length;					/* 10 bytes */
	uint8	bus;					/* 0 = ISA  */
	uint8	source;					/* Bus-relative interrupt source (IRQ) */
	uint32	interrupt;				/* global system interrupt this
									   bus-relative source int will signal */
	uint16	flags;					/* MPS INTI flags. See Table 5-25 in
									   ACPI Spec 4.0a or similar */
} _PACKED acpi_int_source_override;

typedef struct acpi_nmi_source {
	uint8	type;					/* 3 = NMI */
	uint8	length;					/* 8 bytes */
	uint16	flags;					/* Same as MPS INTI flags. See Table 5-25 in
									   ACPI Spec 4.0a or similar */
	uint32  interrupt;				/* global system interrupt this
									   non-maskable interrupt will trigger */
} _PACKED acpi_nmi_source;

typedef struct acpi_local_apic_nmi {
	uint8	type;					/* 4 = local APIC NMI */
	uint8	length;					/* 6 bytes */
	uint8	acpi_processor_id;		/* Processor ID corresponding to processor
									   ID in acpi_local_apic. 0xFF means
									   it applies to all processors */
	uint16	flags;					/* Same as MPS INTI flags. See Table 5-25 in
									   ACPI Spec 4.0a or similar */
	uint8   local_interrupt;		/* Local APIC interrupt input LINTn to which
									   NMI is connected */
} _PACKED acpi_local_apic_nmi;

typedef struct acpi_local_apic_address_override {
	uint8	type;					/* 5 = local APIC address override */
	uint8	length;					/* 12 bytes */
	uint16	reserved;				/* reserved (must be set to zero) */
	uint64	local_apic_address;		/* Physical address of local APIC. See table
										5-28 in ACPI Spec 4.0a for more */
} _PACKED acpi_local_apic_address_override;

typedef struct acpi_io_sapic {
	uint8	type;					/* 6 = I/0 SAPIC (should be used if it
									   exists instead of I/O APIC if both exists
									   for a APIC ID.*/
	uint8	length;					/* 16 bytes */
	uint8	io_apic_id;				/* the id of this SAPIC */
	uint8	reserved;				/* reserved (must be set to zero) */
	uint32	interrupt_base;			/* global system interrupt base */
	uint64	sapic_address;			/* The physical address to access this I/0
									   SAPIC. Each SAPIC resides at a unique
									   address */
} _PACKED acpi_io_sapic;

typedef struct acpi_local_sapic {
	uint8	type;					/* 7 = processor local SAPIC */
	uint8	length;					/* n bytes */
	uint8	acpi_processor_id;
	uint8	local_sapic_id;
	uint8	local_sapic_eid;
	uint8	reserved1;				/* reserved (must be set to zero) */
	uint8	reserved2;				/* reserved (must be set to zero) */
	uint8	reserved3;				/* reserved (must be set to zero) */
	uint32	flags;					/* Local SAPIC flags, see table 5-22 in
									   ACPI Spec 4.0a */
	uint32	processor_uid_nr;		/* Matches _UID of a processor when it is a
									   number */
	char	processor_uid_str[];	/* Matches _UID of a processor when it is a
									   string. Null-terminated */
} _PACKED acpi_local_sapic;

typedef struct acpi_platform_interrupt_source {
	uint8	type;					/* 8 = platform interrupt source */
	uint8	length;					/* 16 bytes */
	uint16	flags;					/* Same as MPS INTI flags. See Table 5-25 in
									   ACPI Spec 4.0a or similar */
	uint8	interrupt_type;			/* 1 PMI, 2 INIT, 3 Corrected Platform
									   Error Interrupt */
	uint8	processor_id;			/* processor ID of destination */
	uint8	processor_eid;			/* processor EID of destination */
	uint8	io_sapic_vector;		/* value that must be used to program the
									   vector field of the I/O SAPIC redirection
									   entry for entries with PMI type. */
	uint32	interrupt;				/* global system interrupt this
									   platform interrupt will trigger */
	uint32	platform_int_flags;		/* Platform Interrupt Source Flags. See
									   Table 5-32 of ACPI Spec 4.0a for desc */
} _PACKED acpi_platform_interrupt_source;

typedef struct acpi_local_x2_apic {
	uint8	type;					/* 9 = processor local x2APIC */
	uint8	length;					/* 16 bytes */
	uint16	reserved;				/* reserved (must be zero) */
	uint32	x2apic_id;				/* processor's local x2APIC ID */
	uint32	flags;					/* 1 = enabled. */
	uint32	processor_uid_nr;		/* Matches _UID of a processor when it is a
									   number */
} _PACKED acpi_local_x2_apic;

typedef struct acpi_local_x2_apic_nmi {
	uint8	type;					/* 0xA = local x2APIC NMI */
	uint8	length;					/* 12 bytes */
	uint16	flags;					/* Same as MPS INTI flags. See Table 5-25 in
									   ACPI Spec 4.0a or similar */
	uint32	acpi_processor_uid;		/* UID corresponding to ID in processor
									   device object. 0xFFFFFFFF means
									   it applies to all processors */
	uint8   local_interrupt;		/* Local x2APIC interrupt input LINTn to
									   which NMI is connected */
	uint8	reserved1;				/* reserved (must be set to zero) */
	uint8	reserved2;				/* reserved (must be set to zero) */
	uint8	reserved3;				/* reserved (must be set to zero) */
} _PACKED acpi_local_x2_apic_nmi;


#endif	/* _KERNEL_ARCH_x86_ARCH_ACPI_H */
