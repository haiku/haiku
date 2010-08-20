/*
 * Copyright 2010, Clemens Zeidler, haiku@clemens-zeidler.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef IRQ_ROUTING_TABLE_H
#define IRQ_ROUTING_TABLE_H


#include <ACPI.h>
#include <PCI.h>


#include "util/Vector.h"


struct irq_routing_entry {
	int				device_address;
	int8			pin;

	acpi_handle		source;
	int				source_index;

	// pci busmanager connection
	uchar			pci_bus;
	uchar			pci_device;
};


typedef Vector<irq_routing_entry> IRQRoutingTable;


struct irq_descriptor {
	irq_descriptor();
	// bit 0 is interrupt 0, bit 2 is interrupt 2, and so on
	int16			irq;
	bool			shareable;
	// B_LOW_ACTIVE_POLARITY or B_HIGH_ACTIVE_POLARITY
	int8			polarity;
	// B_LEVEL_TRIGGERED or B_EDGE_TRIGGERED
	int8			interrupt_mode;
};


// Similar to bus_managers/acpi/include/acrestyp.h definition
typedef struct acpi_prt {
	uint32			length;
	uint32			pin;
	uint64			address;		// here for 64-bit alignment
	uint32			sourceIndex;
	char			source[4];		// pad to 64 bits so sizeof() works in
									// all cases
} acpi_pci_routing_table;

//TODO: Hack until we expose ACPI structs better, currently hardcoded to
// ACPI_RESOURCE_IRQ
struct acpi_resource {
    uint32          type;
    uint32			length;
    
    uint8			descriptorLength;
    uint8			triggering;
    uint8			polarity;
    uint8			sharable;
    uint8			interruptCount;
    uint8			interrupts[];
};


void print_irq_descriptor(irq_descriptor* descriptor);
void print_irq_routing_table(IRQRoutingTable* table);


status_t read_irq_routing_table(pci_module_info *pci, acpi_module_info* acpi,
			IRQRoutingTable* table);
status_t read_irq_descriptor(acpi_module_info* acpi, acpi_handle device,
			const char* method, irq_descriptor* descriptor);

status_t read_current_irq(acpi_module_info* acpi, acpi_handle device,
			irq_descriptor* descriptor);
status_t read_possible_irq(acpi_module_info* acpi, acpi_handle device,
			irq_descriptor* descriptor);

status_t set_acpi_irq(acpi_module_info* acpi, acpi_handle device,
			irq_descriptor* descriptor);

#endif	// IRQ_ROUTING_TABLE_H
