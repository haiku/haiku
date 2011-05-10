/*
 * Copyright 2008-2011, Michael Lotz, mmlr@mlotz.ch.
 * Copyright 2010, Clemens Zeidler, haiku@clemens-zeidler.de.
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2010, Axel Dörfler, axeld@pinc-software.de.
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
#include <vm/vm.h>
#include <vm/vm_priv.h>

#include <arch/cpu.h>
#include <arch/int.h>
#include <arch/smp.h>
#include <arch/user_debugger.h>
#include <arch/vm.h>

#include <arch/x86/apic.h>
#include <arch/x86/descriptors.h>
#include <arch/x86/vm86.h>

#include "interrupts.h"
#include "irq_routing_table.h"

#include <ACPI.h>
#include <AutoDeleter.h>
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


// ACPI interrupt models
#define ACPI_INTERRUPT_MODEL_PIC	0
#define ACPI_INTERRUPT_MODEL_APIC	1
#define ACPI_INTERRUPT_MODEL_SAPIC	2


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
} ioapic;

static ioapic *sIOAPIC = NULL;
static uint32 sIOAPICMaxRedirectionEntry = 23;

typedef struct interrupt_controller_s {
	const char *name;
	void	(*enable_io_interrupt)(int32 num);
	void	(*disable_io_interrupt)(int32 num);
	void	(*configure_io_interrupt)(int32 num, uint32 config);
	bool	(*is_spurious_interrupt)(int32 num);
	bool	(*end_of_interrupt)(int32 num);
} interrupt_controller;

static const interrupt_controller *sCurrentPIC = NULL;

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
static desc_table* sIDTs[B_MAX_CPU_COUNT];

static uint32 sLevelTriggeredInterrupts = 0;
	// binary mask: 1 level, 0 edge

// table with functions handling respective interrupts
typedef void interrupt_handler_function(struct iframe* frame);
#define INTERRUPT_HANDLER_TABLE_SIZE 256
interrupt_handler_function* gInterruptHandlerTable[
	INTERRUPT_HANDLER_TABLE_SIZE];


/*!	Initializes a descriptor in an IDT.
*/
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


/*!	Initializes the descriptor for interrupt vector \a n in the IDT of the
	specified CPU to an interrupt-gate descriptor with the given procedure
	address.
	For CPUs other than the boot CPU it must not be called before
	arch_int_init_post_vm().
*/
static void
set_interrupt_gate(int32 cpu, int n, void (*addr)())
{
	set_gate(&sIDTs[cpu][n], (addr_t)addr, 14, DPL_KERNEL);
}


/*!	Initializes the descriptor for interrupt vector \a n in the IDT of the
	specified CPU to an trap-gate descriptor with the given procedure address.
	For CPUs other than the boot CPU it must not be called before
	arch_int_init_post_vm().
*/
static void
set_trap_gate(int32 cpu, int n, void (*addr)())
{
	set_gate(&sIDTs[cpu][n], (unsigned int)addr, 15, DPL_USER);
}


/*!	Initializes the descriptor for interrupt vector \a n in the IDT of CPU
	\a cpu to a task-gate descripter referring to the TSS segment identified
	by TSS segment selector \a segment.
	For CPUs other than the boot CPU it must not be called before
	arch_int_init_post_vm() (arch_cpu_init_post_vm() is fine).
*/
void
x86_set_task_gate(int32 cpu, int32 n, int32 segment)
{
	sIDTs[cpu][n].a = (segment << 16);
	sIDTs[cpu][n].b = 0x8000 | (0 << 13) | (0x5 << 8); // present, dpl 0, type 5
}


/*!	Returns the virtual IDT address for CPU \a cpu. */
void*
x86_get_idt(int32 cpu)
{
	return sIDTs[cpu];
}


// #pragma mark - PIC


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
}


static void
pic_disable()
{
	// Mask off all interrupts on master and slave
	out8(0xff, PIC_MASTER_MASK);
	out8(0xff, PIC_SLAVE_MASK);
}


// #pragma mark - I/O APIC


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
	sIOAPIC->io_register_select = registerSelect + 1;
	uint64 result = sIOAPIC->io_window_register;
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


static bool
ioapic_end_of_interrupt(int32 num)
{
	apic_end_of_interrupt();
	return true;
}


