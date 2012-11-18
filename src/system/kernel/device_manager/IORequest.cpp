/*
 * Copyright 2008-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "IORequest.h"

#include <string.h>

#include <arch/debug.h>
#include <debug.h>
#include <heap.h>
#include <kernel.h>
#include <thread.h>
#include <util/AutoLock.h>
#include <vm/vm.h>
#include <vm/VMAddressSpace.h>

#include "dma_resources.h"


//#define TRACE_IO_REQUEST
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


struct virtual_vec_cookie {
	uint32			vec_index;
	generic_size_t	vec_offset;
	area_id			mapped_area;
	void*			physical_page_handle;
	addr_t			virtual_address;
};


IOBuffer*
IOBuffer::Create(uint32 count, bool vip)
{
	size_t size = sizeof(IOBuffer) + sizeof(generic_io_vec) * (count - 1);
	IOBuffer* buffer
		= (IOBuffer*)(malloc_etc(size, vip ? HEAP_PRIORITY_VIP : 0));
	if (buffer == NULL)
		return NULL;

	buffer->fCapacity = count;
	buffer->fVecCount = 0;
	buffer->fUser = false;
	buffer->fPhysical = false;
	buffer->fVIP = vip;
	buffer->fMemoryLocked = false;

	return buffer;
}


void
IOBuffer::Delete()
{
	if (this == NULL)
		return;

	free_etc(this, fVIP ? HEAP_PRIORITY_VIP : 0);
}


void
IOBuffer::SetVecs(generic_size_t firstVecOffset, const generic_io_vec* vecs,
	uint32 count, generic_size_t length, uint32 flags)
{
	memcpy(fVecs, vecs, sizeof(generic_io_vec) * count);

	if (count > 0 && firstVecOffset > 0) {
		fVecs[0].base += firstVecOffset;
		fVecs[0].length -= firstVecOffset;
	}

	fVecCount = count;
	fLength = length;
	fPhysical = (flags & B_PHYSICAL_IO_REQUEST) != 0;
	fUser = !fPhysical && IS_USER_ADDRESS(vecs[0].base);
}


status_t
IOBuffer::GetNextVirtualVec(void*& _cookie, iovec& vector)
{
	virtual_vec_cookie* cookie = (virtual_vec_cookie*)_cookie;
	if (cookie == NULL) {
		cookie = new(malloc_flags(fVIP ? HEAP_PRIORITY_VIP : 0))
			virtual_vec_cookie;
		if (cookie == NULL)
			return B_NO_MEMORY;

		cookie->vec_index = 0;
		cookie->vec_offset = 0;
		cookie->mapped_area = -1;
		cookie->physical_page_handle = NULL;
		cookie->virtual_address = 0;
		_cookie = cookie;
	}

	// recycle a potential previously mapped page
	if (cookie->physical_page_handle != NULL) {
// TODO: This check is invalid! The physical page mapper is not required to
// return a non-NULL handle (the generic implementation does not)!
		vm_put_physical_page(cookie->virtual_address,
			cookie->physical_page_handle);
	}

	if (cookie->vec_index >= fVecCount)
		return B_BAD_INDEX;

	if (!fPhysical) {
		vector.iov_base = (void*)(addr_t)fVecs[cookie->vec_index].base;
		vector.iov_len = fVecs[cookie->vec_index++].length;
		return B_OK;
	}

	if (cookie->vec_index == 0
		&& (fVecCount > 1 || fVecs[0].length > B_PAGE_SIZE)) {
		void* mappedAddress;
		addr_t mappedSize;

// TODO: This is a potential violation of the VIP requirement, since
// vm_map_physical_memory_vecs() allocates memory without special flags!
		cookie->mapped_area = vm_map_physical_memory_vecs(
			VMAddressSpace::KernelID(), "io buffer mapped physical vecs",
			&mappedAddress, B_ANY_KERNEL_ADDRESS, &mappedSize,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, fVecs, fVecCount);

		if (cookie->mapped_area >= 0) {
			vector.iov_base = mappedAddress;
			vector.iov_len = mappedSize;
			return B_OK;
		} else
			ktrace_printf("failed to map area: %s\n", strerror(cookie->mapped_area));
	}

	// fallback to page wise mapping
	generic_io_vec& currentVec = fVecs[cookie->vec_index];
	generic_addr_t address = currentVec.base + cookie->vec_offset;
	size_t pageOffset = address % B_PAGE_SIZE;

// TODO: This is a potential violation of the VIP requirement, since
// vm_get_physical_page() may allocate memory without special flags!
	status_t result = vm_get_physical_page(address - pageOffset,
		&cookie->virtual_address, &cookie->physical_page_handle);
	if (result != B_OK)
		return result;

	generic_size_t length = min_c(currentVec.length - cookie->vec_offset,
		B_PAGE_SIZE - pageOffset);

	vector.iov_base = (void*)(cookie->virtual_address + pageOffset);
	vector.iov_len = length;

	cookie->vec_offset += length;
	if (cookie->vec_offset >= currentVec.length) {
		cookie->vec_index++;
		cookie->vec_offset = 0;
	}

	return B_OK;
}


void
IOBuffer::FreeVirtualVecCookie(void* _cookie)
{
	virtual_vec_cookie* cookie = (virtual_vec_cookie*)_cookie;
	if (cookie->mapped_area >= 0)
		delete_area(cookie->mapped_area);
// TODO: A vm_get_physical_page() may still be unmatched!

	free_etc(cookie, fVIP ? HEAP_PRIORITY_VIP : 0);
}


status_t
IOBuffer::LockMemory(team_id team, bool isWrite)
{
	if (fMemoryLocked) {
		panic("memory already locked!");
		return B_BAD_VALUE;
	}

	for (uint32 i = 0; i < fVecCount; i++) {
		status_t status = lock_memory_etc(team, (void*)(addr_t)fVecs[i].base,
			fVecs[i].length, isWrite ? 0 : B_READ_DEVICE);
		if (status != B_OK) {
			_UnlockMemory(team, i, isWrite);
			return status;
		}
	}

	fMemoryLocked = true;
	return B_OK;
}


void
IOBuffer::_UnlockMemory(team_id team, size_t count, bool isWrite)
{
	for (uint32 i = 0; i < count; i++) {
		unlock_memory_etc(team, (void*)(addr_t)fVecs[i].base, fVecs[i].length,
			isWrite ? 0 : B_READ_DEVICE);
	}
}


void
IOBuffer::UnlockMemory(team_id team, bool isWrite)
{
	if (!fMemoryLocked) {
		panic("memory not locked");
		return;
	}

	_UnlockMemory(team, fVecCount, isWrite);
	fMemoryLocked = false;
}


void
IOBuffer::Dump() const
{
	kprintf("IOBuffer at %p\n", this);

	kprintf("  origin:     %s\n", fUser ? "user" : "kernel");
	kprintf("  kind:       %s\n", fPhysical ? "physical" : "virtual");
	kprintf("  length:     %" B_PRIuGENADDR "\n", fLength);
	kprintf("  capacity:   %" B_PRIuSIZE "\n", fCapacity);
	kprintf("  vecs:       %" B_PRIuSIZE "\n", fVecCount);

	for (uint32 i = 0; i < fVecCount; i++) {
		kprintf("    [%" B_PRIu32 "] %#" B_PRIxGENADDR ", %" B_PRIuGENADDR "\n",
			i, fVecs[i].base, fVecs[i].length);
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
				fDMABuffer->VecAt(fSavedVecIndex).length = fSavedVecLength;

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
				generic_io_vec& vec = fDMABuffer->VecAt(fSavedVecIndex);
				vec.base += vec.length - fSavedVecLength;
				vec.length = fSavedVecLength;

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
		uint8* bounceBuffer = (uint8*)fDMABuffer->BounceBufferAddress();
		phys_addr_t bounceBufferStart
			= fDMABuffer->PhysicalBounceBufferAddress();
		phys_addr_t bounceBufferEnd = bounceBufferStart
			+ fDMABuffer->BounceBufferSize();

		const generic_io_vec* vecs = fDMABuffer->Vecs();
		uint32 vecCount = fDMABuffer->VecCount();

		status_t error = B_OK;

		// We iterate through the vecs we have read, moving offset (the device
		// offset) as we go. If [offset, offset + vec.length) intersects with
		// [startOffset, endOffset) we copy to the final location.
		off_t offset = fOffset;
		const off_t startOffset = fOriginalOffset;
		const off_t endOffset = fOriginalOffset + fOriginalLength;

		for (uint32 i = 0; error == B_OK && i < vecCount; i++) {
			const generic_io_vec& vec = vecs[i];
			generic_addr_t base = vec.base;
			generic_size_t length = vec.length;

			if (offset < startOffset) {
				// If the complete vector is before the start offset, skip it.
				if (offset + (off_t)length <= startOffset) {
					offset += length;
					continue;
				}

				// The vector starts before the start offset, but intersects
				// with it. Skip the part we aren't interested in.
				generic_size_t diff = startOffset - offset;
				offset += diff;
				base += diff;
				length -= diff;
			}

			if (offset + (off_t)length > endOffset) {
				// If we're already beyond the end offset, we're done.
				if (offset >= endOffset)
					break;

				// The vector extends beyond the end offset -- cut it.
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
			uint8* bounceBuffer = (uint8*)fDMABuffer->BounceBufferAddress();
			phys_addr_t bounceBufferStart
				= fDMABuffer->PhysicalBounceBufferAddress();
			phys_addr_t bounceBufferEnd = bounceBufferStart
				+ fDMABuffer->BounceBufferSize();

			const generic_io_vec* vecs = fDMABuffer->Vecs();
			uint32 vecCount = fDMABuffer->VecCount();
			generic_size_t vecOffset = 0;
			uint32 i = 0;

			off_t offset = fOffset;
			off_t endOffset = fOffset + fLength;

			if (HasPartialBegin()) {
				// skip first block
				generic_size_t toSkip = fBlockSize;
				while (toSkip > 0) {
					if (vecs[i].length <= toSkip) {
						toSkip -= vecs[i].length;
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
				generic_size_t toSkip = fBlockSize;
				while (toSkip > 0) {
					if (vecs[vecCount - 1].length <= toSkip) {
						toSkip -= vecs[vecCount - 1].length;
						vecCount--;
					} else
						break;
				}

				endOffset -= fBlockSize;
			}

			for (; i < vecCount; i++) {
				const generic_io_vec& vec = vecs[i];
				generic_addr_t base = vec.base + vecOffset;
				generic_size_t length = vec.length - vecOffset;
				vecOffset = 0;

				if (base >= bounceBufferStart && base < bounceBufferEnd) {
					if (offset + (off_t)length > endOffset)
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
IOOperation::SetOriginalRange(off_t offset, generic_size_t length)
{
	fOriginalOffset = fOffset = offset;
	fOriginalLength = fLength = length;
}


void
IOOperation::SetRange(off_t offset, generic_size_t length)
{
	fOffset = offset;
	fLength = length;
}


off_t
IOOperation::Offset() const
{
	return fPhase == PHASE_READ_END ? fOffset + fLength - fBlockSize : fOffset;
}


generic_size_t
IOOperation::Length() const
{
	return fPhase == PHASE_DO_ALL ? fLength : fBlockSize;
}


generic_io_vec*
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
		generic_io_vec* vecs = fDMABuffer->Vecs();
		uint32 vecCount = fDMABuffer->VecCount();
		generic_size_t vecLength = fBlockSize;
		for (uint32 i = 0; i < vecCount; i++) {
			generic_io_vec& vec = vecs[i];
			if (vec.length >= vecLength) {
				fSavedVecIndex = i;
				fSavedVecLength = vec.length;
				vec.length = vecLength;
				break;
			}
			vecLength -= vec.length;
		}
	} else if (fPhase == PHASE_READ_END) {
		generic_io_vec* vecs = fDMABuffer->Vecs();
		uint32 vecCount = fDMABuffer->VecCount();
		generic_size_t vecLength = fBlockSize;
		for (int32 i = vecCount - 1; i >= 0; i--) {
			generic_io_vec& vec = vecs[i];
			if (vec.length >= vecLength) {
				fSavedVecIndex = i;
				fSavedVecLength = vec.length;
				vec.base += vec.length - vecLength;
				vec.length = vecLength;
				break;
			}
			vecLength -= vec.length;
		}
	}
}


status_t
IOOperation::_CopyPartialBegin(bool isWrite, bool& singleBlockOnly)
{
	generic_size_t relativeOffset = OriginalOffset() - fOffset;
	generic_size_t length = fBlockSize - relativeOffset;

	singleBlockOnly = length >= OriginalLength();
	if (singleBlockOnly)
		length = OriginalLength();

	TRACE("_CopyPartialBegin(%s, single only %d)\n",
		isWrite ? "write" : "read", singleBlockOnly);

	if (isWrite) {
		return fParent->CopyData(OriginalOffset(),
			(uint8*)fDMABuffer->BounceBufferAddress() + relativeOffset, length);
	} else {
		return fParent->CopyData(
			(uint8*)fDMABuffer->BounceBufferAddress() + relativeOffset,
			OriginalOffset(), length);
	}
}


status_t
IOOperation::_CopyPartialEnd(bool isWrite)
{
	TRACE("_CopyPartialEnd(%s)\n", isWrite ? "write" : "read");

	const generic_io_vec& lastVec
		= fDMABuffer->VecAt(fDMABuffer->VecCount() - 1);
	off_t lastVecPos = fOffset + fLength - fBlockSize;
	uint8* base = (uint8*)fDMABuffer->BounceBufferAddress()
		+ (lastVec.base + lastVec.length - fBlockSize
		- fDMABuffer->PhysicalBounceBufferAddress());
		// NOTE: this won't work if we don't use the bounce buffer contiguously
		// (because of boundary alignments).
	generic_size_t length = OriginalOffset() + OriginalLength() - lastVecPos;

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
	kprintf("  offset:           %-8" B_PRIdOFF " (original: %" B_PRIdOFF ")\n",
		fOffset, fOriginalOffset);
	kprintf("  length:           %-8" B_PRIuGENADDR " (original: %"
		B_PRIuGENADDR ")\n", fLength, fOriginalLength);
	kprintf("  transferred:      %" B_PRIuGENADDR "\n", fTransferredBytes);
	kprintf("  block size:       %" B_PRIuGENADDR "\n", fBlockSize);
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
	fIsNotified(false),
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


/* static */ IORequest*
IORequest::Create(bool vip)
{
	return vip
		? new(malloc_flags(HEAP_PRIORITY_VIP)) IORequest
		: new(std::nothrow) IORequest;
}


