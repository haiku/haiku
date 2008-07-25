/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */

#include "io_requests.h"

#include <string.h>

#include <team.h>
#include <util/AutoLock.h>
#include <vm.h>

#include "dma_resources.h"


#define TRACE_IO_REQUEST
#ifdef TRACE_IO_REQUEST
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif


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
IOBuffer::LockMemory(team_id team, bool isWrite)
{
	for (uint32 i = 0; i < fVecCount; i++) {
		status_t status = lock_memory_etc(team, fVecs[i].iov_base,
			fVecs[i].iov_len, isWrite ? 0 : B_READ_DEVICE);
		if (status != B_OK) {
			_UnlockMemory(team, i, isWrite);
			return status;
		}
	}

	return B_OK;
}


void
IOBuffer::_UnlockMemory(team_id team, size_t count, bool isWrite)
{
	for (uint32 i = 0; i < count; i++) {
		unlock_memory_etc(team, fVecs[i].iov_base, fVecs[i].iov_len,
			isWrite ? 0 : B_READ_DEVICE);
	}
}


void
IOBuffer::UnlockMemory(team_id team, bool isWrite)
{
	_UnlockMemory(team, fVecCount, isWrite);
}


// #pragma mark -


bool
IOOperation::Finish()
{
	TRACE("IOOperation::Finish()\n");
	if (fStatus == B_OK) {
		if (fParent->IsWrite()) {
			TRACE("  is write\n");
			if (fPhase == PHASE_READ_BEGIN) {
				TRACE("  phase read begin\n");
				// repair phase adjusted vec
				fDMABuffer->VecAt(fSavedVecIndex).iov_len = fSavedVecLength;

				// partial write: copy partial begin to bounce buffer
				bool skipReadEndPhase;
				status_t error = _CopyPartialBegin(true, skipReadEndPhase);
				if (error == B_OK) {
					// We're done with the first phase only (read in begin).
					// Get ready for next phase...
					fPhase = HasPartialEnd() && !skipReadEndPhase
						? PHASE_READ_END : PHASE_DO_ALL;
					_PrepareVecs();
					ResetStatus();
						// TODO: Is there a race condition, if the request is
						// aborted at the same time?
					return false;
				}

				SetStatus(error);
			} else if (fPhase == PHASE_READ_END) {
				TRACE("  phase read end\n");
				// repair phase adjusted vec
				iovec& vec = fDMABuffer->VecAt(fSavedVecIndex);
				vec.iov_base = (uint8*)vec.iov_base
					+ vec.iov_len - fSavedVecLength;
				vec.iov_len = fSavedVecLength;

				// partial write: copy partial end to bounce buffer
				status_t error = _CopyPartialEnd(true);
				if (error == B_OK) {
					// We're done with the second phase only (read in end).
					// Get ready for next phase...
					fPhase = PHASE_DO_ALL;
					ResetStatus();
						// TODO: Is there a race condition, if the request is
						// aborted at the same time?
					return false;
				}

				SetStatus(error);
			}
		}
	}

	if (fParent->IsRead() && UsesBounceBuffer()) {
		TRACE("  read with bounce buffer\n");
		// copy the bounce buffer segments to the final location
		uint8* bounceBuffer = (uint8*)fDMABuffer->BounceBuffer();
		addr_t bounceBufferStart = fDMABuffer->PhysicalBounceBuffer();
		addr_t bounceBufferEnd = bounceBufferStart
			+ fDMABuffer->BounceBufferSize();

		const iovec* vecs = fDMABuffer->Vecs();
		uint32 vecCount = fDMABuffer->VecCount();

		status_t error = B_OK;

		off_t offset = fOffset;
		off_t startOffset = fOriginalOffset;
		off_t endOffset = fOriginalOffset + fOriginalLength;

		for (uint32 i = 0; error == B_OK && i < vecCount; i++) {
			const iovec& vec = vecs[i];
			addr_t base = (addr_t)vec.iov_base;
			size_t length = vec.iov_len;

			if (offset < startOffset) {
				if (offset + length <= startOffset) {
					offset += length;
					continue;
				}

				size_t diff = startOffset - offset;
				base += diff;
				length -= diff;
			}

			if (offset + length > endOffset) {
				if (offset >= endOffset)
					break;

				length = endOffset - offset;
			}

			if (base >= bounceBufferStart && base < bounceBufferEnd) {
				error = fParent->CopyData(
					bounceBuffer + (base - bounceBufferStart), offset, length);
			}

			offset += length;
		}

		if (error != B_OK)
			SetStatus(error);
	}

	return true;
}


