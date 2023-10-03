/*
 * Copyright 2013, Jérôme Duval, korli@users.berlios.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef VIRTIO_RNG_PRIVATE_H
#define VIRTIO_RNG_PRIVATE_H


#include <condition_variable.h>
#include <dpc.h>
#include <lock.h>
#include "random.h"
#include <virtio.h>


//#define TRACE_VIRTIO_RNG
#ifdef TRACE_VIRTIO_RNG
#	define TRACE(x...) dprintf("virtio_rng: " x)
#else
#	define TRACE(x...) ;
#endif
#define ERROR(x...)			dprintf("\33[33mvirtio_rng:\33[0m " x)
#define CALLED() 			TRACE("CALLED %s\n", __PRETTY_FUNCTION__)


extern device_manager_info* gDeviceManager;
extern random_for_controller_interface *gRandom;
extern dpc_module_info *gDPC;


class VirtioRNGDevice {
public:
								VirtioRNGDevice(device_node* node);
								~VirtioRNGDevice();

			status_t			InitCheck();

protected:
	static	int32				HandleTimerHook(struct timer* timer);
	static	void				HandleDPC(void* arg);

private:
	static	void				_RequestCallback(void* driverCookie,
									void *cookie);
			void				_RequestInterrupt();
			status_t			_Enqueue();

			virtio_device_interface* fVirtio;
			virtio_device*		fVirtioDevice;

			status_t			fStatus;
			uint64				fFeatures;
			::virtio_queue		fVirtioQueue;

			spinlock			fInterruptLock;
			ConditionVariable	fInterruptCondition;
			ConditionVariableEntry fInterruptConditionEntry;
			bool				fExpectsInterrupt;

			timer				fTimer;
			void*				fDPCHandle;
};


#endif // VIRTIO_RNG_PRIVATE_H
