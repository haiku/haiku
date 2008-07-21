/*
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

#include <cpu.h>
#include <int.h>
#include <kscheduler.h>
#include <ksyscalls.h>
#include <smp.h>
#include <team.h>
#include <thread.h>
#include <vm.h>
#include <vm_priv.h>

#include <arch/cpu.h>
#include <arch/int.h>
#include <arch/smp.h>
#include <arch/user_debugger.h>
#include <arch/vm.h>

#include <arch/x86/arch_apic.h>
#include <arch/x86/descriptors.h>
#include <arch/x86/vm86.h>

#include "interrupts.h"

#include <safemode.h>
#include <string.h>
#include <stdio.h>


//#define TRACE_ARCH_INT
#ifdef TRACE_ARCH_INT
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


// Definitions for a 82093AA IO APIC controller
#define IO_APIC_IDENTIFICATION				0x00
#define IO_APIC_VERSION						0x01
#define IO_APIC_ARBITRATION					0x02
#define IO_APIC_REDIRECTION_TABLE			0x10 // entry = base + 2 * index

// Fields for the version register
#define IO_APIC_VERSION_SHIFT				0
#define IO_APIC_VERSION_MASK				0xff
#define IO_APIC_MAX_REDIRECTION_ENTRY_SHIFT	16
#define IO_APIC_MAX_REDIRECTION_ENTRY_MASK	0xff

// Fields of each redirection table entry
#define IO_APIC_DESTINATION_FIELD_SHIFT		56
#define IO_APIC_DESTINATION_FIELD_MASK		0x0f
#define IO_APIC_INTERRUPT_MASK_SHIFT		16
#define IO_APIC_INTERRUPT_MASKED			1
#define IO_APIC_INTERRUPT_UNMASKED			0
#define IO_APIC_TRIGGER_MODE_SHIFT			15
#define IO_APIC_TRIGGER_MODE_EDGE			0
#define IO_APIC_TRIGGER_MODE_LEVEL			1
#define IO_APIC_REMOTE_IRR_SHIFT			14
#define IO_APIC_PIN_POLARITY_SHIFT			13
#define IO_APIC_PIN_POLARITY_HIGH_ACTIVE	0
#define IO_APIC_PIN_POLARITY_LOW_ACTIVE		1
#define IO_APIC_DELIVERY_STATUS_SHIFT		12
#define IO_APIC_DELIVERY_STATUS_IDLE		0
#define IO_APIC_DELIVERY_STATUS_PENDING		1
#define IO_APIC_DESTINATION_MODE_SHIFT		11
#define IO_APIC_DESTINATION_MODE_PHYSICAL	0
#define IO_APIC_DESTINATION_MODE_LOGICAL	1
#define IO_APIC_DELIVERY_MODE_SHIFT			8
#define IO_APIC_DELIVERY_MODE_MASK			0x07
#define IO_APIC_DELIVERY_MODE_FIXED			0
#define IO_APIC_DELIVERY_MODE_LOWEST_PRIO	1
#define IO_APIC_DELIVERY_MODE_SMI			2
#define IO_APIC_DELIVERY_MODE_NMI			4
#define IO_APIC_DELIVERY_MODE_INIT			5
#define IO_APIC_DELIVERY_MODE_EXT_INT		7
#define IO_APIC_INTERRUPT_VECTOR_SHIFT		0
#define IO_APIC_INTERRUPT_VECTOR_MASK		0xff

typedef struct ioapic_s {
	volatile uint32	io_register_select;
	uint32			reserved[3];
	volatile uint32	io_window_register;
} ioapic _PACKED;

static ioapic *sIOAPIC = NULL;
static uint32 sIOAPICMaxRedirectionEntry = 23;
static void *sLocalAPIC = NULL;

static uint32 sIRQToIOAPICPin[256];

bool gUsingIOAPIC = false;

typedef struct interrupt_controller_s {
	const char *name;
	void	(*enable_io_interrupt)(int32 num);
	void	(*disable_io_interrupt)(int32 num);
	void	(*configure_io_interrupt)(int32 num, uint32 config);
	bool	(*is_spurious_interrupt)(int32 num);
	void	(*end_of_interrupt)(int32 num);
} interrupt_controller;

static interrupt_controller *sCurrentPIC = NULL;


static const char *kInterruptNames[] = {
	/*  0 */ "Divide Error Exception",
	/*  1 */ "Debug Exception",
	/*  2 */ "NMI Interrupt",
	/*  3 */ "Breakpoint Exception",
	/*  4 */ "Overflow Exception",
	/*  5 */ "BOUND Range Exceeded Exception",
	/*  6 */ "Invalid Opcode Exception",
	/*  7 */ "Device Not Available Exception",
	/*  8 */ "Double Fault Exception",
	/*  9 */ "Coprocessor Segment Overrun",
	/* 10 */ "Invalid TSS Exception",
	/* 11 */ "Segment Not Present",
	/* 12 */ "Stack Fault Exception",
	/* 13 */ "General Protection Exception",
	/* 14 */ "Page-Fault Exception",
	/* 15 */ "-",
	/* 16 */ "x87 FPU Floating-Point Error",
	/* 17 */ "Alignment Check Exception",
	/* 18 */ "Machine-Check Exception",
	/* 19 */ "SIMD Floating-Point Exception",
};
static const int kInterruptNameCount = 20;

