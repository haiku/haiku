/*
 * Copyright 2021, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "VirtioDevice.h"

#include <malloc.h>
#include <string.h>
#include <new>

#include <KernelExport.h>
#include <kernel.h>
#include <debug.h>


static inline void
SetLowHi(vuint32 &low, vuint32 &hi, uint64 val)
{
	low = (uint32)val;
	hi  = (uint32)(val >> 32);
}


// #pragma mark - VirtioQueue


VirtioQueue::VirtioQueue(VirtioDevice *dev, int32 id)
	:
	fDev(dev),
	fId(id),
	fAllocatedDescs(0),
	fQueueHandler(NULL),
	fQueueHandlerCookie(NULL)
{
}


VirtioQueue::~VirtioQueue()
{
}


status_t
VirtioQueue::Init(uint16 requestedSize)
{
	fDev->fRegs->queueSel = fId;
	TRACE("queueNumMax: %d\n", fDev->fRegs->queueNumMax);
	fQueueLen = fDev->fRegs->queueNumMax;
	if (requestedSize != 0 && requestedSize > fQueueLen)
		return B_BUFFER_OVERFLOW;

	fDescCount = fQueueLen;
	fDev->fRegs->queueNum = fQueueLen;
	fLastUsed = 0;

	size_t queueMemSize = 0;
	size_t descsOffset = queueMemSize;
	queueMemSize += ROUNDUP(sizeof(VirtioDesc) * fDescCount, B_PAGE_SIZE);

	size_t availOffset = queueMemSize;
	queueMemSize += ROUNDUP(sizeof(VirtioAvail) + sizeof(uint16) * fQueueLen, B_PAGE_SIZE);

	size_t usedOffset = queueMemSize;
	queueMemSize += ROUNDUP(sizeof(VirtioUsed) + sizeof(VirtioUsedItem) * fQueueLen, B_PAGE_SIZE);

	uint8* queueMem = NULL;
	fArea.SetTo(create_area("VirtIO Queue", (void**)&queueMem,
		B_ANY_KERNEL_ADDRESS, queueMemSize, B_CONTIGUOUS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA));

	if (!fArea.IsSet()) {
		ERROR("can't create area: %08" B_PRIx32, fArea.Get());
		return fArea.Get();
	}

	physical_entry pe;
	status_t res = get_memory_map(queueMem, queueMemSize, &pe, 1);
	if (res < B_OK) {
		ERROR("get_memory_map failed");
		return res;
	}

	TRACE("queueMem: %p\n", queueMem);

	memset(queueMem, 0, queueMemSize);

	fDescs = (VirtioDesc*) (queueMem + descsOffset);
	fAvail = (VirtioAvail*)(queueMem + availOffset);
	fUsed  = (VirtioUsed*) (queueMem + usedOffset);

	if (fDev->fRegs->version >= 2) {
		phys_addr_t descsPhys = (addr_t)fDescs - (addr_t)queueMem + pe.address;
		phys_addr_t availPhys = (addr_t)fAvail - (addr_t)queueMem + pe.address;
		phys_addr_t usedPhys  = (addr_t)fUsed  - (addr_t)queueMem + pe.address;

		SetLowHi(fDev->fRegs->queueDescLow,  fDev->fRegs->queueDescHi,  descsPhys);
		SetLowHi(fDev->fRegs->queueAvailLow, fDev->fRegs->queueAvailHi, availPhys);
		SetLowHi(fDev->fRegs->queueUsedLow,  fDev->fRegs->queueUsedHi,  usedPhys);
	}

	res = fAllocatedDescs.Resize(fDescCount);
	if (res < B_OK)
		return res;

	fCookies.SetTo(new(std::nothrow) void*[fDescCount]);
	if (!fCookies.IsSet())
		return B_NO_MEMORY;

	if (fDev->fRegs->version == 1) {
		uint32_t pfn = pe.address / B_PAGE_SIZE;
		fDev->fRegs->queueAlign = B_PAGE_SIZE;
		fDev->fRegs->queuePfn = pfn;
	} else {
		fDev->fRegs->queueReady = 1;
	}

	return B_OK;
}


int32
VirtioQueue::AllocDesc()
{
	int32 idx = fAllocatedDescs.GetLowestClear();
	if (idx < 0)
		return -1;

	fAllocatedDescs.Set(idx);
	return idx;
}


void
VirtioQueue::FreeDesc(int32 idx)
{
	fAllocatedDescs.Clear(idx);
}


status_t
VirtioQueue::Enqueue(const physical_entry* vector,
	size_t readVectorCount, size_t writtenVectorCount,
	void* cookie)
{
	int32 firstDesc = -1, lastDesc = -1;
	size_t count = readVectorCount + writtenVectorCount;

	if (count == 0)
		return B_OK;

	for (size_t i = 0; i < count; i++) {
		int32 desc = AllocDesc();

		if (desc < 0) {
			ERROR("no free virtio descs, queue: %p\n", this);

			if (firstDesc >= 0) {
				desc = firstDesc;
				while (kVringDescFlagsNext & fDescs[desc].flags) {
					int32_t nextDesc = fDescs[desc].next;
					FreeDesc(desc);
					desc = nextDesc;
				}
				FreeDesc(desc);
			}

			return B_WOULD_BLOCK;
		}

		if (i == 0) {
			firstDesc = desc;
		} else {
			fDescs[lastDesc].flags |= kVringDescFlagsNext;
			fDescs[lastDesc].next = desc;
		}
		fDescs[desc].addr = vector[i].address;
		fDescs[desc].len = vector[i].size;
		fDescs[desc].flags = 0;
		fDescs[desc].next = 0;
		if (i >= readVectorCount)
			fDescs[desc].flags |= kVringDescFlagsWrite;

		lastDesc = desc;
	}

	int32_t idx = fAvail->idx & (fQueueLen - 1);
	fCookies[firstDesc] = cookie;
	fAvail->ring[idx] = firstDesc;
	fAvail->idx++;
	fDev->fRegs->queueNotify = fId;

	return B_OK;
}


bool
VirtioQueue::Dequeue(void** _cookie, uint32* _usedLength)
{
	fDev->fRegs->queueSel = fId;

	if (fUsed->idx == fLastUsed)
		return false;

	int32_t desc = fUsed->ring[fLastUsed & (fQueueLen - 1)].id;

	if (_cookie != NULL)
		*_cookie = fCookies[desc];
	fCookies[desc] = NULL;

	if (_usedLength != NULL)
		*_usedLength = fUsed->ring[fLastUsed & (fQueueLen - 1)].len;

	while (kVringDescFlagsNext & fDescs[desc].flags) {
		int32_t nextDesc = fDescs[desc].next;
		FreeDesc(desc);
		desc = nextDesc;
	}
	FreeDesc(desc);
	fLastUsed++;

	return true;
}


// #pragma mark - VirtioIrqHandler


VirtioIrqHandler::VirtioIrqHandler(VirtioDevice* dev)
	:
	fDev(dev)
{
	fReferenceCount = 0;
}


void
VirtioIrqHandler::FirstReferenceAcquired()
{
	install_io_interrupt_handler(fDev->fIrq, Handle, fDev, 0);
}


void
VirtioIrqHandler::LastReferenceReleased()
{
	remove_io_interrupt_handler(fDev->fIrq, Handle, fDev);
}


int32
VirtioIrqHandler::Handle(void* data)
{
	// TRACE("VirtioIrqHandler::Handle(%p)\n", data);
	VirtioDevice* dev = (VirtioDevice*)data;

	if ((kVirtioIntQueue & dev->fRegs->interruptStatus) != 0) {
		for (int32 i = 0; i < dev->fQueueCnt; i++) {
			VirtioQueue* queue = dev->fQueues[i].Get();
			if (queue->fUsed->idx != queue->fLastUsed
				&& queue->fQueueHandler != NULL) {
				queue->fQueueHandler(dev->fConfigHandlerCookie,
					queue->fQueueHandlerCookie);
				}
		}
		dev->fRegs->interruptAck = kVirtioIntQueue;
	}

	if ((kVirtioIntConfig & dev->fRegs->interruptStatus) != 0) {
		if (dev->fConfigHandler != NULL)
			dev->fConfigHandler(dev->fConfigHandlerCookie);

		dev->fRegs->interruptAck = kVirtioIntConfig;
	}

	return B_HANDLED_INTERRUPT;
}


// #pragma mark - VirtioDevice


VirtioDevice::VirtioDevice()
	:
	fRegs(NULL),
	fQueueCnt(0),
	fIrqHandler(this),
	fConfigHandler(NULL),
	fConfigHandlerCookie(NULL)
{
}


status_t
VirtioDevice::Init(phys_addr_t regs, size_t regsLen, int32 irq, int32 queueCnt)
{
	fRegsArea.SetTo(map_physical_memory("Virtio MMIO",
		regs, regsLen, B_ANY_KERNEL_ADDRESS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
		(void**)&fRegs));

	if (!fRegsArea.IsSet())
		return fRegsArea.Get();

	fIrq = irq;

	// Reset
	fRegs->status = 0;

	return B_OK;
}
