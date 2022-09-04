/*
 * Copyright 2010, Clemens Zeidler, haiku@clemens-zeidler.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef IRQ_ROUTING_TABLE_H
#define IRQ_ROUTING_TABLE_H


#include <ACPI.h>


#include "util/Vector.h"


struct irq_routing_entry {
	// ACPI specifics
	uint64		device_address;
	uint8		pin;

	acpi_handle	source;
	uint32		source_index;
	bool		needs_configuration;

	// PCI bus_manager connection
	uint8		pci_bus;
	uint8		pci_device;
	uint32		pci_function_mask;

	// Distilled configuration info
	uint8		irq;			// Global System Interrupt (GSI)
	uint8		bios_irq;		// BIOS assigned original IRQ
	uint8		polarity;		// B_{HIGH|LOW}_ACTIVE_POLARITY
	uint8		trigger_mode;	// B_{LEVEL|EDGE}_TRIGGERED
};


typedef Vector<irq_routing_entry> IRQRoutingTable;


struct irq_descriptor {
	irq_descriptor();

	uint8			irq;
	bool			shareable;
	// B_LOW_ACTIVE_POLARITY or B_HIGH_ACTIVE_POLARITY
	uint8			polarity;
	// B_LEVEL_TRIGGERED or B_EDGE_TRIGGERED
	uint8			trigger_mode;
};


typedef Vector<irq_descriptor> irq_descriptor_list;


struct pci_address {
	uint8	segment;
	uint8	bus;
	uint8	device;
	uint8	function;
};


struct link_device {
	acpi_handle					handle;
	irq_descriptor				current_irq;
	Vector<irq_descriptor>		possible_irqs;
	Vector<irq_routing_entry*>	used_by;
};


typedef bool (*interrupt_available_check_function)(int32 globalSystemInterrupt);


void print_irq_descriptor(const irq_descriptor& descriptor);
void print_irq_routing_table(const IRQRoutingTable& table);


status_t prepare_irq_routing(acpi_module_info* acpi, IRQRoutingTable& table,
			interrupt_available_check_function checkFunction);
status_t enable_irq_routing(acpi_module_info* acpi,
			IRQRoutingTable& routingTable);

status_t read_current_irq(acpi_module_info* acpi, acpi_handle device,
			irq_descriptor& descriptor);
status_t read_possible_irqs(acpi_module_info* acpi, acpi_handle device,
			irq_descriptor_list& descriptorList);

status_t set_current_irq(acpi_module_info* acpi, acpi_handle device,
			const irq_descriptor& descriptor);

#endif	// IRQ_ROUTING_TABLE_H
