/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "UnixFifo.h"

#include <AutoDeleter.h>

#include "unix.h"


#define UNIX_FIFO_DEBUG_LEVEL	2
#define UNIX_DEBUG_LEVEL		UNIX_FIFO_DEBUG_LEVEL
#include "UnixDebug.h"


#if TRACE_BUFFER_QUEUE
#	define TRACEBQ(x...)	ktrace_printf(x)
#	define TRACEBQ_ONLY(x)	x
#else
#	define TRACEBQ(x...)	do {} while (false)
#	define TRACEBQ_ONLY(x)
#endif


UnixBufferQueue::UnixBufferQueue(size_t capacity)
	:
	fSize(0),
	fCapacity(capacity)
#if TRACE_BUFFER_QUEUE
	, fWritten(0)
	, fRead(0)
#endif
{
	TRACEBQ_ONLY(
		fParanoiaCheckBuffer = (uint8*)malloc(UNIX_FIFO_MAXIMAL_CAPACITY);
		fParanoiaCheckBuffer2 = (uint8*)malloc(UNIX_FIFO_MAXIMAL_CAPACITY);
	)
}


UnixBufferQueue::~UnixBufferQueue()
{
	while (net_buffer* buffer = fBuffers.RemoveHead())
		gBufferModule->free(buffer);

	TRACEBQ_ONLY(
		free(fParanoiaCheckBuffer);
		free(fParanoiaCheckBuffer2);
	)
}


status_t
UnixBufferQueue::Read(size_t size, net_buffer** _buffer)
{
	if (size > fSize)
		size = fSize;

	TRACEBQ("unix: UnixBufferQueue::Read(%lu): fSize: %lu, fRead: %lld, "
		"fWritten: %lld", size, fSize, fRead, fWritten);

	TRACEBQ_ONLY(
		MethodDeleter<UnixBufferQueue> _(this, &UnixBufferQueue::PostReadWrite);
	)

	if (size == 0)
		return B_BAD_VALUE;

TRACEBQ_ONLY(
	if (fParanoiaCheckBuffer) {
		size_t bufferSize = 0;
		for (BufferList::Iterator it = fBuffers.GetIterator();
			net_buffer* buffer = it.Next();) {
			size_t toWrite = min_c(buffer->size, size - bufferSize);
			if (toWrite == 0)
				break;

			gBufferModule->read(buffer, 0,
				fParanoiaCheckBuffer + bufferSize, toWrite);
			bufferSize += toWrite;
		}
	}
	*_buffer = NULL;
)

	// If the first buffer has the right size or is smaller, we can just
	// dequeue it.
	net_buffer* buffer = fBuffers.Head();
	if (buffer->size <= size) {
		fBuffers.RemoveHead();
		fSize -= buffer->size;
		TRACEBQ_ONLY(fRead += buffer->size);
		*_buffer = buffer;

		if (buffer->size == size)
{
TRACEBQ("unix:   read full buffer %p (%lu)", buffer, buffer->size);
TRACEBQ_ONLY(ParanoiaReadCheck(*_buffer));
			return B_OK;
}

		// buffer is too small

 		size_t bytesLeft = size - buffer->size;
TRACEBQ("unix:   read short buffer %p (%lu/%lu)", buffer, size, buffer->size);

		// Append from the following buffers, until we've read as much as we're
		// supposed to.
		while (bytesLeft > 0) {
			net_buffer* nextBuffer = fBuffers.Head();
			size_t toCopy = min_c(bytesLeft, nextBuffer->size);

TRACEBQ("unix:   read next buffer %p (%lu/%lu)", nextBuffer, bytesLeft, nextBuffer->size);
#if 0
			if (gBufferModule->append_cloned(buffer, nextBuffer, 0, toCopy)
					!= B_OK) {
				// Too bad, but we've got some data, so we don't fail.
TRACEBQ_ONLY(ParanoiaReadCheck(*_buffer));
				return B_OK;
			}
#endif

// TODO: Temporary work-around for the append_cloned() call above, which
// doesn't seem to work right. Or maybe that's just in combination with the
// remove_header() below.
{
	void* tmpBuffer = malloc(toCopy);
	if (tmpBuffer == NULL)
		return B_OK;
	MemoryDeleter tmpBufferDeleter(tmpBuffer);

	size_t offset = buffer->size;
	if (gBufferModule->read(nextBuffer, 0, tmpBuffer, toCopy) != B_OK
		|| gBufferModule->append_size(buffer, toCopy, NULL) != B_OK) {
		return B_OK;
	}

	if (gBufferModule->write(buffer, offset, tmpBuffer, toCopy) != B_OK) {
		gBufferModule->remove_trailer(buffer, toCopy);
		return B_OK;
	}
}

			// transfer the ancillary data
			gBufferModule->transfer_ancillary_data(nextBuffer, buffer);

			if (nextBuffer->size > toCopy) {
				// remove the part we've copied
//gBufferModule->read(nextBuffer, toCopy, fParanoiaCheckBuffer,
//nextBuffer->size - toCopy);
				gBufferModule->remove_header(nextBuffer, toCopy);
//TRACEBQ_ONLY(ParanoiaReadCheck(nextBuffer));
			} else {
				// get rid of the buffer completely
				fBuffers.RemoveHead();
				gBufferModule->free(nextBuffer);
			}

			bytesLeft -= toCopy;
			fSize -= toCopy;
			TRACEBQ_ONLY(fRead += toCopy);
		}

TRACEBQ_ONLY(ParanoiaReadCheck(*_buffer));
		return B_OK;
	}

	// buffer is too big

	// Create a new buffer, and copy into it, as much as we need.
	net_buffer* newBuffer = gBufferModule->create(256);
	if (newBuffer == NULL)
		return ENOBUFS;

	status_t error = gBufferModule->append_cloned(newBuffer, buffer, 0, size);
	if (error != B_OK) {
		gBufferModule->free(newBuffer);
		return error;
	}

	// transfer the ancillary data
	gBufferModule->transfer_ancillary_data(buffer, newBuffer);

	// remove the part we've copied
TRACEBQ("unix:   read long buffer %p (%lu/%lu)", buffer, size, buffer->size);
	gBufferModule->remove_header(buffer, size);

	fSize -= size;
	TRACEBQ_ONLY(fRead += size);
	*_buffer = newBuffer;

TRACEBQ_ONLY(ParanoiaReadCheck(*_buffer));
	return B_OK;
}


