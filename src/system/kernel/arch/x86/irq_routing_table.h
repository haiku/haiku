/*
 * Copyright 2010, Clemens Zeidler, haiku@clemens-zeidler.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef IRQ_ROUTING_TABLE_H
#define IRQ_ROUTING_TABLE_H


#include <ACPI.h>


#include "util/Vector.h"


struct irq_routing_entry {
	int			device_address;
	int8		pin;

	acpi_handle	source;
	int			source_index;

	// pci busmanager connection
	uchar		pci_bus;
	uchar		pci_device;
};


typedef Vector<irq_routing_entry> IRQRoutingTable;


struct irq_descriptor {
	irq_descriptor();
	// bit 0 is interrupt 0, bit 2 is interrupt 2, and so on
	uint8			irq;
	bool			shareable;
	// B_LOW_ACTIVE_POLARITY or B_HIGH_ACTIVE_POLARITY
	uint8			polarity;
	// B_LEVEL_TRIGGERED or B_EDGE_TRIGGERED
	uint8			interrupt_mode;
};


// TODO: Hack until we expose ACPI structs better; these are duplicates of
// the types in acrestype.h
struct acpi_pci_routing_table {
	uint32	length;
	uint32	pin;
	uint64	address;
	uint32	source_index;
	char	source[4];
};

struct acpi_resource {
	uint32	type;
	uint32	length;
};

struct acpi_resource_source {
	uint8	index;
	uint16	string_length;
	char*	string_pointer;
};

struct acpi_resource_irq {
	acpi_resource header;
	uint8	descriptor_ength;
	uint8	triggering;
	uint8	polarity;
	uint8	sharable;
	uint8	interrupt_count;
	uint8	interrupts[];
};

struct acpi_resource_extended_irq {
	acpi_resource header;
	uint8	producer_consumer;
	uint8	triggering;
	uint8	polarity;
	uint8	sharable;
	uint8	interrupt_count;
	acpi_resource_source source;
	uint32	interrupts[];
};


void print_irq_descriptor(irq_descriptor* descriptor);
void print_irq_routing_table(IRQRoutingTable* table);


status_t read_irq_routing_table(acpi_module_info* acpi, IRQRoutingTable* table);

status_t read_current_irq(acpi_module_info* acpi, acpi_handle device,
			irq_descriptor* descriptor);
status_t read_possible_irq(acpi_module_info* acpi, acpi_handle device,
			irq_descriptor* descriptor);

status_t set_acpi_irq(acpi_module_info* acpi, acpi_handle device,
			irq_descriptor* descriptor);

#endif	// IRQ_ROUTING_TABLE_H