static void
ioapic_enable_io_interrupt(int32 pin)
{
	if (pin < 0 || pin > (int32)sIOAPICMaxRedirectionEntry)
		return;

	TRACE(("ioapic_enable_io_interrupt: IRQ %ld -> pin %ld\n", num, pin));

	uint64 entry = ioapic_read_64(IO_APIC_REDIRECTION_TABLE + pin * 2);
	entry &= ~(1 << IO_APIC_INTERRUPT_MASK_SHIFT);
	entry |= IO_APIC_INTERRUPT_UNMASKED << IO_APIC_INTERRUPT_MASK_SHIFT;
	ioapic_write_64(IO_APIC_REDIRECTION_TABLE + pin * 2, entry);
}


static void
ioapic_disable_io_interrupt(int32 pin)
{
	if (pin < 0 || pin > (int32)sIOAPICMaxRedirectionEntry)
		return;

	TRACE(("ioapic_disable_io_interrupt: IRQ %ld -> pin %ld\n", num, pin));

	uint64 entry = ioapic_read_64(IO_APIC_REDIRECTION_TABLE + pin * 2);
	entry &= ~(1 << IO_APIC_INTERRUPT_MASK_SHIFT);
	entry |= IO_APIC_INTERRUPT_MASKED << IO_APIC_INTERRUPT_MASK_SHIFT;
	ioapic_write_64(IO_APIC_REDIRECTION_TABLE + pin * 2, entry);
}


static void
ioapic_configure_io_interrupt(int32 pin, uint32 config)
{
	if (pin < 0 || pin > (int32)sIOAPICMaxRedirectionEntry)
		return;

	TRACE(("ioapic_configure_io_interrupt: IRQ %ld -> pin %ld; config 0x%08lx\n",
		num, pin, config));

	uint64 entry = ioapic_read_64(IO_APIC_REDIRECTION_TABLE + pin * 2);
	entry &= ~((1 << IO_APIC_TRIGGER_MODE_SHIFT)
		| (1 << IO_APIC_PIN_POLARITY_SHIFT)
		| (IO_APIC_INTERRUPT_VECTOR_MASK << IO_APIC_INTERRUPT_VECTOR_SHIFT));

	if (config & B_LEVEL_TRIGGERED) {
		entry |= (IO_APIC_TRIGGER_MODE_LEVEL << IO_APIC_TRIGGER_MODE_SHIFT);
		sLevelTriggeredInterrupts |= (1 << pin);
	} else {
		entry |= (IO_APIC_TRIGGER_MODE_EDGE << IO_APIC_TRIGGER_MODE_SHIFT);
		sLevelTriggeredInterrupts &= ~(1 << pin);
	}

	if (config & B_LOW_ACTIVE_POLARITY)
		entry |= (IO_APIC_PIN_POLARITY_LOW_ACTIVE << IO_APIC_PIN_POLARITY_SHIFT);
	else
		entry |= (IO_APIC_PIN_POLARITY_HIGH_ACTIVE << IO_APIC_PIN_POLARITY_SHIFT);

	entry |= (pin + ARCH_INTERRUPT_BASE) << IO_APIC_INTERRUPT_VECTOR_SHIFT;
	ioapic_write_64(IO_APIC_REDIRECTION_TABLE + pin * 2, entry);
}


static void
ioapic_map(kernel_args* args)
{
	if (args->arch_args.apic == NULL) {
		dprintf("no local apic available\n");
		return;
	}

	if (args->arch_args.ioapic == NULL) {
		dprintf("no ioapic available, not using ioapics for interrupt routing\n");
		return;
	}

	// map in the ioapic
	sIOAPIC = (ioapic *)args->arch_args.ioapic;
	if (vm_map_physical_memory(B_SYSTEM_TEAM, "ioapic", (void**)&sIOAPIC,
			B_EXACT_ADDRESS, B_PAGE_SIZE,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
			args->arch_args.ioapic_phys, true) < 0) {
		panic("mapping the ioapic failed");
		return;
	}
}


static status_t
acpi_set_interrupt_model(acpi_module_info* acpiModule, uint32 interruptModel)
{
	acpi_object_type model;
	model.object_type = ACPI_TYPE_INTEGER;
	model.data.integer = interruptModel;

	acpi_objects parameter;
	parameter.count = 1;
	parameter.pointer = &model;

	dprintf("setting ACPI interrupt model to %s\n",
		interruptModel == 0 ? "PIC"
		: (interruptModel == 1 ? "APIC"
		: (interruptModel == 2 ? "SAPIC"
		: "unknown")));

	return acpiModule->evaluate_method(NULL, "\\_PIC", &parameter, NULL);
}


