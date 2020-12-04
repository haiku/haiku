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
#include <arch/x86/arch_smp.h>
#include <arch/x86/pic.h>

// to gain access to the ACPICA types
#include "acpi.h"


//#define TRACE_IOAPIC
#ifdef TRACE_IOAPIC
#	define TRACE(...) dprintf(__VA_ARGS__)
#else
#	define TRACE(...) (void)0
#endif


// ACPI interrupt models
#define ACPI_INTERRUPT_MODEL_PIC	0
#define ACPI_INTERRUPT_MODEL_APIC	1
#define ACPI_INTERRUPT_MODEL_SAPIC	2


// Definitions for a 82093AA IO APIC controller
#define IO_APIC_ID							0x00
#define IO_APIC_VERSION						0x01
#define IO_APIC_ARBITRATION					0x02
#define IO_APIC_REDIRECTION_TABLE			0x10 // entry = base + 2 * index

// Fields for the id register
#define IO_APIC_ID_SHIFT					24
#define IO_APIC_ID_MASK						0xff

// Fields for the version register
#define IO_APIC_VERSION_SHIFT				0
#define IO_APIC_VERSION_MASK				0xff
#define IO_APIC_MAX_REDIRECTION_ENTRY_SHIFT	16
#define IO_APIC_MAX_REDIRECTION_ENTRY_MASK	0xff

// Fields of each redirection table entry
#define IO_APIC_DESTINATION_FIELD_SHIFT		56
#define IO_APIC_DESTINATION_FIELD_MASK		0xff
#define IO_APIC_INTERRUPT_MASKED			(1 << 16)
#define IO_APIC_TRIGGER_MODE_EDGE			(0 << 15)
#define IO_APIC_TRIGGER_MODE_LEVEL			(1 << 15)
#define IO_APIC_TRIGGER_MODE_MASK			(1 << 15)
#define IO_APIC_REMOTE_IRR					(1 << 14)
#define IO_APIC_PIN_POLARITY_HIGH_ACTIVE	(0 << 13)
#define IO_APIC_PIN_POLARITY_LOW_ACTIVE		(1 << 13)
#define IO_APIC_PIN_POLARITY_MASK			(1 << 13)
#define IO_APIC_DELIVERY_STATUS_PENDING		(1 << 12)
#define IO_APIC_DESTINATION_MODE_PHYSICAL	(0 << 11)
#define IO_APIC_DESTINATION_MODE_LOGICAL	(1 << 11)
#define IO_APIC_DESTINATION_MODE_MASK		(1 << 11)
#define IO_APIC_DELIVERY_MODE_MASK			(7 << 8)
#define IO_APIC_DELIVERY_MODE_FIXED			(0 << 8)
#define IO_APIC_DELIVERY_MODE_LOWEST_PRIO	(1 << 8)
#define IO_APIC_DELIVERY_MODE_SMI			(2 << 8)
#define IO_APIC_DELIVERY_MODE_NMI			(4 << 8)
#define IO_APIC_DELIVERY_MODE_INIT			(5 << 8)
#define IO_APIC_DELIVERY_MODE_EXT_INT		(7 << 8)
#define IO_APIC_INTERRUPT_VECTOR_SHIFT		0
#define IO_APIC_INTERRUPT_VECTOR_MASK		0xff

#define MAX_SUPPORTED_REDIRECTION_ENTRIES	64
#define ISA_INTERRUPT_COUNT					16


struct ioapic_registers {
	volatile uint32	io_register_select;
	uint32			reserved[3];
	volatile uint32	io_window_register;
};


struct ioapic {
	uint8				number;
	uint8				apic_id;
	uint32				version;
	uint8				max_redirection_entry;
	uint8				global_interrupt_base;
	uint8				global_interrupt_last;
	uint64				level_triggered_mask;
	uint64				nmi_mask;

	area_id				register_area;
	ioapic_registers*	registers;

	ioapic*				next;
};


static ioapic* sIOAPICs = NULL;
static int32 sSourceOverrides[ISA_INTERRUPT_COUNT];


// #pragma mark - I/O APIC


