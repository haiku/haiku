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
								TransferDescriptor(VirtioQueue* queue,
									uint16 indirectMaxSize);
								~TransferDescriptor();

			status_t			InitCheck() { return fStatus; }

			void				Callback();
			uint16				Size() { return fDescriptorCount; }
			void				SetTo(uint16 size,
									virtio_callback_func callback,
									void *callbackCookie);
			void				Unset();
			struct vring_desc*	Indirect() { return fIndirect; }
			phys_addr_t			PhysAddr() { return fPhysAddr; }
private:
			status_t			fStatus;
			VirtioQueue*		fQueue;
			void*				fCookie;
			virtio_callback_func fCallback;

			struct vring_desc* 	fIndirect;
			size_t 				fAreaSize;
			area_id				fArea;
			phys_addr_t 		fPhysAddr;
			uint16				fDescriptorCount;
};


TransferDescriptor::TransferDescriptor(VirtioQueue* queue, uint16 indirectMaxSize)
	: fQueue(queue),
	fCookie(NULL),
	fCallback(NULL),
	fIndirect(NULL),
	fAreaSize(0),
	fArea(-1),
	fPhysAddr(0),
	fDescriptorCount(0)
{
	fStatus = B_OK;
	struct vring_desc* virtAddr;
	phys_addr_t physAddr;

	if (indirectMaxSize > 0) {
		fAreaSize = indirectMaxSize * sizeof(struct vring_desc);
		fArea = alloc_mem((void **)&virtAddr, &physAddr, fAreaSize, 0,
			"virtqueue");
		if (fArea < B_OK) {
			fStatus = fArea;
			return;
		}
		memset(virtAddr, 0, fAreaSize);
		fIndirect = virtAddr;
		fPhysAddr = physAddr;

		for (uint16 i = 0; i < indirectMaxSize - 1; i++)
			fIndirect[i].next = i + 1;
		fIndirect[indirectMaxSize - 1].next = UINT16_MAX;
	}
}


TransferDescriptor::~TransferDescriptor()
{
	if (fArea > B_OK)
		delete_area(fArea);
}


void
TransferDescriptor::Callback()
{
	if (fCallback != NULL)
		fCallback(fQueue->Device()->DriverCookie(), fCookie);
}


void
TransferDescriptor::SetTo(uint16 size, virtio_callback_func callback,
	void *callbackCookie)
{
	fCookie = callbackCookie;
	fCallback = callback;
	fDescriptorCount = size;
}


void
TransferDescriptor::Unset()
{
	fCookie = NULL;
	fCallback = NULL;
	fDescriptorCount = 0;
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
	fStatus(B_OK),
	fIndirectMaxSize(0)
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

	if ((fDevice->Features() & VIRTIO_FEATURE_RING_INDIRECT_DESC) != 0)
		fIndirectMaxSize = 128;

	for (uint16 i = 0; i < fRingSize; i++) {
		fDescriptors[i] = new TransferDescriptor(this, fIndirectMaxSize);
		if (fDescriptors[i] == NULL || fDescriptors[i]->InitCheck() != B_OK) {
			fStatus = B_NO_MEMORY;
			return;
		}
	}

	DisableInterrupt();

	device->SetupQueue(fQueueNumber, physAddr);
}


VirtioQueue::~VirtioQueue()
{
	delete_area(fArea);
	for (uint16 i = 0; i < fRingSize; i++) {
		delete fDescriptors[i];
	}
	delete[] fDescriptors;
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
	fDescriptors[descriptorIndex]->Unset();
	fRingFree += size;
	size--;

	uint16 index = descriptorIndex;
	while ((fRing.desc[index].flags & VRING_DESC_F_NEXT) != 0) {
		index = fRing.desc[index].next;
		size--;
	}

	if (size > 0)
		panic("VirtioQueue::Finish() descriptors left %d\n", size);

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
	fDescriptors[insertIndex]->SetTo(count, callback,
		callbackCookie);

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
	CALLED();
	size_t count = readVectorCount + writtenVectorCount;
	if (count > fRingFree || count > fIndirectMaxSize)
		return B_BUSY;

	uint16 insertIndex = fRingHeadIndex;
	fDescriptors[insertIndex]->SetTo(1, callback,
		callbackCookie);

	// enqueue
	uint16 index = QueueVector(0, fDescriptors[insertIndex]->Indirect(),
		vector, readVectorCount, writtenVectorCount);

	fRing.desc[insertIndex].addr = fDescriptors[insertIndex]->PhysAddr();
	fRing.desc[insertIndex].len = index * sizeof(struct vring_desc);
	fRing.desc[insertIndex].flags = VRING_DESC_F_INDIRECT;
	fRingHeadIndex = fRing.desc[insertIndex].next;
	fRingFree--;

	UpdateAvailable(insertIndex);

	NotifyHost();

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
