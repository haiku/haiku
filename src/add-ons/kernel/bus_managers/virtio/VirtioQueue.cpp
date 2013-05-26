/*
 * Copyright 2013, Jérôme Duval, korli@users.berlios.de.
 * Distributed under the terms of the MIT License.
 */


#include "VirtioPrivate.h"


static inline uint32
round_to_pagesize(uint32 size)
{
	return (size + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);
}


area_id
alloc_mem(void **virt, phys_addr_t *phy, size_t size, uint32 protection,
	const char *name)
{
	physical_entry pe;
	void * virtadr;
	area_id areaid;
	status_t rv;

	TRACE("allocating %ld bytes for %s\n", size, name);

	size = round_to_pagesize(size);
	areaid = create_area(name, &virtadr, B_ANY_KERNEL_ADDRESS, size,
		B_CONTIGUOUS, protection);
	if (areaid < B_OK) {
		ERROR("couldn't allocate area %s\n", name);
		return B_ERROR;
	}
	rv = get_memory_map(virtadr, size, &pe, 1);
	if (rv < B_OK) {
		delete_area(areaid);
		ERROR("couldn't get mapping for %s\n", name);
		return B_ERROR;
	}
	if (virt)
		*virt = virtadr;
	if (phy)
		*phy = pe.address;
	TRACE("area = %" B_PRId32 ", size = %ld, virt = %p, phy = %#" B_PRIxPHYSADDR "\n",
		areaid, size, virtadr, pe.address);
	return areaid;
}


class TransferDescriptor {
public:
								TransferDescriptor(uint16 size, 
									virtio_callback_func callback,
									void *callbackCookie);
								~TransferDescriptor();

			void				Callback();
			uint16				Size() { return fDescriptorCount; }
private:
			void*				fCookie;
			virtio_callback_func fCallback;
			struct vring_desc* 	fIndirect;
			size_t 				fAreaSize;
			area_id				fArea;
			uint16				fDescriptorCount;
};


TransferDescriptor::TransferDescriptor(uint16 size, 
	virtio_callback_func callback, void *callbackCookie)
	: fCookie(callbackCookie),
	fCallback(callback),
	fDescriptorCount(size)
{
}


TransferDescriptor::~TransferDescriptor()
{
}


void
TransferDescriptor::Callback()
{
	if (fCallback != NULL)
		fCallback(fCookie);
}


//	#pragma mark -


VirtioQueue::VirtioQueue(VirtioDevice* device, uint16 queueNumber,
	uint16 ringSize)
	:
	fDevice(device),
	fQueueNumber(queueNumber),
	fRingSize(ringSize),
	fRingFree(ringSize),
	fRingHeadIndex(0),
	fRingUsedIndex(0),
	fStatus(B_OK)
{
	fDescriptors = new(std::nothrow) TransferDescriptor*[fRingSize];
	if (fDescriptors == NULL) {
		fStatus = B_NO_MEMORY;
		return;
	}

	uint8* virtAddr;
	phys_addr_t physAddr;
	fAreaSize = vring_size(fRingSize, device->Alignment());
	fArea = alloc_mem((void **)&virtAddr, &physAddr, fAreaSize, 0,
		"virtqueue");
	if (fArea < B_OK) {
		fStatus = fArea;
		return;
	}
	memset(virtAddr, 0, fAreaSize);
	vring_init(&fRing, fRingSize, virtAddr, device->Alignment());

	for (uint16 i = 0; i < fRingSize - 1; i++)
		fRing.desc[i].next = i + 1;
	fRing.desc[fRingSize - 1].next = UINT16_MAX;
	
	DisableInterrupt();

	device->SetupQueue(fQueueNumber, physAddr);
}


VirtioQueue::~VirtioQueue()
{
	delete_area(fArea);
}


void
VirtioQueue::DisableInterrupt()
{
	/*if ((fDevice->Features() & VIRTIO_FEATURE_RING_EVENT_IDX) == 0)
		fRing.avail->flags |= VRING_AVAIL_F_NO_INTERRUPT;*/
}