static void
print_ioapic(struct ioapic& ioapic)
{
	dprintf("io-apic %u has range %u-%u, %u entries, version 0x%08" B_PRIx32
		", apic-id %u\n", ioapic.number, ioapic.global_interrupt_base,
		ioapic.global_interrupt_last, ioapic.max_redirection_entry + 1,
		ioapic.version, ioapic.apic_id);
}


static inline struct ioapic*
find_ioapic(int32 gsi)
{
	if (gsi < 0)
		return NULL;

	struct ioapic* current = sIOAPICs;
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
ioapic_write_64(struct ioapic& ioapic, uint8 registerSelect, uint64 value,
	bool maskFirst)
{
	ioapic.registers->io_register_select
		= registerSelect + (maskFirst ? 0 : 1);
	ioapic.registers->io_window_register
		= (uint32)(value >> (maskFirst ? 0 : 32));
	ioapic.registers->io_register_select
		= registerSelect + (maskFirst ? 1 : 0);
	ioapic.registers->io_window_register
		= (uint32)(value >> (maskFirst ? 32 : 0));
}


static void
ioapic_configure_pin(struct ioapic& ioapic, uint8 pin, uint8 vector,
	uint8 triggerPolarity, uint16 deliveryMode)
{
	uint64 entry = ioapic_read_64(ioapic, IO_APIC_REDIRECTION_TABLE + pin * 2);
	entry &= ~(IO_APIC_TRIGGER_MODE_MASK | IO_APIC_PIN_POLARITY_MASK
		| IO_APIC_INTERRUPT_VECTOR_MASK | IO_APIC_DELIVERY_MODE_MASK);

	if (triggerPolarity & B_LEVEL_TRIGGERED) {
		entry |= IO_APIC_TRIGGER_MODE_LEVEL;
		ioapic.level_triggered_mask |= ((uint64)1 << pin);
	} else {
		entry |= IO_APIC_TRIGGER_MODE_EDGE;
		ioapic.level_triggered_mask &= ~((uint64)1 << pin);
	}

	if (triggerPolarity & B_LOW_ACTIVE_POLARITY)
		entry |= IO_APIC_PIN_POLARITY_LOW_ACTIVE;
	else
		entry |= IO_APIC_PIN_POLARITY_HIGH_ACTIVE;

	entry |= deliveryMode;
	entry |= (vector + ARCH_INTERRUPT_BASE) << IO_APIC_INTERRUPT_VECTOR_SHIFT;
	ioapic_write_64(ioapic, IO_APIC_REDIRECTION_TABLE + pin * 2, entry, true);
}


static bool
ioapic_is_spurious_interrupt(int32 gsi)
{
	// the spurious interrupt vector is initialized to the max value in smp
	return gsi == 0xff - ARCH_INTERRUPT_BASE;
}


static bool
ioapic_is_level_triggered_interrupt(int32 gsi)
{
	struct ioapic* ioapic = find_ioapic(gsi);
	if (ioapic == NULL)
		return false;

	uint8 pin = gsi - ioapic->global_interrupt_base;
	return (ioapic->level_triggered_mask & ((uint64)1 << pin)) != 0;
}


static bool
ioapic_end_of_interrupt(int32 num)
{
	apic_end_of_interrupt();
	return true;
}


static void
ioapic_assign_interrupt_to_cpu(int32 gsi, int32 cpu)
{
	if (gsi < ISA_INTERRUPT_COUNT && sSourceOverrides[gsi] != 0)
		gsi = sSourceOverrides[gsi];

	struct ioapic* ioapic = find_ioapic(gsi);
	if (ioapic == NULL)
		return;

	uint32 apicid = x86_get_cpu_apic_id(cpu);

	uint8 pin = gsi - ioapic->global_interrupt_base;
	TRACE("ioapic_assign_interrupt_to_cpu: gsi %" B_PRId32
		" (io-apic %u pin %u) to cpu %" B_PRId32 " (apic_id %" B_PRIx32 ")\n",
		gsi, ioapic->number, pin, cpu, apicid);

	uint64 entry = ioapic_read_64(*ioapic, IO_APIC_REDIRECTION_TABLE + pin * 2);
	entry &= ~(uint64(IO_APIC_DESTINATION_FIELD_MASK)
			<< IO_APIC_DESTINATION_FIELD_SHIFT);
	entry |= uint64(apicid) << IO_APIC_DESTINATION_FIELD_SHIFT;
	ioapic_write_64(*ioapic, IO_APIC_REDIRECTION_TABLE + pin * 2, entry, false);
}


static void
ioapic_enable_io_interrupt(int32 gsi)
{
	// If enabling an overriden source is attempted, enable the override entry
	// instead. An interrupt handler was installed at the override GSI to relay
	// interrupts to the overriden source.
	if (gsi < ISA_INTERRUPT_COUNT && sSourceOverrides[gsi] != 0)
		gsi = sSourceOverrides[gsi];

	struct ioapic* ioapic = find_ioapic(gsi);
	if (ioapic == NULL)
		return;

	x86_set_irq_source(gsi, IRQ_SOURCE_IOAPIC);

	uint8 pin = gsi - ioapic->global_interrupt_base;
	TRACE("ioapic_enable_io_interrupt: gsi %" B_PRId32
		" -> io-apic %u pin %u\n", gsi, ioapic->number, pin);

	uint64 entry = ioapic_read_64(*ioapic, IO_APIC_REDIRECTION_TABLE + pin * 2);
	entry &= ~IO_APIC_INTERRUPT_MASKED;
	ioapic_write_64(*ioapic, IO_APIC_REDIRECTION_TABLE + pin * 2, entry, false);
}


static void
ioapic_disable_io_interrupt(int32 gsi)
{
	struct ioapic* ioapic = find_ioapic(gsi);
	if (ioapic == NULL)
		return;

	uint8 pin = gsi - ioapic->global_interrupt_base;
	TRACE("ioapic_disable_io_interrupt: gsi %" B_PRId32
		" -> io-apic %u pin %u\n", gsi, ioapic->number, pin);

	uint64 entry = ioapic_read_64(*ioapic, IO_APIC_REDIRECTION_TABLE + pin * 2);
	entry |= IO_APIC_INTERRUPT_MASKED;
	ioapic_write_64(*ioapic, IO_APIC_REDIRECTION_TABLE + pin * 2, entry, true);
}


static void
ioapic_configure_io_interrupt(int32 gsi, uint32 config)
{
	struct ioapic* ioapic = find_ioapic(gsi);
	if (ioapic == NULL)
		return;

	uint8 pin = gsi - ioapic->global_interrupt_base;
	TRACE("ioapic_configure_io_interrupt: gsi %" B_PRId32
		" -> io-apic %u pin %u; config 0x%08" B_PRIx32 "\n", gsi,
		ioapic->number, pin, config);

	ioapic_configure_pin(*ioapic, pin, gsi, config,
		IO_APIC_DELIVERY_MODE_FIXED);
}


static status_t
ioapic_map_ioapic(struct ioapic& ioapic, phys_addr_t physicalAddress)
{
	ioapic.register_area = vm_map_physical_memory(B_SYSTEM_TEAM, "io-apic",
		(void**)&ioapic.registers, ioapic.registers != NULL ? B_EXACT_ADDRESS
		: B_ANY_KERNEL_ADDRESS, B_PAGE_SIZE, B_KERNEL_READ_AREA
		| B_KERNEL_WRITE_AREA, physicalAddress, ioapic.registers != NULL);
	if (ioapic.register_area < 0) {
		panic("mapping io-apic %u failed", ioapic.number);
		return ioapic.register_area;
	}

	TRACE("mapped io-apic %u to %p\n", ioapic.number, ioapic.registers);

	ioapic.version = ioapic_read_32(ioapic, IO_APIC_VERSION);
	if (ioapic.version == 0xffffffff) {
		dprintf("io-apic %u seems inaccessible, not using it\n",
			ioapic.number);
		vm_delete_area(B_SYSTEM_TEAM, ioapic.register_area, true);
		ioapic.register_area = -1;
		ioapic.registers = NULL;
		return B_ERROR;
	}

	ioapic.max_redirection_entry
		= ((ioapic.version >> IO_APIC_MAX_REDIRECTION_ENTRY_SHIFT)
			& IO_APIC_MAX_REDIRECTION_ENTRY_MASK);
	if (ioapic.max_redirection_entry >= MAX_SUPPORTED_REDIRECTION_ENTRIES) {
		dprintf("io-apic %u entry count exceeds max supported, only using the "
			"first %u entries", ioapic.number,
			(uint8)MAX_SUPPORTED_REDIRECTION_ENTRIES);
		ioapic.max_redirection_entry = MAX_SUPPORTED_REDIRECTION_ENTRIES - 1;
	}

	ioapic.global_interrupt_last
		= ioapic.global_interrupt_base + ioapic.max_redirection_entry;

	ioapic.nmi_mask = 0;

	return B_OK;
}


static status_t
ioapic_initialize_ioapic(struct ioapic& ioapic, uint8 targetAPIC)
{
	// program the APIC ID
	ioapic_write_32(ioapic, IO_APIC_ID, ioapic.apic_id << IO_APIC_ID_SHIFT);

	// program the interrupt vectors of the io-apic
	ioapic.level_triggered_mask = 0;
	uint8 gsi = ioapic.global_interrupt_base;
	for (uint8 i = 0; i <= ioapic.max_redirection_entry; i++, gsi++) {
		// initialize everything to deliver to the boot CPU in physical mode
		// and masked until explicitly enabled through enable_io_interrupt()
		uint64 entry = ((uint64)targetAPIC << IO_APIC_DESTINATION_FIELD_SHIFT)
			| IO_APIC_INTERRUPT_MASKED | IO_APIC_DESTINATION_MODE_PHYSICAL
			| ((gsi + ARCH_INTERRUPT_BASE) << IO_APIC_INTERRUPT_VECTOR_SHIFT);

		if (gsi == 0) {
			// make GSI 0 into an external interrupt
			entry |= IO_APIC_TRIGGER_MODE_EDGE
				| IO_APIC_PIN_POLARITY_HIGH_ACTIVE
				| IO_APIC_DELIVERY_MODE_EXT_INT;
		} else if (gsi < ISA_INTERRUPT_COUNT) {
			// identity map the legacy ISA interrupts
			entry |= IO_APIC_TRIGGER_MODE_EDGE
				| IO_APIC_PIN_POLARITY_HIGH_ACTIVE
				| IO_APIC_DELIVERY_MODE_FIXED;
		} else {
			// and the rest are PCI interrupts
			entry |= IO_APIC_TRIGGER_MODE_LEVEL
				| IO_APIC_PIN_POLARITY_LOW_ACTIVE
				| IO_APIC_DELIVERY_MODE_FIXED;
			ioapic.level_triggered_mask |= ((uint64)1 << i);
		}

		ioapic_write_64(ioapic, IO_APIC_REDIRECTION_TABLE + 2 * i, entry, true);
	}

	return B_OK;
}


static int32
ioapic_source_override_handler(void* data)
{
	int32 vector = (addr_t)data;
	bool levelTriggered = ioapic_is_level_triggered_interrupt(vector);
	return int_io_interrupt_handler(vector, levelTriggered);
}


static status_t
acpi_enumerate_ioapics(acpi_table_madt* madt)
{
	struct ioapic* lastIOAPIC = sIOAPICs;

	acpi_subtable_header* apicEntry
		= (acpi_subtable_header*)((uint8*)madt + sizeof(acpi_table_madt));
	void* end = ((uint8*)madt + madt->Header.Length);
	while (apicEntry < end) {
		switch (apicEntry->Type) {
			case ACPI_MADT_TYPE_IO_APIC:
			{
				acpi_madt_io_apic* info = (acpi_madt_io_apic*)apicEntry;
				dprintf("found io-apic with address 0x%08" B_PRIx32 ", global "
					"interrupt base %" B_PRIu32 ", apic-id %u\n",
					(uint32)info->Address, (uint32)info->GlobalIrqBase,
					info->Id);

				struct ioapic* ioapic
					= (struct ioapic*)malloc(sizeof(struct ioapic));
				if (ioapic == NULL) {
					dprintf("ran out of memory while allocating io-apic "
						"structure\n");
					return B_NO_MEMORY;
				}

				ioapic->number
					= lastIOAPIC != NULL ? lastIOAPIC->number + 1 : 0;
				ioapic->apic_id = info->Id;
				ioapic->global_interrupt_base = info->GlobalIrqBase;
				ioapic->registers = NULL;
				ioapic->next = NULL;

				dprintf("mapping io-apic %u at physical address %#" B_PRIx32
					"\n", ioapic->number, (uint32)info->Address);
				status_t status = ioapic_map_ioapic(*ioapic, info->Address);
				if (status != B_OK) {
					free(ioapic);
					break;
				}

				print_ioapic(*ioapic);

				if (lastIOAPIC == NULL)
					sIOAPICs = ioapic;
				else
					lastIOAPIC->next = ioapic;

				lastIOAPIC = ioapic;
				break;
			}

			case ACPI_MADT_TYPE_NMI_SOURCE:
			{
				acpi_madt_nmi_source* info
					= (acpi_madt_nmi_source*)apicEntry;
				dprintf("found nmi source global irq %" B_PRIu32 ", flags "
					"0x%04x\n", (uint32)info->GlobalIrq,
					(uint16)info->IntiFlags);

				struct ioapic* ioapic = find_ioapic(info->GlobalIrq);
				if (ioapic == NULL) {
					dprintf("nmi source for gsi that is not mapped to any "
						" io-apic\n");
					break;
				}

				uint8 pin = info->GlobalIrq - ioapic->global_interrupt_base;
				ioapic->nmi_mask |= (uint64)1 << pin;
				break;
			}
		}

		apicEntry
			= (acpi_subtable_header*)((uint8*)apicEntry + apicEntry->Length);
	}

	return B_OK;
}


static inline uint32
acpi_madt_convert_inti_flags(uint16 flags)
{
	uint32 config = 0;
	switch (flags & ACPI_MADT_POLARITY_MASK) {
		case ACPI_MADT_POLARITY_ACTIVE_LOW:
			config = B_LOW_ACTIVE_POLARITY;
			break;
		default:
			dprintf("invalid polarity in inti flags\n");
			// fall through and assume active high
		case ACPI_MADT_POLARITY_ACTIVE_HIGH:
		case ACPI_MADT_POLARITY_CONFORMS:
			config = B_HIGH_ACTIVE_POLARITY;
			break;
	}

	switch (flags & ACPI_MADT_TRIGGER_MASK) {
		case ACPI_MADT_TRIGGER_LEVEL:
			config |= B_LEVEL_TRIGGERED;
			break;
		default:
			dprintf("invalid trigger mode in inti flags\n");
			// fall through and assume edge triggered
		case ACPI_MADT_TRIGGER_CONFORMS:
		case ACPI_MADT_TRIGGER_EDGE:
			config |= B_EDGE_TRIGGERED;
			break;
	}

	return config;
}


static void
acpi_configure_source_overrides(acpi_table_madt* madt)
{
	acpi_subtable_header* apicEntry
		= (acpi_subtable_header*)((uint8*)madt + sizeof(acpi_table_madt));
	void* end = ((uint8*)madt + madt->Header.Length);
	while (apicEntry < end) {
		switch (apicEntry->Type) {
			case ACPI_MADT_TYPE_INTERRUPT_OVERRIDE:
			{
				acpi_madt_interrupt_override* info
					= (acpi_madt_interrupt_override*)apicEntry;
				dprintf("found interrupt override for bus %u, source irq %u, "
					"global irq %" B_PRIu32 ", flags 0x%08" B_PRIx32 "\n",
					info->Bus, info->SourceIrq, (uint32)info->GlobalIrq,
					(uint32)info->IntiFlags);

				if (info->SourceIrq >= ISA_INTERRUPT_COUNT) {
					dprintf("source override exceeds isa interrupt count\n");
					break;
				}

				if (info->SourceIrq != info->GlobalIrq) {
					// we need a vector mapping
					install_io_interrupt_handler(info->GlobalIrq,
						&ioapic_source_override_handler,
						(void*)(addr_t)info->SourceIrq, B_NO_ENABLE_COUNTER);

					sSourceOverrides[info->SourceIrq] = info->GlobalIrq;
				}

				// configure non-standard polarity/trigger modes
				uint32 config = acpi_madt_convert_inti_flags(info->IntiFlags);
				ioapic_configure_io_interrupt(info->GlobalIrq, config);
				break;
			}

			case ACPI_MADT_TYPE_NMI_SOURCE:
			{
				acpi_madt_nmi_source* info
					= (acpi_madt_nmi_source*)apicEntry;
				dprintf("found nmi source global irq %" B_PRIu32 ", flags "
					"0x%04x\n", (uint32)info->GlobalIrq,
					(uint16)info->IntiFlags);

				struct ioapic* ioapic = find_ioapic(info->GlobalIrq);
				if (ioapic == NULL)
					break;

				uint8 pin = info->GlobalIrq - ioapic->global_interrupt_base;
				uint32 config = acpi_madt_convert_inti_flags(info->IntiFlags);
				ioapic_configure_pin(*ioapic, pin, info->GlobalIrq, config,
					IO_APIC_DELIVERY_MODE_NMI);
				break;
			}

#ifdef TRACE_IOAPIC
			case ACPI_MADT_TYPE_LOCAL_APIC:
			{
				// purely informational
				acpi_madt_local_apic* info = (acpi_madt_local_apic*)apicEntry;
				dprintf("found local apic with id %u, processor id %u, "
					"flags 0x%08" B_PRIx32 "\n", info->Id, info->ProcessorId,
					(uint32)info->LapicFlags);
				break;
			}

			case ACPI_MADT_TYPE_LOCAL_APIC_NMI:
			{
				// TODO: take these into account, but at apic.cpp
				acpi_madt_local_apic_nmi* info
					= (acpi_madt_local_apic_nmi*)apicEntry;
				dprintf("found local apic nmi source for processor %u, "
					"flags 0x%04x, local int %u\n", info->ProcessorId,
					(uint16)info->IntiFlags, info->Lint);
				break;
			}

			case ACPI_MADT_TYPE_LOCAL_APIC_OVERRIDE:
			{
				// TODO: take these into account, but at apic.cpp
				acpi_madt_local_apic_override* info
					= (acpi_madt_local_apic_override*)apicEntry;
				dprintf("found local apic override with address 0x%016" B_PRIx64
					"\n", (uint64)info->Address);
				break;
			}

			default:
				dprintf("found unhandled subtable of type %u length %u\n",
					apicEntry->Type, apicEntry->Length);
				break;
#endif
		}

		apicEntry
			= (acpi_subtable_header*)((uint8*)apicEntry + apicEntry->Length);
	}
}


static status_t
acpi_set_interrupt_model(acpi_module_info* acpiModule, uint32 interruptModel)
{
	acpi_object_type model;
	model.object_type = ACPI_TYPE_INTEGER;
	model.integer.integer = interruptModel;

	acpi_objects parameter;
	parameter.count = 1;
	parameter.pointer = &model;

	dprintf("setting ACPI interrupt model to %s\n",
		interruptModel == ACPI_INTERRUPT_MODEL_PIC ? "PIC"
		: (interruptModel == ACPI_INTERRUPT_MODEL_APIC ? "APIC"
		: (interruptModel == ACPI_INTERRUPT_MODEL_SAPIC ? "SAPIC"
		: "unknown")));

	return acpiModule->evaluate_method(NULL, "\\_PIC", &parameter, NULL);
}


bool
ioapic_is_interrupt_available(int32 gsi)
{
	struct ioapic* ioapic = find_ioapic(gsi);
	if (ioapic == NULL)
		return false;

	uint8 pin = gsi - ioapic->global_interrupt_base;
	return (ioapic->nmi_mask & ((uint64)1 << pin)) == 0;
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
		&ioapic_end_of_interrupt,
		&ioapic_assign_interrupt_to_cpu,
	};

	if (args->arch_args.apic == NULL)
		return;

	if (args->arch_args.ioapic_phys == 0) {
		dprintf("no io-apics available, not using io-apics for interrupt "
			"routing\n");
		return;
	}

	if (get_safemode_boolean(B_SAFEMODE_DISABLE_IOAPIC, false)) {
		dprintf("io-apics explicitly disabled, not using io-apics for "
			"interrupt routing\n");
		return;
	}

	// load acpi module
	status_t status;
	acpi_module_info* acpiModule;
	status = get_module(B_ACPI_MODULE_NAME, (module_info**)&acpiModule);
	if (status != B_OK) {
		dprintf("acpi module not available, not configuring io-apics\n");
		return;
	}
	BPrivate::CObjectDeleter<const char, status_t, put_module>
		acpiModulePutter(B_ACPI_MODULE_NAME);

	acpi_table_madt* madt = NULL;
	if (acpiModule->get_table(ACPI_SIG_MADT, 0, (void**)&madt) != B_OK) {
		dprintf("failed to get MADT from ACPI, not configuring io-apics\n");
		return;
	}

	status = acpi_enumerate_ioapics(madt);
	if (status != B_OK) {
		// We don't treat this case as fatal just yet. If we are able to
		// route everything with the available IO-APICs we're fine, if not
		// we will fail at the routing preparation stage further down.
		dprintf("failed to enumerate all io-apics, working with what we got\n");
	}

	// switch to the APIC interrupt model before retrieving the IRQ routing
	// table as it will return different settings depending on the model
	status = acpi_set_interrupt_model(acpiModule, ACPI_INTERRUPT_MODEL_APIC);
	if (status != B_OK) {
		dprintf("failed to put ACPI into APIC interrupt model, ignoring\n");
		// don't abort, as the _PIC method is optional and as long as there
		// aren't different routings based on it this is non-fatal
	}

	IRQRoutingTable table;
	status = prepare_irq_routing(acpiModule, table,
		&ioapic_is_interrupt_available);
	if (status != B_OK) {
		dprintf("IRQ routing preparation failed, not configuring io-apics\n");
		acpi_set_interrupt_model(acpiModule, ACPI_INTERRUPT_MODEL_PIC);
			// revert to PIC interrupt model just in case
		return;
	}

	// use the boot CPU as the target for all interrupts
	uint8 targetAPIC = args->arch_args.cpu_apic_id[0];

	struct ioapic* current = sIOAPICs;
	while (current != NULL) {
		status = ioapic_initialize_ioapic(*current, targetAPIC);
		if (status != B_OK) {
			panic("failed to initialize io-apic %u", current->number);
			acpi_set_interrupt_model(acpiModule, ACPI_INTERRUPT_MODEL_PIC);
			return;
		}

		current = current->next;
	}

#ifdef TRACE_IOAPIC
	dprintf("trying interrupt routing:\n");
	print_irq_routing_table(table);
#endif

	status = enable_irq_routing(acpiModule, table);
	if (status != B_OK) {
		panic("failed to enable IRQ routing");
		// if it failed early on it might still work in PIC mode
		acpi_set_interrupt_model(acpiModule, ACPI_INTERRUPT_MODEL_PIC);
		return;
	}

	print_irq_routing_table(table);

	// configure the source overrides, but let the PCI config below override it
	acpi_configure_source_overrides(madt);

	// configure IO-APIC interrupts from PCI routing table
	for (int i = 0; i < table.Count(); i++) {
		irq_routing_entry& entry = table.ElementAt(i);
		ioapic_configure_io_interrupt(entry.irq,
			entry.polarity | entry.trigger_mode);
	}

	// kill the local ints on the local APIC
	apic_disable_local_ints();
		// TODO: This uses the assumption that our init is running on the
		// boot CPU and only the boot CPU has the local ints configured
		// because it was running in legacy PIC mode. Possibly the other
		// local APICs of the other CPUs have them configured as well. It
		// shouldn't really harm, but should eventually be corrected.

	// disable the legacy PIC
	uint16 legacyInterrupts;
	pic_disable(legacyInterrupts);

	// enable previsouly enabled legacy interrupts
	for (uint8 i = 0; i < 16; i++) {
		if ((legacyInterrupts & (1 << i)) != 0)
			ioapic_enable_io_interrupt(i);
	}

	// mark the interrupt vectors reserved so they aren't used for other stuff
	current = sIOAPICs;
	while (current != NULL) {
		reserve_io_interrupt_vectors(current->max_redirection_entry + 1,
			current->global_interrupt_base, INTERRUPT_TYPE_IRQ);

		for (int32 i = 0; i < current->max_redirection_entry + 1; i++) {
			x86_set_irq_source(current->global_interrupt_base + i,
				IRQ_SOURCE_IOAPIC);
		}

		current = current->next;
	}

	// prefer the ioapic over the normal pic
	dprintf("using io-apics for interrupt routing\n");
	arch_int_set_interrupt_controller(ioapicController);
}
