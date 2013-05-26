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
#define ERROR(x...)			dprintf("\33[33mvirtio:\33[0m " x)
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

			status_t 			NegociateFeatures(uint32 supported,
									uint32* negociated,
									const char* (*get_feature_name)(uint32));
	
			status_t			ReadDeviceConfig(uint8 offset, void* buffer,
									size_t bufferSize);
			status_t			WriteDeviceConfig(uint8 offset,
									const void* buffer, size_t bufferSize);

			status_t			AllocateQueues(size_t count,
									virtio_queue *queues);
			status_t			SetupInterrupt(virtio_intr_func config_handler,
									void* configCookie);

			uint16				Alignment() { return fAlignment; }
			uint32				Features() { return fFeatures; }

			status_t			SetupQueue(uint16 queueNumber,
									phys_addr_t physAddr);
			void				NotifyQueue(uint16 queueNumber);

			status_t			QueueInterrupt(uint16 queueNumber);
			status_t			ConfigInterrupt();

private:
			void				DumpFeatures(const char* title,
									uint32 features,
									const char* (*get_feature_name)(uint32));
			

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
			void* 				fConfigCookie;
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

			bool				IsFull() { return fRingFree == 0; }
			bool				IsEmpty() { return fRingFree == fRingSize; }

			status_t			QueueRequest(const physical_entry* vector,
									size_t readVectorCount,
									size_t writtenVectorCount,
									virtio_callback_func callback,
									void *callbackCookie);
			status_t			QueueRequestIndirect(
									const physical_entry* vector,
									size_t readVectorCount,
									size_t writtenVectorCount,
									virtio_callback_func callback,
									void *callbackCookie);
			void				EnableInterrupt();
			void				DisableInterrupt();
			
private:
			void				UpdateAvailable(uint16 index);
			uint16				QueueVector(uint16 insertIndex,
									struct vring_desc *desc,
									const physical_entry* vector,
									size_t readVectorCount,
									size_t writtenVectorCount);
			void				Finish();

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

			TransferDescriptor**	fDescriptors;
};

#endif // VIRTIO_PRIVATE_H
