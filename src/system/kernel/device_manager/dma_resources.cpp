/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */

#include "dma_resources.h"

#include <kernel.h>
#include <util/AutoLock.h>

#include "io_requests.h"


const size_t kMaxBounceBufferSize = 4 * B_PAGE_SIZE;


DMABuffer*
DMABuffer::Create(size_t count, void* bounceBuffer, addr_t physicalBounceBuffer)
{
	DMABuffer* buffer = (DMABuffer*)malloc(
		sizeof(DMABuffer) + sizeof(iovec) * (count - 1));
	if (buffer == NULL)
		return NULL;

	buffer->fBounceBuffer = bounceBuffer;
	buffer->fPhysicalBounceBuffer = physicalBounceBuffer;
	buffer->fVecCount = count;

	return buffer;
}


void
DMABuffer::SetVecCount(uint32 count)
{
	fVecCount = count;
}


void
DMABuffer::AddVec(void* base, size_t size)
{
	iovec& vec = fVecs[fVecCount++];
	vec.iov_len = size;
}


void
DMABuffer::SetToBounceBuffer(size_t length)
{
	fVecs[0].iov_base = (void*)fPhysicalBounceBuffer;
	fVecs[0].iov_len = length;
	fVecCount = 1;
}


//	#pragma mark -


DMAResource::DMAResource()
{
	mutex_init(&fLock, "dma resource");
}


DMAResource::~DMAResource()
{
	mutex_destroy(&fLock);
	free(fScratchVecs);
}


status_t
DMAResource::Init(const dma_restrictions& restrictions, size_t blockSize,
	uint32 bufferCount)
{
	fRestrictions = restrictions;
	fBlockSize = blockSize == 0 ? 1 : blockSize;
	fBufferCount = bufferCount;
	fBounceBufferSize = 0;

	if (fRestrictions.high_address == 0)
		fRestrictions.high_address = ~(addr_t)0;
	if (fRestrictions.max_segment_count == 0)
		fRestrictions.max_segment_count = 16;
	if (fRestrictions.alignment == 0)
		fRestrictions.alignment = 1;

	if (_NeedsBoundsBuffers()) {
// TODO: Enforce that the bounce buffer size won't cross boundaries.
		fBounceBufferSize = restrictions.max_segment_size;
		if (fBounceBufferSize > kMaxBounceBufferSize)
			fBounceBufferSize = max_c(kMaxBounceBufferSize, fBlockSize);
	}

	fScratchVecs = (iovec*)malloc(
		sizeof(iovec) * fRestrictions.max_segment_count);
	if (fScratchVecs == NULL)
		return B_NO_MEMORY;

	// TODO: create bounce buffers in as few areas as feasible
	for (size_t i = 0; i < fBufferCount; i++) {
		DMABuffer* buffer;
		status_t error = CreateBuffer(fBounceBufferSize, &buffer);
		if (error != B_OK)
			return error;

		fDMABuffers.Add(buffer);
	}

	return B_OK;
}


