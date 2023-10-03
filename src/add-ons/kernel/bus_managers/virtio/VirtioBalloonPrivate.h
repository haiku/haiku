/*
 * Copyright 2018, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef VIRTIO_BALLOON_PRIVATE_H
#define VIRTIO_BALLOON_PRIVATE_H


#include <condition_variable.h>
#include <lock.h>
#include <virtio.h>
#include <vm/vm_page.h>


//#define TRACE_VIRTIO_BALLOON
#ifdef TRACE_VIRTIO_BALLOON
#	define TRACE(x...) dprintf("virtio_balloon: " x)
#else
#	define TRACE(x...) ;
#endif
#define ERROR(x...)			dprintf("\33[33mvirtio_rng:\33[0m " x)
#define CALLED() 			TRACE("CALLED %s\n", __PRETTY_FUNCTION__)


extern device_manager_info* gDeviceManager;


#define PAGES_COUNT	256


struct PageInfo : DoublyLinkedListLinkImpl<PageInfo> {
	vm_page *page;
};


typedef DoublyLinkedList<PageInfo> PageInfoList;



class VirtioBalloonDevice {
public:
								VirtioBalloonDevice(device_node* node);
								~VirtioBalloonDevice();

			status_t			InitCheck();

private:
	static	void				_ConfigCallback(void* driverCookie);
	static	void				_QueueCallback(void* driverCookie,
									void *cookie);

	static	int32				_ThreadEntry(void *data);
			int32				_Thread();

			uint32				_DesiredSize();
			status_t			_UpdateSize();

			device_node*		fNode;

			virtio_device_interface* fVirtio;
			virtio_device*		fVirtioDevice;

			status_t			fStatus;
			uint64				fFeatures;
			::virtio_queue		fVirtioQueues[2];

			physical_entry 		fEntry;
			uint32				fBuffer[PAGES_COUNT];

			thread_id			fThread;
			uint32				fDesiredSize;
			uint32				fActualSize;

			spinlock			fInterruptLock;
			ConditionVariable	fQueueCondition;
			ConditionVariable	fConfigCondition;
			bool				fRunning;

			PageInfoList		fPages;
};


#endif // VIRTIO_BALLOON_PRIVATE_H
