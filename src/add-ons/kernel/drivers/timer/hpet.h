/*
 * Copyright 2008, Dustin Howett, dustin.howett@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_x86_HPET_H
#define _KERNEL_ARCH_x86_HPET_H

#include <OS.h>
#include "arch_acpi.h"

/* All masks are 32 bits wide to represent relative bit locations */
/* Doing it this way is Required since the HPET only supports 32/64-bit aligned reads. */

/* Global Capability Register Masks */
#define HPET_CAP_MASK_REVID			0x00000000000000FFULL
#define HPET_CAP_MASK_NUMTIMERS			0x0000000000001F00ULL
#define HPET_CAP_MASK_WIDTH			0x0000000000002000ULL
#define HPET_CAP_MASK_LEGACY			0x0000000000008000ULL
#define HPET_CAP_MASK_VENDOR_ID			0x00000000FFFF0000ULL
#define HPET_CAP_MASK_PERIOD			0xFFFFFFFF00000000ULL

/* Retrieve Global Capabilities */
#define HPET_GET_REVID(regs)		((regs)->capabilities & HPET_CAP_MASK_REVID)
#define HPET_GET_NUM_TIMERS(regs)	(((regs)->capabilities & HPET_CAP_MASK_NUMTIMERS) >> 8)
#define HPET_IS_64BIT(regs)		(((regs)->capabilities & HPET_CAP_MASK_WIDTH) >> 13)
#define HPET_IS_LEGACY_CAPABLE(regs)	(((regs)->capabilities & HPET_CAP_MASK_LEGACY) >> 15)
#define HPET_GET_VENDOR_ID(regs)	(((regs)->capabilities & HPET_CAP_MASK_VENDOR_ID) >> 16)
#define HPET_GET_PERIOD(regs)		(((regs)->capabilities & HPET_CAP_MASK_PERIOD) >> 32)

/* Global Config Register Masks */
#define HPET_CONF_MASK_ENABLED			0x00000001
#define HPET_CONF_MASK_LEGACY			0x00000002

/* Retrieve Global Configuration */
#define HPET_IS_ENABLED(regs)		((regs)->config & HPET_CONF_MASK_ENABLED)
#define HPET_IS_LEGACY(regs)		(((regs)->config & HPET_CONF_MASK_LEGACY) >> 1)

/* Timer Configuration and Capabilities*/
#define HPET_CAP_TIMER_MASK			0xFFFFFFFF00000000ULL
#define HPET_CONF_TIMER_INT_ROUTE_MASK		0x3e00UL
#define HPET_CONF_TIMER_INT_ROUTE_SHIFT		9	
#define HPET_CONF_TIMER_INT_TYPE		0x00000002UL
#define HPET_CONF_TIMER_INT_ENABLE		0x00000004UL
#define HPET_CONF_TIMER_TYPE			0x00000008UL
#define HPET_CONF_TIMER_VAL_SET			0x00000040UL
#define HPET_CONF_TIMER_32MODE			0x00000100UL
#define HPET_CONF_TIMER_FSB_ENABLE		0x00004000UL
#define HPET_CAP_TIMER_PER_INT			0x00000010UL
#define HPET_CAP_TIMER_SIZE			0x00000020UL
#define HPET_CAP_TIMER_FSB_INT_DEL		0x00008000UL

#define HPET_GET_CAP_TIMER_ROUTE(timer)		(((timer)->config & HPET_CAP_TIMER_MASK) >> 32)
#define HPET_GET_CONF_TIMER_INT_ROUTE(timer)	(((timer)->config & HPET_CONF_TIMER_INT_ROUTE_MASK) >> HPET_CONF_TIMER_INT_ROUTE_SHIFT)

#define ACPI_HPET_SIGNATURE			"HPET"

struct hpet_timer {
	/* Timer Configuration/Capability bits, Reversed because x86 is LSB */	
	volatile uint64 config;
					/* R/W: Each bit represents one allowed interrupt for this timer. */
					/* If interrupt 16 is allowed, bit 16 will be 1. */
	union {	
		volatile uint64 comparator64;	/* R/W: Comparator value */
		volatile uint32 comparator32;	
	} u0;				/* non-periodic mode: fires once when main counter = this comparator */
					/* periodic mode: fires when timer reaches this value, is increased by the original value */
	
	volatile uint64 fsb_route[2];	/* R/W: FSB Interrupt Route values */
};


struct hpet_regs {
	volatile uint64 capabilities;		/* Read Only */	
	
	volatile uint64 reserved1;

	volatile uint64 config;			/* R/W: Config Bits */

	volatile uint64 reserved2;

	/* Interrupt Status bits */
	volatile uint64 interrupt_status;	/* Interrupt Config bits for timers 0-31 */
						/* Level Tigger: 0 = off, 1 = set by hardware, timer is active */
						/* Edge Trigger: ignored */
						/* Writing 0 will not clear these. Must write 1 again. */
	volatile uint64 reserved3[25];

	union {
		volatile uint64 counter64;	/* R/W */
		volatile uint32 counter32;
	} u0;

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