#define MAX_ARGS 16

typedef struct {
	uint32 a, b;
} desc_table;
static desc_table *sIDT = NULL;

static uint32 sLevelTriggeredInterrupts = 0;
	// binary mask: 1 level, 0 edge

// table with functions handling respective interrupts
typedef void interrupt_handler_function(struct iframe* frame);
#define INTERRUPT_HANDLER_TABLE_SIZE 256
interrupt_handler_function* gInterruptHandlerTable[
	INTERRUPT_HANDLER_TABLE_SIZE];


static void
set_gate(desc_table *gate_addr, addr_t addr, int type, int dpl)
{
	unsigned int gate1; // first byte of gate desc
	unsigned int gate2; // second byte of gate desc

	gate1 = (KERNEL_CODE_SEG << 16) | (0x0000ffff & addr);
	gate2 = (0xffff0000 & addr) | 0x8000 | (dpl << 13) | (type << 8);

	gate_addr->a = gate1;
	gate_addr->b = gate2;
}


static void
set_intr_gate(int n, void *addr)
{
	set_gate(&sIDT[n], (addr_t)addr, 14, DPL_KERNEL);
}


static void
set_system_gate(int n, void *addr)
{
	set_gate(&sIDT[n], (unsigned int)addr, 15, DPL_USER);
}


void
x86_set_task_gate(int32 n, int32 segment)
{
	sIDT[n].a = (segment << 16);
	sIDT[n].b = 0x8000 | (0 << 13) | (0x5 << 8); // present, dpl 0, type 5
}


/**	Tests if the interrupt in-service register of the responsible
 *	PIC is set for interrupts 7 and 15, and if that's not the case, 
 *	it must assume it's a spurious interrupt.
 */

static bool
pic_is_spurious_interrupt(int32 num)
{
	int32 isr;

	if (num != 7)
		return false;

	// Note, detecting spurious interrupts on line 15 obviously doesn't
	// work correctly - and since those are extremely rare, anyway, we
	// just ignore them

	out8(PIC_CONTROL3 | PIC_CONTROL3_READ_ISR, PIC_MASTER_CONTROL);
	isr = in8(PIC_MASTER_CONTROL);
	out8(PIC_CONTROL3 | PIC_CONTROL3_READ_IRR, PIC_MASTER_CONTROL);

	return (isr & 0x80) == 0;
}


/**	Sends a non-specified EOI (end of interrupt) notice to the PIC in
 *	question (or both of them).
 *	This clears the PIC interrupt in-service bit.
 */