status_t
IORequest::Init(off_t offset, generic_addr_t buffer, generic_size_t length,
	bool write, uint32 flags)
{
	generic_io_vec vec;
	vec.base = buffer;
	vec.length = length;
	return Init(offset, &vec, 1, length, write, flags);
}


status_t
IORequest::Init(off_t offset, generic_size_t firstVecOffset,
	const generic_io_vec* vecs, size_t count, generic_size_t length, bool write,
	uint32 flags)
{
	fBuffer = IOBuffer::Create(count, (flags & B_VIP_IO_REQUEST) != 0);
	if (fBuffer == NULL)
		return B_NO_MEMORY;

	fBuffer->SetVecs(firstVecOffset, vecs, count, length, flags);

	fOwner = NULL;
	fOffset = offset;
	fLength = length;
	fRelativeParentOffset = 0;
	fTransferSize = 0;
	fFlags = flags;
	Thread* thread = thread_get_current_thread();
	fTeam = thread->team->id;
	fThread = thread->id;
	fIsWrite = write;
	fPartialTransfer = false;
	fSuppressChildNotifications = false;

	// these are for iteration
	fVecIndex = 0;
	fVecOffset = 0;
	fRemainingBytes = length;

	fPendingChildren = 0;

	fStatus = 1;

	return B_OK;
}


