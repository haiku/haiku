/*
 * Copyright 2013, Jérôme Duval, korli@users.berlios.de.
 * Distributed under the terms of the MIT License.
 */


#include "VirtioRNGPrivate.h"

#include <new>
#include <stdlib.h>
#include <string.h>

#include <util/AutoLock.h>


const char *
get_feature_name(uint32 feature)
{
	switch (feature) {
	}
	return NULL;
}


VirtioRNGDevice::VirtioRNGDevice(device_node *node)
	:
	fNode(node),
	fVirtio(NULL),
	fVirtioDevice(NULL),
	fStatus(B_NO_INIT),
	fOffset(BUFFER_SIZE),
	fExpectsInterrupt(false)
{
	CALLED();

	B_INITIALIZE_SPINLOCK(&fInterruptLock);
	fInterruptCondition.Init(this, "virtio rng transfer");

	get_memory_map(fBuffer, BUFFER_SIZE, &fEntry, 1);

	// get the Virtio device from our parent's parent
	device_node *parent = gDeviceManager->get_parent_node(node);
	device_node *virtioParent = gDeviceManager->get_parent_node(parent);
	gDeviceManager->put_node(parent);

	gDeviceManager->get_driver(virtioParent, (driver_module_info **)&fVirtio,
		(void **)&fVirtioDevice);
	gDeviceManager->put_node(virtioParent);

	fVirtio->negotiate_features(fVirtioDevice,
		0, &fFeatures, &get_feature_name);

	fStatus = fVirtio->alloc_queues(fVirtioDevice, 1, &fVirtioQueue);
	if (fStatus != B_OK) {
		ERROR("queue allocation failed (%s)\n", strerror(fStatus));
		return;
	}

	fStatus = fVirtio->setup_interrupt(fVirtioDevice, NULL, this);
	if (fStatus != B_OK) {
		ERROR("interrupt setup failed (%s)\n", strerror(fStatus));
		return;
	}

	fStatus = fVirtio->queue_setup_interrupt(fVirtioQueue, _RequestCallback,
		this);
	if (fStatus != B_OK) {
		ERROR("queue interrupt setup failed (%s)\n", strerror(fStatus));
		return;
	}
}


VirtioRNGDevice::~VirtioRNGDevice()
{
}


status_t
VirtioRNGDevice::InitCheck()
{
	return fStatus;
}


status_t
VirtioRNGDevice::Read(void* _buffer, size_t* _numBytes)
{
	CALLED();

	if (fOffset >= BUFFER_SIZE) {
		{
			InterruptsSpinLocker locker(fInterruptLock);
			fExpectsInterrupt = true;
			fInterruptCondition.Add(&fInterruptConditionEntry);
		}
		status_t result = fVirtio->queue_request(fVirtioQueue, NULL, &fEntry,
			NULL);
		if (result != B_OK) {
			ERROR("queueing failed (%s)\n", strerror(result));
			return result;
		}

		result = fInterruptConditionEntry.Wait(B_CAN_INTERRUPT);

		{
			InterruptsSpinLocker locker(fInterruptLock);
			fExpectsInterrupt = false;
		}

		if (result == B_OK) {
			fOffset = 0;
		} else if (result != B_INTERRUPTED) {
			ERROR("request failed (%s)\n", strerror(result));
		}
	}

	if (fOffset < BUFFER_SIZE) {
		size_t size = min_c(BUFFER_SIZE - fOffset, *_numBytes);
		memcpy(_buffer, fBuffer + fOffset, size);
		fOffset += size;
		*_numBytes = size;
	} else
		*_numBytes = 0;
	return B_OK;
}


void
VirtioRNGDevice::_RequestCallback(void* driverCookie, void* cookie)
{
	VirtioRNGDevice* device = (VirtioRNGDevice*)driverCookie;

	while (device->fVirtio->queue_dequeue(device->fVirtioQueue, NULL, NULL))
		;

	device->_RequestInterrupt();
}


void
VirtioRNGDevice::_RequestInterrupt()
{
	SpinLocker locker(fInterruptLock);
	fInterruptCondition.NotifyAll();
}
