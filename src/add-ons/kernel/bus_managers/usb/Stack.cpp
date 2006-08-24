/*
 * Copyright 2003-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		Niels S. Reedijk
 */

#include <module.h>
#include <util/kernel_cpp.h>
#include "usb_p.h"


Stack::Stack()
	:	fObjectIndex(1),
		fObjectMaxCount(1024),
		fObjectArray(NULL),
		fDriverList(NULL)
{
	TRACE(("usb stack: stack init\n"));

	if (benaphore_init(&fLock, "USB Stack Master Lock") < B_OK) {
		TRACE_ERROR(("usb stack: failed to create benaphore lock\n"));
		return;
	}

	size_t objectArraySize = fObjectMaxCount * sizeof(Object *);
	fObjectArray = (Object **)malloc(objectArraySize);
	memset(fObjectArray, 0, objectArraySize);

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
		TRACE_ERROR(("usb stack: 8-byte chunk area failed to initialise\n"));
		return;
	}

	fListhead8 = (addr_t)fLogical[0];
	for (int32 i = 0; i < B_PAGE_SIZE / 8; i++) {
		memory_chunk *chunk = (memory_chunk *)((addr_t)fLogical[0] + 8 * i);
		chunk->physical = (addr_t)fPhysical[0] + 8 * i;

		if (i < B_PAGE_SIZE / 8 - 1)
			chunk->next_item = (addr_t)fLogical[0] + 8 * (i + 1);
		else
			chunk->next_item = 0;
	}

	// 16-byte heap
	fAreaFreeCount[1] = 0;
	fAreas[1] = AllocateArea(&fLogical[1], &fPhysical[1], B_PAGE_SIZE,
		"16-byte chunk area");

	if (fAreas[1] < B_OK) {
		TRACE_ERROR(("usb stack: 16-byte chunk area failed to initialise\n"));
		return;
	}

	fListhead16 = (addr_t)fLogical[1];
	for (int32 i = 0; i < B_PAGE_SIZE / 16; i++) {
		memory_chunk *chunk = (memory_chunk *)((addr_t)fLogical[1] + 16 * i);
		chunk->physical = (addr_t)fPhysical[1] + 16 * i;

		if (i < B_PAGE_SIZE / 16 - 1)
			chunk->next_item = (addr_t)fLogical[1] + 16 * (i + 1);
		else
			chunk->next_item = 0;
	}

	// 32-byte heap
	fAreaFreeCount[2] = 0;
	fAreas[2] = AllocateArea(&fLogical[2], &fPhysical[2], B_PAGE_SIZE,
		"32-byte chunk area");

	if (fAreas[2] < B_OK) {
		TRACE_ERROR(("usb stack: 32-byte chunk area failed to initialise\n"));
		return;
	}

	fListhead32 = (addr_t)fLogical[2];
	for (int32 i = 0; i < B_PAGE_SIZE / 32; i++) {
		memory_chunk *chunk = (memory_chunk *)((addr_t)fLogical[2] + 32 * i);
		chunk->physical = (addr_t)fPhysical[2] + 32 * i;

		if (i < B_PAGE_SIZE / 32 - 1)
			chunk->next_item = (addr_t)fLogical[2] + 32 * (i + 1);
		else
			chunk->next_item = 0;
	}

	// 64-byte heap
	fAreaFreeCount[3] = 0;
	fAreas[3] = AllocateArea(&fLogical[3], &fPhysical[3], B_PAGE_SIZE,
		"64-byte chunk area");

	if (fAreas[3] < B_OK) {
		TRACE_ERROR(("usb stack: 64-byte chunk area failed to initialise\n"));
		return;
	}

	fListhead64 = (addr_t)fLogical[3];
	for (int32 i = 0; i < B_PAGE_SIZE / 64; i++) {
		memory_chunk *chunk = (memory_chunk *)((addr_t)fLogical[3] + 64 * i);
		chunk->physical = (addr_t)fPhysical[3] + 64 * i;

		if (i < B_PAGE_SIZE / 64 - 1)
			chunk->next_item = (addr_t)fLogical[3] + 64 * (i + 1);
		else
			chunk->next_item = 0;
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
	Lock();
	benaphore_destroy(&fLock);

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


bool
Stack::Lock()
{
	return (benaphore_lock(&fLock) == B_OK);
}


void
Stack::Unlock()
{
	benaphore_unlock(&fLock);
}


usb_id
Stack::GetUSBID(Object *object)
{
	if (!Lock())
		return 0;

	if (fObjectIndex >= fObjectMaxCount) {
		Unlock();
		return 0;
	}

	uint32 id = fObjectIndex++;
	fObjectArray[id] = object;
	Unlock();

	return (usb_id)id;
}


void
Stack::PutUSBID(usb_id id)
{
	if (!Lock())
		return;

	if (id >= fObjectMaxCount) {
		TRACE_ERROR(("usb stack: tried to put invalid usb_id!\n"));
		Unlock();
		return;
	}

	fObjectArray[id] = NULL;
	Unlock();
}


Object *
Stack::GetObject(usb_id id)
{
	if (!Lock())
		return NULL;

	if (id >= fObjectMaxCount) {
		TRACE_ERROR(("usb stack: tried to get object with invalid id\n"));
		Unlock();
		return NULL;
	}

	Object *result = fObjectArray[id];
	Unlock();

	return result;
}


void
Stack::AddBusManager(BusManager *busManager)
{
	fBusManagers.PushBack(busManager);
}


int32
Stack::IndexOfBusManager(BusManager *busManager)
{
	return fBusManagers.IndexOf(busManager);
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
	else if (size <= 64)
		listhead = fListhead64;
	else {
		TRACE_ERROR(("usb stack: Chunk size %d to big\n", size));
		Unlock();
		return B_ERROR;
	}

	if (listhead == 0) {
		TRACE_ERROR(("usb stack: Out of memory on this list\n"));
		Unlock();
		return B_ERROR;
	}

	TRACE(("usb stack: Stack::Allocate() listhead: 0x%08x\n", listhead));
	memory_chunk *chunk = (memory_chunk *)listhead;
	*logicalAddress = (void *)listhead;
	*physicalAddress = (void *)chunk->physical;
	if (chunk->next_item == 0) {
		//TODO: allocate more memory
		listhead = 0;
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
	else if (size <= 64)
		fListhead64 = listhead;

	Unlock();
	TRACE(("usb stack: allocated a new chunk with size %u\n", size));
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
	else if (size <= 64)
		listhead = fListhead64;
	else {
		TRACE_ERROR(("usb stack: Chunk size %d invalid\n", size));
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
	else if (size <= 64)
		fListhead64 = (addr_t)logicalAddress;

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
		TRACE_ERROR(("usb stack: couldn't allocate area %s\n", name));
		return B_ERROR;
	}

	physical_entry physicalEntry;
	status_t result = get_memory_map(logAddress, size, &physicalEntry, 1);
	if (result < B_OK) {
		delete_area(area);
		TRACE_ERROR(("usb stack: couldn't map area %s\n", name));
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


void
Stack::NotifyDeviceChange(Device *device, bool added)
{
	TRACE(("usb stack: device %s\n", added ? "added" : "removed"));

	usb_driver_info *element = fDriverList;
	while (element) {
		if ((added && element->notify_hooks.device_added != NULL)
			|| (!added && element->notify_hooks.device_removed != NULL)) {
			device->ReportDevice(element->support_descriptors,
				element->support_descriptor_count,
				&element->notify_hooks, added);
		}

		element = element->link;
	}
}


status_t
Stack::RegisterDriver(const char *driverName,
	const usb_support_descriptor *descriptors,
	size_t descriptorCount, const char *republishDriverName)
{
	TRACE(("usb stack: register driver \"%s\"\n", driverName));
	usb_driver_info *info = new(std::nothrow) usb_driver_info;
	if (!info)
		return B_NO_MEMORY;

	info->driver_name = strdup(driverName);
	info->republish_driver_name = strdup(republishDriverName);

	size_t descriptorsSize = descriptorCount * sizeof(usb_support_descriptor);
	info->support_descriptors = (usb_support_descriptor *)malloc(descriptorsSize);
	memcpy(info->support_descriptors, descriptors, descriptorsSize);
	info->support_descriptor_count = descriptorCount;

	info->notify_hooks.device_added = NULL;
	info->notify_hooks.device_removed = NULL;
	info->link = NULL;

	if (!Lock()) {
		delete info;
		return B_ERROR;
	}

	if (fDriverList) {
		usb_driver_info *element = fDriverList;
		while (element->link)
			element = element->link;

		element->link = info;
	} else
		fDriverList = info;

	Unlock();
	return B_OK;
}


status_t
Stack::InstallNotify(const char *driverName, const usb_notify_hooks *hooks)
{
	TRACE(("usb stack: installing notify hooks for driver \"%s\"\n", driverName));
	usb_driver_info *element = fDriverList;
	while (element) {
		if (strcmp(element->driver_name, driverName) == 0) {
			// inform driver about any already present devices
			for (int32 i = 0; i < fBusManagers.Count(); i++) {
				Hub *rootHub = fBusManagers.ElementAt(i)->GetRootHub();
				if (rootHub) {
					// Ensure that all devices are already recognized
					rootHub->Explore();

					// Report device will recurse down the whole tree
					rootHub->ReportDevice(element->support_descriptors,
						element->support_descriptor_count, hooks, true);
				}
			}

			element->notify_hooks.device_added = hooks->device_added;
			element->notify_hooks.device_removed = hooks->device_removed;
			return B_OK;
		}

		element = element->link;
	}

	return B_NAME_NOT_FOUND;
}


status_t
Stack::UninstallNotify(const char *driverName)
{
	TRACE(("usb stack: uninstalling notify hooks for driver \"%s\"\n", driverName));
	usb_driver_info *element = fDriverList;
	while (element) {
		if (strcmp(element->driver_name, driverName) == 0) {
			element->notify_hooks.device_added = NULL;
			element->notify_hooks.device_removed = NULL;
			return B_OK;
		}

		element = element->link;
	}

	return B_NAME_NOT_FOUND;
}