static void
pic_end_of_interrupt(int32 num)
{
	if (num < 0 || num > PIC_NUM_INTS)
		return;

	// PIC 8259 controlled interrupt
	if (num >= PIC_SLAVE_INT_BASE)
		out8(PIC_NON_SPECIFIC_EOI, PIC_SLAVE_CONTROL);

	// we always need to acknowledge the master PIC
	out8(PIC_NON_SPECIFIC_EOI, PIC_MASTER_CONTROL);
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


static void
pic_init(void)
{
	static interrupt_controller picController = {
		"8259 PIC",
		&pic_enable_io_interrupt,
		&pic_disable_io_interrupt,
		&pic_configure_io_interrupt,
		&pic_is_spurious_interrupt,
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

	// make the pic controller the current one
	sCurrentPIC = &picController;
	gUsingIOAPIC = false;
}


static inline uint32
ioapic_read_32(uint8 registerSelect)
{
	sIOAPIC->io_register_select = registerSelect;
	return sIOAPIC->io_window_register;
}


static inline void
ioapic_write_32(uint8 registerSelect, uint32 value)
{
	sIOAPIC->io_register_select = registerSelect;
	sIOAPIC->io_window_register = value;
}


static inline uint64
ioapic_read_64(uint8 registerSelect)
{
	uint64 result;
	sIOAPIC->io_register_select = registerSelect + 1;
	result = sIOAPIC->io_window_register;
	result <<= 32;
	sIOAPIC->io_register_select = registerSelect;
	result |= sIOAPIC->io_window_register;
	return result;
}


static inline void
ioapic_write_64(uint8 registerSelect, uint64 value)
{
	sIOAPIC->io_register_select = registerSelect;
	sIOAPIC->io_window_register = (uint32)value;
	sIOAPIC->io_register_select = registerSelect + 1;
	sIOAPIC->io_window_register = (uint32)(value >> 32);
}


static bool
ioapic_is_spurious_interrupt(int32 num)
{
	// the spurious interrupt vector is initialized to the max value in smp
	return num == 0xff - ARCH_INTERRUPT_BASE;
}


static void
ioapic_end_of_interrupt(int32 num)
{
	*(volatile uint32 *)((char *)sLocalAPIC + APIC_EOI) = 0;
}


static void
ioapic_enable_io_interrupt(int32 num)
{
	uint64 entry;
	int32 pin = sIRQToIOAPICPin[num];
	if (pin < 0 || pin > sIOAPICMaxRedirectionEntry)
		return;

	TRACE(("ioapic_enable_io_interrupt: IRQ %ld -> pin %ld\n", num, pin));

	entry = ioapic_read_64(IO_APIC_REDIRECTION_TABLE + pin * 2);
	entry &= ~(1 << IO_APIC_INTERRUPT_MASK_SHIFT);
	entry |= IO_APIC_INTERRUPT_UNMASKED << IO_APIC_INTERRUPT_MASK_SHIFT;
	ioapic_write_64(IO_APIC_REDIRECTION_TABLE + pin * 2, entry);
}


static void
ioapic_disable_io_interrupt(int32 num)
{
	uint64 entry;
	int32 pin = sIRQToIOAPICPin[num];
	if (pin < 0 || pin > sIOAPICMaxRedirectionEntry)
		return;

	TRACE(("ioapic_disable_io_interrupt: IRQ %ld -> pin %ld\n", num, pin));

	entry = ioapic_read_64(IO_APIC_REDIRECTION_TABLE + pin * 2);
	entry &= ~(1 << IO_APIC_INTERRUPT_MASK_SHIFT);
	entry |= IO_APIC_INTERRUPT_MASKED << IO_APIC_INTERRUPT_MASK_SHIFT;
	ioapic_write_64(IO_APIC_REDIRECTION_TABLE + pin * 2, entry);
}


static void
ioapic_configure_io_interrupt(int32 num, uint32 config)
{
	uint64 entry;
	int32 pin = sIRQToIOAPICPin[num];
	if (pin < 0 || pin > sIOAPICMaxRedirectionEntry)
		return;

	TRACE(("ioapic_configure_io_interrupt: IRQ %ld -> pin %ld; config 0x%08lx\n",
		num, pin, config));

	entry = ioapic_read_64(IO_APIC_REDIRECTION_TABLE + pin * 2);
	entry &= ~((1 << IO_APIC_TRIGGER_MODE_SHIFT)
		| (1 << IO_APIC_PIN_POLARITY_SHIFT)
		| (IO_APIC_INTERRUPT_VECTOR_MASK << IO_APIC_INTERRUPT_VECTOR_SHIFT));

	if (config & B_LEVEL_TRIGGERED) {
		entry |= (IO_APIC_TRIGGER_MODE_LEVEL << IO_APIC_TRIGGER_MODE_SHIFT);
		sLevelTriggeredInterrupts |= (1 << num);
	} else {
		entry |= (IO_APIC_TRIGGER_MODE_EDGE << IO_APIC_TRIGGER_MODE_SHIFT);
		sLevelTriggeredInterrupts &= ~(1 << num);
	}

	if (config & B_LOW_ACTIVE_POLARITY)
		entry |= (IO_APIC_PIN_POLARITY_LOW_ACTIVE << IO_APIC_PIN_POLARITY_SHIFT);
	else
		entry |= (IO_APIC_PIN_POLARITY_HIGH_ACTIVE << IO_APIC_PIN_POLARITY_SHIFT);

	entry |= (num + ARCH_INTERRUPT_BASE) << IO_APIC_INTERRUPT_VECTOR_SHIFT;
	ioapic_write_64(IO_APIC_REDIRECTION_TABLE + pin * 2, entry);
}


static void
ioapic_init(kernel_args *args)
{
	uint32 i;
	uint32 version;
	uint64 targetAPIC;
	void *settings;

	static interrupt_controller ioapicController = {
		"82093AA IOAPIC",
		&ioapic_enable_io_interrupt,
		&ioapic_disable_io_interrupt,
		&ioapic_configure_io_interrupt,
		&ioapic_is_spurious_interrupt,
		&ioapic_end_of_interrupt
	};

	if (args->arch_args.apic == NULL) {
		dprintf("no local apic availabe\n");
		return;
	}

	// always map the local apic as it can be used for timers even if we
	// don't end up using the io apic
	sLocalAPIC = args->arch_args.apic;
	if (map_physical_memory("local apic", (void *)args->arch_args.apic_phys,
		B_PAGE_SIZE, B_EXACT_ADDRESS, B_KERNEL_READ_AREA
		| B_KERNEL_WRITE_AREA, &sLocalAPIC) < B_OK) {
		panic("mapping the local apic failed");
		return;
	}

	if (args->arch_args.ioapic == NULL) {
		dprintf("no ioapic available, not using ioapics for interrupt routing\n");
		return;
	}

	settings = load_driver_settings(B_SAFEMODE_DRIVER_SETTINGS);
	if (settings != NULL && get_driver_boolean_parameter(settings,
		B_SAFEMODE_DISABLE_IOAPIC, false, false)) {
		dprintf("ioapic explicitly disabled, not using ioapics for interrupt routing\n");
		unload_driver_settings(settings);
		return;
	} else if (settings != NULL)
		unload_driver_settings(settings);

	// TODO: remove when the PCI IRQ routing through ACPI is available below
	return;

	// map in the ioapic
	sIOAPIC = (ioapic *)args->arch_args.ioapic;
	if (map_physical_memory("ioapic", (void *)args->arch_args.ioapic_phys,
		B_PAGE_SIZE, B_EXACT_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
		(void **)&sIOAPIC) < B_OK) {
		panic("mapping the ioapic failed");
		return;
	}

	version = ioapic_read_32(IO_APIC_VERSION);
	if (version == 0xffffffff) {
		dprintf("ioapic seems inaccessible, not using it\n");
		return;
	}

	sLevelTriggeredInterrupts = 0;
	sIOAPICMaxRedirectionEntry
		= ((version >> IO_APIC_MAX_REDIRECTION_ENTRY_SHIFT)
		& IO_APIC_MAX_REDIRECTION_ENTRY_MASK);

	// use the boot CPU as the target for all interrupts
	targetAPIC = args->arch_args.cpu_apic_id[0];

	// program the interrupt vectors of the ioapic
	for (i = 0; i <= sIOAPICMaxRedirectionEntry; i++) {
		// initialize everything to deliver to the boot CPU in physical mode
		// and masked until explicitly enabled through enable_io_interrupt()
		uint64 entry = (targetAPIC << IO_APIC_DESTINATION_FIELD_SHIFT)
			| (IO_APIC_INTERRUPT_MASKED << IO_APIC_INTERRUPT_MASK_SHIFT)
			| (IO_APIC_DESTINATION_MODE_PHYSICAL << IO_APIC_DESTINATION_MODE_SHIFT)
			| ((i + ARCH_INTERRUPT_BASE) << IO_APIC_INTERRUPT_VECTOR_SHIFT);

		if (i == 0) {
			// make redirection entry 0 into an external interrupt
			entry |= (IO_APIC_TRIGGER_MODE_EDGE << IO_APIC_TRIGGER_MODE_SHIFT)
				| (IO_APIC_PIN_POLARITY_HIGH_ACTIVE << IO_APIC_PIN_POLARITY_SHIFT)
				| (IO_APIC_DELIVERY_MODE_EXT_INT << IO_APIC_DELIVERY_MODE_SHIFT);
		} else if (i < 16) {
			// make 1-15 ISA interrupts
			entry |= (IO_APIC_TRIGGER_MODE_EDGE << IO_APIC_TRIGGER_MODE_SHIFT)
				| (IO_APIC_PIN_POLARITY_HIGH_ACTIVE << IO_APIC_PIN_POLARITY_SHIFT)
				| (IO_APIC_DELIVERY_MODE_FIXED << IO_APIC_DELIVERY_MODE_SHIFT);
		} else {
			// and the rest are PCI interrupts
			entry |= (IO_APIC_TRIGGER_MODE_LEVEL << IO_APIC_TRIGGER_MODE_SHIFT)
				| (IO_APIC_PIN_POLARITY_LOW_ACTIVE << IO_APIC_PIN_POLARITY_SHIFT)
				| (IO_APIC_DELIVERY_MODE_FIXED << IO_APIC_DELIVERY_MODE_SHIFT);
			sLevelTriggeredInterrupts |= (1 << i);
		}

		ioapic_write_64(IO_APIC_REDIRECTION_TABLE + 2 * i, entry);
	}

	// setup default 1:1 mapping
	for (i = 0; i < 256; i++)
		sIRQToIOAPICPin[i] = i;

	// TODO: here ACPI needs to be used to properly set up the PCI IRQ
	// routing.

	// prefer the ioapic over the normal pic
	dprintf("using ioapic for interrupt routing\n");
	sCurrentPIC = &ioapicController;
	gUsingIOAPIC = true;
}


void
arch_int_enable_io_interrupt(int irq)
{
	sCurrentPIC->enable_io_interrupt(irq);
}


void
arch_int_disable_io_interrupt(int irq)
{
	sCurrentPIC->disable_io_interrupt(irq);
}


void
arch_int_configure_io_interrupt(int irq, uint32 config)
{
	sCurrentPIC->configure_io_interrupt(irq, config);
}


void
arch_int_enable_interrupts(void)
{
	asm("sti");
}


int
arch_int_disable_interrupts(void)
{
	int flags;

	asm("pushfl;\n"
		"popl %0;\n"
		"cli" : "=g" (flags));
	return flags & 0x200 ? 1 : 0;
}


void
arch_int_restore_interrupts(int oldstate)
{
	int flags = oldstate ? 0x200 : 0;

	asm("pushfl;\n"
		"popl	%1;\n"
		"andl	$0xfffffdff,%1;\n"
		"orl	%0,%1;\n"
		"pushl	%1;\n"
		"popfl\n"
		: : "r" (flags), "r" (0));
}


bool
arch_int_are_interrupts_enabled(void)
{
	int flags;

	asm("pushfl;\n"
		"popl %0;\n" : "=g" (flags));
	return flags & 0x200 ? 1 : 0;
}


static const char *
exception_name(int number, char *buffer, int32 bufferSize)
{
	if (number >= 0 && number < kInterruptNameCount)
		return kInterruptNames[number];

	snprintf(buffer, bufferSize, "exception %d", number);
	return buffer;
}


static void
invalid_exception(struct iframe* frame)
{
	struct thread* thread = thread_get_current_thread();
	char name[32];
	panic("unhandled trap 0x%lx (%s) at ip 0x%lx, thread %ld!\n",
		frame->vector, exception_name(frame->vector, name, sizeof(name)),
		frame->eip, thread ? thread->id : -1);
}


static void
fatal_exception(struct iframe *frame)
{
	char name[32];
	panic("Fatal exception \"%s\" occurred! Error code: 0x%lx\n",
		exception_name(frame->vector, name, sizeof(name)), frame->error_code);
}


static void
unexpected_exception(struct iframe* frame)
{
	debug_exception_type type;
	int signal;

	if (IFRAME_IS_VM86(frame)) {
		x86_vm86_return((struct vm86_iframe *)frame, (frame->vector == 13) ?
			B_OK : B_ERROR);
		// won't get here
	}

	switch (frame->vector) {
		case 0:		// Divide Error Exception (#DE)
			type = B_DIVIDE_ERROR;
			signal = SIGFPE;
			break;

		case 4:		// Overflow Exception (#OF)
			type = B_OVERFLOW_EXCEPTION;
			signal = SIGTRAP;
			break;

		case 5:		// BOUND Range Exceeded Exception (#BR)
			type = B_BOUNDS_CHECK_EXCEPTION;
			signal = SIGTRAP;
			break;

		case 6:		// Invalid Opcode Exception (#UD)
			type = B_INVALID_OPCODE_EXCEPTION;
			signal = SIGILL;
			break;

		case 13: 	// General Protection Exception (#GP)
			type = B_GENERAL_PROTECTION_FAULT;
			signal = SIGKILL;
			break;

		case 16: 	// x87 FPU Floating-Point Error (#MF)
			type = B_FLOATING_POINT_EXCEPTION;
			signal = SIGFPE;
			break;

		case 17: 	// Alignment Check Exception (#AC)
			type = B_ALIGNMENT_EXCEPTION;
			signal = SIGTRAP;
			break;

		case 19: 	// SIMD Floating-Point Exception (#XF)
			type = B_FLOATING_POINT_EXCEPTION;
			signal = SIGFPE;
			break;

		default:
			invalid_exception(frame);
			return;
	}

	if (IFRAME_IS_USER(frame)) {
		enable_interrupts();

		if (user_debug_exception_occurred(type, signal))
			send_signal(team_get_current_team_id(), signal);
	} else {
		char name[32];
		panic("Unexpected exception \"%s\" occurred in kernel mode! "
			"Error code: 0x%lx\n",
			exception_name(frame->vector, name, sizeof(name)),
			frame->error_code);
	}
}


static void
double_fault_exception(struct iframe* frame)
{
	// The double fault iframe contains no useful information (as
	// per Intel's architecture spec). Thus we simply save the
	// information from the (unhandable) exception which caused the
	// double in our iframe. This will result even in useful stack
	// traces. Only problem is that we trust that at least the
	// TSS is still accessible.
	struct tss *tss = &gCPU[smp_get_current_cpu()].arch.tss;

	frame->cs = tss->cs;
	frame->es = tss->es;
	frame->ds = tss->ds;
	frame->fs = tss->fs;
	frame->gs = tss->gs;
	frame->eip = tss->eip;
	frame->ebp = tss->ebp;
	frame->esp = tss->esp;
	frame->eax = tss->eax;
	frame->ebx = tss->ebx;
	frame->ecx = tss->ecx;
	frame->edx = tss->edx;
	frame->esi = tss->esi;
	frame->edi = tss->edi;
	frame->flags = tss->eflags;

	panic("double fault!\n");
}


static void
page_fault_exception(struct iframe* frame)
{
	struct thread *thread = thread_get_current_thread();
	bool kernelDebugger = debug_debugger_running();
	unsigned int cr2;
	addr_t newip;

	asm("movl %%cr2, %0" : "=r" (cr2));

	if (kernelDebugger) {
		// if this thread has a fault handler, we're allowed to be here
		if (thread && thread->fault_handler != 0) {
			frame->eip = thread->fault_handler;
			return;
		}

		// otherwise, not really
		panic("page fault in debugger without fault handler! Touching "
			"address %p from eip %p\n", (void *)cr2, (void *)frame->eip);
		return;
	} else if ((frame->flags & 0x200) == 0) {
		// if the interrupts were disabled, and we are not running the kernel startup
		// the page fault was not allowed to happen and we must panic
		panic("page fault, but interrupts were disabled. Touching address "
			"%p from eip %p\n", (void *)cr2, (void *)frame->eip);
		return;
	} else if (thread != NULL && thread->page_faults_allowed < 1) {
		panic("page fault not allowed at this place. Touching address "
			"%p from eip %p\n", (void *)cr2, (void *)frame->eip);
		return;
	}

	enable_interrupts();

	vm_page_fault(cr2, frame->eip,
		(frame->error_code & 0x2) != 0,	// write access
		(frame->error_code & 0x4) != 0,	// userland
		&newip);
	if (newip != 0) {
		// the page fault handler wants us to modify the iframe to set the
		// IP the cpu will return to to be this ip
		frame->eip = newip;
	}
}


static void
hardware_interrupt(struct iframe* frame)
{
	int32 vector = frame->vector - ARCH_INTERRUPT_BASE;
	bool levelTriggered = false;
	int ret;

	if (sCurrentPIC->is_spurious_interrupt(vector)) {
		TRACE(("got spurious interrupt at vector %ld\n", vector));
		return;
	}

	if (vector < 32)
		levelTriggered = (sLevelTriggeredInterrupts & (1 << vector)) != 0;

	if (!levelTriggered)
		sCurrentPIC->end_of_interrupt(vector);

	ret = int_io_interrupt_handler(vector, levelTriggered);

	if (levelTriggered)
		sCurrentPIC->end_of_interrupt(vector);

	if (ret == B_INVOKE_SCHEDULER) {
		cpu_status state = disable_interrupts();
		GRAB_THREAD_LOCK();

		scheduler_reschedule();

		RELEASE_THREAD_LOCK();
		restore_interrupts(state);
	}
}


status_t
arch_int_init(struct kernel_args *args)
{
	int i;
	interrupt_handler_function** table;

	// set the global sIDT variable
	sIDT = (desc_table *)args->arch_args.vir_idt;

	// setup the standard programmable interrupt controller
	pic_init();

	set_intr_gate(0,  &trap0);
	set_intr_gate(1,  &trap1);
	set_intr_gate(2,  &trap2);
	set_system_gate(3,  &trap3);
	set_intr_gate(4,  &trap4);
	set_intr_gate(5,  &trap5);
	set_intr_gate(6,  &trap6);
	set_intr_gate(7,  &trap7);
	// trap8 (double fault) is set in arch_cpu.c
	set_intr_gate(9,  &trap9);
	set_intr_gate(10,  &trap10);
	set_intr_gate(11,  &trap11);
	set_intr_gate(12,  &trap12);
	set_intr_gate(13,  &trap13);
	set_intr_gate(14,  &trap14);
//	set_intr_gate(15,  &trap15);
	set_intr_gate(16,  &trap16);
	set_intr_gate(17,  &trap17);
	set_intr_gate(18,  &trap18);
	set_intr_gate(19,  &trap19);

	set_intr_gate(32,  &trap32);
	set_intr_gate(33,  &trap33);
	set_intr_gate(34,  &trap34);
	set_intr_gate(35,  &trap35);
	set_intr_gate(36,  &trap36);
	set_intr_gate(37,  &trap37);
	set_intr_gate(38,  &trap38);
	set_intr_gate(39,  &trap39);
	set_intr_gate(40,  &trap40);
	set_intr_gate(41,  &trap41);
	set_intr_gate(42,  &trap42);
	set_intr_gate(43,  &trap43);
	set_intr_gate(44,  &trap44);
	set_intr_gate(45,  &trap45);
	set_intr_gate(46,  &trap46);
	set_intr_gate(47,  &trap47);
	set_intr_gate(48,  &trap48);
	set_intr_gate(49,  &trap49);
	set_intr_gate(50,  &trap50);
	set_intr_gate(51,  &trap51);
	set_intr_gate(52,  &trap52);
	set_intr_gate(53,  &trap53);
	set_intr_gate(54,  &trap54);
	set_intr_gate(55,  &trap55);

	set_system_gate(98, &trap98);	// for performance testing only
	set_system_gate(99, &trap99);

	set_intr_gate(251, &trap251);
	set_intr_gate(252, &trap252);
	set_intr_gate(253, &trap253);
	set_intr_gate(254, &trap254);
	set_intr_gate(255, &trap255);

	// init interrupt handler table
	table = gInterruptHandlerTable;

	// defaults
	for (i = 0; i < ARCH_INTERRUPT_BASE; i++)
		table[i] = invalid_exception;
	for (i = ARCH_INTERRUPT_BASE; i < INTERRUPT_HANDLER_TABLE_SIZE; i++)
		table[i] = hardware_interrupt;

	table[0] = unexpected_exception;	// Divide Error Exception (#DE)
	table[1] = x86_handle_debug_exception; // Debug Exception (#DB)
	table[2] = fatal_exception;			// NMI Interrupt
	table[3] = x86_handle_breakpoint_exception; // Breakpoint Exception (#BP)
	table[4] = unexpected_exception;	// Overflow Exception (#OF)
	table[5] = unexpected_exception;	// BOUND Range Exceeded Exception (#BR)
	table[6] = unexpected_exception;	// Invalid Opcode Exception (#UD)
	table[7] = fatal_exception;			// Device Not Available Exception (#NM)
	table[8] = double_fault_exception;	// Double Fault Exception (#DF)
	table[9] = fatal_exception;			// Coprocessor Segment Overrun
	table[10] = fatal_exception;		// Invalid TSS Exception (#TS)
	table[11] = fatal_exception;		// Segment Not Present (#NP)
	table[12] = fatal_exception;		// Stack Fault Exception (#SS)
	table[13] = unexpected_exception;	// General Protection Exception (#GP)
	table[14] = page_fault_exception;	// Page-Fault Exception (#PF)
	table[16] = unexpected_exception;	// x87 FPU Floating-Point Error (#MF)
	table[17] = unexpected_exception;	// Alignment Check Exception (#AC)
	table[18] = fatal_exception;		// Machine-Check Exception (#MC)
	table[19] = unexpected_exception;	// SIMD Floating-Point Exception (#XF)

	return B_OK;
}


status_t
arch_int_init_post_vm(struct kernel_args *args)
{
	area_id area;

	ioapic_init(args);

	sIDT = (desc_table *)args->arch_args.vir_idt;
	area = create_area("idt", (void *)&sIDT, B_EXACT_ADDRESS, B_PAGE_SIZE, B_ALREADY_WIRED,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);

	return area >= B_OK ? B_OK : area;
}


status_t
arch_int_init_post_device_manager(struct kernel_args *args)
{
	return B_OK;
}
