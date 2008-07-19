/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */

#include "io_requests.h"

#include <string.h>

#include <team.h>
#include <vm.h>

#include "dma_resources.h"


// partial I/O operation phases
enum {
	PHASE_READ_BEGIN	= 0,
	PHASE_READ_END		= 1,
	PHASE_DO_ALL		= 2
};


IORequestChunk::IORequestChunk()
	:
	fParent(NULL),
	fStatus(1)
{
}


IORequestChunk::~IORequestChunk()
{
}


//	#pragma mark -


IOBuffer*
IOBuffer::Create(size_t count)
{
	IOBuffer* buffer = (IOBuffer*)malloc(
		sizeof(IOBuffer) + sizeof(iovec) * (count - 1));
	if (buffer == NULL)
		return NULL;

	buffer->fCapacity = count;
	buffer->fVecCount = 0;
	buffer->fUser = false;
	buffer->fPhysical = false;

	return buffer;
}


void
IOBuffer::SetVecs(const iovec* vecs, uint32 count, size_t length, uint32 flags)
{
	memcpy(fVecs, vecs, sizeof(iovec) * count);
	fVecCount = count;
	fLength = length;
	fUser = (flags & B_USER_IO_REQUEST) != 0;
	fPhysical = (flags & B_PHYSICAL_IO_REQUEST) != 0;
}


status_t
IOBuffer::LockMemory(bool isWrite)
{
	for (uint32 i = 0; i < fVecCount; i++) {
		status_t status = lock_memory(fVecs[i].iov_base, fVecs[i].iov_len,
			isWrite ? 0 : B_READ_DEVICE);
		if (status != B_OK) {
			_UnlockMemory(i, isWrite);
			return status;
		}
	}

	return B_OK;
}


void
IOBuffer::_UnlockMemory(size_t count, bool isWrite)
{
	for (uint32 i = 0; i < count; i++) {
		unlock_memory(fVecs[i].iov_base, fVecs[i].iov_len,
			isWrite ? 0 : B_READ_DEVICE);
	}
}


void
IOBuffer::UnlockMemory(bool isWrite)
{
	_UnlockMemory(fVecCount, isWrite);
}


// #pragma mark -


bool
IOOperation::Finish()
{
dprintf("IOOperation::Finish()\n");
	if (fStatus == B_OK) {
		if (fParent->IsWrite()) {
dprintf("  is write\n");
			if (fPhase == PHASE_READ_BEGIN) {
dprintf("  phase read begin\n");
				// partial write: copy partial begin to bounce buffer
				bool skipReadEndPhase;
				status_t error = _CopyPartialBegin(true, skipReadEndPhase);
				if (error == B_OK) {
					// We're done with the first phase only (read in begin).
					// Get ready for next phase...
					fPhase = HasPartialEnd() && !skipReadEndPhase
						? PHASE_READ_END : PHASE_DO_ALL;
					SetStatus(1);
						// TODO: Is there a race condition, if the request is
						// aborted at the same time?
					return false;
				}

				SetStatus(error);
			} else if (fPhase == PHASE_READ_END) {
dprintf("  phase read end\n");
				// partial write: copy partial end to bounce buffer
				status_t error = _CopyPartialEnd(true);
				if (error == B_OK) {
					// We're done with the second phase only (read in end).
					// Get ready for next phase...
					fPhase = PHASE_DO_ALL;
					SetStatus(1);
						// TODO: Is there a race condition, if the request is
						// aborted at the same time?
					return false;
				}

				SetStatus(error);
			}
		}
	}

	if (fParent->IsRead() && UsesBounceBuffer()) {
dprintf("  read with bounce buffer\n");
		// copy the bounce buffer segments to the final location
		uint8* bounceBuffer = (uint8*)fDMABuffer->BounceBuffer();
		addr_t bounceBufferStart = fDMABuffer->PhysicalBounceBuffer();
		addr_t bounceBufferEnd = bounceBufferStart
			+ fDMABuffer->BounceBufferSize();

		const iovec* vecs = fDMABuffer->Vecs();
		uint32 vecCount = fDMABuffer->VecCount();
		uint32 i = 0;

		off_t offset = Offset();

		status_t error = B_OK;
		bool partialBlockOnly = false;
		if (HasPartialBegin()) {
			error = _CopyPartialBegin(false, partialBlockOnly);
			offset += vecs[0].iov_len;
			i++;
		}

		if (error == B_OK && HasPartialEnd() && !partialBlockOnly) {
			error = _CopyPartialEnd(false);
			vecCount--;
		}

		for (; error == B_OK && i < vecCount; i++) {
			const iovec& vec = vecs[i];
			addr_t base = (addr_t)vec.iov_base;
			if (base >= bounceBufferStart && base < bounceBufferEnd) {
				error = fParent->CopyData(
					bounceBuffer + (base - bounceBufferStart), offset,
					vec.iov_len);
			}
			offset += vec.iov_len;
		}

		if (error != B_OK)
			SetStatus(error);
	}

	// notify parent request
	if (fParent != NULL)
		fParent->ChunkFinished(this, fStatus);

	return true;
}


