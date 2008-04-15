/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "UnixFifo.h"

#include "unix.h"


#define UNIX_FIFO_DEBUG_LEVEL	2
#define UNIX_DEBUG_LEVEL		UNIX_FIFO_DEBUG_LEVEL
#include "UnixDebug.h"


UnixBufferQueue::UnixBufferQueue(size_t capacity)
	:
	fSize(0),
	fCapacity(capacity)
{
}


UnixBufferQueue::~UnixBufferQueue()
{
	while (net_buffer* buffer = fBuffers.RemoveHead())
		gBufferModule->free(buffer);
}


status_t
UnixBufferQueue::Read(size_t size, net_buffer** _buffer)
{
	if (size > fSize)
		size = fSize;

	if (size == 0)
		return B_BAD_VALUE;

	// If the first buffer has the right size or is smaller, we can just
	// dequeue it.
	net_buffer* buffer = fBuffers.Head();
	if (buffer->size <= size) {
		fBuffers.RemoveHead();
		fSize -= buffer->size;
		*_buffer = buffer;

		if (buffer->size == size)
			return B_OK;

		// buffer is too small

 		size_t bytesLeft = size - buffer->size;

		// Append from the following buffers, until we've read as much as we're
		// supposed to.
		while (bytesLeft > 0) {
			net_buffer* nextBuffer = fBuffers.Head();
			size_t toCopy = min_c(bytesLeft, nextBuffer->size);

			if (gBufferModule->append_cloned(buffer, nextBuffer, 0, toCopy)
					!= B_OK) {
				// Too bad, but we've got some data, so we don't fail.
				return B_OK;
			}

			// transfer the ancillary data
			gBufferModule->transfer_ancillary_data(nextBuffer, buffer);

			if (nextBuffer->size > toCopy) {
				// remove the part we've copied
				gBufferModule->remove_header(nextBuffer, toCopy);
			} else {
				// get rid of the buffer completely
				fBuffers.RemoveHead();
				gBufferModule->free(nextBuffer);
			}

			bytesLeft -= toCopy;
			fSize -= toCopy;
		}

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
	gBufferModule->remove_header(buffer, size);

	fSize -= size;
	*_buffer = newBuffer;

	return B_OK;
}


status_t
UnixBufferQueue::Write(net_buffer* buffer)
{
	if (buffer->size > Writable())
		return ENOBUFS;

	fBuffers.Add(buffer);
	fSize += buffer->size;

	return B_OK;
}


void
UnixBufferQueue::SetCapacity(size_t capacity)
{
	fCapacity = capacity;
}


// #pragma mark -


UnixFifo::UnixFifo(size_t capacity)
	:
	fBuffer(capacity),
	fReaders(),
	fWriters(),
	fReadRequested(0),
	fWriteRequested(0),
	fReaderSem(-1),
	fWriterSem(-1),
	fShutdown(0)

{
	fLock.sem = -1;
}


UnixFifo::~UnixFifo()
{
	if (fReaderSem >= 0)
		delete_sem(fReaderSem);

	if (fWriterSem >= 0)
		delete_sem(fWriterSem);

	if (fLock.sem >= 0)
		benaphore_destroy(&fLock);
}


status_t
UnixFifo::Init()
{
	status_t error = benaphore_init(&fLock, "unix fifo");

	fReaderSem = create_sem(0, "unix fifo readers");
	fWriterSem = create_sem(0, "unix fifo writers");

	if (error != B_OK || fReaderSem < 0 || fWriterSem < 0)
		return ENOBUFS;

	return B_OK;
}


void
UnixFifo::Shutdown(uint32 shutdown)
{
	fShutdown |= shutdown;

	if (shutdown != 0) {
		// Shutting down either end also effects the other, so notify both.
		release_sem_etc(fWriterSem, 1, B_RELEASE_ALL);
		release_sem_etc(fReaderSem, 1, B_RELEASE_ALL);
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
		release_sem_etc(fReaderSem, 1, B_RELEASE_ALL);
	}

	if (error == B_OK && *_buffer != NULL && (*_buffer)->size > 0
			&& !fWriters.IsEmpty() && !IsWriteShutdown()) {
		// We read something and there are writers. Notify them
		release_sem_etc(fWriterSem, 1, B_RELEASE_ALL);
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
		release_sem_etc(fWriterSem, 1, B_RELEASE_ALL);
	}

	if (error == B_OK && request.size > 0 && !fReaders.IsEmpty()
			&& !IsReadShutdown()) {
		// We've written something and there are readers. Notify them
		release_sem_etc(fReaderSem, 1, B_RELEASE_ALL);
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
		release_sem_etc(fWriterSem, 1, B_RELEASE_ALL);
}


status_t
UnixFifo::_Read(Request& request, size_t numBytes, bigtime_t timeout,
	net_buffer** _buffer)
{
	// wait for the request to reach the front of the queue
	if (fReaders.Head() != &request && timeout == 0)
		RETURN_ERROR(B_WOULD_BLOCK);

	while (fReaders.Head() != &request && !IsReadShutdown()) {
		benaphore_unlock(&fLock);

		status_t error = acquire_sem_etc(fReaderSem, 1,
			B_ABSOLUTE_TIMEOUT | B_CAN_INTERRUPT, timeout);

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
		benaphore_unlock(&fLock);

		status_t error = acquire_sem_etc(fReaderSem, 1,
			B_ABSOLUTE_TIMEOUT | B_CAN_INTERRUPT, timeout);

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
		benaphore_unlock(&fLock);

		status_t error = acquire_sem_etc(fWriterSem, 1,
			B_ABSOLUTE_TIMEOUT | B_CAN_INTERRUPT, timeout);

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
		benaphore_unlock(&fLock);

		status_t error = acquire_sem_etc(fWriterSem, 1,
			B_ABSOLUTE_TIMEOUT | B_CAN_INTERRUPT, timeout);

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