static void
ioapic_init(kernel_args* args)
{
	static const interrupt_controller ioapicController = {
		"82093AA IOAPIC",
		&ioapic_enable_io_interrupt,
		&ioapic_disable_io_interrupt,
		&ioapic_configure_io_interrupt,
		&ioapic_is_spurious_interrupt,
		&ioapic_end_of_interrupt
	};

	if (sIOAPIC == NULL)
		return;

#if 0
	if (get_safemode_boolean(B_SAFEMODE_DISABLE_IOAPIC, false)) {
		dprintf("ioapic explicitly disabled, not using ioapics for interrupt "
			"routing\n");
		return;
	}
#else
	// TODO: This can be removed once IO-APIC code is broadly tested
	if (!get_safemode_boolean(B_SAFEMODE_ENABLE_IOAPIC, false)) {
		dprintf("ioapic not enabled, not using ioapics for interrupt "
			"routing\n");
		return;
	}
#endif

	uint32 version = ioapic_read_32(IO_APIC_VERSION);
	if (version == 0xffffffff) {
		dprintf("ioapic seems inaccessible, not using it\n");
		return;
	}

	// load acpi module
	status_t status;
	acpi_module_info* acpiModule;
	status = get_module(B_ACPI_MODULE_NAME, (module_info**)&acpiModule);
	if (status != B_OK) {
		dprintf("acpi module not available, not configuring ioapic\n");
		return;
	}
	BPrivate::CObjectDeleter<const char, status_t>
		acpiModulePutter(B_ACPI_MODULE_NAME, put_module);

	// switch to the APIC interrupt model before retrieving the irq routing
	// table as it will return different settings depending on the model
	status = acpi_set_interrupt_model(acpiModule, ACPI_INTERRUPT_MODEL_APIC);
	if (status != B_OK) {
		dprintf("failed to put ACPI into APIC interrupt model, ignoring\n");
		// don't abort, as the _PIC method is optional and as long as there
		// aren't different routings based on it this is non-fatal
	}

	sLevelTriggeredInterrupts = 0;
	sIOAPICMaxRedirectionEntry
		= ((version >> IO_APIC_MAX_REDIRECTION_ENTRY_SHIFT)
			& IO_APIC_MAX_REDIRECTION_ENTRY_MASK);

	TRACE(("ioapic has %lu entries\n", sIOAPICMaxRedirectionEntry + 1));

	IRQRoutingTable table;
	status = prepare_irq_routing(acpiModule, table,
		sIOAPICMaxRedirectionEntry + 1);
	if (status != B_OK) {
		dprintf("IRQ routing preparation failed, not configuring ioapic.\n");
		acpi_set_interrupt_model(acpiModule, ACPI_INTERRUPT_MODEL_PIC);
			// revert to PIC interrupt model just in case
		return;
	}

	print_irq_routing_table(table);

	status = enable_irq_routing(acpiModule, table);
	if (status != B_OK) {
		panic("failed to enable IRQ routing");
		// if it failed early on it might still work in PIC mode
		acpi_set_interrupt_model(acpiModule, ACPI_INTERRUPT_MODEL_PIC);
		return;
	}

	// use the boot CPU as the target for all interrupts
	uint64 targetAPIC = args->arch_args.cpu_apic_id[0];

	// program the interrupt vectors of the ioapic
	for (uint32 i = 0; i <= sIOAPICMaxRedirectionEntry; i++) {
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

	// configure io apic interrupts from PCI routing table
	for (int i = 0; i < table.Count(); i++) {
		irq_routing_entry& entry = table.ElementAt(i);
		ioapic_configure_io_interrupt(entry.irq,
			entry.polarity | entry.trigger_mode);
	}

	// disable the legacy PIC
	pic_disable();

	// prefer the ioapic over the normal pic
	dprintf("using ioapic for interrupt routing\n");
	sCurrentPIC = &ioapicController;
}


// #pragma mark -


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


#undef arch_int_enable_interrupts
#undef arch_int_disable_interrupts
#undef arch_int_restore_interrupts
#undef arch_int_are_interrupts_enabled


void
arch_int_enable_interrupts(void)
{
	arch_int_enable_interrupts_inline();
}


int
arch_int_disable_interrupts(void)
{
	return arch_int_disable_interrupts_inline();
}


void
arch_int_restore_interrupts(int oldState)
{
	arch_int_restore_interrupts_inline(oldState);
}


bool
arch_int_are_interrupts_enabled(void)
{
	return arch_int_are_interrupts_enabled_inline();
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
	Thread* thread = thread_get_current_thread();
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
			signal = SIGILL;
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
		struct sigaction action;
		Thread* thread = thread_get_current_thread();

		enable_interrupts();

		// If the thread has a signal handler for the signal, we simply send it
		// the signal. Otherwise we notify the user debugger first.
		if (sigaction(signal, NULL, &action) == 0
			&& action.sa_handler != SIG_DFL
			&& action.sa_handler != SIG_IGN) {
			send_signal(thread->id, signal);
		} else if (user_debug_exception_occurred(type, signal))
			send_signal(team_get_current_team_id(), signal);
	} else {
		char name[32];
		panic("Unexpected exception \"%s\" occurred in kernel mode! "
			"Error code: 0x%lx\n",
			exception_name(frame->vector, name, sizeof(name)),
			frame->error_code);
	}
}