/*!	Note: SetPartial() must be called first!
*/
status_t
IOOperation::Prepare(IORequest* request)
{
	if (fParent != NULL)
		fParent->RemoveOperation(this);

	fParent = request;

	// set initial phase
	fPhase = PHASE_DO_ALL;
	if (fParent->IsWrite()) {
		// Copy data to bounce buffer segments, save the partial begin/end vec,
		// which will be copied after their respective read phase.
		if (UsesBounceBuffer()) {
			TRACE("  write with bounce buffer\n");
			uint8* bounceBuffer = (uint8*)fDMABuffer->BounceBuffer();
			addr_t bounceBufferStart = fDMABuffer->PhysicalBounceBuffer();
			addr_t bounceBufferEnd = bounceBufferStart
				+ fDMABuffer->BounceBufferSize();

			const iovec* vecs = fDMABuffer->Vecs();
			uint32 vecCount = fDMABuffer->VecCount();
			size_t vecOffset = 0;
			uint32 i = 0;

			off_t offset = fOffset;
			off_t endOffset = fOffset + fLength;

			if (HasPartialBegin()) {
				// skip first block
				size_t toSkip = fBlockSize;
				while (toSkip > 0) {
					if (vecs[i].iov_len <= toSkip) {
						toSkip -= vecs[i].iov_len;
						i++;
					} else {
						vecOffset = toSkip;
						break;
					}
				}

				offset += fBlockSize;
			}

			if (HasPartialEnd()) {
				// skip last block
				size_t toSkip = fBlockSize;
				while (toSkip > 0) {
					if (vecs[vecCount - 1].iov_len <= toSkip) {
						toSkip -= vecs[vecCount - 1].iov_len;
						vecCount--;
					} else
						break;
				}

				endOffset -= fBlockSize;
			}

			for (; i < vecCount; i++) {
				const iovec& vec = vecs[i];
				addr_t base = (addr_t)vec.iov_base + vecOffset;
				size_t length = vec.iov_len - vecOffset;
				vecOffset = 0;

				if (base >= bounceBufferStart && base < bounceBufferEnd) {
					if (offset + length > endOffset)
						length = endOffset - offset;
					status_t error = fParent->CopyData(offset,
						bounceBuffer + (base - bounceBufferStart), length);
					if (error != B_OK)
						return error;
				}

				offset += length;
			}
		}

		if (HasPartialBegin())
			fPhase = PHASE_READ_BEGIN;
		else if (HasPartialEnd())
			fPhase = PHASE_READ_END;

		_PrepareVecs();
	}

	ResetStatus();

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


off_t
IOOperation::Offset() const
{
	return fPhase == PHASE_READ_END ? fOffset + fLength - fBlockSize : fOffset;
}


size_t
IOOperation::Length() const
{
	return fPhase == PHASE_DO_ALL ? fLength : fBlockSize;
}


iovec*
IOOperation::Vecs() const
{
	switch (fPhase) {
		case PHASE_READ_END:
			return fDMABuffer->Vecs() + fSavedVecIndex;
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
			return fSavedVecIndex + 1;
		case PHASE_READ_END:
			return fDMABuffer->VecCount() - fSavedVecIndex;
		case PHASE_DO_ALL:
		default:
			return fDMABuffer->VecCount();
	}
}


void
IOOperation::SetPartial(bool partialBegin, bool partialEnd)
{
	TRACE("partial begin %d, end %d\n", partialBegin, partialEnd);
	fPartialBegin = partialBegin;
	fPartialEnd = partialEnd;
}


bool
IOOperation::IsWrite() const
{
	return fParent->IsWrite() && fPhase == PHASE_DO_ALL;
}


bool
IOOperation::IsRead() const
{
	return fParent->IsRead();
}


void
IOOperation::_PrepareVecs()
{
	// we need to prepare the vecs for consumption by the drivers
	if (fPhase == PHASE_READ_BEGIN) {
		iovec* vecs = fDMABuffer->Vecs();
		uint32 vecCount = fDMABuffer->VecCount();
		size_t vecLength = fBlockSize;
		for (uint32 i = 0; i < vecCount; i++) {
			iovec& vec = vecs[i];
			if (vec.iov_len >= vecLength) {
				fSavedVecIndex = i;
				fSavedVecLength = vec.iov_len;
				vec.iov_len = vecLength;
				break;
			}
			vecLength -= vec.iov_len;
		}
	} else if (fPhase == PHASE_READ_END) {
		iovec* vecs = fDMABuffer->Vecs();
		uint32 vecCount = fDMABuffer->VecCount();
		size_t vecLength = fBlockSize;
		for (int32 i = vecCount - 1; i >= 0; i--) {
			iovec& vec = vecs[i];
			if (vec.iov_len >= vecLength) {
				fSavedVecIndex = i;
				fSavedVecLength = vec.iov_len;
				vec.iov_base = (uint8*)vec.iov_base
					+ vec.iov_len - vecLength;
				vec.iov_len = vecLength;
				break;
			}
			vecLength -= vec.iov_len;
		}
	}
}


status_t
IOOperation::_CopyPartialBegin(bool isWrite, bool& singleBlockOnly)
{
	size_t relativeOffset = OriginalOffset() - fOffset;
	size_t length = fBlockSize - relativeOffset;

	singleBlockOnly = length >= OriginalLength();
	if (singleBlockOnly)
		length = OriginalLength();

	TRACE("_CopyPartialBegin(%s, single only %d)\n",
		isWrite ? "write" : "read", singleBlockOnly);

	if (isWrite) {
		return fParent->CopyData(OriginalOffset(),
			(uint8*)fDMABuffer->BounceBuffer() + relativeOffset, length);
	} else {
		return fParent->CopyData(
			(uint8*)fDMABuffer->BounceBuffer() + relativeOffset,
			OriginalOffset(), length);
	}
}


status_t
IOOperation::_CopyPartialEnd(bool isWrite)
{
	TRACE("_CopyPartialEnd(%s)\n", isWrite ? "write" : "read");

	const iovec& lastVec = fDMABuffer->VecAt(fDMABuffer->VecCount() - 1);
	off_t lastVecPos = fOffset + fLength - fBlockSize;
	uint8* base = (uint8*)fDMABuffer->BounceBuffer() + ((addr_t)lastVec.iov_base
		+ lastVec.iov_len - fBlockSize - fDMABuffer->PhysicalBounceBuffer());
		// NOTE: this won't work if we don't use the bounce buffer contiguously
		// (because of boundary alignments).
	size_t length = OriginalOffset() + OriginalLength() - lastVecPos;

	if (isWrite)
		return fParent->CopyData(lastVecPos, base, length);

	return fParent->CopyData(base, lastVecPos, length);
}


// #pragma mark -


IORequest::IORequest()
	:
	fFinishedCallback(NULL),
	fFinishedCookie(NULL),
	fIterationCallback(NULL),
	fIterationCookie(NULL)
{
	mutex_init(&fLock, "I/O request lock");
	fFinishedCondition.Init(this, "I/O request finished");
}


IORequest::~IORequest()
{
	mutex_lock(&fLock);
	mutex_destroy(&fLock);
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

	fPendingChildren = 0;

	fStatus = 1;

	return B_OK;
}


void
IORequest::SetFinishedCallback(io_request_callback callback, void* cookie)
{
	fFinishedCallback = callback;
	fFinishedCookie = cookie;
}


void
IORequest::SetIterationCallback(io_request_callback callback, void* cookie)
{
	fIterationCallback = callback;
	fIterationCookie = cookie;
}


status_t
IORequest::Wait(uint32 flags, bigtime_t timeout)
{
	MutexLocker locker(fLock);

	if (IsFinished())
		return Status();

	ConditionVariableEntry entry;
	fFinishedCondition.Add(&entry);

	locker.Unlock();

	status_t error = entry.Wait(B_CAN_INTERRUPT);
	if (error != B_OK)
		return error;

	return Status();
}


void
IORequest::NotifyFinished()
{
	TRACE("IORequest::NotifyFinished(): request: %p\n", this);

	MutexLocker locker(fLock);

	if (fStatus == B_OK && RemainingBytes() > 0) {
		// The request is not really done yet. If it has an iteration callback,
		// call it.
		if (fIterationCallback != NULL) {
			locker.Unlock();
			fIterationCallback(fIterationCookie, this);
		}
	} else {
		// Cache the callbacks before we unblock waiters and unlock. Any of the
		// following could delete this request, so we don't want to touch it
		// once we have started telling others that it is done.
		IORequest* parent = fParent;
		io_request_callback finishedCallback = fFinishedCallback;
		void* finishedCookie = fFinishedCookie;
		status_t status = fStatus;

		// unblock waiters
		fFinishedCondition.NotifyAll();

		locker.Unlock();

		// notify callback
		if (finishedCallback != NULL)
			finishedCallback(finishedCookie, this);

		// notify parent
		if (parent != NULL)
			parent->SubrequestFinished(this, status);
	}
}


/*!	Returns whether this request or any of it's ancestors has a finished or
	notification callback. Used to decide whether NotifyFinished() can be called
	synchronously.
*/
bool
IORequest::HasCallbacks() const
{
	if (fFinishedCallback != NULL || fIterationCallback != NULL)
		return true;

	return fParent != NULL && fParent->HasCallbacks();
}


void
IORequest::OperationFinished(IOOperation* operation, status_t status)
{
	TRACE("IORequest::OperationFinished(%p, %#lx): request: %p\n", operation,
		status, this);

	MutexLocker locker(fLock);

	fChildren.Remove(operation);
	operation->SetParent(NULL);

	if (status != B_OK && fStatus == 1)
		fStatus = status;

	if (--fPendingChildren > 0)
		return;

	// last child finished

	// set status, if not done yet
	if (fStatus == 1)
		fStatus = B_OK;

	// unlock the memory
	// TODO: That should only happen for the request that locked the memory,
	// not for its ancestors.
	if (fBuffer->IsVirtual())
		fBuffer->UnlockMemory(fTeam, fIsWrite);
}


void
IORequest::SubrequestFinished(IORequest* request, status_t status)
{
	TRACE("IORequest::SubrequestFinished(%p, %#lx): request: %p\n", request,
		status, this);

	MutexLocker locker(fLock);

	if (status != B_OK && fStatus == 1)
		fStatus = status;

	if (--fPendingChildren > 0)
		return;

	// last child finished

	// set status, if not done yet
	if (fStatus == 1)
		fStatus = B_OK;

	locker.Unlock();

	NotifyFinished();
}


void
IORequest::Advance(size_t bySize)
{
	TRACE("IORequest::Advance(%lu): remaining: %lu -> %lu\n", bySize,
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
	MutexLocker locker(fLock);
	fChildren.Add(operation);
	fPendingChildren++;
}


void
IORequest::RemoveOperation(IOOperation* operation)
{
	MutexLocker locker(fLock);
	fChildren.Remove(operation);
	operation->SetParent(NULL);
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
	status_t (*copyFunction)(void*, void*, size_t, team_id, bool);
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
			(uint8*)vecs[0].iov_base + vecOffset, toCopy, fTeam, copyIn);
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
	team_id team, bool copyIn)
{
	TRACE("  IORequest::_CopySimple(%p, %p, %lu, %d)\n", bounceBuffer, external,
		size, copyIn);
	if (copyIn)
		memcpy(bounceBuffer, external, size);
	else
		memcpy(external, bounceBuffer, size);
	return B_OK;
}


/* static */ status_t
IORequest::_CopyPhysical(void* _bounceBuffer, void* _external, size_t size,
	team_id team, bool copyIn)
{
	uint8* bounceBuffer = (uint8*)_bounceBuffer;
	addr_t external = (addr_t)_external;

	while (size > 0) {
		addr_t virtualAddress;
		status_t error = vm_get_physical_page(external, &virtualAddress, 0);
		if (error != B_OK)
			return error;

		size_t toCopy = min_c(size, B_PAGE_SIZE);
		_CopySimple(bounceBuffer, (void*)virtualAddress, toCopy, team, copyIn);

		vm_put_physical_page(virtualAddress);

		size -= toCopy;
		bounceBuffer += toCopy;
		external += toCopy;
	}

	return B_OK;
}


/* static */ status_t
IORequest::_CopyUser(void* _bounceBuffer, void* _external, size_t size,
	team_id team, bool copyIn)
{
	uint8* bounceBuffer = (uint8*)_bounceBuffer;
	uint8* external = (uint8*)_external;

	while (size > 0) {
		static const int32 kEntryCount = 8;
		physical_entry entries[kEntryCount];

		uint32 count = kEntryCount;
		status_t error = get_memory_map_etc(team, external, size, entries,
			&count);
		if (error != B_OK && error != B_BUFFER_OVERFLOW) {
			panic("IORequest::_CopyUser(): Failed to get physical memory for "
				"user memory %p\n", external);
			return B_BAD_ADDRESS;
		}

		for (uint32 i = 0; i < count; i++) {
			const physical_entry& entry = entries[i];
			error = _CopyPhysical(bounceBuffer, entry.address,
				entry.size, team, copyIn);
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
