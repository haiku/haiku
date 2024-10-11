/*
 * Copyright 2003-2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		Niels S. Reedijk
 */


#include <module.h>
#include <unistd.h>
#include <util/kernel_cpp.h>
#include <util/AutoLock.h>

#include "usb_private.h"
#include "PhysicalMemoryAllocator.h"

#include <fs/devfs.h>
#include <kdevice_manager.h>


Stack::Stack()
	:	fExploreThread(-1),
		fExploreSem(-1),
		fAllocator(NULL),
		fObjectIndex(1),
		fObjectMaxCount(1024),
		fObjectArray(NULL),
		fDriverList(NULL)
{
	TRACE("stack init\n");

	mutex_init(&fStackLock, "usb stack lock");
	mutex_init(&fExploreLock, "usb explore lock");
	fExploreSem = create_sem(0, "usb explore sem");
	if (fExploreSem < B_OK) {
		TRACE_ERROR("failed to create semaphore\n");
		return;
	}


	size_t objectArraySize = fObjectMaxCount * sizeof(Object *);
	fObjectArray = (Object **)malloc(objectArraySize);
	if (fObjectArray == NULL) {
		TRACE_ERROR("failed to allocate object array\n");
		return;
	}

	memset(fObjectArray, 0, objectArraySize);

	fAllocator = new(std::nothrow) PhysicalMemoryAllocator("USB Stack Allocator",
		8, B_PAGE_SIZE * 32, 64);
	if (!fAllocator || fAllocator->InitCheck() < B_OK) {
		TRACE_ERROR("failed to allocate the allocator\n");
		delete fAllocator;
		fAllocator = NULL;
		return;
	}

	fExploreThread = spawn_kernel_thread(ExploreThread, "usb explore",
		B_LOW_PRIORITY, this);
	resume_thread(fExploreThread);
}


Stack::~Stack()
{
	int32 result;
	delete_sem(fExploreSem);
	fExploreSem = -1;
	wait_for_thread(fExploreThread, &result);

	mutex_lock(&fStackLock);
	mutex_destroy(&fStackLock);
	mutex_lock(&fExploreLock);
	mutex_destroy(&fExploreLock);

	// Release the bus modules
	for (Vector<BusManager *>::Iterator i = fBusManagers.Begin();
		i != fBusManagers.End(); i++) {
		delete (*i);
	}

	delete fAllocator;
	free(fObjectArray);
}


status_t
Stack::InitCheck()
{
	return B_OK;
}


bool
Stack::Lock()
{
	return (mutex_lock(&fStackLock) == B_OK);
}


void
Stack::Unlock()
{
	mutex_unlock(&fStackLock);
}


usb_id
Stack::GetUSBID(Object *object)
{
	if (!Lock())
		return fObjectMaxCount;

	uint32 id = fObjectIndex;
	uint32 tries = fObjectMaxCount;
	while (tries-- > 0) {
		if (fObjectArray[id] == NULL) {
			fObjectIndex = (id + 1) % fObjectMaxCount;
			fObjectArray[id] = object;
			Unlock();
			return (usb_id)id;
		}

		id = (id + 1) % fObjectMaxCount;
	}

	TRACE_ERROR("the stack has run out of usb_ids\n");
	Unlock();
	return 0;
}


void
Stack::PutUSBID(Object *object)
{
	if (!Lock())
		return;

	usb_id id = object->USBID();
	if (id >= fObjectMaxCount) {
		TRACE_ERROR("tried to put an invalid usb_id\n");
		Unlock();
		return;
	}
	if (fObjectArray[id] != object) {
		TRACE_ERROR("tried to put an object with incorrect usb_id\n");
		Unlock();
		return;
	}

	fObjectArray[id] = NULL;

#if KDEBUG
	// Validate that no children of this object are still in the stack.
	for (usb_id i = 0; i < fObjectMaxCount; i++) {
		if (fObjectArray[i] == NULL)
			continue;

		ASSERT_PRINT(fObjectArray[i]->Parent() != object,
			"%s", fObjectArray[i]->TypeName());
	}
#endif

	Unlock();
}


Object *
Stack::GetObject(usb_id id)
{
	if (!Lock())
		return NULL;

	if (id >= fObjectMaxCount) {
		TRACE_ERROR("tried to get object with invalid usb_id\n");
		Unlock();
		return NULL;
	}

	Object *result = fObjectArray[id];

	if (result != NULL)
		result->AcquireReference();

	Unlock();
	return result;
}


Object *
Stack::GetObjectNoLock(usb_id id) const
{
	ASSERT(debug_debugger_running());
	if (id >= fObjectMaxCount)
		return NULL;
	return fObjectArray[id];
}


