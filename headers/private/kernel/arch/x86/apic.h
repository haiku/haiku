/*
 * Copyright 2008, Dustin Howett, dustin.howett@gmail.com. All rights reserved.
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_ARCH_x86_APIC_H
#define _KERNEL_ARCH_x86_APIC_H

#include <boot/kernel_args.h>
#include <SupportDefs.h>

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
*/

#if !_BOOT_MODE

bool		apic_available();
uint32		apic_read(uint32 offset);
void		apic_write(uint32 offset, uint32 data);
uint32		apic_local_id();
void		apic_end_of_interrupt();

void		apic_disable_local_ints();

status_t	apic_init(kernel_args *args);
status_t	apic_per_cpu_init(kernel_args *args, int32 cpu);

#endif // !_BOOT_MODE

#endif	/* _KERNEL_ARCH_x86_APIC_H */