void
x86_double_fault_exception(struct iframe* frame)
{
	int cpu = x86_double_fault_get_cpu();

	// The double fault iframe contains no useful information (as
	// per Intel's architecture spec). Thus we simply save the
	// information from the (unhandlable) exception which caused the
	// double in our iframe. This will result even in useful stack
	// traces. Only problem is that we trust that at least the
	// TSS is still accessible.
	struct tss *tss = &gCPU[cpu].arch.tss;

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

	// Use a special handler for page faults which avoids the triple fault
	// pitfalls.
	set_interrupt_gate(cpu, 14, &trap14_double_fault);

	debug_double_fault(cpu);
}


void
x86_page_fault_exception_double_fault(struct iframe* frame)
{
	uint32 cr2;
	asm("movl %%cr2, %0" : "=r" (cr2));

	// Only if this CPU has a fault handler, we're allowed to be here.
	cpu_ent& cpu = gCPU[x86_double_fault_get_cpu()];
	addr_t faultHandler = cpu.fault_handler;
	if (faultHandler != 0) {
		debug_set_page_fault_info(cr2, frame->eip,
			(frame->error_code & 0x2) != 0 ? DEBUG_PAGE_FAULT_WRITE : 0);
		frame->eip = faultHandler;
		frame->ebp = cpu.fault_handler_stack_pointer;
		return;
	}

	// No fault handler. This is bad. Since we originally came from a double
	// fault, we don't try to reenter the kernel debugger. Instead we just
	// print the info we've got and enter an infinite loop.
	kprintf("Page fault in double fault debugger without fault handler! "
		"Touching address %p from eip %p. Entering infinite loop...\n",
		(void*)cr2, (void*)frame->eip);

	while (true);
}


