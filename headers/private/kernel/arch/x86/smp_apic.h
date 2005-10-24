/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_ARCH_x86_SMP_APIC_H
#define _KERNEL_ARCH_x86_SMP_APIC_H

#define MP_FLT_SIGNATURE		'_PM_'
#define MP_CTH_SIGNATURE		'PCMP'

#define APIC_ENABLE				0x100
#define APIC_FOCUS				(~(1 << 9))
#define APIC_SIV				(0xff)

// offsets to APIC register
#define APIC_ID					0x020
#define APIC_VERSION			0x030
#define APIC_TPRI				0x080
#define APIC_EOI				0x0b0
#define APIC_LDR				0x0d0
#define APIC_SIVR				0x0f0
#define APIC_ESR				0x280
#define APIC_ICR1				0x300
#define APIC_ICR2				0x310
#define APIC_LVTT				0x320
#define APIC_LINT0				0x350
#define APIC_LINT1				0x360
#define APIC_LVT3				0x370
#define APIC_ICRT				0x380
#define APIC_CCRT				0x390
#define APIC_TDCR				0x3e0

/* ICR defines */
#define APIC_ICR1_WRITE_MASK				0xfff3f000
#define APIC_ICR1_READ_MASK					0xfff32000
#define APIC_ICR2_MASK						0x00ffffff

#define APIC_ICR1_DELIVERY_MODE_FIXED		0
#define APIC_ICR1_DELIVERY_MODE_LOWESTPRI	(1 << 8)
#define APIC_ICR1_DELIVERY_MODE_INIT		(5 << 8)
#define APIC_ICR1_DELIVERY_MODE_STARTUP		(6 << 8)

#define APIC_ICR1_DEST_MODE_PHYSICAL		0
#define APIC_ICR1_DEST_MODE_LOGICAL			(1 << 11)

#define APIC_ICR1_ASSERT					(1 << 13)
#define APIC_ICR1_TRIGGER_MODE_LEVEL		(1 << 14)
#define APIC_ICR1_DELIVERY_STATUS			(1 << 12)

#define APIC_ICR1_DEST_FIELD				0
#define APIC_ICR1_DEST_SELF					(1 << 18)
#define APIC_ICR1_DEST_ALL					(2 << 18)
#define APIC_ICR1_DEST_ALL_BUT_SELF			(3 << 18)

/* other defines */

#define APIC_TDCR_2				0x00
#define APIC_TDCR_4				0x01
#define APIC_TDCR_8				0x02
#define APIC_TDCR_16			0x03
#define APIC_TDCR_32			0x08
#define APIC_TDCR_64			0x09
#define APIC_TDCR_128			0x0a
#define APIC_TDCR_1				0x0b

#define APIC_LVTT_MASK			0x000310ff
#define APIC_LVTT_VECTOR		0x000000ff
#define APIC_LVTT_DS			0x00001000
#define APIC_LVTT_M				0x00010000
#define APIC_LVTT_TM			0x00020000

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

#define APIC_DEST_STARTUP		0x00600

#define LOPRIO_LEVEL			0x00000010

#define IOAPIC_ID				0x0
#define IOAPIC_VERSION			0x1
#define IOAPIC_ARB				0x2
#define IOAPIC_REDIR_TABLE		0x10

#define IPI_CACHE_FLUSH			0x40
#define IPI_INV_TLB				0x41
#define IPI_INV_PTE				0x42
#define IPI_INV_RESCHED			0x43
#define IPI_STOP				0x44

#define MP_EXT_PE				0
#define MP_EXT_BUS				1
#define MP_EXT_IO_APIC			2
#define MP_EXT_IO_INT			3
#define MP_EXT_LOCAL_INT		4

#define MP_EXT_PE_LEN			20
#define MP_EXT_BUS_LEN			8
#define MP_EXT_IO_APIC_LEN		8
#define MP_EXT_IO_INT_LEN		8
#define MP_EXT_LOCAL_INT_LEN	8

struct mp_config_table {
	uint32	signature;			/* "PCMP" */
	uint16	table_len;			/* length of this structure */
	uint8	mp_rev;				/* spec supported, 1 for 1.1 or 4 for 1.4 */
	uint8	checksum;			/* checksum, all bytes add up to zero */
	char	oem[8];				/* oem identification, not null-terminated */
	char	product[12];		/* product name, not null-terminated */
	void	*oem_table_ptr;		/* addr of oem-defined table, zero if none */
	uint16	oem_len;			/* length of oem table */
	uint16	num_entries;		/* number of entries in base table */
	uint32	apic;				/* address of apic */
	uint16	ext_len;			/* length of extended section */
	uint8	ext_checksum;		/* checksum of extended table entries */
};

struct mp_flt_struct {
	uint32	signature;			/* "_MP_" */
	struct mp_config_table *mpc; /* address of mp configuration table */
	uint8	mpc_len;			/* length of this structure in 16-byte units */
	uint8	mp_rev;				/* spec supported, 1 for 1.1 or 4 for 1.4 */
	uint8	checksum;			/* checksum, all bytes add up to zero */
	uint8	mp_feature_1;		/* mp system configuration type if no mpc */
	uint8	mp_feature_2;		/* imcrp */
	uint8	mp_feature_3, mp_feature_4, mp_feature_5; /* reserved */
};

struct mp_ext_pe {
	uint8	type;
	uint8	apic_id;
	uint8	apic_version;
	uint8	cpu_flags;
	uint32	signature;			/* stepping, model, family, each four bits */
	uint32	feature_flags;
	uint32	res1, res2;
};

struct mp_ext_ioapic {
	uint8	type;
	uint8	ioapic_id;
	uint8	ioapic_version;
	uint8	ioapic_flags;
	uint32	*addr;
};

struct mp_ext_bus {
	uint8	type;
	uint8	bus_id;
	char	name[6];
};

struct mp_ext_interrupt {
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

#endif	/* _KERNEL_ARCH_x86_SMP_APIC_H */
