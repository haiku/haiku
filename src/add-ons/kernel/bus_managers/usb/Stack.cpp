/*
 * Copyright 2003-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels S. Reedijk
 */

#include <module.h>
#include <util/kernel_cpp.h>
#include "usb_p.h"


#define TRACE_STACK
#ifdef TRACE_STACK
#define TRACE(x)	dprintf x
#else
#define TRACE(x)	/* nothing */
#endif


Stack::Stack()
{
	TRACE(("usb stack: stack init\n"));

	// Create the master lock
	fMasterLock = create_sem(1, "usb master lock");
	set_sem_owner(fMasterLock, B_SYSTEM_TEAM);

	// Create the data lock
	fDataLock = create_sem(1, "usb data lock");
	set_sem_owner(fDataLock, B_SYSTEM_TEAM);

	// Set the global "data" variable to this
	usb_stack = this;

	// Initialise the memory chunks: create 8, 16 and 32 byte-heaps
	// NOTE: This is probably the most ugly code you will see in the
	// whole stack. Unfortunately this is needed because of the fact
	// that the compiler doesn't like us to apply pointer arithmethic
	// to (void *) pointers. 

	// 8-byte heap
	fAreaFreeCount[0] = 0;
	fAreas[0] = AllocateArea(&fLogical[0], &fPhysical[0], B_PAGE_SIZE,
		"8-byte chunk area");

	if (fAreas[0] < B_OK) {
		TRACE(("usb stack: 8-byte chunk area failed to initialise\n"));
		return;
	}

	fListhead8 = (addr_t)fLogical[0];
	for (int32 i = 0; i < B_PAGE_SIZE / 8; i++) {
		memory_chunk *chunk = (memory_chunk *)((addr_t)fLogical[0] + 8 * i);
		chunk->physical = (addr_t)fPhysical[0] + 8 * i;

		if (i < B_PAGE_SIZE / 8 - 1)
			chunk->next_item = (addr_t)fLogical[0] + 8 * (i + 1);
		else
			chunk->next_item = NULL;
	}

	// 16-byte heap
	fAreaFreeCount[1] = 0;
	fAreas[1] = AllocateArea(&fLogical[1], &fPhysical[1], B_PAGE_SIZE,
		"16-byte chunk area");

	if (fAreas[1] < B_OK) {
		TRACE(("usb stack: 16-byte chunk area failed to initialise\n"));
		return;
	}

	fListhead16 = (addr_t)fLogical[1];
	for (int32 i = 0; i < B_PAGE_SIZE / 16; i++) {
		memory_chunk *chunk = (memory_chunk *)((addr_t)fLogical[1] + 16 * i);
		chunk->physical = (addr_t)fPhysical[1] + 16 * i;

		if (i < B_PAGE_SIZE / 16 - 1)
			chunk->next_item = (addr_t)fLogical[1] + 16 * (i + 1);
		else
			chunk->next_item = NULL;
	}

	// 32-byte heap
	fAreaFreeCount[2] = 0;
	fAreas[2] = AllocateArea(&fLogical[2], &fPhysical[2], B_PAGE_SIZE,
		"32-byte chunk area");

	if (fAreas[2] < B_OK) {
		TRACE(("usb stack: 32-byte chunk area failed to initialise\n"));
		return;
	}

	fListhead32 = (addr_t)fLogical[2];
	for (int32 i = 0; i < B_PAGE_SIZE / 32; i++) {
		memory_chunk *chunk = (memory_chunk *)((addr_t)fLogical[2] + 32 * i);
		chunk->physical = (addr_t)fPhysical[2] + 32 * i;

		if (i < B_PAGE_SIZE / 32 - 1)
			chunk->next_item = (addr_t)fLogical[2] + 32 * (i + 1);
		else
			chunk->next_item = NULL;
	}

	// Check for host controller modules
	void *moduleList = open_module_list("busses/usb");
	char moduleName[B_PATH_NAME_LENGTH];
	size_t bufferSize = sizeof(moduleName);

	TRACE(("usb stack: Looking for host controller modules\n"));
	while(read_next_module_name(moduleList, moduleName, &bufferSize) == B_OK) {
		bufferSize = sizeof(moduleName);
		TRACE(("usb stack: Found module %s\n", moduleName));

		host_controller_info *module = NULL;
		if (get_module(moduleName, (module_info **)&module) != B_OK)
			continue;

		if (module->add_to(*this) != B_OK)
			continue;

		TRACE(("usb stack: module %s successfully loaded\n", moduleName));
	}

	if (fBusManagers.Count() == 0)
		return;
}


