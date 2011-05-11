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


struct ioapic_registers {
	volatile uint32	io_register_select;
	uint32			reserved[3];
	volatile uint32	io_window_register;
};


struct ioapic {
	uint8				number;
	uint8				max_redirection_entry;
	uint8				global_interrupt_base;
	uint8				global_interrupt_last;
	uint64				level_triggered_mask;

	area_id				register_area;
	ioapic_registers*	registers;

	ioapic*				next;
};


static ioapic sIOAPICs = { 0, 0, 0, 0, 0, -1, NULL, NULL };


// #pragma mark - I/O APIC


static inline struct ioapic*
find_ioapic(int32 gsi)
{
	if (gsi < 0)
		return NULL;

	struct ioapic* current = &sIOAPICs;
	while (current != NULL) {
		if (gsi >= current->global_interrupt_base
			&& gsi <= current->global_interrupt_last) {
			return current;
		}

		current = current->next;
	}

	return NULL;
}


static inline uint32
ioapic_read_32(struct ioapic& ioapic, uint8 registerSelect)
{
	ioapic.registers->io_register_select = registerSelect;
	return ioapic.registers->io_window_register;
}


static inline void
ioapic_write_32(struct ioapic& ioapic, uint8 registerSelect, uint32 value)
{
	ioapic.registers->io_register_select = registerSelect;
	ioapic.registers->io_window_register = value;
}


static inline uint64
ioapic_read_64(struct ioapic& ioapic, uint8 registerSelect)
{
	ioapic.registers->io_register_select = registerSelect + 1;
	uint64 result = ioapic.registers->io_window_register;
	result <<= 32;
	ioapic.registers->io_register_select = registerSelect;
	result |= ioapic.registers->io_window_register;
	return result;
}


static inline void
ioapic_write_64(struct ioapic& ioapic, uint8 registerSelect, uint64 value)
{
	ioapic.registers->io_register_select = registerSelect;
	ioapic.registers->io_window_register = (uint32)value;
	ioapic.registers->io_register_select = registerSelect + 1;
	ioapic.registers->io_window_register = (uint32)(value >> 32);
}


static bool
ioapic_is_spurious_interrupt(int32 num)
{
	// the spurious interrupt vector is initialized to the max value in smp
	return num == 0xff - ARCH_INTERRUPT_BASE;
}


static bool
ioapic_is_level_triggered_interrupt(int32 gsi)
{
	struct ioapic* ioapic = find_ioapic(gsi);
	if (ioapic == NULL)
		return false;

	uint8 pin = gsi - ioapic->global_interrupt_base;
	return (ioapic->level_triggered_mask & (1 << pin)) != 0;
}


static bool
ioapic_end_of_interrupt(int32 num)
{
	apic_end_of_interrupt();
	return true;
}


static void
ioapic_enable_io_interrupt(int32 gsi)
{
	struct ioapic* ioapic = find_ioapic(gsi);
	if (ioapic == NULL)
		return;

	uint8 pin = gsi - ioapic->global_interrupt_base;
	TRACE(("ioapic_enable_io_interrupt: gsi %ld -> io-apic %u pin %u\n",
		gsi, ioapic->number, pin));

	uint64 entry = ioapic_read_64(*ioapic, IO_APIC_REDIRECTION_TABLE + pin * 2);
	entry &= ~(1 << IO_APIC_INTERRUPT_MASK_SHIFT);
	entry |= IO_APIC_INTERRUPT_UNMASKED << IO_APIC_INTERRUPT_MASK_SHIFT;
	ioapic_write_64(*ioapic, IO_APIC_REDIRECTION_TABLE + pin * 2, entry);
		// TODO: Take writing order into account! We must not unmask the entry
		// before the other half is valid.
}