/*!	Note: SetPartial() must be called first!
*/
status_t
IOOperation::SetRequest(IORequest* request)
{
	if (fParent != NULL)
		fParent->RemoveOperation(this);

	fParent = request;

	// set initial phase
	fPhase = PHASE_DO_ALL;
	if (fParent->IsWrite()) {
		if (HasPartialBegin())
			fPhase = PHASE_READ_BEGIN;
		else if (HasPartialEnd())
			fPhase = PHASE_READ_END;

		// Copy data to bounce buffer segments, save the partial begin/end vec,
		// which will be copied after their respective read phase.
		if (UsesBounceBuffer()) {
dprintf("  write with bounce buffer\n");
			uint8* bounceBuffer = (uint8*)fDMABuffer->BounceBuffer();
			addr_t bounceBufferStart = fDMABuffer->PhysicalBounceBuffer();
			addr_t bounceBufferEnd = bounceBufferStart
				+ fDMABuffer->BounceBufferSize();

			const iovec* vecs = fDMABuffer->Vecs();
			uint32 vecCount = fDMABuffer->VecCount();
			uint32 i = 0;

			off_t offset = Offset();

			if (HasPartialBegin()) {
				offset += vecs[0].iov_len;
				i++;
			}

			if (HasPartialEnd())
				vecCount--;

			for (; i < vecCount; i++) {
				const iovec& vec = vecs[i];
				addr_t base = (addr_t)vec.iov_base;
				if (base >= bounceBufferStart && base < bounceBufferEnd) {
					status_t error = fParent->CopyData(offset,
						bounceBuffer + (base - bounceBufferStart), vec.iov_len);
					if (error != B_OK)
						return error;
				}
				offset += vec.iov_len;
			}
		}
	}

	fStatus = 1;

	if (fParent != NULL)
		fParent->AddOperation(this);

	return B_OK;
}


void
IOOperation::SetOriginalRange(off_t offset, size_t length)
{
	fOriginalOffset = fOffset = offset;
	fOriginalLength = fLength = length;
}


void
IOOperation::SetRange(off_t offset, size_t length)
{
	fOffset = offset;
	fLength = length;
}


iovec*
IOOperation::Vecs() const
{
	switch (fPhase) {
		case PHASE_READ_END:
			return fDMABuffer->Vecs() + (fDMABuffer->VecCount() - 1);
		case PHASE_READ_BEGIN:
		case PHASE_DO_ALL:
		default:
			return fDMABuffer->Vecs();
	}
}


uint32
IOOperation::VecCount() const
{
	switch (fPhase) {
		case PHASE_READ_BEGIN:
		case PHASE_READ_END:
			return 1;
		case PHASE_DO_ALL:
		default:
			return fDMABuffer->VecCount();
	}
}


void
IOOperation::SetPartial(bool partialBegin, bool partialEnd)
{
	fPartialBegin = partialBegin;
	fPartialEnd = partialEnd;
}


bool
IOOperation::IsWrite() const
{
	return fParent->IsWrite() && fPhase != PHASE_DO_ALL;
}


bool
IOOperation::IsRead() const
{
	return fParent->IsRead();
}


status_t
IOOperation::_CopyPartialBegin(bool isWrite, bool& partialBlockOnly)
{
	size_t relativeOffset = OriginalOffset() - Offset();
	size_t length = fDMABuffer->VecAt(0).iov_len;

	partialBlockOnly = relativeOffset + OriginalLength() <= length;
	if (partialBlockOnly)
		length = relativeOffset + OriginalLength();

	if (isWrite) {
		return fParent->CopyData(OriginalOffset(),
			(uint8*)fDMABuffer->BounceBuffer() + relativeOffset,
			length - relativeOffset);
	} else {
		return fParent->CopyData(
			(uint8*)fDMABuffer->BounceBuffer() + relativeOffset,
			OriginalOffset(), length - relativeOffset);
	}
}


