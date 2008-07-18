/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */

#include "io_requests.h"

#include <string.h>

#include <vm.h>

#include "dma_resources.h"


IORequestChunk::~IORequestChunk()
{
}


//	#pragma mark -


status_t
IOBuffer::LockMemory(bool isWrite)
{
	for (uint32 i = 0; i < fCount; i++) {
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
	_UnlockMemory(fCount, isWrite);
}


// #pragma mark -


bool
IOOperation::Finish()
{
	if (fStatus == B_OK) {
		if (IsPartialOperation() && IsWrite()) {
			// partial write: copy partial request to bounce buffer
			status_t error = fParent->CopyData(OriginalOffset(),
				(uint8*)fDMABuffer->BounceBuffer()
					+ (Offset() - OriginalOffset()),
				OriginalLength());
			if (error == B_OK) {
				// We're done with the first phase only (read-in block). Now
				// do the actual write.
				SetPartialOperation(false);
				SetStatus(1);
					// TODO: Is there a race condition, if the request is
					// aborted at the same time?
				return false;
			}

			SetStatus(error);
		}
	}

	if (IsRead() && UsesBounceBuffer()) {
		// copy the bounce buffer to the final location
		status_t error = fParent->CopyData((uint8*)fDMABuffer->BounceBuffer()
				+ (Offset() - OriginalOffset()), OriginalOffset(),
			OriginalLength());
		if (error != B_OK)
			SetStatus(error);
	}

	// notify parent request
	if (fParent != NULL)
		fParent->ChunkFinished(this, fStatus);

	return true;
}


void
IOOperation::SetRequest(IORequest* request)
{
	if (fParent != NULL)
		fParent->RemoveOperation(this);

	fParent = request;
	fStatus = 1;

	if (fParent != NULL)
		fParent->AddOperation(this);
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


void
IOOperation::SetPartialOperation(bool partialOperation)
{
	fIsPartitialOperation = partialOperation;
}


bool
IOOperation::IsWrite() const
{
	return fParent->IsWrite();
}


bool
IOOperation::IsRead() const
{
	return fParent->IsRead();
}


// #pragma mark -


void
IORequest::Advance(size_t bySize)
{
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
	uint8* buffer = (uint8*)_buffer;

	if (offset < fOffset || offset + size > fOffset + size) {
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
		_CopySimple(bounceBuffer, (void*)external, toCopy, copyIn);

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
