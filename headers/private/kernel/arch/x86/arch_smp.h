/*
 * Copyright 2008, Dustin Howett, dustin.howett@gmail.com. All rights reserved.
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_ARCH_x86_ARCH_SMP_H
#define _KERNEL_ARCH_x86_ARCH_SMP_H

#define MP_FLOATING_SIGNATURE			'_PM_'
#define MP_CONFIG_TABLE_SIGNATURE		'PCMP'

/*
#define IPI_CACHE_FLUSH			0x40
#define IPI_INV_TLB				0x41
#define IPI_INV_PTE				0x42
#define IPI_INV_RESCHED			0x43
#define IPI_STOP				0x44
*/

struct mp_config_table {
	uint32	signature;			/* "PCMP" */
	uint16	base_table_length;	/* length of the base table entries and this structure */
	uint8	spec_revision;		/* spec supported, 1 for 1.1 or 4 for 1.4 */
	uint8	checksum;			/* checksum, all bytes add up to zero */
	char	oem[8];				/* oem identification, not null-terminated */
	char	product[12];		/* product name, not null-terminated */
	void	*oem_table;			/* addr of oem-defined table, zero if none */
	uint16	oem_length;			/* length of oem table */
	uint16	num_base_entries;	/* number of entries in base table */
	uint32	apic;				/* address of apic */
	uint16	ext_length;			/* length of extended section */
	uint8	ext_checksum;		/* checksum of extended table entries */
	uint8	reserved;
};

struct mp_floating_struct {
	uint32	signature;			/* "_MP_" */
	struct mp_config_table *config_table; /* address of mp configuration table */
	uint8	config_length;		/* length of the table in 16-byte units */
	uint8	spec_revision;		/* spec supported, 1 for 1.1 or 4 for 1.4 */
	uint8	checksum;			/* checksum, all bytes add up to zero */
	uint8	mp_feature_1;		/* mp system configuration type if no mpc */
	uint8	mp_feature_2;		/* imcrp */
	uint8	mp_feature_3, mp_feature_4, mp_feature_5; /* reserved */
};

/* base config entry types */
enum {
	MP_BASE_PROCESSOR = 0,
	MP_BASE_BUS,
	MP_BASE_IO_APIC,
	MP_BASE_IO_INTR,
	MP_BASE_LOCAL_INTR,
};

struct mp_base_processor {
	uint8	type;
	uint8	apic_id;
	uint8	apic_version;
	uint8	cpu_flags;
	uint32	signature;			/* stepping, model, family, each four bits */
	uint32	feature_flags;
	uint32	res1, res2;
};

struct mp_base_ioapic {
	uint8	type;
	uint8	ioapic_id;
	uint8	ioapic_version;
	uint8	ioapic_flags;
	uint32	*addr;
};

struct mp_base_bus {
	uint8	type;
	uint8	bus_id;
	char	name[6];
};

struct mp_base_interrupt {
	uint8	type;
	uint8	interrupt_type;
	uint16	polarity : 2;
	uint16	trigger_mode : 2;
	uint16	_reserved : 12;
	uint8	source_bus_id;
	uint8	source_bus_irq;
	uint8	dest_apic_id;
	uint8	dest_apic_int;
};

enum {
	MP_INTR_TYPE_INT = 0,
	MP_INTR_TYPE_NMI,
	MP_INTR_TYPE_SMI,
	MP_INTR_TYPE_ExtINT,
};

#endif	/* _KERNEL_ARCH_x86_ARCH_SMP_H */
