/*
 * Copyright 2008, Dustin Howett, dustin.howett@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_x86_HPET_H
#define _KERNEL_ARCH_x86_HPET_H

#include <arch/x86/arch_acpi.h>

/* All masks are 32 bits wide to represent relative bit locations */
/* Doing it this way is Required since the HPET only supports 32/64-bit aligned reads. */

/* Global Capability Register Masks */
#define HPET_CAP_MASK_REVID			0x000000FFL
#define HPET_CAP_MASK_NUMTIMERS			0x00001F00L
#define HPET_CAP_MASK_WIDTH			0x00002000L
#define HPET_CAP_MASK_LEGACY			0x00008000L
#define HPET_CAP_MASK_VENDOR_ID			0xFFFF0000L

/* Retrieve Global Capabilities */
#define HPET_GET_REVID(regs)		((regs)->capability & HPET_CAP_MASK_REVID)
#define HPET_GET_NUM_TIMERS(regs)	(((regs)->capability & HPET_CAP_MASK_NUMTIMERS) >> 8)
#define HPET_IS_64BIT(regs)		(((regs)->capability & HPET_CAP_MASK_WIDTH) >> 13)
#define HPET_IS_LEGACY_CAPABLE(regs)	(((regs)->capability & HPET_CAP_MASK_LEGACY) >> 15)
#define HPET_GET_VENDOR_ID(regs)	(((regs)->capability & HPET_CAP_MASK_VENDOR_ID) >> 16)

/* Global Config Register Masks */
#define HPET_CONF_MASK_ENABLED			0x00000001
#define HPET_CONF_MASK_LEGACY			0x00000002

/* Retrieve Global Configuration */
#define HPET_IS_ENABLED(regs)		((regs)->config & HPET_CONF_MASK_ENABLED)
#define HPET_IS_LEGACY(regs)		(((regs)->config & HPET_CONF_MASK_LEGACY) >> 1)

/* Timer Configuration */
#define HPET_TN_INT_TYPE_CNF			0x00000002
#define HPET_TN_INT_ENB_CNF			0x00000004
#define HPET_TN_TYPE_CNF			0x00000008
#define HPET_TN_PER_INT_CAP			0x00000010
#define HPET_TN_SIZE_CAP			0x00000020
#define HPET_TN_VAL_SET_CNF			0x00000040
#define HPET_TN_32MODE_CNF			0x00000100
#define HPET_TN_FSB_EN_CNF		0x00004000
#define HPET_TN_FSB_INT_DEL_CAP		0x00008000


#define ACPI_HPET_SIGNATURE			"HPET"

struct hpet_timer {
	/* Timer Configuration/Capability bits, Reversed because x86 is LSB */	
	volatile uint32 config;
	volatile uint32 interrupts;	/* R/W: Each bit represents one allowed interrupt for this timer. */
					/* If interrupt 16 is allowed, bit 16 will be 1. */

	volatile uint64 comparator;	/* R/W: Comparator value */
					/* non-periodic mode: fires once when main counter = this comparator */
					/* periodic mode: fires when timer reaches this value, is increased by the original value */

	
	volatile uint32 fsb_value;	/* R/W: FSB Interrupt Route values */
	volatile uint32 fsb_addr;	/* R/W: Where fsb_value should be written */

	volatile uint64 reserved;
};


struct hpet_regs {
	/* Capability bits */
	volatile uint32 capability;	/* Read Only: Capabilities */	
	volatile uint32 period;		/* Read Only: Period */
	
	volatile uint64 reserved1;

	volatile uint64 config;		/* R/W: Config Bits */

	volatile uint64 reserved2;

	/* Interrupt Status bits */
	volatile uint64 timer_interrupts;	/* Interrupt Config bits for timers 0-31 */
					/* Level Tigger: 0 = off, 1 = set by hardware, timer is active */
					/* Edge Trigger: ignored */
					/* Writing 0 will not clear these. Must write 1 again. */

	volatile uint8 reserved3[200];

	volatile uint64 counter;	/* R/W */

	volatile uint64 reserved4;

	volatile struct hpet_timer timer[1];
};


typedef struct acpi_hpet {
	acpi_descriptor_header	header;	/* "HPET" signature and acpi header */
	uint16	vendor_id;
	uint8	legacy_capable : 1;
	uint8	reserved1 : 1;
	uint8	countersize : 1;
	uint8	comparators : 5;
	uint8	hw_revision;
	struct hpet_addr {
		uint8	address_space;
		uint8	register_width;
		uint8	register_offset;
		uint8	reserved;
		uint64	address;
	} hpet_address;
	uint8	number;
	uint16	min_tick;
} _PACKED acpi_hpet;

#endif
