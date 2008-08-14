/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */

#include "io_requests.h"

#include <string.h>

#include <heap.h>
#include <kernel.h>
#include <team.h>
#include <util/AutoLock.h>
#include <vm.h>

#include "dma_resources.h"


//#define TRACE_IO_REQUEST
#ifdef TRACE_IO_REQUEST
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif


#define VIP_HEAP_SIZE	1024 * 1024

// partial I/O operation phases
enum {
	PHASE_READ_BEGIN	= 0,
	PHASE_READ_END		= 1,
	PHASE_DO_ALL		= 2
};

heap_allocator* sVIPHeap;


// #pragma mark -


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
IOBuffer::Delete()
{
	free(this);
}


void
IOBuffer::SetVecs(size_t firstVecOffset, const iovec* vecs, uint32 count,
	size_t length, uint32 flags)
{
	memcpy(fVecs, vecs, sizeof(iovec) * count);
	if (count > 0 && firstVecOffset > 0) {
		fVecs[0].iov_base = (uint8*)fVecs[0].iov_base + firstVecOffset;
		fVecs[0].iov_len -= firstVecOffset;
	}

	fVecCount = count;
	fLength = length;
	fUser = IS_USER_ADDRESS(vecs[0].iov_base);
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


void
IOBuffer::Dump() const
{
	kprintf("IOBuffer at %p\n", this);

	kprintf("  origin:     %s\n", fUser ? "user" : "kernel");
	kprintf("  kind:       %s\n", fPhysical ? "physical" : "virtual");
	kprintf("  length:     %lu\n", fLength);
	kprintf("  capacity:   %lu\n", fCapacity);
	kprintf("  vecs:       %lu\n", fVecCount);

	for (uint32 i = 0; i < fVecCount; i++) {
		kprintf("    [%lu] %p, %lu\n", i, fVecs[i].iov_base, fVecs[i].iov_len);
	}
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

	fTransferredBytes = 0;

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


void
IOOperation::Dump() const
{
	kprintf("io_operation at %p\n", this);

	kprintf("  parent:           %p\n", fParent);
	kprintf("  status:           %s\n", strerror(fStatus));
	kprintf("  dma buffer:       %p\n", fDMABuffer);
	kprintf("  offset:           %-8Ld (original: %Ld)\n", fOffset,
		fOriginalOffset);
	kprintf("  length:           %-8lu (original: %lu)\n", fLength,
		fOriginalLength);
	kprintf("  transferred:      %lu\n", fTransferredBytes);
	kprintf("  block size:       %lu\n", fBlockSize);
	kprintf("  saved vec index:  %u\n", fSavedVecIndex);
	kprintf("  saved vec length: %u\n", fSavedVecLength);
	kprintf("  r/w:              %s\n", IsWrite() ? "write" : "read");
	kprintf("  phase:            %s\n", fPhase == PHASE_READ_BEGIN
		? "read begin" : fPhase == PHASE_READ_END ? "read end"
		: fPhase == PHASE_DO_ALL ? "do all" : "unknown");
	kprintf("  partial begin:    %s\n", fPartialBegin ? "yes" : "no");
	kprintf("  partial end:      %s\n", fPartialEnd ? "yes" : "no");
	kprintf("  bounce buffer:    %s\n", fUsesBounceBuffer ? "yes" : "no");

	set_debug_variable("_parent", (addr_t)fParent);
	set_debug_variable("_buffer", (addr_t)fDMABuffer);
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
	DeleteSubRequests();
	fBuffer->Delete();
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
IORequest::Init(off_t offset, size_t firstVecOffset, const iovec* vecs,
	size_t count, size_t length, bool write, uint32 flags)
{
	fBuffer = IOBuffer::Create(count);
	if (fBuffer == NULL)
		return B_NO_MEMORY;

	fBuffer->SetVecs(firstVecOffset, vecs, count, length, flags);

	fOffset = offset;
	fLength = length;
	fRelativeParentOffset = 0;
	fTransferSize = 0;
	fFlags = flags;
	fTeam = team_get_current_team_id();
	fIsWrite = write;
	fPartialTransfer = 0;

	// these are for iteration
	fVecIndex = 0;
	fVecOffset = 0;
	fRemainingBytes = length;

	fPendingChildren = 0;

	fStatus = 1;

	return B_OK;
}


status_t
IORequest::CreateSubRequest(off_t parentOffset, off_t offset, size_t length,
	IORequest*& _subRequest)
{
	ASSERT(parentOffset >= fOffset && length <= fLength
		&& parentOffset - fOffset <= fLength - length);

	// find start vec
	size_t vecOffset = parentOffset - fOffset;
	iovec* vecs = fBuffer->Vecs();
	int32 vecCount = fBuffer->VecCount();
	int32 startVec = 0;
	for (; startVec < vecCount; startVec++) {
		const iovec& vec = vecs[startVec];
		if (vecOffset < vec.iov_len)
			break;

		vecOffset -= vec.iov_len;
	}

	// count vecs
	size_t currentVecOffset = vecOffset;
	int32 endVec = startVec;
	size_t remainingLength = length;
	for (; endVec < vecCount; endVec++) {
		const iovec& vec = vecs[endVec];
		if (vec.iov_len - currentVecOffset >= remainingLength)
			break;

		remainingLength -= vec.iov_len - currentVecOffset;
		currentVecOffset = 0;
	}

	// create subrequest
	IORequest* subRequest = new(std::nothrow) IORequest;
		// TODO: Heed B_VIP_IO_REQUEST!
	if (subRequest == NULL)
		return B_NO_MEMORY;

	status_t error = subRequest->Init(offset, vecOffset, vecs + startVec,
		endVec - startVec + 1, length, fIsWrite, fFlags);
	if (error != B_OK) {
		delete subRequest;
		return error;
	}

	subRequest->fRelativeParentOffset = parentOffset - fOffset;

	_subRequest = subRequest;
	subRequest->SetParent(this);

	MutexLocker _(fLock);

	fChildren.Add(subRequest);
	fPendingChildren++;

	return B_OK;
}


void
IORequest::DeleteSubRequests()
{
	while (IORequestChunk* chunk = fChildren.RemoveHead())
		delete chunk;
}


void
IORequest::SetFinishedCallback(io_request_finished_callback callback,
	void* cookie)
{
	fFinishedCallback = callback;
	fFinishedCookie = cookie;
}


void
IORequest::SetIterationCallback(io_request_iterate_callback callback,
	void* cookie)
{
	fIterationCallback = callback;
	fIterationCookie = cookie;
}


io_request_finished_callback
IORequest::FinishedCallback(void** _cookie) const
{
	if (_cookie != NULL)
		*_cookie = fFinishedCookie;
	return fFinishedCallback;
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

	status_t error = entry.Wait(flags, timeout);
	if (error != B_OK)
		return error;

	return Status();
}


void
IORequest::NotifyFinished()
{
	TRACE("IORequest::NotifyFinished(): request: %p\n", this);

	MutexLocker locker(fLock);

	if (fStatus == B_OK && !fPartialTransfer && RemainingBytes() > 0) {
		// The request is not really done yet. If it has an iteration callback,
		// call it.
		if (fIterationCallback != NULL) {
			ResetStatus();
			locker.Unlock();
			bool partialTransfer = false;
			status_t error = fIterationCallback(fIterationCookie, this,
				&partialTransfer);
			if (error == B_OK && !partialTransfer)
				return;

			// Iteration failed, which means we're responsible for notifying the
			// requests finished.
			locker.Lock();
			fStatus = error;
			fPartialTransfer = true;
		}
	}

	// Cache the callbacks before we unblock waiters and unlock. Any of the
	// following could delete this request, so we don't want to touch it
	// once we have started telling others that it is done.
	IORequest* parent = fParent;
	io_request_finished_callback finishedCallback = fFinishedCallback;
	void* finishedCookie = fFinishedCookie;
	status_t status = fStatus;
	size_t lastTransferredOffset = fRelativeParentOffset + fTransferSize;
	bool partialTransfer = status != B_OK || fPartialTransfer;

	// unblock waiters
	fFinishedCondition.NotifyAll();

	locker.Unlock();

	// notify callback
	if (finishedCallback != NULL) {
		finishedCallback(finishedCookie, this, status, partialTransfer,
			lastTransferredOffset);
	}

	// notify parent
	if (parent != NULL) {
		parent->SubRequestFinished(this, status, partialTransfer,
			lastTransferredOffset);
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
IORequest::SetStatusAndNotify(status_t status)
{
	MutexLocker locker(fLock);

	if (fStatus != 1)
		return;

	fStatus = status;

	locker.Unlock();

	NotifyFinished();
}


void
IORequest::OperationFinished(IOOperation* operation, status_t status,
	bool partialTransfer, size_t transferEndOffset)
{
	TRACE("IORequest::OperationFinished(%p, %#lx): request: %p\n", operation,
		status, this);

	MutexLocker locker(fLock);

	fChildren.Remove(operation);
	operation->SetParent(NULL);

	if (status != B_OK || partialTransfer) {
		if (fTransferSize > transferEndOffset)
			fTransferSize = transferEndOffset;
		fPartialTransfer = true;
	}

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
IORequest::SubRequestFinished(IORequest* request, status_t status,
	bool partialTransfer, size_t transferEndOffset)
{
	TRACE("IORequest::SubrequestFinished(%p, %#lx, %d, %lu): request: %p\n",
		request, status, partialTransfer, transferEndOffset, this);

	MutexLocker locker(fLock);

	if (status != B_OK || partialTransfer) {
		if (fTransferSize > transferEndOffset)
			fTransferSize = transferEndOffset;
		fPartialTransfer = true;
	}

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
IORequest::SetTransferredBytes(bool partialTransfer, size_t transferredBytes)
{
	TRACE("%p->IORequest::SetTransferredBytes(%d, %lu)\n", this,
		partialTransfer, transferredBytes);

	MutexLocker _(fLock);

	fPartialTransfer = partialTransfer;
	fTransferSize = transferredBytes;
}


void
IORequest::Advance(size_t bySize)
{
	TRACE("IORequest::Advance(%lu): remaining: %lu -> %lu\n", bySize,
		fRemainingBytes, fRemainingBytes - bySize);
	fRemainingBytes -= bySize;
	fTransferSize += bySize;

	iovec* vecs = fBuffer->Vecs();
	uint32 vecCount = fBuffer->VecCount();
	while (fVecIndex < vecCount
			&& vecs[fVecIndex].iov_len - fVecOffset <= bySize) {
		bySize -= vecs[fVecIndex].iov_len - fVecOffset;
		fVecOffset = 0;
		fVecIndex++;
	}

	fVecOffset += bySize;
}


IORequest*
IORequest::FirstSubRequest()
{
	return dynamic_cast<IORequest*>(fChildren.Head());
}


IORequest*
IORequest::NextSubRequest(IORequest* previous)
{
	if (previous == NULL)
		return NULL;
	return dynamic_cast<IORequest*>(fChildren.GetNext(previous));
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
		addr_t pageOffset = external % B_PAGE_SIZE;
		addr_t virtualAddress;
		status_t error = vm_get_physical_page(external - pageOffset,
			&virtualAddress, 0);
		if (error != B_OK)
			return error;

		size_t toCopy = min_c(size, B_PAGE_SIZE - pageOffset);
		_CopySimple(bounceBuffer, (void*)(virtualAddress + pageOffset), toCopy,
			team, copyIn);

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


void
IORequest::Dump() const
{
	kprintf("io_request at %p\n", this);

	kprintf("  parent:            %p\n", fParent);
	kprintf("  status:            %s\n", strerror(fStatus));
	kprintf("  mutex:             %p\n", &fLock);
	kprintf("  IOBuffer:          %p\n", fBuffer);
	kprintf("  offset:            %Ld\n", fOffset);
	kprintf("  length:            %lu\n", fLength);
	kprintf("  transfer size:     %lu\n", fTransferSize);
	kprintf("  relative offset:   %lu\n", fRelativeParentOffset);
	kprintf("  pending children:  %ld\n", fPendingChildren);
	kprintf("  flags:             %#lx\n", fFlags);
	kprintf("  team:              %ld\n", fTeam);
	kprintf("  r/w:               %s\n", fIsWrite ? "write" : "read");
	kprintf("  partial transfer:  %s\n", fPartialTransfer ? "yes" : "no");
	kprintf("  finished cvar:     %p\n", &fFinishedCondition);
	kprintf("  iteration:\n");
	kprintf("    vec index:       %lu\n", fVecIndex);
	kprintf("    vec offset:      %lu\n", fVecOffset);
	kprintf("    remaining bytes: %lu\n", fRemainingBytes);
	kprintf("  callbacks:\n");
	kprintf("    finished %p, cookie %p\n", fFinishedCallback, fFinishedCookie);
	kprintf("    iteration %p, cookie %p\n", fIterationCallback,
		fIterationCookie);
	kprintf("  children:\n");

	IORequestChunkList::ConstIterator iterator = fChildren.GetIterator();
	while (iterator.HasNext()) {
		kprintf("    %p\n", iterator.Next());
	}

	set_debug_variable("_parent", (addr_t)fParent);
	set_debug_variable("_mutex", (addr_t)&fLock);
	set_debug_variable("_buffer", (addr_t)fBuffer);
	set_debug_variable("_cvar", (addr_t)&fFinishedCondition);
}


// #pragma mark - allocator


void*
vip_io_request_malloc(size_t size)
{
	return heap_memalign(sVIPHeap, 0, size);
}


void
vip_io_request_free(void* address)
{
	heap_free(sVIPHeap, address);
}


void
io_request_free(void* address)
{
	if (heap_free(sVIPHeap, address) != B_OK)
		free(address);
}


void
vip_io_request_allocator_init()
{
	static const heap_class heapClass = {
		"VIP I/O",					/* name */
		100,						/* initial percentage */
		B_PAGE_SIZE / 8,			/* max allocation size */
		B_PAGE_SIZE,				/* page size */
		8,							/* min bin size */
		4,							/* bin alignment */
		8,							/* min count per page */
		16							/* max waste per page */
	};

	void* address = NULL;
	area_id area = create_area("VIP I/O heap", &address, B_ANY_KERNEL_ADDRESS,
		VIP_HEAP_SIZE, B_FULL_LOCK, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (area < B_OK) {
		panic("vip_io_request_allocator_init(): couldn't allocate VIP I/O "
			"heap area");
		return;
	}

	sVIPHeap = heap_create_allocator("VIP I/O heap", (addr_t)address,
		VIP_HEAP_SIZE, &heapClass);
	if (sVIPHeap == NULL) {
		panic("vip_io_request_allocator_init(): failed to create VIP I/O "
			"heap\n");
		return;
	}
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