int32
Stack::ExploreThread(void *data)
{
	Stack *stack = (Stack *)data;

	while (acquire_sem_etc(stack->fExploreSem, 1, B_RELATIVE_TIMEOUT,
			USB_DELAY_HUB_EXPLORE) != B_BAD_SEM_ID) {
		stack->Explore();
	}

	return B_OK;
}


void
Stack::Explore()
{
	recursive_lock* dmLock = device_manager_get_lock();
	if (find_thread(NULL) != fExploreThread
			&& RECURSIVE_LOCK_HOLDER(dmLock) == find_thread(NULL)) {
		// This should only happen during the initial device scan, during which
		// we should be able to acquire the explore lock immediately (since the
		// explore thread will be waiting on the device manager lock as below),
		// but in case we aren't, use a timeout to avoid lock-order-inversion deadlocks.
		if (mutex_lock_with_timeout(&fExploreLock, B_RELATIVE_TIMEOUT, 1000) != B_OK) {
			release_sem(fExploreSem);
			return;
		}
	} else {
		// Temporarily acquire the device manager lock, to ensure it isn't scanning.
		RecursiveLocker dmLocker(dmLock);

		if (mutex_lock(&fExploreLock) != B_OK)
			return;

		dmLocker.Unlock();
	}

	int32 semCount = 0;
	get_sem_count(fExploreSem, &semCount);
	if (semCount > 0)
		acquire_sem_etc(fExploreSem, semCount, B_RELATIVE_TIMEOUT, 0);

	rescan_item *rescanList = NULL;
	change_item *changeItem = NULL;
	for (int32 i = 0; i < fBusManagers.Count(); i++) {
		Hub *rootHub = fBusManagers.ElementAt(i)->GetRootHub();
		if (rootHub)
			rootHub->Explore(&changeItem);
	}

	while (changeItem) {
		NotifyDeviceChange(changeItem->device, &rescanList, changeItem->added);
		if (!changeItem->added) {
			// everyone possibly holding a reference is now notified so we
			// can delete the device
			changeItem->device->GetBusManager()->FreeDevice(changeItem->device);
		}

		change_item *next = changeItem->link;
		delete changeItem;
		changeItem = next;
	}

	mutex_unlock(&fExploreLock);
	RescanDrivers(rescanList);
}

void
Stack::AddBusManager(BusManager *busManager)
{
	MutexLocker _(fExploreLock);
	fBusManagers.PushBack(busManager);
}


int32
Stack::IndexOfBusManager(BusManager *busManager) const
{
	return fBusManagers.IndexOf(busManager);
}


BusManager *
Stack::BusManagerAt(int32 index) const
{
	return fBusManagers.ElementAt(index);
}


status_t
Stack::AllocateChunk(void **logicalAddress, phys_addr_t *physicalAddress,
	size_t size)
{
	return fAllocator->Allocate(size, logicalAddress, physicalAddress);
}


status_t
Stack::FreeChunk(void *logicalAddress, phys_addr_t physicalAddress,
	size_t size)
{
	return fAllocator->Deallocate(size, logicalAddress, physicalAddress);
}