status_t
UnixBufferQueue::Write(net_buffer* buffer)
{
	TRACEBQ("unix: UnixBufferQueue::Write(%lu): fSize: %lu, fRead: %lld, "
		"fWritten: %lld", buffer->size, fSize, fRead, fWritten);

	TRACEBQ_ONLY(
		MethodDeleter<UnixBufferQueue> _(this, &UnixBufferQueue::PostReadWrite);
	)

	if (buffer->size > Writable())
		return ENOBUFS;

	fBuffers.Add(buffer);
	fSize += buffer->size;
	TRACEBQ_ONLY(fWritten += buffer->size);

	return B_OK;
}


void
UnixBufferQueue::SetCapacity(size_t capacity)
{
	fCapacity = capacity;
}


#if TRACE_BUFFER_QUEUE

void
UnixBufferQueue::ParanoiaReadCheck(net_buffer* buffer)
{
	if (!buffer || !fParanoiaCheckBuffer || !fParanoiaCheckBuffer2)
		return;

	gBufferModule->read(buffer, 0, fParanoiaCheckBuffer2, buffer->size);

	if (memcmp(fParanoiaCheckBuffer, fParanoiaCheckBuffer2, buffer->size)
			!= 0) {
		// find offset of first difference
		size_t i = 0;
		for (; i < buffer->size; i++) {
			if (fParanoiaCheckBuffer[i] != fParanoiaCheckBuffer2[i])
				break;
		}

		panic("unix: UnixBufferQueue::ParanoiaReadCheck(): incorrect read! "
			"offset of first difference: %lu", i);
	}
}


