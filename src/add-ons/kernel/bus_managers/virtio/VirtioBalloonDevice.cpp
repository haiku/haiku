/*
 * Copyright 2018, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include "VirtioBalloonPrivate.h"

#include <new>
#include <stdlib.h>
#include <string.h>

#include <util/AutoLock.h>

#include "virtio_balloon.h"


const char*
get_feature_name(uint32 feature)
{
	switch (feature) {
		case VIRTIO_BALLOON_F_MUST_TELL_HOST:
			return "must tell host";
		case VIRTIO_BALLOON_F_STATS_VQ:
			return "stats vq";
	}
	return NULL;
}


VirtioBalloonDevice::VirtioBalloonDevice(device_node* node)
	:
	fNode(node),
	fVirtio(NULL),
	fVirtioDevice(NULL),
	fStatus(B_NO_INIT),
	fDesiredSize(0),
	fActualSize(0)
{
	CALLED();

	B_INITIALIZE_SPINLOCK(&fInterruptLock);
	fQueueCondition.Init(this, "virtio balloon transfer");
	fConfigCondition.Init(this, "virtio balloon config");

	get_memory_map(fBuffer, sizeof(fBuffer), &fEntry, 1);

	// get the Virtio device from our parent's parent
	device_node* parent = gDeviceManager->get_parent_node(node);
	device_node* virtioParent = gDeviceManager->get_parent_node(parent);
	gDeviceManager->put_node(parent);

	gDeviceManager->get_driver(virtioParent, (driver_module_info**)&fVirtio,
		(void**)&fVirtioDevice);
	gDeviceManager->put_node(virtioParent);

	fVirtio->negotiate_features(fVirtioDevice,
		0, &fFeatures, &get_feature_name);

	fStatus = fVirtio->alloc_queues(fVirtioDevice, 2, fVirtioQueues);
	if (fStatus != B_OK) {
		ERROR("queue allocation failed (%s)\n", strerror(fStatus));
		return;
	}

	fStatus = fVirtio->setup_interrupt(fVirtioDevice, _ConfigCallback, this);
	if (fStatus != B_OK) {
		ERROR("interrupt setup failed (%s)\n", strerror(fStatus));
		return;
	}

	fStatus = fVirtio->queue_setup_interrupt(fVirtioQueues[0],
		_QueueCallback, fVirtioQueues[0]);
	if (fStatus != B_OK) {
		ERROR("queue interrupt setup failed (%s)\n", strerror(fStatus));
		return;
	}

	fStatus = fVirtio->queue_setup_interrupt(fVirtioQueues[1],
		_QueueCallback, fVirtioQueues[1]);
	if (fStatus != B_OK) {
		ERROR("queue interrupt setup failed (%s)\n", strerror(fStatus));
		return;
	}

	fThread = spawn_kernel_thread(&_ThreadEntry, "virtio balloon thread",
		B_NORMAL_PRIORITY, this);
	if (fThread < 0) {
		fStatus = fThread;
		return;
	}
	resume_thread(fThread);
}


VirtioBalloonDevice::~VirtioBalloonDevice()
{
	fRunning = false;
	if (fThread >= 0) {
		int32 result;
		wait_for_thread(fThread, &result);
		fThread = -1;
	}
}


status_t
VirtioBalloonDevice::InitCheck()
{
	return fStatus;
}


int32
VirtioBalloonDevice::_ThreadEntry(void* data)
{
	VirtioBalloonDevice* device = (VirtioBalloonDevice*)data;
	return device->_Thread();
}


int32
VirtioBalloonDevice::_Thread()
{
	CALLED();

	while (fRunning) {
		if (fDesiredSize == fActualSize) {
			TRACE("waiting for a config change\n");
			ConditionVariableEntry configConditionEntry;
			fConfigCondition.Add(&configConditionEntry);
			status_t result = configConditionEntry.Wait(B_CAN_INTERRUPT);
			if (result != B_OK)
				continue;

			fDesiredSize = _DesiredSize();
			TRACE("finished waiting: requested %" B_PRIu32 " instead of %" B_PRIu32
				"\n", fDesiredSize, fActualSize);
		}
		::virtio_queue queue;
		if (fDesiredSize > fActualSize) {
			int32 count = min_c(PAGES_COUNT, fDesiredSize - fActualSize);
			TRACE("allocating %" B_PRIu32 " pages\n", count);
			queue = fVirtioQueues[0];

			vm_page_reservation reservation;
			vm_page_reserve_pages(&reservation, count, VM_PRIORITY_SYSTEM);
			for (int i = 0; i < count; i++) {
				//TRACE("allocating page %" B_PRIu32 " pages\n", i);
				vm_page* page = vm_page_allocate_page(&reservation,
					PAGE_STATE_WIRED);
				if (page == NULL) {
					TRACE("allocating failed\n");
					count = i;
					break;
				}
				PageInfo* info = new PageInfo;
				info->page = page;
				fBuffer[i] = (phys_addr_t)page->physical_page_number
					>> (PAGE_SHIFT - VIRTIO_BALLOON_PFN_SHIFT);
				fPages.Add(info);
			}
			fEntry.size = count * sizeof(uint32);
			fActualSize += count;
		} else {
			int32 count = min_c(PAGES_COUNT, fActualSize - fDesiredSize);
			TRACE("freeing %" B_PRIu32 " pages\n", count);
			queue = fVirtioQueues[1];

			for (int i = 0; i < count; i++) {
				PageInfo* info = fPages.RemoveHead();
				if (info == NULL) {
					TRACE("remove failed\n");
					count = i;
					break;
				}
				vm_page* page = info->page;
				fBuffer[i] = (phys_addr_t)page->physical_page_number
					>> (PAGE_SHIFT - VIRTIO_BALLOON_PFN_SHIFT);
				vm_page_free(NULL, page);
			}
			fEntry.size = count * sizeof(uint32);
			fActualSize -= count;
		}

		ConditionVariableEntry queueConditionEntry;
		fQueueCondition.Add(&queueConditionEntry);

		// alloc or release
		TRACE("queue request\n");
		status_t result = fVirtio->queue_request(queue, &fEntry, NULL, NULL);
		if (result != B_OK) {
			ERROR("queueing failed (%s)\n", strerror(result));
			return result;
		}

		while (!fVirtio->queue_dequeue(queue, NULL, NULL)) {
			TRACE("wait for response\n");
			queueConditionEntry.Wait(B_CAN_INTERRUPT);
		}

		TRACE("update size\n");
		_UpdateSize();
	}

	return 0;
}


void
VirtioBalloonDevice::_ConfigCallback(void* driverCookie)
{
	CALLED();
	VirtioBalloonDevice* device = (VirtioBalloonDevice*)driverCookie;

	SpinLocker locker(device->fInterruptLock);
	device->fConfigCondition.NotifyAll();
}


void
VirtioBalloonDevice::_QueueCallback(void* driverCookie, void* cookie)
{
	CALLED();
	VirtioBalloonDevice* device = (VirtioBalloonDevice*)driverCookie;

	SpinLocker locker(device->fInterruptLock);
	device->fQueueCondition.NotifyAll();
}


uint32
VirtioBalloonDevice::_DesiredSize()
{
	CALLED();
	uint32 desiredSize;

	status_t status = fVirtio->read_device_config(fVirtioDevice,
		offsetof(struct virtio_balloon_config, num_pages),
		&desiredSize, sizeof(desiredSize));
	if (status != B_OK)
		return 0;

	return B_LENDIAN_TO_HOST_INT32(desiredSize);
}


status_t
VirtioBalloonDevice::_UpdateSize()
{
	CALLED();
	TRACE("_UpdateSize %u\n", fActualSize);
	uint32 actualSize = B_HOST_TO_LENDIAN_INT32(fActualSize);
	return fVirtio->write_device_config(fVirtioDevice,
		offsetof(struct virtio_balloon_config, actual),
		&actualSize, sizeof(actualSize));
}
