/*
 * Copyright 2009, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#include "StreamingRingBuffer.h"

#include <Autolock.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRACE(x...)			/*debug_printf("StreamingRingBuffer: "x)*/
#define TRACE_ERROR(x...)	debug_printf("StreamingRingBuffer: "x)


StreamingRingBuffer::StreamingRingBuffer(size_t bufferSize)
	:
	fReaderWaiting(false),
	fWriterWaiting(false),
	fReaderNotifier(-1),
	fWriterNotifier(-1),
	fReaderLocker("StreamingRingBuffer reader"),
	fWriterLocker("StreamingRingBuffer writer"),
	fDataLocker("StreamingRingBuffer data"),
	fBuffer(NULL),
	fBufferSize(bufferSize),
	fReadable(0),
	fReadPosition(0),
	fWritePosition(0)
{
	fReaderNotifier = create_sem(0, "StreamingRingBuffer read notify");
	fWriterNotifier = create_sem(0, "StreamingRingBuffer write notify");

	fBuffer = (uint8 *)malloc(fBufferSize);
	if (fBuffer == NULL)
		fBufferSize = 0;
}


StreamingRingBuffer::~StreamingRingBuffer()
{
	delete_sem(fReaderNotifier);
	delete_sem(fWriterNotifier);
	free(fBuffer);
}


status_t
StreamingRingBuffer::InitCheck()
{
	if (fReaderNotifier < 0)
		return fReaderNotifier;
	if (fWriterNotifier < 0)
		return fWriterNotifier;
	if (fBuffer == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


int32
StreamingRingBuffer::Read(void *buffer, size_t length, bool onlyBlockOnNoData)
{
	BAutolock readerLock(fReaderLocker);
	if (!readerLock.IsLocked())
		return B_ERROR;

	BAutolock dataLock(fDataLocker);
	if (!dataLock.IsLocked())
		return B_ERROR;

	int32 readSize = 0;
	while (length > 0) {
		size_t copyLength = min_c(length, fBufferSize - fReadPosition);
		copyLength = min_c(copyLength, fReadable);

		if (copyLength == 0) {
			if (onlyBlockOnNoData && readSize > 0)
				return readSize;

			fReaderWaiting = true;
			dataLock.Unlock();

			status_t result;
			do {
				TRACE("waiting in reader\n");
				result = acquire_sem(fReaderNotifier);
				TRACE("done waiting in reader with status: 0x%08lx\n", result);
			} while (result == B_INTERRUPTED);

			if (result != B_OK)
				return result;

			if (!dataLock.Lock())
				return B_ERROR;

			continue;
		}

		// support discarding input
		if (buffer != NULL) {
			memcpy(buffer, fBuffer + fReadPosition, copyLength);
			buffer = (uint8 *)buffer + copyLength;
		}

		fReadPosition = (fReadPosition + copyLength) % fBufferSize;
		fReadable -= copyLength;
		readSize += copyLength;
		length -= copyLength;

		if (fWriterWaiting) {
			release_sem_etc(fWriterNotifier, 1, B_DO_NOT_RESCHEDULE);
			fWriterWaiting = false;
		}
	}

	return readSize;
}


status_t
StreamingRingBuffer::Write(const void *buffer, size_t length)
{
	BAutolock writerLock(fWriterLocker);
	if (!writerLock.IsLocked())
		return B_ERROR;

	BAutolock dataLock(fDataLocker);
	if (!dataLock.IsLocked())
		return B_ERROR;

	while (length > 0) {
		size_t copyLength = min_c(length, fBufferSize - fWritePosition);
		copyLength = min_c(copyLength, fBufferSize - fReadable);

		if (copyLength == 0) {
			fWriterWaiting = true;
			dataLock.Unlock();

			status_t result;
			do {
				TRACE("waiting in writer\n");
				result = acquire_sem(fWriterNotifier);
				TRACE("done waiting in writer with status: 0x%08lx\n", result);
			} while (result == B_INTERRUPTED);

			if (result != B_OK)
				return result;

			if (!dataLock.Lock())
				return B_ERROR;

			continue;
		}

		memcpy(fBuffer + fWritePosition, buffer, copyLength);
		fWritePosition = (fWritePosition + copyLength) % fBufferSize;
		fReadable += copyLength;

		buffer = (uint8 *)buffer + copyLength;
		length -= copyLength;

		if (fReaderWaiting) {
			release_sem_etc(fReaderNotifier, 1, B_DO_NOT_RESCHEDULE);
			fReaderWaiting = false;
		}
	}

	return B_OK;
}
