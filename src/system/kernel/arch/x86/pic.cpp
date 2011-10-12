/*
 * Copyright 2002-2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

#include <arch/x86/pic.h>

#include <arch/cpu.h>
#include <arch/int.h>

#include <arch/x86/arch_int.h>

#include <int.h>


//#define TRACE_PIC
#ifdef TRACE_PIC
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


// Definitions for the PIC 8259 controller
// (this is not a complete list, only what we're actually using)

#define PIC_MASTER_CONTROL		0x20
#define PIC_MASTER_MASK			0x21
#define PIC_SLAVE_CONTROL		0xa0
#define PIC_SLAVE_MASK			0xa1
#define PIC_MASTER_INIT1		PIC_MASTER_CONTROL
#define PIC_MASTER_INIT2		PIC_MASTER_MASK
#define PIC_MASTER_INIT3		PIC_MASTER_MASK
#define PIC_MASTER_INIT4		PIC_MASTER_MASK
#define PIC_SLAVE_INIT1			PIC_SLAVE_CONTROL
#define PIC_SLAVE_INIT2			PIC_SLAVE_MASK
#define PIC_SLAVE_INIT3			PIC_SLAVE_MASK
#define PIC_SLAVE_INIT4			PIC_SLAVE_MASK

// the edge/level trigger control registers
#define PIC_MASTER_TRIGGER_MODE	0x4d0
#define PIC_SLAVE_TRIGGER_MODE	0x4d1

#define PIC_INIT1				0x10
#define PIC_INIT1_SEND_INIT4	0x01
#define PIC_INIT3_IR2_IS_SLAVE	0x04
#define PIC_INIT3_SLAVE_ID2		0x02
#define PIC_INIT4_x86_MODE		0x01

#define PIC_CONTROL3			0x08
#define PIC_CONTROL3_READ_ISR	0x03
#define PIC_CONTROL3_READ_IRR	0x02

#define PIC_NON_SPECIFIC_EOI	0x20

#define PIC_SLAVE_INT_BASE		8
#define PIC_NUM_INTS			0x0f


static uint16 sLevelTriggeredInterrupts = 0;
	// binary mask: 1 level, 0 edge


/*!	Tests if the interrupt in-service register of the responsible
	PIC is set for interrupts 7 and 15, and if that's not the case,
	it must assume it's a spurious interrupt.
*/
static bool
pic_is_spurious_interrupt(int32 num)
{
	if (num != 7)
		return false;

	// Note, detecting spurious interrupts on line 15 obviously doesn't
	// work correctly - and since those are extremely rare, anyway, we
	// just ignore them

	out8(PIC_CONTROL3 | PIC_CONTROL3_READ_ISR, PIC_MASTER_CONTROL);
	int32 isr = in8(PIC_MASTER_CONTROL);
	out8(PIC_CONTROL3 | PIC_CONTROL3_READ_IRR, PIC_MASTER_CONTROL);

	return (isr & 0x80) == 0;
}


static bool
pic_is_level_triggered_interrupt(int32 num)
{
	if (num < 0 || num > PIC_NUM_INTS)
		return false;

	return (sLevelTriggeredInterrupts & (1 << num)) != 0;
}


/*!	Sends a non-specified EOI (end of interrupt) notice to the PIC in
	question (or both of them).
	This clears the PIC interrupt in-service bit.
*/
static bool
pic_end_of_interrupt(int32 num)
{
	if (num < 0 || num > PIC_NUM_INTS)
		return false;

	// PIC 8259 controlled interrupt
	if (num >= PIC_SLAVE_INT_BASE)
		out8(PIC_NON_SPECIFIC_EOI, PIC_SLAVE_CONTROL);

	// we always need to acknowledge the master PIC
	out8(PIC_NON_SPECIFIC_EOI, PIC_MASTER_CONTROL);
	return true;
}


static void
pic_enable_io_interrupt(int32 num)
{
	// interrupt is specified "normalized"
	if (num < 0 || num > PIC_NUM_INTS)
		return;

	// enable PIC 8259 controlled interrupt

	TRACE(("pic_enable_io_interrupt: irq %ld\n", num));

	if (num < PIC_SLAVE_INT_BASE)
		out8(in8(PIC_MASTER_MASK) & ~(1 << num), PIC_MASTER_MASK);
	else
		out8(in8(PIC_SLAVE_MASK) & ~(1 << (num - PIC_SLAVE_INT_BASE)), PIC_SLAVE_MASK);
}


