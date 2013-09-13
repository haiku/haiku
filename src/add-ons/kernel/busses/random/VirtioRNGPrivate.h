/*
 * Copyright 2013, Jérôme Duval, korli@users.berlios.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef VIRTIO_RNG_PRIVATE_H
#define VIRTIO_RNG_PRIVATE_H


#include <condition_variable.h>
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


#define BUFFER_SIZE 16


class VirtioRNGDevice {
public:
								VirtioRNGDevice(device_node* node);
								~VirtioRNGDevice();

			status_t			InitCheck();

			status_t			Read(void* buffer, size_t* numBytes);


private:
	static	void				_RequestCallback(void* driverCookie,
									void *cookie);
			void				_RequestInterrupt();

			device_node*		fNode;

			virtio_device_interface* fVirtio;
			virtio_device*		fVirtioDevice;

			status_t			fStatus;
			uint32				fFeatures;
			::virtio_queue		fVirtioQueue;

			physical_entry 		fEntry;
			uint8				fBuffer[BUFFER_SIZE];
			uint32				fOffset;

			spinlock			fInterruptLock;
			ConditionVariable	fInterruptCondition;
			ConditionVariableEntry fInterruptConditionEntry;
			bool				fExpectsInterrupt;
};


#endif // VIRTIO_RNG_PRIVATE_H
