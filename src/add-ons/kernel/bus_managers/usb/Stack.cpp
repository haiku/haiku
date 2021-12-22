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
#include "usb_private.h"
#include "PhysicalMemoryAllocator.h"

#include <fs/devfs.h>


Stack::Stack()
	:	fExploreThread(-1),
		fFirstExploreDone(false),
		fStopThreads(false),
		fAllocator(NULL),
		fObjectIndex(1),
		fObjectMaxCount(1024),
		fObjectArray(NULL),
		fDriverList(NULL)
{
	TRACE("stack init\n");

	mutex_init(&fStackLock, "usb stack lock");
	mutex_init(&fExploreLock, "usb explore lock");

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

	// Check for host controller modules.
	//
	// While using a fixed list of names is inflexible, it allows us to control
	// the order in which we try modules. There are controllers/BIOSes that
	// require UHCI/OHCI to be initialized before EHCI or otherwise they
	// refuse to publish any high-speed devices.
	//
	// On other systems, the ordering is probably ensured because the EHCI
	// controller is required to have a higher PCI function number than the
	// companion host controllers (per the EHCI specs) and it would therefore
	// be enumerated as the last item. As this does not apply to us, we have to
	// ensure ordering using another method.
	//
	// Furthermore, on some systems, there can be ports shared between
	// EHCI and XHCI, defaulting to EHCI. The XHCI module will switch these
	// ports before the EHCI module discovers them.
	const char *moduleNames[] = {
		"busses/usb/xhci",
		"busses/usb/uhci",
		"busses/usb/ohci",
		"busses/usb/ehci",
		NULL
	};

	TRACE("looking for host controller modules\n");
	for (uint32 i = 0; moduleNames[i]; i++) {
		TRACE("looking for module %s\n", moduleNames[i]);

		usb_host_controller_info *module = NULL;
		if (get_module(moduleNames[i], (module_info **)&module) != B_OK)
			continue;

		TRACE("adding module %s\n", moduleNames[i]);
		if (module->add_to(this) < B_OK) {
			put_module(moduleNames[i]);
			continue;
		}

		TRACE("module %s successfully loaded\n", moduleNames[i]);
	}

	if (fBusManagers.Count() == 0) {
		TRACE_ERROR("no bus managers available\n");
		return;
	}

	fExploreThread = spawn_kernel_thread(ExploreThread, "usb explore",
		B_LOW_PRIORITY, this);
	resume_thread(fExploreThread);

	// wait for the first explore to complete. this ensures that a driver that
	// is opening the module does not get rescanned while or before installing
	// its hooks.
	while (!fFirstExploreDone)
		snooze(10000);
}


Stack::~Stack()
{
	int32 result;
	fStopThreads = true;
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
	if (fBusManagers.Count() == 0)
		return ENODEV;
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
		result->SetBusy(true);

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

	while (!stack->fStopThreads) {
		if (mutex_lock(&stack->fExploreLock) != B_OK)
			break;

		rescan_item *rescanList = NULL;
		change_item *changeItem = NULL;
		for (int32 i = 0; i < stack->fBusManagers.Count(); i++) {
			Hub *rootHub = stack->fBusManagers.ElementAt(i)->GetRootHub();
			if (rootHub)
				rootHub->Explore(&changeItem);
		}

		while (changeItem) {
			stack->NotifyDeviceChange(changeItem->device, &rescanList, changeItem->added);
			if (!changeItem->added) {
				// everyone possibly holding a reference is now notified so we
				// can delete the device
				changeItem->device->GetBusManager()->FreeDevice(changeItem->device);
			}

			change_item *next = changeItem->link;
			delete changeItem;
			changeItem = next;
		}

		stack->fFirstExploreDone = true;
		mutex_unlock(&stack->fExploreLock);
		stack->RescanDrivers(rescanList);
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
		B_32_BIT_CONTIGUOUS, 0);
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
