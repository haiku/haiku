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
get_feature_name(uint64 feature)
{
	switch (feature) {
	}
	return NULL;
}


VirtioRNGDevice::VirtioRNGDevice(device_node *node)
	:
	fVirtio(NULL),
	fVirtioDevice(NULL),
	fStatus(B_NO_INIT),
	fExpectsInterrupt(false),
	fDPCHandle(NULL)
{
	CALLED();

	B_INITIALIZE_SPINLOCK(&fInterruptLock);
	fInterruptCondition.Init(this, "virtio rng transfer");

	// get the Virtio device from our parent's parent
	device_node *virtioParent = gDeviceManager->get_parent_node(node);

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

	fStatus = gDPC->new_dpc_queue(&fDPCHandle, "virtio_rng timer",
		B_LOW_PRIORITY);
	if (fStatus != B_OK) {
		ERROR("dpc setup failed (%s)\n", strerror(fStatus));
		return;
	}

	if (fStatus == B_OK) {
		fTimer.user_data = this;
		fStatus = add_timer(&fTimer, &HandleTimerHook, 300 * 1000 * 1000, B_PERIODIC_TIMER);
		if (fStatus != B_OK)
			ERROR("timer setup failed (%s)\n", strerror(fStatus));
	}

	// trigger also now
	gDPC->queue_dpc(fDPCHandle, HandleDPC, this);
}


VirtioRNGDevice::~VirtioRNGDevice()
{
	cancel_timer(&fTimer);
	gDPC->delete_dpc_queue(fDPCHandle);

}


status_t
VirtioRNGDevice::InitCheck()
{
	return fStatus;
}


status_t
VirtioRNGDevice::_Enqueue()
{
	CALLED();

	uint64 value = 0;
	physical_entry entry;
	get_memory_map(&value, sizeof(value), &entry, 1);

	{
		InterruptsSpinLocker locker(fInterruptLock);
		fExpectsInterrupt = true;
		fInterruptCondition.Add(&fInterruptConditionEntry);
	}
	status_t result = fVirtio->queue_request(fVirtioQueue, NULL, &entry, NULL);
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
		if (value != 0) {
			gRandom->queue_randomness(value);
			TRACE("enqueue %" B_PRIx64 "\n", value);
		}
	} else if (result != B_OK && result != B_INTERRUPTED) {
		ERROR("request failed (%s)\n", strerror(result));
	}

	return result;
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


/*static*/ int32
VirtioRNGDevice::HandleTimerHook(struct timer* timer)
{
	VirtioRNGDevice* device = reinterpret_cast<VirtioRNGDevice*>(timer->user_data);

	gDPC->queue_dpc(device->fDPCHandle, HandleDPC, device);
	return B_HANDLED_INTERRUPT;
}


/*static*/ void
VirtioRNGDevice::HandleDPC(void *arg)
{
	VirtioRNGDevice* device = reinterpret_cast<VirtioRNGDevice*>(arg);
	device->_Enqueue();
}

