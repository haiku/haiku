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
DMABuffer::Create(size_t count, void* bounceBuffer, addr_t physicalBounceBuffer,
	size_t bounceBufferSize)
{
	DMABuffer* buffer = (DMABuffer*)malloc(
		sizeof(DMABuffer) + sizeof(iovec) * (count - 1));
	if (buffer == NULL)
		return NULL;

	buffer->fBounceBuffer = bounceBuffer;
	buffer->fPhysicalBounceBuffer = physicalBounceBuffer;
	buffer->fBounceBufferSize = bounceBufferSize;
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
	vec.iov_base = base;
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
	if (fRestrictions.max_transfer_size == 0)
		fRestrictions.max_transfer_size = ~(size_t)0;
	if (fRestrictions.max_segment_size == 0)
		fRestrictions.max_segment_size = ~(size_t)0;

	if (_NeedsBoundsBuffers()) {
// TODO: Enforce that the bounce buffer size won't cross boundaries.
		fBounceBufferSize = fRestrictions.max_segment_size;
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
		area = create_area("dma buffer", &bounceBuffer, B_PHYSICAL_BASE_ADDRESS,
			size, B_CONTIGUOUS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
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
		bounceBuffer, physicalBase, fBounceBufferSize);
	if (buffer == NULL) {
		delete_area(area);
		return B_NO_MEMORY;
	}

	*_buffer = buffer;
	return B_OK;
}


inline void
DMAResource::_RestrictBoundaryAndSegmentSize(addr_t base, addr_t& length)
{
	if (length > fRestrictions.max_segment_size)
		length = fRestrictions.max_segment_size;
	if (fRestrictions.boundary > 0) {
		addr_t baseBoundary = base / fRestrictions.boundary;
		if (baseBoundary
				!= (base + (length - 1)) / fRestrictions.boundary) {
			length = (baseBoundary + 1) * fRestrictions.boundary - base;
		}
	}
}


status_t
DMAResource::TranslateNext(IORequest* request, IOOperation* operation)
{
	IOBuffer* buffer = request->Buffer();
	off_t originalOffset = request->Offset() + request->Length()
		- request->RemainingBytes();
	off_t offset = originalOffset;

	// current iteration state
	uint32 vecIndex = request->VecIndex();
	uint32 vecOffset = request->VecOffset();
	size_t totalLength = min_c(request->RemainingBytes(),
		fRestrictions.max_transfer_size);

	MutexLocker locker(fLock);

	DMABuffer* dmaBuffer = fDMABuffers.RemoveHead();
	if (dmaBuffer == NULL)
		return B_BUSY;

	dmaBuffer->SetVecCount(0);

	iovec* vecs = NULL;
	uint32 segmentCount = 0;

	bool partialBegin = (offset & (fBlockSize - 1)) != 0;
dprintf("  offset %Ld, block size %lu -> %s\n", offset, fBlockSize, partialBegin ? "partial" : "whole");

	if (buffer->IsVirtual()) {
		// Unless we need the bounce buffer anyway, we have to translate the
		// virtual addresses to physical addresses, so we can check the DMA
		// restrictions.
dprintf("  IS VIRTUAL\n");
		if (true) {
// TODO: !partialOperation || totalLength >= fBlockSize
// TODO: Maybe enforce fBounceBufferSize >= 2 * fBlockSize.
			size_t transferLeft = totalLength;
			vecs = fScratchVecs;

dprintf("  CREATE PHYSICAL MAP %ld\n", buffer->VecCount());
			for (uint32 i = vecIndex; i < buffer->VecCount(); i++) {
				iovec& vec = buffer->VecAt(i);
				addr_t base = (addr_t)vec.iov_base + vecOffset;
				size_t size = vec.iov_len - vecOffset;
				vecOffset = 0;
				if (size > transferLeft)
					size = transferLeft;
dprintf("  size = %lu\n", size);

				while (size > 0 && segmentCount
						< fRestrictions.max_segment_count) {
					physical_entry entry;
					get_memory_map((void*)base, size, &entry, 1);

					vecs[segmentCount].iov_base = entry.address;
					vecs[segmentCount].iov_len = entry.size;

					transferLeft -= entry.size;
					size -= entry.size;
					segmentCount++;
				}

				if (transferLeft == 0)
					break;
			}

			totalLength -= transferLeft;
		}

		vecIndex = 0;
		vecOffset = 0;
	} else {
		// We do already have physical addresses.
		locker.Unlock();
		vecs = buffer->Vecs();
		segmentCount = min_c(buffer->VecCount(),
			fRestrictions.max_segment_count);
	}

dprintf("  physical count %lu\n", segmentCount);
for (uint32 i = 0; i < segmentCount; i++) {
	dprintf("    [%lu] %p, %lu\n", i, vecs[i].iov_base, vecs[i].iov_len);
}
	// check alignment, boundaries, etc. and set vecs in DMA buffer

	size_t dmaLength = 0;
	addr_t physicalBounceBuffer = dmaBuffer->PhysicalBounceBuffer();
	size_t bounceLeft = fBounceBufferSize;

	// If the offset isn't block-aligned, use the bounce buffer to bridge the
	// gap to the start of the vec.
	if (partialBegin) {
		off_t diff = offset & (fBlockSize - 1);
		addr_t base = physicalBounceBuffer;
		size_t length = (diff + fRestrictions.alignment - 1)
			& ~(fRestrictions.alignment - 1);

		physicalBounceBuffer += length;
		bounceLeft -= length;

		dmaBuffer->AddVec((void*)base, length);
		dmaLength += length;

		vecOffset += length - diff;
		offset -= diff;
dprintf("  partial begin, using bounce buffer: offset: %lld, length: %lu\n", offset, length);
	}

	for (uint32 i = vecIndex; i < segmentCount;) {
		if (dmaBuffer->VecCount() >= fRestrictions.max_segment_count)
			break;

		const iovec& vec = vecs[i];
		if (vec.iov_len <= vecOffset) {
			vecOffset -= vec.iov_len;
			i++;
			continue;
		}

		addr_t base = (addr_t)vec.iov_base + vecOffset;
		size_t length = vec.iov_len - vecOffset;

		// Cut the vec according to transfer size, segment size, and boundary.

		if (dmaLength + length > fRestrictions.max_transfer_size)
{
			length = fRestrictions.max_transfer_size - dmaLength;
dprintf("  vec %lu: restricting length to %lu due to transfer size limit\n", i, length);
}
		_RestrictBoundaryAndSegmentSize(base, length);

		size_t useBounceBuffer = 0;

		// Check low address: use bounce buffer for range to low address.
		// Check alignment: if not aligned, use bounce buffer for complete vec.
		if (base < fRestrictions.low_address)
{
			useBounceBuffer = fRestrictions.low_address - base;
dprintf("  vec %lu: below low address, using bounce buffer: %lu\n", i, useBounceBuffer);
}
		else if (base & (fRestrictions.alignment - 1))
{
			useBounceBuffer = length;
dprintf("  vec %lu: misalignment, using bounce buffer: %lu\n", i, useBounceBuffer);
}

// TODO: Enforce high address restriction!

		// If length is 0, use bounce buffer for complete vec.
		if (length == 0) {
			length = vec.iov_len - vecOffset;
			useBounceBuffer = length;
dprintf("  vec %lu: 0 length, using bounce buffer: %lu\n", i, useBounceBuffer);
		}

		if (useBounceBuffer > 0) {
			if (bounceLeft == 0) {
dprintf("  vec %lu: out of bounce buffer space\n", i);
				// We don't have any bounce buffer space left, we need to move
				// this request to the next I/O operation.
				break;
			}

			base = physicalBounceBuffer;

			if (useBounceBuffer > length)
				useBounceBuffer = length;
			if (useBounceBuffer > bounceLeft)
				useBounceBuffer = bounceLeft;
			length = useBounceBuffer;
		}

		// check boundary and max segment size.
		_RestrictBoundaryAndSegmentSize(base, length);
dprintf("  vec %lu: final length restriction: %lu\n", i, length);

		if (useBounceBuffer) {
			// alignment could still be wrong
			if (useBounceBuffer & (fRestrictions.alignment - 1)) {
				useBounceBuffer
					= (useBounceBuffer + fRestrictions.alignment - 1)
						& ~(fRestrictions.alignment - 1);
				if (dmaLength + useBounceBuffer
						> fRestrictions.max_transfer_size) {
					useBounceBuffer = (fRestrictions.max_transfer_size
						- dmaLength) & ~(fRestrictions.alignment - 1);
				}
			}

			physicalBounceBuffer += useBounceBuffer;
			bounceLeft -= useBounceBuffer;
		}

		vecOffset += length;

		// TODO: we might be able to join the vec with its preceding vec
		// (but then we'd need to take the segment size into account again)
		dmaBuffer->AddVec((void*)base, length);
		dmaLength += length;
	}

	// If total length not block aligned, use bounce buffer for padding.
	if ((dmaLength & (fBlockSize - 1)) != 0) {
dprintf("  dmaLength not block aligned: %lu\n", dmaLength);
		size_t length = (dmaLength + fBlockSize - 1) & ~(fBlockSize - 1);

		// If total length > max transfer size, segment count > max segment
		// count, truncate.
		if (length > fRestrictions.max_transfer_size
			|| dmaBuffer->VecCount() == fRestrictions.max_segment_count
			|| bounceLeft < length - dmaLength) {
			// cut the part of dma length
dprintf("  can't align length due to max transfer size, segment count "
"restrictions, or lacking bounce buffer space\n");
			size_t toCut = dmaLength
				& (max_c(fBlockSize, fRestrictions.alignment) - 1);
			dmaLength -= toCut;
			if (dmaLength == 0) {
				// This can only happen, when we have too many small segments
				// and hit the max segment count. In this case we just use the
				// bounce buffer for as much as possible of the total length.
				dmaBuffer->SetVecCount(0);
				addr_t base = dmaBuffer->PhysicalBounceBuffer();
				dmaLength = min_c(totalLength, fBounceBufferSize)
					& ~(max_c(fBlockSize, fRestrictions.alignment) - 1);
				_RestrictBoundaryAndSegmentSize(base, dmaLength);
				dmaBuffer->AddVec((void*)base, dmaLength);
			} else {
				int32 dmaVecCount = dmaBuffer->VecCount();
				for (int32 i = dmaVecCount - 1; toCut > 0 && i >= 0; i--) {
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
		} else {
dprintf("  adding %lu bytes final bounce buffer\n", length - dmaLength);
			dmaBuffer->AddVec((void*)physicalBounceBuffer, length - dmaLength);
			dmaLength = length;
		}
	}

	operation->SetBuffer(dmaBuffer);
	operation->SetOriginalRange(originalOffset,
		min_c(offset + dmaLength, request->Offset() + request->Length())
			- originalOffset);
	operation->SetRange(offset, dmaLength);
	operation->SetPartial(partialBegin,
		offset + dmaLength > request->Offset() + request->Length());

	status_t error = operation->SetRequest(request);
	if (error != B_OK)
		return error;

	request->Advance(operation->OriginalLength());

	return B_OK;
}


void
DMAResource::RecycleBuffer(DMABuffer* buffer)
{
	if (buffer == NULL)
		return;

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
