/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "UnixFifo.h"

#include <new>

#include <AutoDeleter.h>

#include <net_stack.h>
#include <util/ring_buffer.h>

#include "unix.h"


#define UNIX_FIFO_DEBUG_LEVEL	0
#define UNIX_DEBUG_LEVEL		UNIX_FIFO_DEBUG_LEVEL
#include "UnixDebug.h"


// #pragma mark - UnixRequest


UnixRequest::UnixRequest(const iovec* vecs, size_t count,
		ancillary_data_container* ancillaryData)
	:
	fVecs(vecs),
	fVecCount(count),
	fAncillaryData(ancillaryData),
	fTotalSize(0),
	fBytesTransferred(0),
	fVecIndex(0),
	fVecOffset(0)
{
	for (size_t i = 0; i < fVecCount; i++)
		fTotalSize += fVecs[i].iov_len;
}


void
UnixRequest::AddBytesTransferred(size_t size)
{
	fBytesTransferred += size;

	// also adjust the current iovec index/offset
	while (fVecIndex < fVecCount
			&& fVecs[fVecIndex].iov_len - fVecOffset <= size) {
		size -= fVecs[fVecIndex].iov_len - fVecOffset;
		fVecIndex++;
		fVecOffset = 0;
	}

	if (fVecIndex < fVecCount)
		fVecOffset += size;
}


bool
UnixRequest::GetCurrentChunk(void*& data, size_t& size)
{
	while (fVecIndex < fVecCount
			&& fVecOffset >= fVecs[fVecIndex].iov_len) {
		fVecIndex++;
		fVecOffset = 0;
	}
	if (fVecIndex >= fVecCount)
		return false;

	data = (uint8*)fVecs[fVecIndex].iov_base + fVecOffset;
	size = fVecs[fVecIndex].iov_len - fVecOffset;
	return true;
}


void
UnixRequest::SetAncillaryData(ancillary_data_container* data)
{
	fAncillaryData = data;
}


void
UnixRequest::AddAncillaryData(ancillary_data_container* data)
{
	if (fAncillaryData != NULL) {
		gStackModule->move_ancillary_data(data, fAncillaryData);
		gStackModule->delete_ancillary_data_container(data);
	} else
		fAncillaryData = data;
}


// #pragma mark - UnixBufferQueue


UnixBufferQueue::UnixBufferQueue(size_t capacity)
	:
	fBuffer(NULL),
	fCapacity(capacity)
{
}


UnixBufferQueue::~UnixBufferQueue()
{
	while (AncillaryDataEntry* entry = fAncillaryData.RemoveHead()) {
		gStackModule->delete_ancillary_data_container(entry->data);
		delete entry;
	}

	delete_ring_buffer(fBuffer);
}


status_t
UnixBufferQueue::Init()
{
	fBuffer = create_ring_buffer(fCapacity);
	if (fBuffer == NULL)
		return B_NO_MEMORY;
	return B_OK;
}


size_t
UnixBufferQueue::Readable() const
{
	return ring_buffer_readable(fBuffer);
}


size_t
UnixBufferQueue::Writable() const
{
	return ring_buffer_writable(fBuffer);
}


status_t
UnixBufferQueue::Read(UnixRequest& request)
{
	bool user = gStackModule->is_syscall();

	size_t readable = Readable();
	void* data;
	size_t size;

	while (readable > 0 && request.GetCurrentChunk(data, size)) {
		if (size > readable)
			size = readable;

		ssize_t bytesRead;
		if (user)
			bytesRead = ring_buffer_user_read(fBuffer, (uint8*)data, size);
		else
			bytesRead = ring_buffer_read(fBuffer, (uint8*)data, size);

		if (bytesRead < 0)
			return bytesRead;
		if (bytesRead == 0)
			return B_ERROR;

		// Adjust ancillary data entry offsets, respectively attach the ones
		// that belong to the read data to the request.
		if (AncillaryDataEntry* entry = fAncillaryData.Head()) {
			size_t offsetDelta = bytesRead;
			while (entry != NULL && offsetDelta > entry->offset) {
				// entry data have been read -- add ancillary data to request
				fAncillaryData.RemoveHead();
				offsetDelta -= entry->offset;
				request.AddAncillaryData(entry->data);
				delete entry;

				entry = fAncillaryData.Head();
			}

			if (entry != NULL)
				entry->offset -= offsetDelta;
		}

		request.AddBytesTransferred(bytesRead);
		readable -= bytesRead;
	}

	return B_OK;
}


