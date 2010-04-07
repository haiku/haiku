/*
 * Copyright 2009, Clemens Zeidler haiku@clemens-zeidler.de. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include "irq_routing_table.h"


#include <int.h>


//#define TRACE_PRT
#ifdef TRACE_PRT
#	define TRACE(x...) dprintf("IRQRoutingTable: "x)
#else
#	define TRACE(x...)
#endif


const int kIRQDescriptor = 0x04;


const char* kACPIPciRootName = "PNP0A03";


irq_discriptor::irq_discriptor()
	:
	irq(0),
	shareable(false),
	polarity(B_HIGH_ACTIVE_POLARITY),
	interrupt_mode(B_EDGE_TRIGGERED)
{

}


void
print_irq_discriptor(irq_discriptor* discriptor)
{
	for (int i = 0; i < 16; i++) {
		if (discriptor->irq >> i & 0x01)
			dprintf("interrupt: %i\n", i);
	}
	
	const char* activeHighString = "active high";
	const char* activeLowString = " active low";
	const char* levelTriggeredString = "level triggered";
	const char* edgeTriggeredString = " edge triggered";

	dprintf("shareable: %i, polarity: %s, interrupt_mode: %s\n",
		discriptor->shareable, discriptor->polarity == B_HIGH_ACTIVE_POLARITY
			? activeHighString : activeLowString,
		discriptor->interrupt_mode == B_LEVEL_TRIGGERED	? levelTriggeredString
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

		dprintf("pci_bus: %i\n", entry.pci_bus);
		dprintf("pci_device: %i\n", entry.pci_device);
	}
}


static status_t
find_pci_device(pci_module_info *pci, irq_routing_entry* entry)
{
	pci_info info;
	int pciN = 0;
	while ((*pci->get_nth_pci_info)(pciN, &info) == B_OK) {
		pciN ++;

		int16 deviceAddress = entry->device_address >> 16 & 0xFFFF;
		if (info.device == deviceAddress
			&& info.u.h0.interrupt_pin == entry->pin + 1) {
			entry->pci_bus = info.bus;
			entry->pci_device = info.device;
			return B_OK;
		}
	}

	return B_ERROR;
}


static status_t
read_device_irq_routing_table(pci_module_info *pci, acpi_module_info* acpi,
	acpi_handle device, IRQRoutingTable* table)
{
	acpi_data buffer;
	buffer.pointer = 0;
	buffer.length = ACPI_ALLOCATE_BUFFER;
	status_t status = acpi->get_irq_routing_table(device, &buffer);
	if (status != B_OK)
		return status;

	acpi_object_type* object = (acpi_object_type*)buffer.pointer;
	if (object->object_type != ACPI_TYPE_PACKAGE) {
		free(buffer.pointer);
		return B_ERROR;
	}

	irq_routing_entry irqEntry;
	acpi_pci_routing_table* acpiTable
		= (acpi_pci_routing_table*)buffer.pointer;
	while (acpiTable->length) {
		irqEntry.device_address = acpiTable->address;
		irqEntry.pin = acpiTable->pin;
		irqEntry.source = acpiTable->source;
		irqEntry.source_index = acpiTable->sourceIndex;

		// connect to pci device
		if (find_pci_device(pci, &irqEntry) != B_OK) {
			TRACE("no pci dev found, device %x, pin %i\n",
				irqEntry.device_address, irqEntry.pin);
			acpiTable = (acpi_pci_routing_table*)((uint8*)acpiTable
				+ acpiTable->length);
			continue;
		}

		table->PushBack(irqEntry);
		acpiTable = (acpi_pci_routing_table*)((uint8*)acpiTable
			+ acpiTable->length);
	}

	free(buffer.pointer);
	return B_OK;
}


status_t
read_irq_routing_table(pci_module_info *pci, acpi_module_info* acpi,
	IRQRoutingTable* table)
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
	status = read_device_irq_routing_table(pci, acpi, rootPciHandle, table);
	if (status != B_OK)
		return status;

	TRACE("find p2p \n");

	char result[255];
	result[0] = 0;
	void *counter = NULL;
	while (acpi->get_next_entry(ACPI_TYPE_DEVICE, rootPciName, result, 255,
		&counter) == B_OK) {
		acpi_handle brigde;
		status = acpi->get_handle(NULL, result, &brigde);
		if (status != B_OK)
			continue;

		status = read_device_irq_routing_table(pci, acpi, brigde, table);
		if (status == B_OK)
			TRACE("p2p found %s\n", result);
	}

	return status;
}


status_t
read_irq_discriptor(acpi_module_info* acpi, acpi_handle device,
	const char* method, irq_discriptor* discriptor)
{
	acpi_data buffer;
	buffer.pointer = NULL;
	buffer.length = ACPI_ALLOCATE_BUFFER;
	status_t status = acpi->evaluate_method(device, method, NULL,
		&buffer);
	if (status != B_OK) {
		free(buffer.pointer);
		return status;
	}

	acpi_object_type* resourceBuffer = (acpi_object_type*)buffer.pointer;
	if (resourceBuffer[0].object_type != ACPI_TYPE_BUFFER)
		return B_ERROR;

	int8* resourcePointer = (int8*)resourceBuffer[0].data.buffer.buffer;
	int8 integer = resourcePointer[0];
	if (integer >> 3 != kIRQDescriptor) {
		TRACE("resource is not a irq discriptor\n");
		return B_ERROR;
	}

	int8 size = resourcePointer[0] & 0x07;
	if (size < 2) {
		TRACE("invalid resource size\n");
		return B_ERROR;
	}

	discriptor->irq = resourcePointer[2];
	discriptor->irq = discriptor->irq << 8;
	discriptor->irq |= resourcePointer[1];

	// if size equal 2 we are don't else read the third entry
	if (size == 2)
		return B_OK;

	int irqInfo = resourcePointer[3];
	int bit;
	
	bit = irqInfo & 0x01;
	discriptor->interrupt_mode = (bit == 0) ? B_LEVEL_TRIGGERED
		: B_EDGE_TRIGGERED;
	bit = irqInfo >> 3 & 0x01;
	discriptor->polarity = (bit == 0) ? B_HIGH_ACTIVE_POLARITY
		: B_LOW_ACTIVE_POLARITY;
	discriptor->shareable = irqInfo >> 4 & 0x01;

	return B_OK;
}


status_t
read_current_irq(acpi_module_info* acpi, acpi_handle device,
	irq_discriptor* discriptor)
{
	return read_irq_discriptor(acpi, device, "_CRS", discriptor);
}


status_t
read_possible_irq(acpi_module_info* acpi, acpi_handle device,
	irq_discriptor* discriptor)
{
	return read_irq_discriptor(acpi, device, "_PRS", discriptor);
}


status_t
set_acpi_irq(acpi_module_info* acpi, acpi_handle device,
	irq_discriptor* discriptor)
{
	acpi_object_type outBuffer;
	outBuffer.object_type = ACPI_TYPE_BUFFER;

	int8 data[4];
	data[0] = 0x23;
	data[1] = discriptor->irq & 0xFF;
	data[2] = discriptor->irq >> 8;

	data[3] = 0;
	int8 bit;
	bit = (discriptor->interrupt_mode == B_HIGH_ACTIVE_POLARITY) ? 0 : 1;
	data[3] |= bit;
	bit = (discriptor->polarity == B_LEVEL_TRIGGERED) ? 0 : 1;
	data[3] |= bit << 3;
	bit = discriptor->shareable ? 1 : 0;
	data[3] |= bit << 4;

	outBuffer.data.buffer.length = sizeof(data);
	outBuffer.data.buffer.buffer = data;

	acpi_objects parameter;
	parameter.count = 1;
	parameter.pointer = &outBuffer;
	status_t status = acpi->evaluate_method(device, "_SRS", &parameter, NULL);

	return status;
}