status_t
IOOperation::_CopyPartialEnd(bool isWrite)
{
	const iovec& lastVec = fDMABuffer->VecAt(fDMABuffer->VecCount() - 1);
	off_t lastVecPos = Offset() + Length() - lastVec.iov_len;
	if (isWrite) {
		return fParent->CopyData(lastVecPos,
			(uint8*)fDMABuffer->BounceBuffer()
				+ ((addr_t)lastVec.iov_base
					- fDMABuffer->PhysicalBounceBuffer()),
			OriginalOffset() + OriginalLength() - lastVecPos);
	} else {
		return fParent->CopyData((uint8*)fDMABuffer->BounceBuffer()
				+ ((addr_t)lastVec.iov_base
					- fDMABuffer->PhysicalBounceBuffer()),
			lastVecPos, OriginalOffset() + OriginalLength() - lastVecPos);
	}
}


// #pragma mark -


IORequest::IORequest()
{
}


IORequest::~IORequest()
{
}


status_t
IORequest::Init(off_t offset, void* buffer, size_t length, bool write,
	uint32 flags)
{
	iovec vec;
	vec.iov_base = buffer;
	vec.iov_len = length;
	return Init(offset, &vec, 1, length, write, flags);
}


status_t
IORequest::Init(off_t offset, iovec* vecs, size_t count, size_t length,
	bool write, uint32 flags)
{
	fBuffer = IOBuffer::Create(count);
	if (fBuffer == NULL)
		return B_NO_MEMORY;

	fBuffer->SetVecs(vecs, count, length, flags);

	fOffset = offset;
	fLength = length;
	fFlags = flags;
	fTeam = team_get_current_team_id();
	fIsWrite = write;

	// these are for iteration
	fVecIndex = 0;
	fVecOffset = 0;
	fRemainingBytes = length;

	return B_OK;
}


void
IORequest::ChunkFinished(IORequestChunk* chunk, status_t status)
{
	// TODO: we would need to update status atomically
	if (fStatus <= 0) {
		// we're already done
		return;
	}

	fStatus = status;

	if (fParent != NULL)
		fParent->ChunkFinished(this, Status());
}


void
IORequest::Advance(size_t bySize)
{
dprintf("IORequest::Advance(%lu): remaining: %lu -> %lu\n", bySize,
fRemainingBytes, fRemainingBytes - bySize);
	fRemainingBytes -= bySize;

	iovec* vecs = fBuffer->Vecs();
	while (vecs[fVecIndex].iov_len - fVecOffset <= bySize) {
		bySize -= vecs[fVecIndex].iov_len - fVecOffset;
		fVecOffset = 0;
		fVecIndex++;
	}

	fVecOffset += bySize;
}


void
IORequest::AddOperation(IOOperation* operation)
{
	// TODO: locking?
	fChildren.Add(operation);
}


void
IORequest::RemoveOperation(IOOperation* operation)
{
	// TODO: locking?
	fChildren.Remove(operation);
}


status_t
IORequest::CopyData(off_t offset, void* buffer, size_t size)
{
	return _CopyData(buffer, offset, size, true);
}


status_t
IORequest::CopyData(const void* buffer, off_t offset, size_t size)
{
	return _CopyData((void*)buffer, offset, size, false);
}


status_t
IORequest::_CopyData(void* _buffer, off_t offset, size_t size, bool copyIn)
{
	if (size == 0)
		return B_OK;

	uint8* buffer = (uint8*)_buffer;

	if (offset < fOffset || offset + size > fOffset + fLength) {
		panic("IORequest::_CopyData(): invalid range: (%lld, %lu)", offset,
			size);
		return B_BAD_VALUE;
	}

	// If we can, we directly copy from/to the virtual buffer. The memory is
	// locked in this case.
	status_t (*copyFunction)(void*, void*, size_t, bool);
	if (fBuffer->IsPhysical()) {
		copyFunction = &IORequest::_CopyPhysical;
	} else {
		copyFunction = fBuffer->IsUser()
			? &IORequest::_CopyUser : &IORequest::_CopySimple;
	}

	// skip bytes if requested
	iovec* vecs = fBuffer->Vecs();
	size_t skipBytes = offset - fOffset;
	size_t vecOffset = 0;
	while (skipBytes > 0) {
		if (vecs[0].iov_len > skipBytes) {
			vecOffset = skipBytes;
			break;
		}

		skipBytes -= vecs[0].iov_len;
		vecs++;
	}

	// copy iovec-wise
	while (size > 0) {
		size_t toCopy = min_c(size, vecs[0].iov_len - vecOffset);
		status_t error = copyFunction(buffer,
			(uint8*)vecs[0].iov_base + vecOffset, toCopy, copyIn);
		if (error != B_OK)
			return error;

		buffer += toCopy;
		size -= toCopy;
		vecs++;
		vecOffset = 0;
	}

	return B_OK;
}