status_t
UnixBufferQueue::Write(UnixRequest& request)
{
	bool user = gStackModule->is_syscall();

	size_t writable = Writable();
	void* data;
	size_t size;

	// If the request has ancillary data create an entry first.
	AncillaryDataEntry* ancillaryEntry = NULL;
	ObjectDeleter<AncillaryDataEntry> ancillaryEntryDeleter;
	if (writable > 0 && request.AncillaryData() != NULL) {
		ancillaryEntry = new(std::nothrow) AncillaryDataEntry;
		if (ancillaryEntry == NULL)
			return B_NO_MEMORY;

		ancillaryEntryDeleter.SetTo(ancillaryEntry);
		ancillaryEntry->data = request.AncillaryData();
		ancillaryEntry->offset = Readable();

		// The offsets are relative to the previous entry.
		AncillaryDataList::Iterator it = fAncillaryData.GetIterator();
		while (AncillaryDataEntry* entry = it.Next())
			ancillaryEntry->offset -= entry->offset;
			// TODO: This is inefficient when the list is long. Rather also
			// store and maintain the absolute offset of the last queued entry.
	}

	// write as much as we can
	while (writable > 0 && request.GetCurrentChunk(data, size)) {
		if (size > writable)
			size = writable;

		ssize_t bytesWritten;
		if (user)
			bytesWritten = ring_buffer_user_write(fBuffer, (uint8*)data, size);
		else
			bytesWritten = ring_buffer_write(fBuffer, (uint8*)data, size);

		if (bytesWritten < 0)
			return bytesWritten;
		if (bytesWritten == 0)
			return B_ERROR;

		if (ancillaryEntry != NULL) {
			fAncillaryData.Add(ancillaryEntry);
			ancillaryEntryDeleter.Detach();
			request.SetAncillaryData(NULL);
			ancillaryEntry = NULL;
		}

		request.AddBytesTransferred(bytesWritten);
		writable -= bytesWritten;
	}

	return B_OK;
}


status_t
UnixBufferQueue::SetCapacity(size_t capacity)
{
// TODO:...
return B_ERROR;
}


// #pragma mark -


UnixFifo::UnixFifo(size_t capacity)
	:
	fBuffer(capacity),
	fReaders(),
	fWriters(),
	fReadRequested(0),
	fWriteRequested(0),
	fShutdown(0)

{
	fReadCondition.Init(this, "unix fifo read");
	fWriteCondition.Init(this, "unix fifo write");
	mutex_init(&fLock, "unix fifo");
}


UnixFifo::~UnixFifo()
{
	mutex_destroy(&fLock);
}


status_t
UnixFifo::Init()
{
	return fBuffer.Init();
}


void
UnixFifo::Shutdown(uint32 shutdown)
{
	TRACE("[%ld] %p->UnixFifo::Shutdown(0x%lx)\n", find_thread(NULL), this,
		shutdown);

	fShutdown |= shutdown;

	if (shutdown != 0) {
		// Shutting down either end also effects the other, so notify both.
		fReadCondition.NotifyAll();
		fWriteCondition.NotifyAll();
	}
}


