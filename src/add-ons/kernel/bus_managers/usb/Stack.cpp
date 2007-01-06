/*
 * Copyright 2003-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		Niels S. Reedijk
 */

#include <module.h>
#include <unistd.h>
#include <util/kernel_cpp.h>
#include "usb_p.h"
#include "PhysicalMemoryAllocator.h"


Stack::Stack()
	:	fExploreThread(-1),
		fFirstExploreDone(false),
		fStopThreads(false),
		fObjectIndex(1),
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

	fAllocator = new(std::nothrow) PhysicalMemoryAllocator("USB Stack Allocator",
		8, B_PAGE_SIZE * 4, 64);
	if (!fAllocator || fAllocator->InitCheck() < B_OK) {
		TRACE_ERROR(("usb stack: failed to allocate the allocator\n"));
		delete fAllocator;
		return;
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

		if (module->add_to(this) < B_OK)
			continue;

		TRACE(("usb stack: module %s successfully loaded\n", moduleName));
	}

	if (fBusManagers.Count() == 0) {
		TRACE_ERROR(("usb stack: no bus managers available\n"));
		return;
	}

	if (benaphore_init(&fExploreLock, "usb explore lock") < B_OK) {
		TRACE_ERROR(("usb stack: failed to create benaphore explore lock\n"));
		return;
	}

	fExploreThread = spawn_kernel_thread(ExploreThread, "usb explore",
		B_LOW_PRIORITY, this);
	resume_thread(fExploreThread);

	// wait for the first explore to complete
	// this ensures that we get all initial devices under InstallNotify
	while (!fFirstExploreDone)
		snooze(1000);
}


Stack::~Stack()
{
	int32 result;
	fStopThreads = true;
	wait_for_thread(fExploreThread, &result);

	Lock();
	benaphore_destroy(&fLock);

	//Release the bus modules
	for (Vector<BusManager *>::Iterator i = fBusManagers.Begin();
		i != fBusManagers.End(); i++) {
		delete (*i);
	}

	delete fAllocator;
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


int32
Stack::ExploreThread(void *data)
{
	Stack *stack = (Stack *)data;

	while (!stack->fStopThreads) {
		if (benaphore_lock(&stack->fExploreLock) != B_OK)
			continue;

		for (int32 i = 0; i < stack->fBusManagers.Count(); i++) {
			Hub *rootHub = stack->fBusManagers.ElementAt(i)->GetRootHub();
			if (rootHub)
				rootHub->Explore();
		}

		stack->fFirstExploreDone = true;
		benaphore_unlock(&stack->fExploreLock);
		snooze(USB_DELAY_HUB_EXPLORE);
	}

	return B_OK;
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
Stack::AllocateChunk(void **logicalAddress, void **physicalAddress, size_t size)
{
	return fAllocator->Allocate(size, logicalAddress, physicalAddress);
}


status_t
Stack::FreeChunk(void *logicalAddress, void *physicalAddress, size_t size)
{	
	return fAllocator->Deallocate(size, logicalAddress, physicalAddress);
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
			status_t result = device->ReportDevice(element->support_descriptors,
				element->support_descriptor_count,
				&element->notify_hooks, &element->cookies, added);

			if (result == B_OK) {
				const char *name = element->driver_name;
				if (element->republish_driver_name)
					name = element->republish_driver_name;
				int devFS = open("/dev", O_WRONLY);
				write(devFS, name, strlen(name));
				close(devFS);
			}
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
	info->cookies = NULL;
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
			// ensure that no devices are added/removed while we are
			// reporting devices
			if (benaphore_lock(&fExploreLock) != B_OK)
				return B_ERROR;

			// inform driver about any already present devices
			for (int32 i = 0; i < fBusManagers.Count(); i++) {
				Hub *rootHub = fBusManagers.ElementAt(i)->GetRootHub();
				if (rootHub) {
					// Report device will recurse down the whole tree
					rootHub->ReportDevice(element->support_descriptors,
						element->support_descriptor_count, hooks,
						&element->cookies, true);
				}
			}

			element->notify_hooks.device_added = hooks->device_added;
			element->notify_hooks.device_removed = hooks->device_removed;
			benaphore_unlock(&fExploreLock);
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
			// ensure that no devices are added/removed while we are
			// reporting devices
			if (benaphore_lock(&fExploreLock) != B_OK)
				return B_ERROR;

			// trigger the device removed hook
			for (int32 i = 0; i < fBusManagers.Count(); i++) {
				Hub *rootHub = fBusManagers.ElementAt(i)->GetRootHub();
				if (rootHub)
					rootHub->ReportDevice(element->support_descriptors,
						element->support_descriptor_count,
						&element->notify_hooks, &element->cookies, false);
			}

			element->notify_hooks.device_added = NULL;
			element->notify_hooks.device_removed = NULL;
			benaphore_unlock(&fExploreLock);
			return B_OK;
		}

		element = element->link;
	}

	return B_NAME_NOT_FOUND;
}
