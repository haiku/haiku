/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_ARCH_x86_SMP_APIC_H
#define _KERNEL_ARCH_x86_SMP_APIC_H

#define MP_FLOATING_SIGNATURE			'_PM_'
#define MP_CONFIG_TABLE_SIGNATURE		'PCMP'

#define APIC_ENABLE				0x100
#define APIC_FOCUS				(~(1 << 9))
#define APIC_SIV				(0xff)

// offsets to APIC register
#define APIC_ID							0x020
#define APIC_VERSION					0x030
#define APIC_TASK_PRIORITY				0x080
#define APIC_ARBITRATION_PRIORITY		0x090
#define APIC_PROCESSOR_PRIORITY			0x0a0
#define APIC_EOI						0x0b0
#define APIC_LOGICAL_DEST				0x0d0
#define APIC_DEST_FORMAT				0x0e0
#define APIC_SPURIOUS_INTR_VECTOR		0x0f0
#define APIC_ERROR_STATUS				0x280
#define APIC_INTR_COMMAND_1				0x300	// bits 0-31
#define APIC_INTR_COMMAND_2				0x310	// bits 32-63
#define APIC_LVT_TIMER					0x320
#define APIC_LVT_THERMAL_SENSOR			0x330
#define APIC_LVT_PERFMON_COUNTERS		0x340
#define APIC_LVT_LINT0					0x350
#define APIC_LVT_LINT1					0x360
#define APIC_LVT_ERROR					0x370
#define APIC_INITIAL_TIMER_COUNT		0x380
#define APIC_CURRENT_TIMER_COUNT		0x390
#define APIC_TIMER_DIVIDE_CONFIG		0x3e0

/* standard APIC interrupt defines */
#define APIC_DELIVERY_MODE_FIXED				0
#define APIC_DELIVERY_MODE_LOWESTPRI			(1 << 8)	// ICR1 only
#define APIC_DELIVERY_MODE_SMI					(2 << 8)
#define APIC_DELIVERY_MODE_NMI					(4 << 8)
#define APIC_DELIVERY_MODE_INIT					(5 << 8)
#define APIC_DELIVERY_MODE_STARTUP				(6 << 8)	// ICR1 only
#define APIC_DELIVERY_MODE_ExtINT				(7 << 8)	// LINT0/1 only

#define APIC_DELIVERY_STATUS					(1 << 12)
#define APIC_TRIGGER_MODE_LEVEL					(1 << 15)

/* Interrupt Command defines */
#define APIC_INTR_COMMAND_1_MASK				0xfff3f000
#define APIC_INTR_COMMAND_2_MASK				0x00ffffff

#define APIC_INTR_COMMAND_1_DEST_MODE_PHYSICAL	0
#define APIC_INTR_COMMAND_1_DEST_MODE_LOGICAL	(1 << 11)

#define APIC_INTR_COMMAND_1_ASSERT				(1 << 14)

#define APIC_INTR_COMMAND_1_DEST_FIELD			0
#define APIC_INTR_COMMAND_1_DEST_SELF			(1 << 18)
#define APIC_INTR_COMMAND_1_DEST_ALL			(2 << 18)
#define APIC_INTR_COMMAND_1_DEST_ALL_BUT_SELF	(3 << 18)

/* Local Vector Table defines */
#define APIC_LVT_MASKED							(1 << 16)

// timer defines
#define APIC_LVT_TIMER_MASK						0xfffcef00

// LINT0/1 defines 
#define APIC_LVT_LINT_MASK						0xfffe0800
#define APIC_LVT_LINT_INPUT_POLARITY			(1 << 13)

// Timer Divide Config Divisors
#define APIC_TIMER_DIVIDE_CONFIG_1				0x0b
#define APIC_TIMER_DIVIDE_CONFIG_2				0x00
#define APIC_TIMER_DIVIDE_CONFIG_4				0x01
#define APIC_TIMER_DIVIDE_CONFIG_8				0x02
#define APIC_TIMER_DIVIDE_CONFIG_16				0x03
#define APIC_TIMER_DIVIDE_CONFIG_32				0x08
#define APIC_TIMER_DIVIDE_CONFIG_64				0x09
#define APIC_TIMER_DIVIDE_CONFIG_128			0x0a

/*
#define APIC_LVT_DM				0x00000700
#define APIC_LVT_DM_ExtINT		0x00000700
#define APIC_LVT_DM_NMI			0x00000400
#define APIC_LVT_IIPP			0x00002000
#define APIC_LVT_TM				0x00008000
#define APIC_LVT_M				0x00010000
#define APIC_LVT_OS				0x00020000

#define APIC_TPR_PRIO			0x000000ff
#define APIC_TPR_INT			0x000000f0
#define APIC_TPR_SUB			0x0000000f

#define APIC_SVR_SWEN			0x00000100
#define APIC_SVR_FOCUS			0x00000200

#define IOAPIC_ID				0x0
#define IOAPIC_VERSION			0x1
#define IOAPIC_ARB				0x2
#define IOAPIC_REDIR_TABLE		0x10

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

#endif	/* _KERNEL_ARCH_x86_SMP_APIC_H */