static void
ioapic_disable_io_interrupt(int32 gsi)
{
	struct ioapic* ioapic = find_ioapic(gsi);
	if (ioapic == NULL)
		return;

	uint8 pin = gsi - ioapic->global_interrupt_base;
	TRACE(("ioapic_disable_io_interrupt: gsi %ld -> io-apic %u pin %u\n",
		gsi, ioapic->number, pin));

	uint64 entry = ioapic_read_64(*ioapic, IO_APIC_REDIRECTION_TABLE + pin * 2);
	entry &= ~(1 << IO_APIC_INTERRUPT_MASK_SHIFT);
	entry |= IO_APIC_INTERRUPT_MASKED << IO_APIC_INTERRUPT_MASK_SHIFT;
	ioapic_write_64(*ioapic, IO_APIC_REDIRECTION_TABLE + pin * 2, entry);
		// TODO: Take writing order into account! We must not modify the entry
		// before it is masked.
}


static void
ioapic_configure_io_interrupt(int32 gsi, uint32 config)
{
	struct ioapic* ioapic = find_ioapic(gsi);
	if (ioapic == NULL)
		return;

	uint8 pin = gsi - ioapic->global_interrupt_base;
	TRACE(("ioapic_configure_io_interrupt: gsi %ld -> io-apic %u pin %u; "
		"config 0x%08lx\n", gsi, ioapic->number, pin, config));

	uint64 entry = ioapic_read_64(*ioapic, IO_APIC_REDIRECTION_TABLE + pin * 2);
	entry &= ~((1 << IO_APIC_TRIGGER_MODE_SHIFT)
		| (1 << IO_APIC_PIN_POLARITY_SHIFT)
		| (IO_APIC_INTERRUPT_VECTOR_MASK << IO_APIC_INTERRUPT_VECTOR_SHIFT));

	if (config & B_LEVEL_TRIGGERED) {
		entry |= (IO_APIC_TRIGGER_MODE_LEVEL << IO_APIC_TRIGGER_MODE_SHIFT);
		ioapic->level_triggered_mask |= (1 << pin);
	} else {
		entry |= (IO_APIC_TRIGGER_MODE_EDGE << IO_APIC_TRIGGER_MODE_SHIFT);
		ioapic->level_triggered_mask &= ~(1 << pin);
	}

	if (config & B_LOW_ACTIVE_POLARITY)
		entry |= (IO_APIC_PIN_POLARITY_LOW_ACTIVE << IO_APIC_PIN_POLARITY_SHIFT);
	else
		entry |= (IO_APIC_PIN_POLARITY_HIGH_ACTIVE << IO_APIC_PIN_POLARITY_SHIFT);

	entry |= (pin + ARCH_INTERRUPT_BASE) << IO_APIC_INTERRUPT_VECTOR_SHIFT;
	ioapic_write_64(*ioapic, IO_APIC_REDIRECTION_TABLE + pin * 2, entry);
}


static status_t
ioapic_map_ioapic(struct ioapic& ioapic, phys_addr_t physicalAddress)
{
	ioapic.register_area = vm_map_physical_memory(B_SYSTEM_TEAM, "io-apic",
		(void**)&ioapic.registers, ioapic.registers != NULL ? B_EXACT_ADDRESS
		: B_ANY_KERNEL_ADDRESS, B_PAGE_SIZE, B_KERNEL_READ_AREA
		| B_KERNEL_WRITE_AREA, physicalAddress, true);
	if (ioapic.register_area < 0) {
		panic("mapping io-apic %u failed", ioapic.number);
		return ioapic.register_area;
	}

	uint32 version = ioapic_read_32(ioapic, IO_APIC_VERSION);
	if (version == 0xffffffff) {
		dprintf("io-apic %u seems inaccessible, not using it\n",
			ioapic.number);
		vm_delete_area(B_SYSTEM_TEAM, ioapic.register_area, true);
		ioapic.register_area = -1;
		ioapic.registers = NULL;
		return B_ERROR;
	}

	ioapic.max_redirection_entry
		= ((version >> IO_APIC_MAX_REDIRECTION_ENTRY_SHIFT)
			& IO_APIC_MAX_REDIRECTION_ENTRY_MASK);

	ioapic.global_interrupt_last
		= ioapic.global_interrupt_base + ioapic.max_redirection_entry;

	dprintf("io-apic %u has range %u-%u, %u entries\n", ioapic.number,
		ioapic.global_interrupt_base, ioapic.global_interrupt_last,
		ioapic.max_redirection_entry + 1);

	return B_OK;
}