void
VirtioQueue::EnableInterrupt()
{
	/*if ((fDevice->Features() & VIRTIO_FEATURE_RING_EVENT_IDX) == 0)
		fRing.avail->flags &= ~VRING_AVAIL_F_NO_INTERRUPT;*/
}


void
VirtioQueue::NotifyHost()
{
	fDevice->NotifyQueue(fQueueNumber);
}


status_t
VirtioQueue::Interrupt()
{
	CALLED();
	DisableInterrupt();

	while (fRingUsedIndex != fRing.used->idx)
		Finish();
	
	EnableInterrupt();
	return B_OK;
}


void
VirtioQueue::Finish()
{
	TRACE("Finish() fRingUsedIndex: %u\n", fRingUsedIndex);

	uint16 usedIndex = fRingUsedIndex++ & (fRingSize - 1);
	TRACE("Finish() usedIndex: %u\n", usedIndex);
	struct vring_used_elem *element = &fRing.used->ring[usedIndex];
	uint16 descriptorIndex = element->id;
		// uint32 length = element->len;

	fDescriptors[descriptorIndex]->Callback();
	uint16 size = fDescriptors[descriptorIndex]->Size();
	fRingFree += size;
	size--;

	uint16 index = descriptorIndex;
	while ((fRing.desc[index].flags & VRING_DESC_F_NEXT) != 0) {
		index = fRing.desc[index].next;
		size--;
	}
	
	if (size > 0)
		panic("VirtioQueue::Finish() descriptors left %d\n", size);

	// TODO TransferDescriptors are leaked, can't delete in interrupt handler.

	fRing.desc[index].next = fRingHeadIndex;
	fRingHeadIndex = descriptorIndex;
	TRACE("Finish() fRingHeadIndex: %u\n", fRingHeadIndex);
}


status_t
VirtioQueue::QueueRequest(const physical_entry* vector, size_t readVectorCount,
	size_t writtenVectorCount, virtio_callback_func callback,
	void *callbackCookie)
{
	CALLED();
	size_t count = readVectorCount + writtenVectorCount;
	if (count < 1)
		return B_BAD_VALUE;
	if ((fDevice->Features() & VIRTIO_FEATURE_RING_INDIRECT_DESC) != 0) {
		return QueueRequestIndirect(vector, readVectorCount, 
			writtenVectorCount, callback, callbackCookie);
	}

	if (count > fRingFree)
		return B_BUSY;

	uint16 insertIndex = fRingHeadIndex;
	fDescriptors[insertIndex] = new(std::nothrow) TransferDescriptor(count,
		callback, callbackCookie);
	if (fDescriptors[insertIndex] == NULL)
		return B_NO_MEMORY;
	
	// enqueue
	uint16 index = QueueVector(insertIndex, fRing.desc, vector,
		readVectorCount, writtenVectorCount);

	fRingHeadIndex = index;
	fRingFree -= count;

	UpdateAvailable(insertIndex);

	NotifyHost();
	
	return B_OK;
}


status_t
VirtioQueue::QueueRequestIndirect(const physical_entry* vector,
	size_t readVectorCount,	size_t writtenVectorCount,
	virtio_callback_func callback, void *callbackCookie)
{
	// TODO
	return B_OK;
}


void
VirtioQueue::UpdateAvailable(uint16 index)
{
	CALLED();
	uint16 available = fRing.avail->idx & (fRingSize - 1);
	fRing.avail->ring[available] = index;
	fRing.avail->idx++;
}


uint16
VirtioQueue::QueueVector(uint16 insertIndex, struct vring_desc *desc,
	const physical_entry* vector, size_t readVectorCount,
	size_t writtenVectorCount)
{
	CALLED();
	uint16 index = insertIndex;
	size_t total = readVectorCount + writtenVectorCount;
	for (size_t i = 0; i < total; i++) {
		desc[index].addr = vector[i].address;
		desc[index].len =  vector[i].size;
		desc[index].flags = 0;
		if (i >= readVectorCount)
			desc[index].flags |= VRING_DESC_F_WRITE;
		if (i < total - 1)
			desc[index].flags |= VRING_DESC_F_NEXT;
		index = desc[index].next;
	}

	return index;
}