status_t
IORequest::CreateSubRequest(off_t parentOffset, off_t offset,
	generic_size_t length, IORequest*& _subRequest)
{
	ASSERT(parentOffset >= fOffset && length <= fLength
		&& parentOffset - fOffset <= (off_t)(fLength - length));

	// find start vec
	generic_size_t vecOffset = parentOffset - fOffset;
	generic_io_vec* vecs = fBuffer->Vecs();
	int32 vecCount = fBuffer->VecCount();
	int32 startVec = 0;
	for (; startVec < vecCount; startVec++) {
		const generic_io_vec& vec = vecs[startVec];
		if (vecOffset < vec.length)
			break;

		vecOffset -= vec.length;
	}

	// count vecs
	generic_size_t currentVecOffset = vecOffset;
	int32 endVec = startVec;
	generic_size_t remainingLength = length;
	for (; endVec < vecCount; endVec++) {
		const generic_io_vec& vec = vecs[endVec];
		if (vec.length - currentVecOffset >= remainingLength)
			break;

		remainingLength -= vec.length - currentVecOffset;
		currentVecOffset = 0;
	}

	// create subrequest
	IORequest* subRequest = Create((fFlags & B_VIP_IO_REQUEST) != 0);
	if (subRequest == NULL)
		return B_NO_MEMORY;

	status_t error = subRequest->Init(offset, vecOffset, vecs + startVec,
		endVec - startVec + 1, length, fIsWrite, fFlags & ~B_DELETE_IO_REQUEST);
	if (error != B_OK) {
		delete subRequest;
		return error;
	}

	subRequest->fRelativeParentOffset = parentOffset - fOffset;
	subRequest->fTeam = fTeam;
	subRequest->fThread = fThread;

	_subRequest = subRequest;
	subRequest->SetParent(this);

	MutexLocker _(fLock);

	fChildren.Add(subRequest);
	fPendingChildren++;
	TRACE("IORequest::CreateSubRequest(): request: %p, subrequest: %p\n", this,
		subRequest);

	return B_OK;
}