static status_t
ioapic_initialize_ioapic(struct ioapic& ioapic, uint64 targetAPIC)
{
	// program the interrupt vectors of the io-apic
	ioapic.level_triggered_mask = 0;
	for (uint8 i = 0; i <= ioapic.max_redirection_entry; i++) {
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
			ioapic.level_triggered_mask |= (1 << i);
		}

		ioapic_write_64(ioapic, IO_APIC_REDIRECTION_TABLE + 2 * i, entry);
	}

	return B_OK;
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
		dprintf("no io-apic available, not using io-apics for interrupt "
			"routing\n");
		return;
	}

	// map in the first IO-APIC
	sIOAPICs.registers = (ioapic_registers*)args->arch_args.ioapic;
	ioapic_map_ioapic(sIOAPICs, args->arch_args.ioapic_phys);
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

	if (sIOAPICs.register_area < 0 || sIOAPICs.registers == NULL)
		return;

#if 0
	if (get_safemode_boolean(B_SAFEMODE_DISABLE_IOAPIC, false)) {
		dprintf("io-apics explicitly disabled, not using io-apics for "
			"interrupt routing\n");
		return;
	}
#else
	// TODO: This can be removed once IO-APIC code is broadly tested
	if (!get_safemode_boolean(B_SAFEMODE_ENABLE_IOAPIC, false)) {
		dprintf("io-apics not enabled, not using io-apics for interrupt "
			"routing\n");
		return;
	}
#endif

	// load acpi module
	status_t status;
	acpi_module_info* acpiModule;
	status = get_module(B_ACPI_MODULE_NAME, (module_info**)&acpiModule);
	if (status != B_OK) {
		dprintf("acpi module not available, not configuring io-apics\n");
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

	// TODO: read out all IO-APICs from ACPI and set them up

	IRQRoutingTable table;
	status = prepare_irq_routing(acpiModule, table,
		sIOAPICs.max_redirection_entry + 1);
	if (status != B_OK) {
		dprintf("IRQ routing preparation failed, not configuring io-apics\n");
		acpi_set_interrupt_model(acpiModule, ACPI_INTERRUPT_MODEL_PIC);
			// revert to PIC interrupt model just in case
		return;
	}

	print_irq_routing_table(table);

	// use the boot CPU as the target for all interrupts
	uint64 targetAPIC = args->arch_args.cpu_apic_id[0];

	struct ioapic* current = &sIOAPICs;
	while (current != NULL) {
		status = ioapic_initialize_ioapic(*current, targetAPIC);
		if (status != B_OK) {
			panic("failed to initialize io-apic %u", current->number);
			acpi_set_interrupt_model(acpiModule, ACPI_INTERRUPT_MODEL_PIC);
			return;
		}

		current = current->next;
	}

	status = enable_irq_routing(acpiModule, table);
	if (status != B_OK) {
		panic("failed to enable IRQ routing");
		// if it failed early on it might still work in PIC mode
		acpi_set_interrupt_model(acpiModule, ACPI_INTERRUPT_MODEL_PIC);
		return;
	}

	// configure IO-APIC interrupts from PCI routing table
	for (int i = 0; i < table.Count(); i++) {
		irq_routing_entry& entry = table.ElementAt(i);
		ioapic_configure_io_interrupt(entry.irq,
			entry.polarity | entry.trigger_mode);
	}

	// disable the legacy PIC
	pic_disable();

	// prefer the ioapic over the normal pic
	dprintf("using io-apics for interrupt routing\n");
	arch_int_set_interrupt_controller(ioapicController);
}