ssize_t
UnixFifo::Read(const iovec* vecs, size_t vecCount,
	ancillary_data_container** _ancillaryData, bigtime_t timeout)
{
	TRACE("[%ld] %p->UnixFifo::Read(%p, %ld, %lld)\n", find_thread(NULL),
		this, vecs, vecCount, timeout);

	if (IsReadShutdown() && fBuffer.Readable() == 0)
		RETURN_ERROR(UNIX_FIFO_SHUTDOWN);

	UnixRequest request(vecs, vecCount, NULL);
	fReaders.Add(&request);
	fReadRequested += request.TotalSize();

	status_t error = _Read(request, timeout);

	bool firstInQueue = fReaders.Head() == &request;
	fReaders.Remove(&request);
	fReadRequested -= request.TotalSize();

	if (firstInQueue && !fReaders.IsEmpty() && fBuffer.Readable() > 0
			&& !IsReadShutdown()) {
		// There's more to read, other readers, and we were first in the queue.
		// So we need to notify the others.
		fReadCondition.NotifyAll();
	}

	if (request.BytesTransferred() > 0 && !fWriters.IsEmpty()
			&& !IsWriteShutdown()) {
		// We read something and there are writers. Notify them
		fWriteCondition.NotifyAll();
	}

	*_ancillaryData = request.AncillaryData();

	if (request.BytesTransferred() > 0) {
		if (request.BytesTransferred() > SSIZE_MAX)
			RETURN_ERROR(SSIZE_MAX);
		RETURN_ERROR((ssize_t)request.BytesTransferred());
	}

	RETURN_ERROR(error);
}


ssize_t
UnixFifo::Write(const iovec* vecs, size_t vecCount,
	ancillary_data_container* ancillaryData, bigtime_t timeout)
{
	TRACE("[%ld] %p->UnixFifo::Write(%p, %ld, %p, %lld)\n", find_thread(NULL),
		this, vecs, vecCount, ancillaryData, timeout);

	if (IsWriteShutdown())
		RETURN_ERROR(UNIX_FIFO_SHUTDOWN);

	if (IsReadShutdown())
		RETURN_ERROR(EPIPE);

	UnixRequest request(vecs, vecCount, ancillaryData);
	fWriters.Add(&request);
	fWriteRequested += request.TotalSize();

	status_t error = _Write(request, timeout);

	bool firstInQueue = fWriters.Head() == &request;
	fWriters.Remove(&request);
	fWriteRequested -= request.TotalSize();

	if (firstInQueue && !fWriters.IsEmpty() && fBuffer.Writable() > 0
			&& !IsWriteShutdown()) {
		// There's more space for writing, other writers, and we were first in
		// the queue. So we need to notify the others.
		fWriteCondition.NotifyAll();
	}

	if (request.BytesTransferred() > 0 && !fReaders.IsEmpty()
			&& !IsReadShutdown()) {
		// We've written something and there are readers. Notify them.
		fReadCondition.NotifyAll();
	}

	if (request.BytesTransferred() > 0) {
		if (request.BytesTransferred() > SSIZE_MAX)
			RETURN_ERROR(SSIZE_MAX);
		RETURN_ERROR((ssize_t)request.BytesTransferred());
	}

	RETURN_ERROR(error);
}


size_t
UnixFifo::Readable() const
{
	size_t readable = fBuffer.Readable();
	return (off_t)readable > fReadRequested ? readable - fReadRequested : 0;
}


size_t
UnixFifo::Writable() const
{
	size_t writable = fBuffer.Writable();
	return (off_t)writable > fWriteRequested ? writable - fWriteRequested : 0;
}


status_t
UnixFifo::SetBufferCapacity(size_t capacity)
{
	// check against allowed minimal/maximal value
	if (capacity > UNIX_FIFO_MAXIMAL_CAPACITY)
		capacity = UNIX_FIFO_MAXIMAL_CAPACITY;
	else if (capacity < UNIX_FIFO_MINIMAL_CAPACITY)
		capacity = UNIX_FIFO_MINIMAL_CAPACITY;

	size_t oldCapacity = fBuffer.Capacity();
	if (capacity == oldCapacity)
		return B_OK;

	// set capacity
	status_t error = fBuffer.SetCapacity(capacity);
	if (error != B_OK)
		return error;

	// wake up waiting writers, if the capacity increased
	if (!fWriters.IsEmpty() && !IsWriteShutdown())
		fWriteCondition.NotifyAll();

	return B_OK;
}