void
IORequest::DeleteSubRequests()
{
	while (IORequestChunk* chunk = fChildren.RemoveHead())
		delete chunk;
	fPendingChildren = 0;
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

	if (IsFinished() && fIsNotified)
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

	ASSERT(!fIsNotified);
	ASSERT(fPendingChildren == 0);
	ASSERT(fChildren.IsEmpty()
		|| dynamic_cast<IOOperation*>(fChildren.Head()) == NULL);

	// unlock the memory
	if (fBuffer->IsMemoryLocked())
		fBuffer->UnlockMemory(fTeam, fIsWrite);

	// Cache the callbacks before we unblock waiters and unlock. Any of the
	// following could delete this request, so we don't want to touch it
	// once we have started telling others that it is done.
	IORequest* parent = fParent;
	io_request_finished_callback finishedCallback = fFinishedCallback;
	void* finishedCookie = fFinishedCookie;
	status_t status = fStatus;
	generic_size_t lastTransferredOffset
		= fRelativeParentOffset + fTransferSize;
	bool partialTransfer = status != B_OK || fPartialTransfer;
	bool deleteRequest = (fFlags & B_DELETE_IO_REQUEST) != 0;

	// unblock waiters
	fIsNotified = true;
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

	if (deleteRequest)
		delete this;
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
	bool partialTransfer, generic_size_t transferEndOffset)
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
}


