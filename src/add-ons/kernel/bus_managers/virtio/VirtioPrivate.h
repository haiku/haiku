/*
 * Copyright 2013, Jérôme Duval, korli@users.berlios.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef VIRTIO_PRIVATE_H
#define VIRTIO_PRIVATE_H


#include <new>
#include <stdio.h>
#include <string.h>

#include <lock.h>
#include <util/AutoLock.h>
#include <virtio.h>

#include "virtio_ring.h"


//#define VIRTIO_TRACE
#ifdef VIRTIO_TRACE
#	define TRACE(x...)		dprintf("\33[33mvirtio:\33[0m " x)
#else
#	define TRACE(x...)
#endif
#define TRACE_ALWAYS(x...)	dprintf("\33[33mvirtio:\33[0m " x)
#define ERROR(x...)			TRACE_ALWAYS(x)
#define CALLED() 			TRACE("CALLED %s\n", __PRETTY_FUNCTION__)


#define VIRTIO_SIM_MODULE_NAME		"bus_managers/virtio/sim/driver_v1"


class VirtioDevice;
class VirtioQueue;

extern device_manager_info *gDeviceManager;


class VirtioDevice {
public:
								VirtioDevice(device_node *node);
								~VirtioDevice();

			status_t			InitCheck();
			uint32				ID() const { return fID; }

			status_t 			NegotiateFeatures(uint32 supported,
									uint32* negotiated,
									const char* (*get_feature_name)(uint32));
			status_t 			ClearFeature(uint32 feature);

			status_t			ReadDeviceConfig(uint8 offset, void* buffer,
									size_t bufferSize);
			status_t			WriteDeviceConfig(uint8 offset,
									const void* buffer, size_t bufferSize);

			status_t			AllocateQueues(size_t count,
									virtio_queue *queues);
			void				FreeQueues();
			status_t			SetupInterrupt(virtio_intr_func config_handler,
									void *driverCookie);
			status_t			FreeInterrupts();

			uint16				Alignment() const { return fAlignment; }
			uint32				Features() const { return fFeatures; }

			void*				DriverCookie() { return fDriverCookie; }

			status_t			SetupQueue(uint16 queueNumber,
									phys_addr_t physAddr);
			void				NotifyQueue(uint16 queueNumber);

			status_t			QueueInterrupt(uint16 queueNumber);
			status_t			ConfigInterrupt();

private:
			void				_DumpFeatures(const char* title,
									uint32 features,
									const char* (*get_feature_name)(uint32));
			void				_DestroyQueues(size_t count);



			device_node *		fNode;
			uint32				fID;
			virtio_sim_interface *fController;
			void *				fCookie;
			status_t			fStatus;
			VirtioQueue**		fQueues;
			size_t				fQueueCount;
			uint32				fFeatures;
			uint16				fAlignment;

			virtio_intr_func	fConfigHandler;
			void* 				fDriverCookie;
};


class TransferDescriptor;


class VirtioQueue {
public:
								VirtioQueue(VirtioDevice *device,
									uint16 queueNumber, uint16 ringSize);
								~VirtioQueue();
			status_t			InitCheck() { return fStatus; }

			void				NotifyHost();
			status_t			Interrupt();

			bool				IsFull() const { return fRingFree == 0; }
			bool				IsEmpty() const { return fRingFree == fRingSize; }
			uint16				Size() const { return fRingSize; }

			VirtioDevice*		Device() { return fDevice; }

			status_t			QueueRequest(const physical_entry* vector,
									size_t readVectorCount,
									size_t writtenVectorCount,
									void *cookie);
			status_t			QueueRequestIndirect(
									const physical_entry* vector,
									size_t readVectorCount,
									size_t writtenVectorCount,
									void *cookie);

			status_t			SetupInterrupt(virtio_callback_func handler,
									void *cookie);

			void				EnableInterrupt();
			void				DisableInterrupt();

			bool				Dequeue(void** _cookie = NULL,
									uint32* _usedLength = NULL);

private:
			void				UpdateAvailable(uint16 index);
			uint16				QueueVector(uint16 insertIndex,
									struct vring_desc *desc,
									const physical_entry* vector,
									size_t readVectorCount,
									size_t writtenVectorCount);

			VirtioDevice*		fDevice;
			uint16				fQueueNumber;
			uint16				fRingSize;
			uint16				fRingFree;

			struct vring		fRing;
			uint16				fRingHeadIndex;
			uint16				fRingUsedIndex;
			status_t			fStatus;
			size_t 				fAreaSize;
			area_id				fArea;

			uint16				fIndirectMaxSize;

			TransferDescriptor**	fDescriptors;

			virtio_callback_func fCallback;
			void*				fCookie;

};

#endif // VIRTIO_PRIVATE_H
