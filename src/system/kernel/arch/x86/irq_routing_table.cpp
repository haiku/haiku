/*
 * Copyright 2009, Clemens Zeidler haiku@clemens-zeidler.de.
 * All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include "irq_routing_table.h"


#include <int.h>


#define TRACE_PRT
#ifdef TRACE_PRT
#	define TRACE(x...) dprintf("IRQRoutingTable: "x)
#else
#	define TRACE(x...)
#endif


const int kIRQDescriptor = 0x04;


const char* kACPIPciRootName = "PNP0A03";


irq_descriptor::irq_descriptor()
	:
	irq(0),
	shareable(false),
	polarity(B_HIGH_ACTIVE_POLARITY),
	interrupt_mode(B_EDGE_TRIGGERED)
{
}


void
print_irq_descriptor(irq_descriptor* descriptor)
{
	for (int i = 0; i < 16; i++) {
		if (descriptor->irq >> i & 0x01)
			dprintf("interrupt: %i\n", i);
	}

	const char* activeHighString = "active high";
	const char* activeLowString = " active low";
	const char* levelTriggeredString = "level triggered";
	const char* edgeTriggeredString = " edge triggered";

	dprintf("irq: %u, shareable: %u, polarity: %s, interrupt_mode: %s\n",
		descriptor->irq, descriptor->shareable,
		descriptor->polarity == B_HIGH_ACTIVE_POLARITY ? activeHighString
			: activeLowString,
		descriptor->interrupt_mode == B_LEVEL_TRIGGERED	? levelTriggeredString
			: edgeTriggeredString);
}


void
print_irq_routing_table(IRQRoutingTable* table)
{
	dprintf("Print table %i entries\n", (int)table->Count());
	for (int i = 0; i < table->Count(); i++) {
		irq_routing_entry& entry = table->ElementAt(i);
		dprintf("address: %x\n", entry.device_address);
		dprintf("pin: %i\n", entry.pin);
		dprintf("source: %x\n", int(entry.source));
		dprintf("source index: %i\n", entry.source_index);
	}
}


static status_t
read_device_irq_routing_table(acpi_module_info* acpi, acpi_handle device,
	IRQRoutingTable* table)
{
	acpi_data buffer;
	buffer.pointer = NULL;
	buffer.length = ACPI_ALLOCATE_BUFFER;
	status_t status = acpi->get_irq_routing_table(device, &buffer);
	if (status != B_OK)
		return status;

	irq_routing_entry irqEntry;
	acpi_pci_routing_table* acpiTable = (acpi_pci_routing_table*)buffer.pointer;
	while (acpiTable->length) {
		acpi_handle source;
		bool noSource = acpiTable->source == NULL || acpiTable->source[0] == 0;
		if (noSource
			|| acpi->get_handle(NULL, acpiTable->source, &source) == B_OK) {

			irqEntry.device_address = acpiTable->address;
			irqEntry.pin = acpiTable->pin;
			irqEntry.source = noSource ? 0 : source;
			irqEntry.source_index = acpiTable->source_index;
			table->PushBack(irqEntry);
		}

		acpiTable = (acpi_pci_routing_table*)((uint8*)acpiTable
			+ acpiTable->length);
	}

	free(buffer.pointer);
	return B_OK;
}


status_t
read_irq_routing_table(acpi_module_info* acpi, IRQRoutingTable* table)
{
	char rootPciName[255];
	acpi_handle rootPciHandle;
	rootPciName[0] = 0;
	status_t status = acpi->get_device(kACPIPciRootName, 0, rootPciName, 255);
	if (status != B_OK)
		return status;

	status = acpi->get_handle(NULL, rootPciName, &rootPciHandle);
	if (status != B_OK)
		return status;
	TRACE("Read root pci bus irq rooting table\n");
	status = read_device_irq_routing_table(acpi, rootPciHandle, table);
	if (status != B_OK)
		return status;

	TRACE("find routing tables \n");

	char name[255];
	name[0] = 0;
	void *counter = NULL;
	while (acpi->get_next_entry(ACPI_TYPE_DEVICE, rootPciName, name,
		sizeof(name), &counter) == B_OK) {
		acpi_handle brigde;
		status = acpi->get_handle(NULL, name, &brigde);
		if (status != B_OK)
			continue;

		status = read_device_irq_routing_table(acpi, brigde, table);
		if (status == B_OK)
			TRACE("routing table found %s\n", name);
	}

	return table->Count() > 0 ? B_OK : B_ERROR;
}


static status_t
read_irq_descriptor(acpi_module_info* acpi, acpi_handle device,
	bool readCurrent, irq_descriptor* descriptor)
{
	acpi_data buffer;
	buffer.pointer = NULL;
	buffer.length = ACPI_ALLOCATE_BUFFER;

	status_t status;
	if (readCurrent)
		status = acpi->get_current_resources(device, &buffer);
	else
		status = acpi->get_possible_resources(device, &buffer);

	if (status != B_OK) {
		dprintf("failed to read %s resources for irq\n",
			readCurrent ? "current" : "possible");
		free(buffer.pointer);
		return status;
	}

	descriptor->irq = 0;
	acpi_resource* resource = (acpi_resource*)buffer.pointer;
	// TODO: Don't hardcode END TAG
	while (resource->type != 7) {
		// TODO: Don't hardcode IRQ and Extended IRQ
		switch (resource->type) {
			case 0: // IRQ
			{
				acpi_resource_irq* irq = (acpi_resource_irq*)resource;
				if (irq->interrupt_count < 1) {
					dprintf("acpi irq resource with no interrupts\n");
					break;
				}

				descriptor->irq = irq->interrupts[0];
				descriptor->shareable = irq->sharable != 0;
				descriptor->interrupt_mode = irq->triggering == 0
					? B_LEVEL_TRIGGERED : B_EDGE_TRIGGERED;
				descriptor->polarity = irq->polarity == 0
					? B_HIGH_ACTIVE_POLARITY : B_LOW_ACTIVE_POLARITY;

#ifdef TRACE_PRT
				dprintf("acpi irq resource (%s):\n",
					readCurrent ? "current" : "possible");
				dprintf("\ttriggering: %s\n",
					irq->triggering == 0 ? "level" : "edge");
				dprintf("\tpolarity: %s active\n",
					irq->polarity == 0 ? "high" : "low");
				dprintf("\tsharable: %s\n", irq->sharable != 0 ? "yes" : "no");
				dprintf("\tcount: %u\n", irq->interrupt_count);
				if (irq->interrupt_count > 0) {
					dprintf("\tinterrupts:");
					for (uint16 i = 0; i < irq->interrupt_count; i++)
						dprintf(" %u", irq->interrupts[i]);
					dprintf("\n");
				}
#endif
				break;
			}

			case 15: // Extended IRQ
			{
				acpi_resource_extended_irq* irq
					= (acpi_resource_extended_irq*)resource;
				if (irq->interrupt_count < 1) {
					dprintf("acpi extended irq resource with no interrupts\n");
					break;
				}

				descriptor->irq = irq->interrupts[0];
				descriptor->shareable = irq->sharable != 0;
				descriptor->interrupt_mode = irq->triggering == 0
					? B_LEVEL_TRIGGERED : B_EDGE_TRIGGERED;
				descriptor->polarity = irq->polarity == 0
					? B_HIGH_ACTIVE_POLARITY : B_LOW_ACTIVE_POLARITY;

#ifdef TRACE_PRT
				dprintf("acpi extended irq resource (%s):\n",
					readCurrent ? "current" : "possible");
				dprintf("\tproducer: %s\n",
					irq->producer_consumer ? "yes" : "no");
				dprintf("\ttriggering: %s\n",
					irq->triggering == 0 ? "level" : "edge");
				dprintf("\tpolarity: %s active\n",
					irq->polarity == 0 ? "high" : "low");
				dprintf("\tsharable: %s\n", irq->sharable != 0 ? "yes" : "no");
				dprintf("\tcount: %u\n", irq->interrupt_count);
				if (irq->interrupt_count > 0) {
					dprintf("\tinterrupts:");
					for (uint16 i = 0; i < irq->interrupt_count; i++)
						dprintf(" %lu", irq->interrupts[i]);
					dprintf("\n");
				}
#endif
				break;
			}
		}

		if (descriptor->irq != 0)
			break;

		resource = (acpi_resource*)((uint8*)resource + resource->length);
	}

	free(buffer.pointer);
	return descriptor->irq != 0 ? B_OK : B_ERROR;
}


status_t
read_current_irq(acpi_module_info* acpi, acpi_handle device,
	irq_descriptor* descriptor)
{
	return read_irq_descriptor(acpi, device, true, descriptor);
}


status_t
read_possible_irq(acpi_module_info* acpi, acpi_handle device,
	irq_descriptor* descriptor)
{
	return read_irq_descriptor(acpi, device, false, descriptor);
}


status_t
set_acpi_irq(acpi_module_info* acpi, acpi_handle device,
	irq_descriptor* descriptor)
{
	acpi_object_type outBuffer;
	outBuffer.object_type = ACPI_TYPE_BUFFER;

	int8 data[4];
	data[0] = 0x23;
	data[1] = descriptor->irq & 0xFF;
	data[2] = descriptor->irq >> 8;

	data[3] = 0;
	int8 bit;
	bit = (descriptor->interrupt_mode == B_HIGH_ACTIVE_POLARITY) ? 0 : 1;
	data[3] |= bit;
	bit = (descriptor->polarity == B_LEVEL_TRIGGERED) ? 0 : 1;
	data[3] |= bit << 3;
	bit = descriptor->shareable ? 1 : 0;
	data[3] |= bit << 4;

	outBuffer.data.buffer.length = sizeof(data);
	outBuffer.data.buffer.buffer = data;

	acpi_objects parameter;
	parameter.count = 1;
	parameter.pointer = &outBuffer;
	status_t status = acpi->evaluate_method(device, "_SRS", &parameter, NULL);

	return status;
}