Stack::~Stack()
{
	//Release the bus modules
	for (Vector<BusManager *>::Iterator i = fBusManagers.Begin();
		i != fBusManagers.End(); i++) {
		delete (*i);
	}

	delete_area(fAreas[0]);
	delete_area(fAreas[1]);
	delete_area(fAreas[2]);
}	


status_t
Stack::InitCheck()
{
	if (fBusManagers.Count() == 0)
		return ENODEV;

	return B_OK;
}


void
Stack::Lock()
{
	acquire_sem(fMasterLock);
}


void
Stack::Unlock()
{
	release_sem(fMasterLock);
}


void
Stack::AddBusManager(BusManager *busManager)
{
	fBusManagers.PushBack(busManager);
}


status_t
Stack::AllocateChunk(void **logicalAddress, void **physicalAddress, uint8 size)
{
	Lock();

	addr_t listhead;
	if (size <= 8)
		listhead = fListhead8;
	else if (size <= 16)
		listhead = fListhead16;
	else if (size <= 32)
		listhead = fListhead32;
	else {
		TRACE(("usb stack: Chunk size %d to big\n", size));
		Unlock();
		return B_ERROR;
	}

	if (listhead == NULL) {
		TRACE(("usb stack: Out of memory on this list\n"));
		Unlock();
		return B_ERROR;
	}

	//TRACE(("usb stack: Stack::Allocate() listhead: 0x%08x\n", listhead));
	memory_chunk *chunk = (memory_chunk *)listhead;
	*logicalAddress = (void *)listhead;
	*physicalAddress = (void *)chunk->physical;
	if (chunk->next_item == NULL) {
		//TODO: allocate more memory
		listhead = NULL;
	} else {
		listhead = chunk->next_item;
	}

	// Update our listhead pointers
	if (size <= 8)
		fListhead8 = listhead;
	else if (size <= 16)
		fListhead16 = listhead;
	else if (size <= 32)
		fListhead32 = listhead;

	Unlock();
	//TRACE(("usb stack: allocated a new chunk with size %u\n", size));
	return B_OK;
}


status_t
Stack::FreeChunk(void *logicalAddress, void *physicalAddress, uint8 size)
{	
	Lock();

	addr_t listhead;
	if (size <= 8)
		listhead = fListhead8;
	else if (size <= 16)
		listhead = fListhead16;
	else if (size <= 32)
		listhead = fListhead32;
	else {
		TRACE(("usb stack: Chunk size %d invalid\n", size));
		Unlock();
		return B_ERROR;
	}

	memory_chunk *chunk = (memory_chunk *)logicalAddress;
	chunk->next_item = listhead;
	chunk->physical = (addr_t)physicalAddress;

	if (size <= 8)
		fListhead8 = (addr_t)logicalAddress;
	else if (size <= 16)
		fListhead16 = (addr_t)logicalAddress;
	else if (size <= 32)
		fListhead32 = (addr_t)logicalAddress;

	Unlock();
	return B_OK;
}


area_id
Stack::AllocateArea(void **logicalAddress, void **physicalAddress, size_t size,
	const char *name)
{
	TRACE(("usb stack: allocating %ld bytes for %s\n", size, name));

	void *logAddress;
	size = (size + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);
	area_id area = create_area(name, &logAddress, B_ANY_KERNEL_ADDRESS, size,
		B_FULL_LOCK | B_CONTIGUOUS, 0);

	if (area < B_OK) {
		TRACE(("usb stack: couldn't allocate area %s\n", name));
		return B_ERROR;
	}

	physical_entry physicalEntry;
	status_t result = get_memory_map(logAddress, size, &physicalEntry, 1);
	if (result < B_OK) {
		delete_area(area);
		TRACE(("usb stack: couldn't map area %s\n", name));
		return B_ERROR;
	}

	memset(logAddress, 0, size);
	if (logicalAddress)
		*logicalAddress = logAddress;

	if (physicalAddress)
		*physicalAddress = physicalEntry.address;

	TRACE(("usb stack: area = 0x%08x, size = %ld, log = 0x%08x, phy = 0x%08x\n",
		area, size, logAddress, physicalEntry.address));
	return area;
}


Stack *usb_stack = NULL;