/* static */ status_t
IORequest::_CopySimple(void* bounceBuffer, void* external, size_t size,
	bool copyIn)
{
dprintf("  IORequest::_CopySimple(%p, %p, %lu, %d)\n", bounceBuffer, external, size, copyIn);
	if (copyIn)
		memcpy(bounceBuffer, external, size);
	else
		memcpy(external, bounceBuffer, size);
	return B_OK;
}


/* static */ status_t
IORequest::_CopyPhysical(void* _bounceBuffer, void* _external, size_t size,
	bool copyIn)
{
	uint8* bounceBuffer = (uint8*)_bounceBuffer;
	addr_t external = (addr_t)_external;

	while (size > 0) {
		addr_t virtualAddress;
		status_t error = vm_get_physical_page(external, &virtualAddress, 0);
		if (error != B_OK)
			return error;

		size_t toCopy = min_c(size, B_PAGE_SIZE);
		_CopySimple(bounceBuffer, (void*)virtualAddress, toCopy, copyIn);

		vm_put_physical_page(virtualAddress);

		size -= toCopy;
		bounceBuffer += toCopy;
		external += toCopy;
	}

	return B_OK;
}


/* static */ status_t
IORequest::_CopyUser(void* _bounceBuffer, void* _external, size_t size,
	bool copyIn)
{
	uint8* bounceBuffer = (uint8*)_bounceBuffer;
	uint8* external = (uint8*)_external;

	while (size > 0) {
		physical_entry entries[8];
		int32 count = get_memory_map(external, size, entries, 8);
		if (count <= 0) {
			panic("IORequest::_CopyUser(): Failed to get physical memory for "
				"user memory %p\n", external);
			return B_BAD_ADDRESS;
		}

		for (int32 i = 0; i < count; i++) {
			const physical_entry& entry = entries[i];
			status_t error = _CopyPhysical(bounceBuffer, entry.address,
				entry.size, copyIn);
			if (error != B_OK)
				return error;

			size -= entry.size;
			bounceBuffer += entry.size;
			external += entry.size;
		}
	}

	return B_OK;
}


// #pragma mark -


#if 0

/*!	Creates an I/O request with the specified buffer and length.

	\param write write access if true, read access if false.
	\param flags allows several flags to be specified:
		\c B_USER_IO_REQUEST the buffer is assumed to be a userland buffer
			and handled with special care.
		\c B_ASYNC_IO_REQUEST the I/O request is to be fulfilled asynchronously.
		\c B_PHYSICAL_IO_REQUEST the buffer specifies a physical rather than a
			virtual address.
	\param _request If successful, the location pointed to by this parameter
		will contain a pointer to the created request.
*/
status_t
create_io_request(void* buffer, size_t length, bool write, uint32 flags,
	io_request** _request)
{
	return B_ERROR;
}


/*!	Creates an I/O request from the specified I/O vector and length.
	See above for more info.
*/
status_t
create_io_request_vecs(iovec* vecs, size_t count, size_t length, bool write,
	uint32 flags, io_request** _request)
{
	return B_ERROR;
}


/*!	Prepares the I/O request by locking its memory, and, if \a virtualOnly
	is \c false, will retrieve the physical pages.
*/
status_t
prepare_io_request(io_request* request, bool virtualOnly)
{
	return B_ERROR;
}


/*!	Prepares the I/O request by locking its memory, and mapping/moving the
	pages as needed to fulfill the DMA restrictions.
	If needed, a bounce buffer is used for DMA.
*/
status_t
prepare_io_request_dma(io_request* request, dma_resource* dmaResource)
{
	return B_ERROR;
}


/*!	Returns the buffers of the I/O request mapped into kernel memory.
	This can be used by drivers to fill an I/O request manually.
*/
status_t
map_io_request(io_request* request, iovec* vecs, size_t count)
{
	return B_ERROR;
}


/*!	Get the memory map of the DMA buffer for this I/O request.
	This can be used to retrieve the physical pages to feed the hardware's
	DMA engine with.
*/
status_t
get_io_request_memory_map(dma_buffer* buffer, io_request* request, iovec* vecs,
	size_t count)
{
	return B_ERROR;
}


/*!	Unmaps any previously mapped data, and will copy the data back from any
	bounce buffers if necessary.
*/
status_t
complete_io_request_dma(io_request* request, dma_resource* dmaResource)
{
	return B_ERROR;
}


/*!	Unmaps any previously mapped data.
*/
status_t
complete_io_request(io_request* request)
{
	return B_ERROR;
}


void
delete_io_request(io_request* request)
{
}

#endif	// 0