void
IORequest::SubRequestFinished(IORequest* request, status_t status,
	bool partialTransfer, generic_size_t transferEndOffset)
{
	TRACE("IORequest::SubrequestFinished(%p, %#" B_PRIx32 ", %d, %"
		B_PRIuGENADDR "): request: %p\n", request, status, partialTransfer, transferEndOffset, this);

	MutexLocker locker(fLock);

	if (status != B_OK || partialTransfer) {
		if (fTransferSize > transferEndOffset)
			fTransferSize = transferEndOffset;
		fPartialTransfer = true;
	}

	if (status != B_OK && fStatus == 1)
		fStatus = status;

	if (--fPendingChildren > 0 || fSuppressChildNotifications)
		return;

	// last child finished

	// set status, if not done yet
	if (fStatus == 1)
		fStatus = B_OK;

	locker.Unlock();

	NotifyFinished();
}


void
IORequest::SetUnfinished()
{
	MutexLocker _(fLock);
	ResetStatus();
}


void
IORequest::SetTransferredBytes(bool partialTransfer,
	generic_size_t transferredBytes)
{
	TRACE("%p->IORequest::SetTransferredBytes(%d, %" B_PRIuGENADDR ")\n", this,
		partialTransfer, transferredBytes);

	MutexLocker _(fLock);

	fPartialTransfer = partialTransfer;
	fTransferSize = transferredBytes;
}


void
IORequest::SetSuppressChildNotifications(bool suppress)
{
	fSuppressChildNotifications = suppress;
}


