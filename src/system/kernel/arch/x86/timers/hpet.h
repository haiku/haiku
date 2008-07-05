/*
 * Copyright 2008, Dustin Howett, dustin.howett@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_x86_TIMERS_HPET_H
#define _KERNEL_ARCH_x86_TIMERS_HPET_H

#include <SupportDefs.h>

/* All masks are 64 bits wide to represent bit locations;
   the HPET registers are 64 bits wide. */

/* Global Capability Register Masks */
#define HPET_CAP_MASK_ID			0x00000000000000FF
#define HPET_CAP_MASK_NUMTIMERS			0x0000000000001F00
#define HPET_CAP_MASK_WIDTH			0x0000000000002000
#define HPET_CAP_MASK_LEGACY			0x0000000000008000
#define HPET_CAP_MASK_VENDOR_ID			0x00000000FFFF0000
#define HPET_CAP_MASK_PERIOD			0xFFFFFFFF00000000

/* Convenience macros for Capability masks */
#define HPET_GET_ID(regs)		((regs)->caps & HPET_CAP_MASK_ID)
#define HPET_GET_NUM_TIMERS(regs)	(((regs)->caps & HPET_CAP_MASK_NUMTIMERS) >> 8)
#define HPET_IS_64BIT_CAPABLE(regs)	(((regs)->caps & HPET_CAP_MASK_WIDTH) >> 13)
#define HPET_IS_LEGACY_CAPABLE(regs)	(((regs)->caps & HPET_CAP_MASK_LEGACY) >> 15)
#define HPET_GET_VENDOR_ID(regs)	(((regs)->caps & HPET_CAP_MASK_VENDOR_ID) >> 16)
#define HPET_GET_PERIOD(regs)		(((regs)->caps & HPET_CAP_MASK_PERIOD) >> 32)

/* Global Configuration Masks */
#define HPET_CONF_MASK_ENABLED			0x0000000000000001
#define HPET_CONF_MASK_LEGACY			0x0000000000000002

/* Convenience macros for Config masks */
#define HPET_IS_ENABLED(regs)		((regs)->config & HPET_CONF_MASK_ENABLED)
#define HPET_IS_LEGACY(regs)		(((regs)->config & HPET_CONF_MASK_LEGACY) >> 1)

struct hpet_timer {
	uint64 config;			/* Timer configuration and capabilities */
	uint64 comparator;		/* Comparator value */
	uint64 introute;		/* FSB Interrupt Routing */
	uint64 reserved;
};


struct hpet_regs {
	uint64 caps;			/* HPET Capabilities and ID */
	uint64 reserved1;
	uint64 config;			/* General Configuration */
	uint64 reserved2;
	uint64 intstatus;		/* General Interrupt Status */
	uint8 reserved3[200];
	uint64 mainvalue;		/* Main Counter Value */
	uint64 reserved4;
	struct hpet_timer timers[1];	/* Timers */
};

/* Method prototypes */
static int hpet_get_prio(void);
static status_t hpet_set_hardware_timer(bigtime_t relativeTimeout);
static status_t hpet_clear_hardware_timer(void);
static status_t hpet_init(struct kernel_args *args);

#endif /* _KERNEL_ARCH_x86_TIMERS_HPET_H */