void
UnixBufferQueue::PostReadWrite()
{
	TRACEBQ("unix: post read/write: fSize: %lu, fRead: %lld, fWritten: %lld",
		fSize, fRead, fWritten);

	if (fWritten - fRead != fSize) {
		panic("UnixBufferQueue::PostReadWrite(): fSize: %lu, fRead: %lld, "
			"fWritten: %lld", fSize, fRead, fWritten);
	}

	// check buffer size sum
	size_t bufferSize = 0;
	for (BufferList::Iterator it = fBuffers.GetIterator();
		 net_buffer* buffer = it.Next();) {
		bufferSize += buffer->size;
	}

	if (bufferSize != fSize) {
		panic("UnixBufferQueue::PostReadWrite(): fSize: %lu, bufferSize: %lu",
			fSize, bufferSize);
	}
}

#endif	// TRACE_BUFFER_QUEUE


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
	fLock.sem = -1;
}


UnixFifo::~UnixFifo()
{
	if (fLock.sem >= 0)
		benaphore_destroy(&fLock);
}


status_t
UnixFifo::Init()
{
	return benaphore_init(&fLock, "unix fifo");
}


void
UnixFifo::Shutdown(uint32 shutdown)
{
	fShutdown |= shutdown;

	if (shutdown != 0) {
		// Shutting down either end also effects the other, so notify both.
		fReadCondition.NotifyAll();
		fWriteCondition.NotifyAll();
	}
}


status_t
UnixFifo::Read(size_t numBytes, bigtime_t timeout, net_buffer** _buffer)
{
	TRACE("[%ld] UnixFifo::Read(%lu, %lld)\n", find_thread(NULL), numBytes,
		timeout);

	if (IsReadShutdown())
		return UNIX_FIFO_SHUTDOWN;

	Request request(numBytes);
	fReaders.Add(&request);
	fReadRequested += request.size;

	status_t error = _Read(request, numBytes, timeout, _buffer);

	bool firstInQueue = fReaders.Head() == &request;
	fReaders.Remove(&request);
	fReadRequested -= request.size;

	if (firstInQueue && !fReaders.IsEmpty() && fBuffer.Readable() > 0
			&& !IsReadShutdown()) {
		// There's more to read, other readers, and we were first in the queue.
		// So we need to notify the others.
		fReadCondition.NotifyAll();
	}

	if (error == B_OK && *_buffer != NULL && (*_buffer)->size > 0
			&& !fWriters.IsEmpty() && !IsWriteShutdown()) {
		// We read something and there are writers. Notify them
		fWriteCondition.NotifyAll();
	}

	RETURN_ERROR(error);
}


status_t
UnixFifo::Write(net_buffer* buffer, bigtime_t timeout)
{
	if (IsWriteShutdown())
		return UNIX_FIFO_SHUTDOWN;

	Request request(buffer->size);
	fWriters.Add(&request);
	fWriteRequested += request.size;

	status_t error = _Write(request, buffer, timeout);

	bool firstInQueue = fWriters.Head() == &request;
	fWriters.Remove(&request);
	fWriteRequested -= request.size;

	if (firstInQueue && !fWriters.IsEmpty() && fBuffer.Writable() > 0
			&& !IsWriteShutdown()) {
		// There's more space for writing, other writers, and we were first in
		// the queue. So we need to notify the others.
		fWriteCondition.NotifyAll();
	}

	if (error == B_OK && request.size > 0 && !fReaders.IsEmpty()
			&& !IsReadShutdown()) {
		// We've written something and there are readers. Notify them
		fReadCondition.NotifyAll();
	}

	RETURN_ERROR(error);
}


size_t
UnixFifo::Readable() const
{
	size_t readable = fBuffer.Readable();
	return readable > fReadRequested ? readable - fReadRequested : 0;
}


size_t
UnixFifo::Writable() const
{
	size_t writable = fBuffer.Writable();
	return writable > fWriteRequested ? writable - fWriteRequested : 0;
}