void
IORequest::Advance(generic_size_t bySize)
{
	TRACE("IORequest::Advance(%" B_PRIuGENADDR "): remaining: %" B_PRIuGENADDR
		" -> %" B_PRIuGENADDR "\n", bySize, fRemainingBytes,
		fRemainingBytes - bySize);
	fRemainingBytes -= bySize;
	fTransferSize += bySize;

	generic_io_vec* vecs = fBuffer->Vecs();
	uint32 vecCount = fBuffer->VecCount();
	while (fVecIndex < vecCount
			&& vecs[fVecIndex].length - fVecOffset <= bySize) {
		bySize -= vecs[fVecIndex].length - fVecOffset;
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
	TRACE("IORequest::AddOperation(%p): request: %p\n", operation, this);
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

	if (offset < fOffset || offset + (off_t)size > fOffset + (off_t)fLength) {
		panic("IORequest::_CopyData(): invalid range: (%" B_PRIdOFF ", %lu)",
			offset, size);
		return B_BAD_VALUE;
	}

	// If we can, we directly copy from/to the virtual buffer. The memory is
	// locked in this case.
	status_t (*copyFunction)(void*, generic_addr_t, size_t, team_id, bool);
	if (fBuffer->IsPhysical()) {
		copyFunction = &IORequest::_CopyPhysical;
	} else {
		copyFunction = fBuffer->IsUser()
			? &IORequest::_CopyUser : &IORequest::_CopySimple;
	}

	// skip bytes if requested
	generic_io_vec* vecs = fBuffer->Vecs();
	generic_size_t skipBytes = offset - fOffset;
	generic_size_t vecOffset = 0;
	while (skipBytes > 0) {
		if (vecs[0].length > skipBytes) {
			vecOffset = skipBytes;
			break;
		}

		skipBytes -= vecs[0].length;
		vecs++;
	}

	// copy vector-wise
	while (size > 0) {
		generic_size_t toCopy = min_c(size, vecs[0].length - vecOffset);
		status_t error = copyFunction(buffer, vecs[0].base + vecOffset, toCopy,
			fTeam, copyIn);
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
IORequest::_CopySimple(void* bounceBuffer, generic_addr_t external, size_t size,
	team_id team, bool copyIn)
{
	TRACE("  IORequest::_CopySimple(%p, %#" B_PRIxGENADDR ", %lu, %d)\n",
		bounceBuffer, external, size, copyIn);
	if (copyIn)
		memcpy(bounceBuffer, (void*)(addr_t)external, size);
	else
		memcpy((void*)(addr_t)external, bounceBuffer, size);
	return B_OK;
}


/* static */ status_t
IORequest::_CopyPhysical(void* bounceBuffer, generic_addr_t external,
	size_t size, team_id team, bool copyIn)
{
	if (copyIn)
		return vm_memcpy_from_physical(bounceBuffer, external, size, false);

	return vm_memcpy_to_physical(external, bounceBuffer, size, false);
}


/* static */ status_t
IORequest::_CopyUser(void* _bounceBuffer, generic_addr_t _external, size_t size,
	team_id team, bool copyIn)
{
	uint8* bounceBuffer = (uint8*)_bounceBuffer;
	uint8* external = (uint8*)(addr_t)_external;

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
			error = _CopyPhysical(bounceBuffer, entry.address, entry.size, team,
				copyIn);
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

	kprintf("  owner:             %p\n", fOwner);
	kprintf("  parent:            %p\n", fParent);
	kprintf("  status:            %s\n", strerror(fStatus));
	kprintf("  mutex:             %p\n", &fLock);
	kprintf("  IOBuffer:          %p\n", fBuffer);
	kprintf("  offset:            %" B_PRIdOFF "\n", fOffset);
	kprintf("  length:            %" B_PRIuGENADDR "\n", fLength);
	kprintf("  transfer size:     %" B_PRIuGENADDR "\n", fTransferSize);
	kprintf("  relative offset:   %" B_PRIuGENADDR "\n", fRelativeParentOffset);
	kprintf("  pending children:  %" B_PRId32 "\n", fPendingChildren);
	kprintf("  flags:             %#" B_PRIx32 "\n", fFlags);
	kprintf("  team:              %" B_PRId32 "\n", fTeam);
	kprintf("  thread:            %" B_PRId32 "\n", fThread);
	kprintf("  r/w:               %s\n", fIsWrite ? "write" : "read");
	kprintf("  partial transfer:  %s\n", fPartialTransfer ? "yes" : "no");
	kprintf("  finished cvar:     %p\n", &fFinishedCondition);
	kprintf("  iteration:\n");
	kprintf("    vec index:       %" B_PRIu32 "\n", fVecIndex);
	kprintf("    vec offset:      %" B_PRIuGENADDR "\n", fVecOffset);
	kprintf("    remaining bytes: %" B_PRIuGENADDR "\n", fRemainingBytes);
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