static void
page_fault_exception(struct iframe* frame)
{
	Thread *thread = thread_get_current_thread();
	uint32 cr2;
	addr_t newip;

	asm("movl %%cr2, %0" : "=r" (cr2));

	if (debug_debugger_running()) {
		// If this CPU or this thread has a fault handler, we're allowed to be
		// here.
		if (thread != NULL) {
			cpu_ent* cpu = &gCPU[smp_get_current_cpu()];
			if (cpu->fault_handler != 0) {
				debug_set_page_fault_info(cr2, frame->eip,
					(frame->error_code & 0x2) != 0
						? DEBUG_PAGE_FAULT_WRITE : 0);
				frame->eip = cpu->fault_handler;
				frame->ebp = cpu->fault_handler_stack_pointer;
				return;
			}

			if (thread->fault_handler != 0) {
				kprintf("ERROR: thread::fault_handler used in kernel "
					"debugger!\n");
				debug_set_page_fault_info(cr2, frame->eip,
					(frame->error_code & 0x2) != 0
						? DEBUG_PAGE_FAULT_WRITE : 0);
				frame->eip = thread->fault_handler;
				return;
			}
		}

		// otherwise, not really
		panic("page fault in debugger without fault handler! Touching "
			"address %p from eip %p\n", (void *)cr2, (void *)frame->eip);
		return;
	} else if ((frame->flags & 0x200) == 0) {
		// interrupts disabled

		// If a page fault handler is installed, we're allowed to be here.
		// TODO: Now we are generally allowing user_memcpy() with interrupts
		// disabled, which in most cases is a bug. We should add some thread
		// flag allowing to explicitly indicate that this handling is desired.
		if (thread && thread->fault_handler != 0) {
			if (frame->eip != thread->fault_handler) {
				frame->eip = thread->fault_handler;
				return;
			}

			// The fault happened at the fault handler address. This is a
			// certain infinite loop.
			panic("page fault, interrupts disabled, fault handler loop. "
				"Touching address %p from eip %p\n", (void*)cr2,
				(void*)frame->eip);
		}

		// If we are not running the kernel startup the page fault was not
		// allowed to happen and we must panic.
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
	Thread* thread = thread_get_current_thread();

	if (sCurrentPIC->is_spurious_interrupt(vector)) {
		TRACE(("got spurious interrupt at vector %ld\n", vector));
		return;
	}

	if (vector < 32)
		levelTriggered = (sLevelTriggeredInterrupts & (1 << vector)) != 0;

	if (!levelTriggered) {
		// if it's not handled by the current pic then it's an apic generated
		// interrupt like local interrupts, msi or ipi.
		if (!sCurrentPIC->end_of_interrupt(vector))
			apic_end_of_interrupt();
	}

	int_io_interrupt_handler(vector, levelTriggered);

	if (levelTriggered) {
		if (!sCurrentPIC->end_of_interrupt(vector))
			apic_end_of_interrupt();
	}

	cpu_status state = disable_interrupts();
	if (thread->cpu->invoke_scheduler) {
		GRAB_THREAD_LOCK();
		scheduler_reschedule();
		RELEASE_THREAD_LOCK();
		restore_interrupts(state);
	} else if (thread->post_interrupt_callback != NULL) {
		restore_interrupts(state);
		void (*callback)(void*) = thread->post_interrupt_callback;
		void* data = thread->post_interrupt_data;

		thread->post_interrupt_callback = NULL;
		thread->post_interrupt_data = NULL;

		callback(data);
	}
}


status_t
arch_int_init(struct kernel_args *args)
{
	int i;
	interrupt_handler_function** table;

	// set the global sIDT variable
	sIDTs[0] = (desc_table *)args->arch_args.vir_idt;

	// setup the standard programmable interrupt controller
	pic_init();

	set_interrupt_gate(0, 0, &trap0);
	set_interrupt_gate(0, 1, &trap1);
	set_interrupt_gate(0, 2, &trap2);
	set_trap_gate(0, 3, &trap3);
	set_interrupt_gate(0, 4, &trap4);
	set_interrupt_gate(0, 5, &trap5);
	set_interrupt_gate(0, 6, &trap6);
	set_interrupt_gate(0, 7, &trap7);
	// trap8 (double fault) is set in arch_cpu.c
	set_interrupt_gate(0, 9, &trap9);
	set_interrupt_gate(0, 10, &trap10);
	set_interrupt_gate(0, 11, &trap11);
	set_interrupt_gate(0, 12, &trap12);
	set_interrupt_gate(0, 13, &trap13);
	set_interrupt_gate(0, 14, &trap14);
//	set_interrupt_gate(0, 15, &trap15);
	set_interrupt_gate(0, 16, &trap16);
	set_interrupt_gate(0, 17, &trap17);
	set_interrupt_gate(0, 18, &trap18);
	set_interrupt_gate(0, 19, &trap19);

	// legacy or ioapic interrupts
	set_interrupt_gate(0, 32, &trap32);
	set_interrupt_gate(0, 33, &trap33);
	set_interrupt_gate(0, 34, &trap34);
	set_interrupt_gate(0, 35, &trap35);
	set_interrupt_gate(0, 36, &trap36);
	set_interrupt_gate(0, 37, &trap37);
	set_interrupt_gate(0, 38, &trap38);
	set_interrupt_gate(0, 39, &trap39);
	set_interrupt_gate(0, 40, &trap40);
	set_interrupt_gate(0, 41, &trap41);
	set_interrupt_gate(0, 42, &trap42);
	set_interrupt_gate(0, 43, &trap43);
	set_interrupt_gate(0, 44, &trap44);
	set_interrupt_gate(0, 45, &trap45);
	set_interrupt_gate(0, 46, &trap46);
	set_interrupt_gate(0, 47, &trap47);

	// additional ioapic interrupts
	set_interrupt_gate(0, 48, &trap48);
	set_interrupt_gate(0, 49, &trap49);
	set_interrupt_gate(0, 50, &trap50);
	set_interrupt_gate(0, 51, &trap51);
	set_interrupt_gate(0, 52, &trap52);
	set_interrupt_gate(0, 53, &trap53);
	set_interrupt_gate(0, 54, &trap54);
	set_interrupt_gate(0, 55, &trap55);

	// configurable msi or msi-x interrupts
	set_interrupt_gate(0, 56, &trap56);
	set_interrupt_gate(0, 57, &trap57);
	set_interrupt_gate(0, 58, &trap58);
	set_interrupt_gate(0, 59, &trap59);
	set_interrupt_gate(0, 60, &trap60);
	set_interrupt_gate(0, 61, &trap61);
	set_interrupt_gate(0, 62, &trap62);
	set_interrupt_gate(0, 63, &trap63);
	set_interrupt_gate(0, 64, &trap64);
	set_interrupt_gate(0, 65, &trap65);
	set_interrupt_gate(0, 66, &trap66);
	set_interrupt_gate(0, 67, &trap67);
	set_interrupt_gate(0, 68, &trap68);
	set_interrupt_gate(0, 69, &trap69);
	set_interrupt_gate(0, 70, &trap70);
	set_interrupt_gate(0, 71, &trap71);
	set_interrupt_gate(0, 72, &trap72);
	set_interrupt_gate(0, 73, &trap73);
	set_interrupt_gate(0, 74, &trap74);
	set_interrupt_gate(0, 75, &trap75);
	set_interrupt_gate(0, 76, &trap76);
	set_interrupt_gate(0, 77, &trap77);
	set_interrupt_gate(0, 78, &trap78);
	set_interrupt_gate(0, 79, &trap79);
	set_interrupt_gate(0, 80, &trap80);
	set_interrupt_gate(0, 81, &trap81);
	set_interrupt_gate(0, 82, &trap82);
	set_interrupt_gate(0, 83, &trap83);
	set_interrupt_gate(0, 84, &trap84);
	set_interrupt_gate(0, 85, &trap85);
	set_interrupt_gate(0, 86, &trap86);
	set_interrupt_gate(0, 87, &trap87);
	set_interrupt_gate(0, 88, &trap88);
	set_interrupt_gate(0, 89, &trap89);
	set_interrupt_gate(0, 90, &trap90);
	set_interrupt_gate(0, 91, &trap91);
	set_interrupt_gate(0, 92, &trap92);
	set_interrupt_gate(0, 93, &trap93);
	set_interrupt_gate(0, 94, &trap94);
	set_interrupt_gate(0, 95, &trap95);
	set_interrupt_gate(0, 96, &trap96);
	set_interrupt_gate(0, 97, &trap97);

	set_trap_gate(0, 98, &trap98);	// for performance testing only
	set_trap_gate(0, 99, &trap99);	// syscall interrupt

	// configurable msi or msi-x interrupts
	set_interrupt_gate(0, 100, &trap100);
	set_interrupt_gate(0, 101, &trap101);
	set_interrupt_gate(0, 102, &trap102);
	set_interrupt_gate(0, 103, &trap103);
	set_interrupt_gate(0, 104, &trap104);
	set_interrupt_gate(0, 105, &trap105);
	set_interrupt_gate(0, 106, &trap106);
	set_interrupt_gate(0, 107, &trap107);
	set_interrupt_gate(0, 108, &trap108);
	set_interrupt_gate(0, 109, &trap109);
	set_interrupt_gate(0, 110, &trap110);
	set_interrupt_gate(0, 111, &trap111);
	set_interrupt_gate(0, 112, &trap112);
	set_interrupt_gate(0, 113, &trap113);
	set_interrupt_gate(0, 114, &trap114);
	set_interrupt_gate(0, 115, &trap115);
	set_interrupt_gate(0, 116, &trap116);
	set_interrupt_gate(0, 117, &trap117);
	set_interrupt_gate(0, 118, &trap118);
	set_interrupt_gate(0, 119, &trap119);
	set_interrupt_gate(0, 120, &trap120);
	set_interrupt_gate(0, 121, &trap121);
	set_interrupt_gate(0, 122, &trap122);
	set_interrupt_gate(0, 123, &trap123);
	set_interrupt_gate(0, 124, &trap124);
	set_interrupt_gate(0, 125, &trap125);
	set_interrupt_gate(0, 126, &trap126);
	set_interrupt_gate(0, 127, &trap127);
	set_interrupt_gate(0, 128, &trap128);
	set_interrupt_gate(0, 129, &trap129);
	set_interrupt_gate(0, 130, &trap130);
	set_interrupt_gate(0, 131, &trap131);
	set_interrupt_gate(0, 132, &trap132);
	set_interrupt_gate(0, 133, &trap133);
	set_interrupt_gate(0, 134, &trap134);
	set_interrupt_gate(0, 135, &trap135);
	set_interrupt_gate(0, 136, &trap136);
	set_interrupt_gate(0, 137, &trap137);
	set_interrupt_gate(0, 138, &trap138);
	set_interrupt_gate(0, 139, &trap139);
	set_interrupt_gate(0, 140, &trap140);
	set_interrupt_gate(0, 141, &trap141);
	set_interrupt_gate(0, 142, &trap142);
	set_interrupt_gate(0, 143, &trap143);
	set_interrupt_gate(0, 144, &trap144);
	set_interrupt_gate(0, 145, &trap145);
	set_interrupt_gate(0, 146, &trap146);
	set_interrupt_gate(0, 147, &trap147);
	set_interrupt_gate(0, 148, &trap148);
	set_interrupt_gate(0, 149, &trap149);
	set_interrupt_gate(0, 150, &trap150);
	set_interrupt_gate(0, 151, &trap151);
	set_interrupt_gate(0, 152, &trap152);
	set_interrupt_gate(0, 153, &trap153);
	set_interrupt_gate(0, 154, &trap154);
	set_interrupt_gate(0, 155, &trap155);
	set_interrupt_gate(0, 156, &trap156);
	set_interrupt_gate(0, 157, &trap157);
	set_interrupt_gate(0, 158, &trap158);
	set_interrupt_gate(0, 159, &trap159);
	set_interrupt_gate(0, 160, &trap160);
	set_interrupt_gate(0, 161, &trap161);
	set_interrupt_gate(0, 162, &trap162);
	set_interrupt_gate(0, 163, &trap163);
	set_interrupt_gate(0, 164, &trap164);
	set_interrupt_gate(0, 165, &trap165);
	set_interrupt_gate(0, 166, &trap166);
	set_interrupt_gate(0, 167, &trap167);
	set_interrupt_gate(0, 168, &trap168);
	set_interrupt_gate(0, 169, &trap169);
	set_interrupt_gate(0, 170, &trap170);
	set_interrupt_gate(0, 171, &trap171);
	set_interrupt_gate(0, 172, &trap172);
	set_interrupt_gate(0, 173, &trap173);
	set_interrupt_gate(0, 174, &trap174);
	set_interrupt_gate(0, 175, &trap175);
	set_interrupt_gate(0, 176, &trap176);
	set_interrupt_gate(0, 177, &trap177);
	set_interrupt_gate(0, 178, &trap178);
	set_interrupt_gate(0, 179, &trap179);
	set_interrupt_gate(0, 180, &trap180);
	set_interrupt_gate(0, 181, &trap181);
	set_interrupt_gate(0, 182, &trap182);
	set_interrupt_gate(0, 183, &trap183);
	set_interrupt_gate(0, 184, &trap184);
	set_interrupt_gate(0, 185, &trap185);
	set_interrupt_gate(0, 186, &trap186);
	set_interrupt_gate(0, 187, &trap187);
	set_interrupt_gate(0, 188, &trap188);
	set_interrupt_gate(0, 189, &trap189);
	set_interrupt_gate(0, 190, &trap190);
	set_interrupt_gate(0, 191, &trap191);
	set_interrupt_gate(0, 192, &trap192);
	set_interrupt_gate(0, 193, &trap193);
	set_interrupt_gate(0, 194, &trap194);
	set_interrupt_gate(0, 195, &trap195);
	set_interrupt_gate(0, 196, &trap196);
	set_interrupt_gate(0, 197, &trap197);
	set_interrupt_gate(0, 198, &trap198);
	set_interrupt_gate(0, 199, &trap199);
	set_interrupt_gate(0, 200, &trap200);
	set_interrupt_gate(0, 201, &trap201);
	set_interrupt_gate(0, 202, &trap202);
	set_interrupt_gate(0, 203, &trap203);
	set_interrupt_gate(0, 204, &trap204);
	set_interrupt_gate(0, 205, &trap205);
	set_interrupt_gate(0, 206, &trap206);
	set_interrupt_gate(0, 207, &trap207);
	set_interrupt_gate(0, 208, &trap208);
	set_interrupt_gate(0, 209, &trap209);
	set_interrupt_gate(0, 210, &trap210);
	set_interrupt_gate(0, 211, &trap211);
	set_interrupt_gate(0, 212, &trap212);
	set_interrupt_gate(0, 213, &trap213);
	set_interrupt_gate(0, 214, &trap214);
	set_interrupt_gate(0, 215, &trap215);
	set_interrupt_gate(0, 216, &trap216);
	set_interrupt_gate(0, 217, &trap217);
	set_interrupt_gate(0, 218, &trap218);
	set_interrupt_gate(0, 219, &trap219);
	set_interrupt_gate(0, 220, &trap220);
	set_interrupt_gate(0, 221, &trap221);
	set_interrupt_gate(0, 222, &trap222);
	set_interrupt_gate(0, 223, &trap223);
	set_interrupt_gate(0, 224, &trap224);
	set_interrupt_gate(0, 225, &trap225);
	set_interrupt_gate(0, 226, &trap226);
	set_interrupt_gate(0, 227, &trap227);
	set_interrupt_gate(0, 228, &trap228);
	set_interrupt_gate(0, 229, &trap229);
	set_interrupt_gate(0, 230, &trap230);
	set_interrupt_gate(0, 231, &trap231);
	set_interrupt_gate(0, 232, &trap232);
	set_interrupt_gate(0, 233, &trap233);
	set_interrupt_gate(0, 234, &trap234);
	set_interrupt_gate(0, 235, &trap235);
	set_interrupt_gate(0, 236, &trap236);
	set_interrupt_gate(0, 237, &trap237);
	set_interrupt_gate(0, 238, &trap238);
	set_interrupt_gate(0, 239, &trap239);
	set_interrupt_gate(0, 240, &trap240);
	set_interrupt_gate(0, 241, &trap241);
	set_interrupt_gate(0, 242, &trap242);
	set_interrupt_gate(0, 243, &trap243);
	set_interrupt_gate(0, 244, &trap244);
	set_interrupt_gate(0, 245, &trap245);
	set_interrupt_gate(0, 246, &trap246);
	set_interrupt_gate(0, 247, &trap247);
	set_interrupt_gate(0, 248, &trap248);
	set_interrupt_gate(0, 249, &trap249);
	set_interrupt_gate(0, 250, &trap250);

	// smp / apic local interrupts
	set_interrupt_gate(0, 251, &trap251);
	set_interrupt_gate(0, 252, &trap252);
	set_interrupt_gate(0, 253, &trap253);
	set_interrupt_gate(0, 254, &trap254);
	set_interrupt_gate(0, 255, &trap255);

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
	table[8] = x86_double_fault_exception; // Double Fault Exception (#DF)
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
	// Always init the local apic as it can be used for timers even if we
	// don't end up using the io apic
	apic_init(args);

	// We need to map in the I/O APIC here, since we would lose the already
	// wired mapping before arch_int_init_io() is called.
	ioapic_map(args);

	// create IDT area for the boot CPU
	area_id area = create_area("idt", (void**)&sIDTs[0], B_EXACT_ADDRESS,
		B_PAGE_SIZE, B_ALREADY_WIRED, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (area < 0)
		return area;

	// create IDTs for the off-boot CPU
	size_t idtSize = 256 * 8;
		// 256 8 bytes-sized descriptors
	int32 cpuCount = smp_get_num_cpus();
	if (cpuCount > 0) {
		size_t areaSize = ROUNDUP(cpuCount * idtSize, B_PAGE_SIZE);
		desc_table* idt;
		virtual_address_restrictions virtualRestrictions = {};
		virtualRestrictions.address_specification = B_ANY_KERNEL_ADDRESS;
		physical_address_restrictions physicalRestrictions = {};
		area = create_area_etc(B_SYSTEM_TEAM, "idt", areaSize, B_CONTIGUOUS,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, CREATE_AREA_DONT_WAIT,
			&virtualRestrictions, &physicalRestrictions, (void**)&idt);
		if (area < 0)
			return area;

		for (int32 i = 1; i < cpuCount; i++) {
			sIDTs[i] = idt;
			memcpy(idt, sIDTs[0], idtSize);
			idt += 256;
			// The CPU's IDTR will be set in arch_cpu_init_percpu().
		}
	}

	return area >= B_OK ? B_OK : area;
}


status_t
arch_int_init_io(kernel_args* args)
{
	ioapic_init(args);
	return B_OK;
}


status_t
arch_int_init_post_device_manager(struct kernel_args *args)
{
	return B_OK;
}
