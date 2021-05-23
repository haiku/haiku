/*
 * Copyright 2021, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _VIRTIODEVICE_H_
#define _VIRTIODEVICE_H_


#include <virtio.h>
#include <virtio_defs.h>
#include <AutoDeleter.h>
#include <AutoDeleterOS.h>
#include <Referenceable.h>
#include <util/AutoLock.h>


//#define TRACE_VIRTIO
#ifdef TRACE_VIRTIO
#	define TRACE(x...) dprintf("virtio_mmio: " x)
#else
#	define TRACE(x...) ;
#endif

#define TRACE_ALWAYS(x...) dprintf("virtio_mmio: " x)
#define ERROR(x...) dprintf("virtio_mmio: " x)


struct VirtioIrqHandler;
struct VirtioDevice;

struct VirtioQueue {
	VirtioDevice *fDev;
	int32 fId;
	size_t fQueueLen;
	AreaDeleter fArea;
	volatile VirtioDesc  *fDescs;
	volatile VirtioAvail *fAvail;
	volatile VirtioUsed  *fUsed;
	ArrayDeleter<uint32> fFreeDescs;
	uint16 fLastUsed;
	ArrayDeleter<void*> fCookies;

	BReference<VirtioIrqHandler> fQueueHandlerRef;
	virtio_callback_func fQueueHandler;
	void *fQueueHandlerCookie;

	VirtioQueue(VirtioDevice *dev, int32 id);
	~VirtioQueue();
	status_t Init();

	int32 AllocDesc();
	void FreeDesc(int32 idx);

	status_t Enqueue(
		const physical_entry* vector,
		size_t readVectorCount, size_t writtenVectorCount,
		void* cookie
	);

	bool Dequeue(void** _cookie, uint32* _usedLength);
};


struct VirtioIrqHandler: public BReferenceable
{
	VirtioDevice* fDev;

	VirtioIrqHandler(VirtioDevice* dev);

	virtual void FirstReferenceAcquired();
	virtual void LastReferenceReleased();

	static int32 Handle(void* data);
};


struct VirtioDevice
{
	AreaDeleter fRegsArea;
	volatile VirtioRegs *fRegs;
	int32 fIrq;
	int32 fQueueCnt;
	ArrayDeleter<ObjectDeleter<VirtioQueue> > fQueues;

	VirtioIrqHandler fIrqHandler;

	BReference<VirtioIrqHandler> fConfigHandlerRef;
	virtio_intr_func fConfigHandler;
	void* fConfigHandlerCookie;

	VirtioDevice();
	status_t Init(phys_addr_t regs, size_t regsLen, int32 irq, int32 queueCnt);
};


#endif	// _VIRTIODEVICE_H_