status_t
DMAResource::CreateBuffer(size_t size, DMABuffer** _buffer)
{
	void* bounceBuffer = NULL;
	addr_t physicalBase = 0;
	area_id area = -1;

	if (size != 0) {
		if (fRestrictions.alignment > B_PAGE_SIZE
			|| fRestrictions.boundary > B_PAGE_SIZE)
			panic("not yet implemented");

		size = ROUNDUP(size, B_PAGE_SIZE);

		bounceBuffer = (void*)fRestrictions.low_address;
// TODO: We also need to enforce the boundary restrictions.
		area = create_area("dma buffer", &bounceBuffer, size,
			B_PHYSICAL_BASE_ADDRESS, B_CONTIGUOUS,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
		if (area < B_OK)
			return area;

		physical_entry entry;
		if (get_memory_map(bounceBuffer, size, &entry, 1) != B_OK) {
			panic("get_memory_map() failed.");
			delete_area(area);
			return B_ERROR;
		}

		physicalBase = (addr_t)entry.address;

		if (fRestrictions.high_address < physicalBase + size) {
			delete_area(area);
			return B_NO_MEMORY;
		}
	}

	DMABuffer* buffer = DMABuffer::Create(fRestrictions.max_segment_count,
		bounceBuffer, physicalBase);
	if (buffer == NULL) {
		delete_area(area);
		return B_NO_MEMORY;
	}

	*_buffer = buffer;
	return B_OK;
}


status_t
DMAResource::TranslateNext(IORequest* request, IOOperation* operation)
{
	IOBuffer* buffer = request->Buffer();
	off_t offset = request->Offset();

	MutexLocker locker(fLock);

	DMABuffer* dmaBuffer = fDMABuffers.RemoveHead();
	if (dmaBuffer == NULL)
		return B_BUSY;

	iovec* vecs = NULL;
	uint32 segmentCount = 0;
	size_t totalLength = min_c(buffer->Length(),
		fRestrictions.max_transfer_size);

	bool partialOperation = (offset & (fBlockSize - 1)) != 0;
	bool needsBounceBuffer = partialOperation;

	if (buffer->IsVirtual()) {
		// Unless we need the bounce buffer anyway, we have to translate the
		// virtual addresses to physical addresses, so we can check the DMA
		// restrictions.
		if (!needsBounceBuffer) {
			size_t transferLeft = totalLength;
			vecs = fScratchVecs;

// TODO: take iteration state of the IORequest into account!
			for (uint32 i = 0; i < buffer->VecCount(); i++) {
				iovec& vec = buffer->VecAt(i);
				size_t size = vec.iov_len;
				if (size > transferLeft)
					size = transferLeft;

				addr_t base = (addr_t)vec.iov_base;
				while (size > 0 && segmentCount
						< fRestrictions.max_segment_count) {
					physical_entry entry;
					get_memory_map((void*)base, size, &entry, 1);

					vecs[segmentCount].iov_base = entry.address;
					vecs[segmentCount].iov_len = entry.size;

					transferLeft -= entry.size;
					segmentCount++;
				}

				if (transferLeft == 0)
					break;
			}

			totalLength -= transferLeft;
		}
	} else {
		// We do already have physical adresses.
		locker.Unlock();
		vecs = buffer->Vecs();
		segmentCount = min_c(buffer->VecCount(),
			fRestrictions.max_segment_count);
	}

//	locker.Lock();

	// check alignment, boundaries, etc. and set vecs in DMA buffer

	size_t dmaLength = 0;
	iovec vec;
	if (vecs != NULL)
		vec = vecs[0];
	for (uint32 i = 0; i < segmentCount;) {
		addr_t base = (addr_t)vec.iov_base;
		size_t length = vec.iov_len;

		if ((base & (fRestrictions.alignment - 1)) != 0) {
			needsBounceBuffer = true;
			break;
		}

		if (((base + length) & (fRestrictions.alignment - 1)) != 0) {
			length = ((base + length) & ~(fRestrictions.alignment - 1)) - base;
			if (length == 0) {
				needsBounceBuffer = true;
				break;
			}
		}

		if (fRestrictions.boundary > 0) {
			addr_t baseBoundary = base / fRestrictions.boundary;
			if (baseBoundary != (base + (length - 1)) / fRestrictions.boundary)
				length = (baseBoundary + 1) * fRestrictions.boundary - base;
		}

		dmaBuffer->AddVec((void*)base, length);
		dmaLength += length;

		if ((vec.iov_len -= length) > 0) {
			vec.iov_base = (void*)((addr_t)vec.iov_base + length);
		} else {
			if (++i < segmentCount)
				vec = vecs[i];
		}
	}

	if (dmaLength < fBlockSize) {
		dmaLength = 0;
		needsBounceBuffer = true;
		partialOperation = true;
	} else if ((dmaLength & (fBlockSize - 1)) != 0) {
		size_t toCut = dmaLength & (fBlockSize - 1);
		dmaLength -= toCut;
		int32 dmaVecCount = dmaBuffer->VecCount();
		for (int32 i = dmaVecCount - 1 && toCut > 0; i >= 0; i--) {
			iovec& vec = dmaBuffer->VecAt(i);
			size_t length = vec.iov_len;
			if (length <= toCut) {
				dmaVecCount--;
				toCut -= length;
			} else {
				vec.iov_len -= toCut;
				break;
			}
		}

		dmaBuffer->SetVecCount(dmaVecCount);
	}

	operation->SetOriginalRange(offset, dmaLength);

	if (needsBounceBuffer) {
		// If the size of the buffer we could transfer is pathologically small,
		// we always use the bounce buffer.
		// TODO: Use a better heuristics than bounce buffer size / 2, Or even
		// better attach the bounce buffer to the DMA buffer.
		if (dmaLength < fBounceBufferSize / 2) {
			if (partialOperation) {
				off_t diff = offset & (fBlockSize - 1);
				offset -= diff;
				dmaLength += diff;
			}

			addr_t base = (addr_t)vecs[0].iov_base;
			size_t length = vecs[0].iov_len;
			if ((base & (fRestrictions.alignment - 1)) != 0) {
				addr_t diff = base - (base & ~(fRestrictions.alignment - 1));
				length += diff;
			}

			dmaLength = max_c(totalLength, fBlockSize);
			dmaLength = (dmaLength + fBlockSize - 1) & ~(fBlockSize - 1);
			dmaLength = min_c(dmaLength, fBounceBufferSize);
			dmaBuffer->SetToBounceBuffer(dmaLength);

			operation->SetRange(offset, dmaLength);
		} else
			needsBounceBuffer = false;
	}

	operation->SetPartialOperation(partialOperation);
	operation->SetRequest(request);
	request->Advance(operation->OriginalLength());

	return B_OK;
}


void
DMAResource::RecycleBuffer(DMABuffer* buffer)
{
	MutexLocker _(fLock);
	fDMABuffers.Add(buffer);
}


bool
DMAResource::_NeedsBoundsBuffers() const
{
	return fRestrictions.alignment > 1
		|| fRestrictions.low_address != 0
		|| fRestrictions.high_address != ~(addr_t)0
		|| fBlockSize > 1;
}




#if 0


status_t
create_dma_resource(restrictions)
{
	// Restrictions are: transfer size, address space, alignment
	// segment min/max size, num segments
}


void
delete_dma_resource(resource)
{
}


dma_buffer_alloc(resource, size)
{
}


dma_buffer_free(buffer)
{
//	Allocates or frees memory in that DMA buffer.
}

#endif	// 0