status_t
UnixFifo::_Read(UnixRequest& request, bigtime_t timeout)
{
	// wait for the request to reach the front of the queue
	if (fReaders.Head() != &request && timeout == 0)
		RETURN_ERROR(B_WOULD_BLOCK);

	while (fReaders.Head() != &request
		&& !(IsReadShutdown() && fBuffer.Readable() == 0)) {
		ConditionVariableEntry entry;
		fReadCondition.Add(&entry);

		mutex_unlock(&fLock);
		status_t error = entry.Wait(B_ABSOLUTE_TIMEOUT | B_CAN_INTERRUPT,
			timeout);
		mutex_lock(&fLock);

		if (error != B_OK)
			RETURN_ERROR(error);
	}

	if (fBuffer.Readable() == 0) {
		if (IsReadShutdown())
			RETURN_ERROR(UNIX_FIFO_SHUTDOWN);

		if (IsWriteShutdown())
			RETURN_ERROR(0);

		if (timeout == 0)
			RETURN_ERROR(B_WOULD_BLOCK);
	}

	// wait for any data to become available
// TODO: Support low water marks!
	while (fBuffer.Readable() == 0
			&& !IsReadShutdown() && !IsWriteShutdown()) {
		ConditionVariableEntry entry;
		fReadCondition.Add(&entry);

		mutex_unlock(&fLock);
		status_t error = entry.Wait(B_ABSOLUTE_TIMEOUT | B_CAN_INTERRUPT,
			timeout);
		mutex_lock(&fLock);

		if (error != B_OK)
			RETURN_ERROR(error);
	}

	if (fBuffer.Readable() == 0) {
		if (IsReadShutdown())
			RETURN_ERROR(UNIX_FIFO_SHUTDOWN);
		if (IsWriteShutdown())
			RETURN_ERROR(0);
	}

	RETURN_ERROR(fBuffer.Read(request));
}


status_t
UnixFifo::_Write(UnixRequest& request, bigtime_t timeout)
{
	if (timeout == 0)
		RETURN_ERROR(_WriteNonBlocking(request));

	// wait for the request to reach the front of the queue
	while (fWriters.Head() != &request && !IsWriteShutdown()) {
		ConditionVariableEntry entry;
		fWriteCondition.Add(&entry);

		mutex_unlock(&fLock);
		status_t error = entry.Wait(B_ABSOLUTE_TIMEOUT | B_CAN_INTERRUPT,
			timeout);
		mutex_lock(&fLock);

		if (error != B_OK)
			RETURN_ERROR(error);
	}

	if (IsWriteShutdown())
		RETURN_ERROR(UNIX_FIFO_SHUTDOWN);

	if (IsReadShutdown())
		RETURN_ERROR(EPIPE);

	if (request.TotalSize() == 0)
		return 0;

	status_t error = B_OK;

	while (error == B_OK && request.BytesRemaining() > 0) {
		// wait for any space to become available
		while (error == B_OK && fBuffer.Writable() == 0 && !IsWriteShutdown()
				&& !IsReadShutdown()) {
			ConditionVariableEntry entry;
			fWriteCondition.Add(&entry);

			mutex_unlock(&fLock);
			error = entry.Wait(B_ABSOLUTE_TIMEOUT | B_CAN_INTERRUPT, timeout);
			mutex_lock(&fLock);

			if (error != B_OK)
				RETURN_ERROR(error);
		}

		if (IsWriteShutdown())
			RETURN_ERROR(UNIX_FIFO_SHUTDOWN);

		if (IsReadShutdown())
			RETURN_ERROR(EPIPE);

		// write as much as we can
		error = fBuffer.Write(request);

		if (error == B_OK) {
// TODO: Whenever we've successfully written a part, we should reset the
// timeout!
		}
	}

	RETURN_ERROR(error);
}


status_t
UnixFifo::_WriteNonBlocking(UnixRequest& request)
{
	// We need to be first in queue and space should be available right now,
	// otherwise we need to fail.
	if (fWriters.Head() != &request || fBuffer.Writable() == 0)
		RETURN_ERROR(B_WOULD_BLOCK);

	if (request.TotalSize() == 0)
		return 0;

	// Write as much as we can.
	RETURN_ERROR(fBuffer.Write(request));
}

