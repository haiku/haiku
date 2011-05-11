/*
 * Copyright 2008-2011, Michael Lotz, mmlr@mlotz.ch.
 * Distributed under the terms of the MIT License.
 */

#include <arch/x86/ioapic.h>

#include <int.h>
#include <vm/vm.h>

#include "irq_routing_table.h"

#include <ACPI.h>
#include <AutoDeleter.h>
#include <safemode.h>
#include <string.h>
#include <stdio.h>

#include <arch/x86/apic.h>
#include <arch/x86/arch_int.h>
#include <arch/x86/pic.h>


//#define TRACE_IOAPIC
#ifdef TRACE_IOAPIC
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


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

static uint64 sLevelTriggeredInterrupts = 0;
	// binary mask: 1 level, 0 edge


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
ioapic_is_level_triggered_interrupt(int32 pin)
{
	if (pin < 0 || pin > (int32)sIOAPICMaxRedirectionEntry)
		return false;

	return (sLevelTriggeredInterrupts & (1 << pin)) != 0;
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

	TRACE(("ioapic_enable_io_interrupt: pin %ld\n", pin));

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

	TRACE(("ioapic_disable_io_interrupt: pin %ld\n", pin));

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

	TRACE(("ioapic_configure_io_interrupt: pin %ld; config 0x%08lx\n", pin,
		config));

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


void
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

	// map in the (first) IO-APIC
	sIOAPIC = (ioapic *)args->arch_args.ioapic;
	if (vm_map_physical_memory(B_SYSTEM_TEAM, "ioapic", (void**)&sIOAPIC,
			B_EXACT_ADDRESS, B_PAGE_SIZE,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
			args->arch_args.ioapic_phys, true) < 0) {
		panic("mapping the ioapic failed");
		return;
	}
}


void
ioapic_init(kernel_args* args)
{
	static const interrupt_controller ioapicController = {
		"82093AA IOAPIC",
		&ioapic_enable_io_interrupt,
		&ioapic_disable_io_interrupt,
		&ioapic_configure_io_interrupt,
		&ioapic_is_spurious_interrupt,
		&ioapic_is_level_triggered_interrupt,
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
	arch_int_set_interrupt_controller(ioapicController);
}
