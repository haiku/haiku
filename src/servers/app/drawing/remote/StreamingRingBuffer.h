/*
 * Copyright 2009, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */
#ifndef STREAMING_RING_BUFFER_H
#define STREAMING_RING_BUFFER_H

#include <OS.h>
#include <SupportDefs.h>
#include <Locker.h>

class StreamingRingBuffer {
public:
								StreamingRingBuffer(size_t bufferSize);
								~StreamingRingBuffer();

		status_t				InitCheck();

		// blocking read and write
		int32					Read(void *buffer, size_t length,
									bool onlyBlockOnNoData = false);
		status_t				Write(const void *buffer, size_t length);

private:
		bool					_Lock();
		void					_Unlock();

		bool					fReaderWaiting;
		bool					fWriterWaiting;
		sem_id					fReaderNotifier;
		sem_id					fWriterNotifier;

		BLocker					fReaderLocker;
		BLocker					fWriterLocker;
		BLocker					fDataLocker;

		uint8 *					fBuffer;
		size_t					fBufferSize;
		size_t					fReadable;
		int32					fReadPosition;
		int32					fWritePosition;
};

#endif // STREAMING_RING_BUFFER_H