static void
pic_disable_io_interrupt(int32 num)
{
	// interrupt is specified "normalized"
	// never disable slave pic line IRQ 2
	if (num < 0 || num > PIC_NUM_INTS || num == 2)
		return;

	// disable PIC 8259 controlled interrupt

	TRACE(("pic_disable_io_interrupt: irq %ld\n", num));

	if (num < PIC_SLAVE_INT_BASE)
		out8(in8(PIC_MASTER_MASK) | (1 << num), PIC_MASTER_MASK);
	else
		out8(in8(PIC_SLAVE_MASK) | (1 << (num - PIC_SLAVE_INT_BASE)), PIC_SLAVE_MASK);
}


static void
pic_configure_io_interrupt(int32 num, uint32 config)
{
	uint8 value;
	int32 localBit;
	if (num < 0 || num > PIC_NUM_INTS || num == 2)
		return;

	TRACE(("pic_configure_io_interrupt: irq %ld; config 0x%08lx\n", num, config));

	if (num < PIC_SLAVE_INT_BASE) {
		value = in8(PIC_MASTER_TRIGGER_MODE);
		localBit = num;
	} else {
		value = in8(PIC_SLAVE_TRIGGER_MODE);
		localBit = num - PIC_SLAVE_INT_BASE;
	}

	if (config & B_LEVEL_TRIGGERED)
		value |= 1 << localBit;
	else
		value &= ~(1 << localBit);

	if (num < PIC_SLAVE_INT_BASE)
		out8(value, PIC_MASTER_TRIGGER_MODE);
	else
		out8(value, PIC_SLAVE_TRIGGER_MODE);

	sLevelTriggeredInterrupts = in8(PIC_MASTER_TRIGGER_MODE)
		| (in8(PIC_SLAVE_TRIGGER_MODE) << 8);
}


void
pic_init()
{
	static const interrupt_controller picController = {
		"8259 PIC",
		&pic_enable_io_interrupt,
		&pic_disable_io_interrupt,
		&pic_configure_io_interrupt,
		&pic_is_spurious_interrupt,
		&pic_is_level_triggered_interrupt,
		&pic_end_of_interrupt
	};

	// Start initialization sequence for the master and slave PICs
	out8(PIC_INIT1 | PIC_INIT1_SEND_INIT4, PIC_MASTER_INIT1);
	out8(PIC_INIT1 | PIC_INIT1_SEND_INIT4, PIC_SLAVE_INIT1);

	// Set start of interrupts to 0x20 for master, 0x28 for slave
	out8(ARCH_INTERRUPT_BASE, PIC_MASTER_INIT2);
	out8(ARCH_INTERRUPT_BASE + PIC_SLAVE_INT_BASE, PIC_SLAVE_INIT2);

	// Specify cascading through interrupt 2
	out8(PIC_INIT3_IR2_IS_SLAVE, PIC_MASTER_INIT3);
	out8(PIC_INIT3_SLAVE_ID2, PIC_SLAVE_INIT3);

	// Set both to operate in 8086 mode
	out8(PIC_INIT4_x86_MODE, PIC_MASTER_INIT4);
	out8(PIC_INIT4_x86_MODE, PIC_SLAVE_INIT4);

	out8(0xfb, PIC_MASTER_MASK);	// Mask off all interrupts (except slave pic line IRQ 2).
	out8(0xff, PIC_SLAVE_MASK); 	// Mask off interrupts on the slave.

	// determine which interrupts are level or edge triggered

#if 0
	// should set everything possible to level triggered
	out8(0xf8, PIC_MASTER_TRIGGER_MODE);
	out8(0xde, PIC_SLAVE_TRIGGER_MODE);
#endif

	sLevelTriggeredInterrupts = in8(PIC_MASTER_TRIGGER_MODE)
		| (in8(PIC_SLAVE_TRIGGER_MODE) << 8);

	TRACE(("PIC level trigger mode: 0x%08lx\n", sLevelTriggeredInterrupts));

	reserve_io_interrupt_vectors(16, 0);

	// make the pic controller the current one
	arch_int_set_interrupt_controller(picController);
}


void
pic_disable(uint16& enabledInterrupts)
{
	enabledInterrupts = ~(in8(PIC_MASTER_MASK) | in8(PIC_SLAVE_MASK) << 8);
	enabledInterrupts &= 0xfffb; // remove slave PIC from the mask

	// Mask off all interrupts on master and slave
	out8(0xff, PIC_MASTER_MASK);
	out8(0xff, PIC_SLAVE_MASK);

	free_io_interrupt_vectors(16, 0);
}