void
UnixFifo::SetBufferCapacity(size_t capacity)
{
	// check against allowed minimal/maximal value
	if (capacity > UNIX_FIFO_MAXIMAL_CAPACITY)
		capacity = UNIX_FIFO_MAXIMAL_CAPACITY;
	else if (capacity < UNIX_FIFO_MINIMAL_CAPACITY)
		capacity = UNIX_FIFO_MINIMAL_CAPACITY;

	size_t oldCapacity = fBuffer.Capacity();
	if (capacity == oldCapacity)
		return;

	// set capacity
	fBuffer.SetCapacity(capacity);

	// wake up waiting writers, if the capacity increased
	if (!fWriters.IsEmpty() && !IsWriteShutdown())
		fWriteCondition.NotifyAll();
}


status_t
UnixFifo::_Read(Request& request, size_t numBytes, bigtime_t timeout,
	net_buffer** _buffer)
{
	// wait for the request to reach the front of the queue
	if (fReaders.Head() != &request && timeout == 0)
		RETURN_ERROR(B_WOULD_BLOCK);

	while (fReaders.Head() != &request && !IsReadShutdown()) {
		ConditionVariableEntry entry;
		fReadCondition.Add(&entry, B_CAN_INTERRUPT);

		benaphore_unlock(&fLock);
		status_t error = entry.Wait(B_ABSOLUTE_TIMEOUT, timeout);
		benaphore_lock(&fLock);

		if (error != B_OK)
			RETURN_ERROR(error);
	}

	if (IsReadShutdown())
		return UNIX_FIFO_SHUTDOWN;

	if (fBuffer.Readable() == 0) {
		if (IsWriteShutdown()) {
			*_buffer = NULL;
			RETURN_ERROR(B_OK);
		}

		if (timeout == 0)
			RETURN_ERROR(B_WOULD_BLOCK);
	}

	// wait for any data to become available
// TODO: Support low water marks!
	while (fBuffer.Readable() == 0
			&& !IsReadShutdown() && !IsWriteShutdown()) {
		ConditionVariableEntry entry;
		fReadCondition.Add(&entry, B_CAN_INTERRUPT);

		benaphore_unlock(&fLock);
		status_t error = entry.Wait(B_ABSOLUTE_TIMEOUT, timeout);
		benaphore_lock(&fLock);

		if (error != B_OK)
			RETURN_ERROR(error);
	}

	if (IsReadShutdown())
		return UNIX_FIFO_SHUTDOWN;

	if (fBuffer.Readable() == 0 && IsWriteShutdown()) {
		*_buffer = NULL;
		RETURN_ERROR(B_OK);
	}

	RETURN_ERROR(fBuffer.Read(numBytes, _buffer));
}


status_t
UnixFifo::_Write(Request& request, net_buffer* buffer, bigtime_t timeout)
{
	// wait for the request to reach the front of the queue
	if (fWriters.Head() != &request && timeout == 0)
		RETURN_ERROR(B_WOULD_BLOCK);

	while (fWriters.Head() != &request && !IsWriteShutdown()) {
		ConditionVariableEntry entry;
		fWriteCondition.Add(&entry, B_CAN_INTERRUPT);

		benaphore_unlock(&fLock);
		status_t error = entry.Wait(B_ABSOLUTE_TIMEOUT, timeout);
		benaphore_lock(&fLock);

		if (error != B_OK)
			RETURN_ERROR(error);
	}

	if (IsWriteShutdown())
		return UNIX_FIFO_SHUTDOWN;

	if (IsReadShutdown())
		return EPIPE;

	// wait for any space to become available
	if (fBuffer.Writable() < request.size && timeout == 0)
		RETURN_ERROR(B_WOULD_BLOCK);

	while (fBuffer.Writable() < request.size && !IsWriteShutdown()
			&& !IsReadShutdown()) {
		ConditionVariableEntry entry;
		fWriteCondition.Add(&entry, B_CAN_INTERRUPT);

		benaphore_unlock(&fLock);
		status_t error = entry.Wait(B_ABSOLUTE_TIMEOUT, timeout);
		benaphore_lock(&fLock);

		if (error != B_OK)
			RETURN_ERROR(error);
	}

	if (IsWriteShutdown())
		return UNIX_FIFO_SHUTDOWN;

	if (IsReadShutdown())
		return EPIPE;

	RETURN_ERROR(fBuffer.Write(buffer));
}
