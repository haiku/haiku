/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */

#include "dma_resources.h"

#include <block_io.h>

#include <kernel.h>
#include <util/AutoLock.h>

#include "io_requests.h"


//#define TRACE_DMA_RESOURCE
#ifdef TRACE_DMA_RESOURCE
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif


extern device_manager_info gDeviceManagerModule;

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


bool
DMABuffer::UsesBounceBufferAt(uint32 index)
{
	if (index >= fVecCount)
		return false;

	return (addr_t)fVecs[index].iov_base >= fPhysicalBounceBuffer
		&& (addr_t)fVecs[index].iov_base
				< fPhysicalBounceBuffer + fBounceBufferSize;
}


void
DMABuffer::Dump() const
{
	kprintf("DMABuffer at %p\n", this);

	kprintf("  bounce buffer:      %p (physical %#lx)\n", fBounceBuffer,
		fPhysicalBounceBuffer);
	kprintf("  bounce buffer size: %lu\n", fBounceBufferSize);
	kprintf("  vecs:               %lu\n", fVecCount);

	for (uint32 i = 0; i < fVecCount; i++) {
		kprintf("    [%lu] %p, %lu\n", i, fVecs[i].iov_base, fVecs[i].iov_len);
	}
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
DMAResource::Init(device_node* node, size_t blockSize)
{
	dma_restrictions restrictions;
	memset(&restrictions, 0, sizeof(dma_restrictions));

	// TODO: add DMA attributes instead of reusing block_io's

	uint32 value;
	if (gDeviceManagerModule.get_attr_uint32(node,
			B_BLOCK_DEVICE_DMA_ALIGNMENT, &value, true) == B_OK)
		restrictions.alignment = value + 1;

	if (gDeviceManagerModule.get_attr_uint32(node,
			B_BLOCK_DEVICE_DMA_BOUNDARY, &value, true) == B_OK)
		restrictions.boundary = value + 1;

	if (gDeviceManagerModule.get_attr_uint32(node,
			B_BLOCK_DEVICE_MAX_SG_BLOCK_SIZE, &value, true) == B_OK)
		restrictions.max_segment_size = value;

	if (gDeviceManagerModule.get_attr_uint32(node,
			B_BLOCK_DEVICE_MAX_BLOCKS_ITEM, &value, true) == B_OK)
		restrictions.max_transfer_size = value * blockSize;

	uint32 bufferCount;
	if (gDeviceManagerModule.get_attr_uint32(node,
			B_BLOCK_DEVICE_MAX_SG_BLOCKS, &bufferCount, true) != B_OK)
		bufferCount = 16;

	return Init(restrictions, blockSize, bufferCount);
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
		fBounceBufferSize = fRestrictions.max_segment_size
			* min_c(fRestrictions.max_segment_count, 4);
		if (fBounceBufferSize > kMaxBounceBufferSize)
			fBounceBufferSize = kMaxBounceBufferSize;
		TRACE("DMAResource::Init(): chose bounce buffer size %lu\n",
			fBounceBufferSize);
	}

	dprintf("DMAResource@%p: low/high %lx/%lx, max segment count %lu, align %lu, "
		"boundary %lu, max transfer %lu, max segment size %lu\n", this,
		fRestrictions.low_address, fRestrictions.high_address,
		fRestrictions.max_segment_count, fRestrictions.alignment,
		fRestrictions.boundary, fRestrictions.max_transfer_size,
		fRestrictions.max_segment_size);

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
		if (fRestrictions.alignment > B_PAGE_SIZE)
			dprintf("dma buffer restrictions not yet implemented: alignment %lu\n", fRestrictions.alignment);
		if (fRestrictions.boundary > B_PAGE_SIZE)
			dprintf("dma buffer restrictions not yet implemented: boundary %lu\n", fRestrictions.boundary);

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


void
DMAResource::_CutBuffer(DMABuffer& buffer, addr_t& physicalBounceBuffer,
	size_t& bounceLeft, size_t toCut)
{
	int32 vecCount = buffer.VecCount();
	for (int32 i = vecCount - 1; toCut > 0 && i >= 0; i--) {
		iovec& vec = buffer.VecAt(i);
		size_t length = vec.iov_len;
		bool inBounceBuffer = buffer.UsesBounceBufferAt(i);

		if (length <= toCut) {
			vecCount--;
			toCut -= length;

			if (inBounceBuffer) {
				bounceLeft += length;
				physicalBounceBuffer -= length;
			}
		} else {
			vec.iov_len -= toCut;

			if (inBounceBuffer) {
				bounceLeft += toCut;
				physicalBounceBuffer -= toCut;
			}
			break;
		}
	}

	buffer.SetVecCount(vecCount);
}


/*!	Adds \a length bytes from the bounce buffer to the DMABuffer \a buffer.
	Takes care of boundary, and segment restrictions. \a length must be aligned.
	If \a fixedLength is requested, this function will fail if it cannot
	satisfy the request.

	\return 0 if the request cannot be satisfied. There could have been some
		additions to the DMA buffer, and you will need to cut them back.
	TODO: is that what we want here?
	\return >0 the number of bytes added to the buffer.
*/
size_t
DMAResource::_AddBounceBuffer(DMABuffer& buffer, addr_t& physicalBounceBuffer,
	size_t& bounceLeft, size_t length, bool fixedLength)
{
	if (bounceLeft < length) {
		if (fixedLength)
			return 0;

		length = bounceLeft;
	}

	size_t bounceUsed = 0;

	uint32 vecCount = buffer.VecCount();
	if (vecCount > 0) {
		// see if we can join the bounce buffer with the previously last vec
		iovec& vec = buffer.VecAt(vecCount - 1);
		addr_t vecBase = (addr_t)vec.iov_base;
		size_t vecLength = vec.iov_len;

		if (vecBase + vecLength == physicalBounceBuffer) {
			vecLength += length;
			_RestrictBoundaryAndSegmentSize(vecBase, vecLength);

			size_t lengthDiff = vecLength - vec.iov_len;
			length -= lengthDiff;

			physicalBounceBuffer += lengthDiff;
			bounceLeft -= lengthDiff;
			bounceUsed += lengthDiff;

			vec.iov_len = vecLength;
		}
	}

	while (length > 0) {
		// We need to add another bounce vec

		if (vecCount == fRestrictions.max_segment_count)
			return fixedLength ? 0 : bounceUsed;

		addr_t vecLength = length;
		_RestrictBoundaryAndSegmentSize(physicalBounceBuffer, vecLength);

		buffer.AddVec((void*)physicalBounceBuffer, vecLength);
		vecCount++;

		physicalBounceBuffer += vecLength;
		bounceLeft -= vecLength;
		bounceUsed += vecLength;
		length -= vecLength;
	}

	return bounceUsed;
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

	size_t partialBegin = offset & (fBlockSize - 1);
	TRACE("  offset %Ld, remaining size: %lu, block size %lu -> partial: %lu\n",
		offset, request->RemainingBytes(), fBlockSize, partialBegin);

	if (buffer->IsVirtual()) {
		// Unless we need the bounce buffer anyway, we have to translate the
		// virtual addresses to physical addresses, so we can check the DMA
		// restrictions.
		TRACE("  buffer is virtual %s\n", buffer->IsUser() ? "user" : "kernel");
		// TODO: !partialOperation || totalLength >= fBlockSize
		// TODO: Maybe enforce fBounceBufferSize >= 2 * fBlockSize.
		if (true) {
			size_t transferLeft = totalLength;
			vecs = fScratchVecs;

			TRACE("  create physical map (for %ld vecs)\n", buffer->VecCount());
			for (uint32 i = vecIndex; i < buffer->VecCount(); i++) {
				iovec& vec = buffer->VecAt(i);
				addr_t base = (addr_t)vec.iov_base + vecOffset;
				size_t size = vec.iov_len - vecOffset;
				vecOffset = 0;
				if (size > transferLeft)
					size = transferLeft;

				while (size > 0 && segmentCount
						< fRestrictions.max_segment_count) {
					physical_entry entry;
					uint32 count = 1;
					get_memory_map_etc(request->Team(), (void*)base, size,
						&entry, &count);

					vecs[segmentCount].iov_base = entry.address;
					vecs[segmentCount].iov_len = entry.size;

					transferLeft -= entry.size;
					base += entry.size;
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
		segmentCount = min_c(buffer->VecCount() - vecIndex,
			fRestrictions.max_segment_count);
	}

#ifdef TRACE_DMA_RESOURCE
	TRACE("  physical count %lu\n", segmentCount);
	for (uint32 i = 0; i < segmentCount; i++) {
		TRACE("    [%lu] %p, %lu\n", i, vecs[vecIndex + i].iov_base,
			vecs[vecIndex + i].iov_len);
	}
#endif

	// check alignment, boundaries, etc. and set vecs in DMA buffer

	size_t dmaLength = 0;
	addr_t physicalBounceBuffer = dmaBuffer->PhysicalBounceBuffer();
	size_t bounceLeft = fBounceBufferSize;
	size_t transferLeft = totalLength;

	// If the offset isn't block-aligned, use the bounce buffer to bridge the
	// gap to the start of the vec.
	if (partialBegin > 0) {
		size_t length;
		if (request->IsWrite()) {
			// we always need to read in a whole block for the partial write
			length = fBlockSize;
		} else {
			length = (partialBegin + fRestrictions.alignment - 1)
				& ~(fRestrictions.alignment - 1);
		}

		if (_AddBounceBuffer(*dmaBuffer, physicalBounceBuffer, bounceLeft,
				length, true) == 0) {
			TRACE("  adding partial begin failed, length %lu!\n", length);
			return B_BAD_VALUE;
		}

		dmaLength += length;

		vecOffset += length - partialBegin;
		offset -= partialBegin;
		TRACE("  partial begin, using bounce buffer: offset: %lld, length: "
			"%lu\n", offset, length);
	}

	for (uint32 i = vecIndex; i < vecIndex + segmentCount;) {
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
		if (length > transferLeft)
			length = transferLeft;

		// Cut the vec according to transfer size, segment size, and boundary.

		if (dmaLength + length > fRestrictions.max_transfer_size) {
			length = fRestrictions.max_transfer_size - dmaLength;
			TRACE("  vec %lu: restricting length to %lu due to transfer size "
				"limit\n", i, length);
		}
		_RestrictBoundaryAndSegmentSize(base, length);

		size_t useBounceBufferSize = 0;

		// Check low address: use bounce buffer for range to low address.
		// Check alignment: if not aligned, use bounce buffer for complete vec.
		if (base < fRestrictions.low_address) {
			useBounceBufferSize = fRestrictions.low_address - base;
			TRACE("  vec %lu: below low address, using bounce buffer: %lu\n", i,
				useBounceBufferSize);
		} else if (base & (fRestrictions.alignment - 1)) {
			useBounceBufferSize = length;
			TRACE("  vec %lu: misalignment, using bounce buffer: %lu\n", i,
				useBounceBufferSize);
		}

		// Enforce high address restriction
		if (base > fRestrictions.high_address)
			useBounceBufferSize = length;
		else if (base + length > fRestrictions.high_address)
			length = fRestrictions.high_address - base;

		// Align length as well
		if (useBounceBufferSize == 0)
			length &= ~(fRestrictions.alignment - 1);

		// If length is 0, use bounce buffer for complete vec.
		if (length == 0) {
			length = vec.iov_len - vecOffset;
			useBounceBufferSize = length;
			TRACE("  vec %lu: 0 length, using bounce buffer: %lu\n", i,
				useBounceBufferSize);
		}

		if (useBounceBufferSize > 0) {
			// alignment could still be wrong (we round up here)
			useBounceBufferSize = (useBounceBufferSize
				+ fRestrictions.alignment - 1) & ~(fRestrictions.alignment - 1);

			length = _AddBounceBuffer(*dmaBuffer, physicalBounceBuffer,
				bounceLeft, useBounceBufferSize, false);
			if (length == 0) {
				TRACE("  vec %lu: out of bounce buffer space\n", i);
				// We don't have any bounce buffer space left, we need to move
				// this request to the next I/O operation.
				break;
			}
			TRACE("  vec %lu: final bounce length: %lu\n", i, length);
		} else {
			TRACE("  vec %lu: final length restriction: %lu\n", i, length);
			dmaBuffer->AddVec((void*)base, length);
		}

		dmaLength += length;
		vecOffset += length;
		transferLeft -= length;
	}

	// If we're writing partially, we always need to have a block sized bounce
	// buffer (or else we would overwrite memory to be written on the read in
	// the first phase).
	if (request->IsWrite() && (dmaLength & (fBlockSize - 1)) != 0) {
		size_t diff = dmaLength  & (fBlockSize - 1);
		TRACE("  partial end write: %lu, diff %lu\n", dmaLength, diff);

		_CutBuffer(*dmaBuffer, physicalBounceBuffer, bounceLeft, diff);
		dmaLength -= diff;

		if (_AddBounceBuffer(*dmaBuffer, physicalBounceBuffer,
				bounceLeft, fBlockSize, true) == 0) {
			// If we cannot write anything, we can't process the request at all
			TRACE("  adding bounce buffer failed!!!\n");
			if (dmaLength == 0)
				return B_BAD_VALUE;
		} else
			dmaLength += fBlockSize;
	}

	// If total length not block aligned, use bounce buffer for padding (read
	// case only).
	while ((dmaLength & (fBlockSize - 1)) != 0) {
		TRACE("  dmaLength not block aligned: %lu\n", dmaLength);
			size_t length = (dmaLength + fBlockSize - 1) & ~(fBlockSize - 1);

		// If total length > max transfer size, segment count > max segment
		// count, truncate.
		// TODO: sometimes we can replace the last vec with the bounce buffer
		// to let it match the restrictions.
		if (length > fRestrictions.max_transfer_size
			|| dmaBuffer->VecCount() == fRestrictions.max_segment_count
			|| bounceLeft < length - dmaLength) {
			// cut the part of dma length
			TRACE("  can't align length due to max transfer size, segment "
				"count restrictions, or lacking bounce buffer space\n");
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

				physicalBounceBuffer = base + dmaLength;
				bounceLeft = fBounceBufferSize - dmaLength;
			} else {
				_CutBuffer(*dmaBuffer, physicalBounceBuffer, bounceLeft, toCut);
			}
		} else {
			TRACE("  adding %lu bytes final bounce buffer\n",
				length - dmaLength);
			length -= dmaLength;
			length = _AddBounceBuffer(*dmaBuffer, physicalBounceBuffer,
				bounceLeft, length, true);
			if (length == 0)
				panic("don't do this to me!");
			dmaLength += length;
		}
	}

	off_t requestEnd = request->Offset() + request->Length();

	operation->SetBuffer(dmaBuffer);
	operation->SetBlockSize(fBlockSize);
	operation->SetOriginalRange(originalOffset,
		min_c(offset + dmaLength, requestEnd) - originalOffset);
	operation->SetRange(offset, dmaLength);
	operation->SetPartial(partialBegin != 0, offset + dmaLength > requestEnd);
	operation->SetUsesBounceBuffer(bounceLeft < fBounceBufferSize);

	status_t error = operation->Prepare(request);
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