area_id
Stack::AllocateArea(void **logicalAddress, phys_addr_t *physicalAddress, size_t size,
	const char *name)
{
	TRACE("allocating %ld bytes for %s\n", size, name);

	void *logAddress;
	size = (size + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);
	area_id area = create_area(name, &logAddress, B_ANY_KERNEL_ADDRESS, size,
		B_32_BIT_CONTIGUOUS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
		// TODO: Use B_CONTIGUOUS when the TODOs regarding 64 bit physical
		// addresses are fixed (if possible).

	if (area < B_OK) {
		TRACE_ERROR("couldn't allocate area %s\n", name);
		return B_ERROR;
	}

	physical_entry physicalEntry;
	status_t result = get_memory_map(logAddress, size, &physicalEntry, 1);
	if (result < B_OK) {
		delete_area(area);
		TRACE_ERROR("couldn't map area %s\n", name);
		return B_ERROR;
	}

	memset(logAddress, 0, size);
	if (logicalAddress)
		*logicalAddress = logAddress;

	if (physicalAddress)
		*physicalAddress = (phys_addr_t)physicalEntry.address;

	TRACE("area = %" B_PRId32 ", size = %" B_PRIuSIZE ", log = %p, phy = %#"
		B_PRIxPHYSADDR "\n", area, size, logAddress, physicalEntry.address);
	return area;
}


void
Stack::NotifyDeviceChange(Device *device, rescan_item **rescanList, bool added)
{
	TRACE("device %s\n", added ? "added" : "removed");

	usb_driver_info *element = fDriverList;
	while (element) {
		status_t result = device->ReportDevice(element->support_descriptors,
			element->support_descriptor_count, &element->notify_hooks,
			&element->cookies, added, false);

		if (result >= B_OK) {
			const char *driverName = element->driver_name;
			if (element->republish_driver_name)
				driverName = element->republish_driver_name;

			bool already = false;
			rescan_item *rescanItem = *rescanList;
			while (rescanItem) {
				if (strcmp(rescanItem->name, driverName) == 0) {
					// this driver is going to be rescanned already
					already = true;
					break;
				}
				rescanItem = rescanItem->link;
			}

			if (!already) {
				rescanItem = new(std::nothrow) rescan_item;
				if (!rescanItem)
					return;

				rescanItem->name = driverName;
				rescanItem->link = *rescanList;
				*rescanList = rescanItem;
			}
		}

		element = element->link;
	}
}


void
Stack::RescanDrivers(rescan_item *rescanItem)
{
	while (rescanItem) {
		// the device is supported by this driver. it either got notified
		// already by the hooks or it is not loaded at this time. in any
		// case we will rescan the driver so it either is loaded and can
		// scan for supported devices or its publish_devices hook will be
		// called to expose changed devices.

		// use the private devfs API to republish a device
		devfs_rescan_driver(rescanItem->name);

		rescan_item *next = rescanItem->link;
		delete rescanItem;
		rescanItem = next;
	}
}


status_t
Stack::RegisterDriver(const char *driverName,
	const usb_support_descriptor *descriptors,
	size_t descriptorCount, const char *republishDriverName)
{
	TRACE("register driver \"%s\"\n", driverName);
	if (!driverName)
		return B_BAD_VALUE;

	if (!Lock())
		return B_ERROR;

	usb_driver_info *element = fDriverList;
	while (element) {
		if (strcmp(element->driver_name, driverName) == 0) {
			// we already have an entry for this driver, just update it
			free((char *)element->republish_driver_name);
			element->republish_driver_name = strdup(republishDriverName);

			free(element->support_descriptors);
			size_t descriptorsSize = descriptorCount * sizeof(usb_support_descriptor);
			element->support_descriptors = (usb_support_descriptor *)malloc(descriptorsSize);
			memcpy(element->support_descriptors, descriptors, descriptorsSize);
			element->support_descriptor_count = descriptorCount;

			Unlock();
			return B_OK;
		}

		element = element->link;
	}

	// this is a new driver, add it to the driver list
	usb_driver_info *info = new(std::nothrow) usb_driver_info;
	if (!info) {
		Unlock();
		return B_NO_MEMORY;
	}

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
	TRACE("installing notify hooks for driver \"%s\"\n", driverName);

	usb_driver_info *element = fDriverList;
	while (element) {
		if (strcmp(element->driver_name, driverName) == 0) {
			if (mutex_lock(&fExploreLock) != B_OK)
				return B_ERROR;

			// inform driver about any already present devices
			for (int32 i = 0; i < fBusManagers.Count(); i++) {
				Hub *rootHub = fBusManagers.ElementAt(i)->GetRootHub();
				if (rootHub) {
					// Report device will recurse down the whole tree
					rootHub->ReportDevice(element->support_descriptors,
						element->support_descriptor_count, hooks,
						&element->cookies, true, true);
				}
			}

			element->notify_hooks.device_added = hooks->device_added;
			element->notify_hooks.device_removed = hooks->device_removed;
			mutex_unlock(&fExploreLock);
			return B_OK;
		}

		element = element->link;
	}

	return B_NAME_NOT_FOUND;
}


status_t
Stack::UninstallNotify(const char *driverName)
{
	TRACE("uninstalling notify hooks for driver \"%s\"\n", driverName);

	usb_driver_info *element = fDriverList;
	while (element) {
		if (strcmp(element->driver_name, driverName) == 0) {
			if (mutex_lock(&fExploreLock) != B_OK)
				return B_ERROR;

			// trigger the device removed hook
			for (int32 i = 0; i < fBusManagers.Count(); i++) {
				Hub *rootHub = fBusManagers.ElementAt(i)->GetRootHub();
				if (rootHub)
					rootHub->ReportDevice(element->support_descriptors,
						element->support_descriptor_count,
						&element->notify_hooks, &element->cookies, false, true);
			}

			element->notify_hooks.device_added = NULL;
			element->notify_hooks.device_removed = NULL;
			mutex_unlock(&fExploreLock);
			return B_OK;
		}

		element = element->link;
	}

	return B_NAME_NOT_FOUND;
}

