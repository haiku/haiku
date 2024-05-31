/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "AdaptiveBuffering.h"

#include <stdlib.h>


//#define TRACE(x...) printf(x)
#define TRACE(x...) ;


AdaptiveBuffering::AdaptiveBuffering(size_t initialBufferSize,
		size_t maxBufferSize, uint32 count)
	:
	fWriterThread(-1),
	fBuffers(NULL),
	fReadBytes(NULL),
	fBufferCount(count),
	fReadIndex(0),
	fWriteIndex(0),
	fReadCount(0),
	fWriteCount(0),
	fMaxBufferSize(maxBufferSize),
	fCurrentBufferSize(initialBufferSize),
	fReadSem(-1),
	fWriteSem(-1),
	fFinishedSem(-1),
	fWriteStatus(B_OK),
	fWriteTime(0),
	fFinished(false),
	fQuit(false)
{
}


AdaptiveBuffering::~AdaptiveBuffering()
{
	_QuitWriter();

	delete_sem(fReadSem);
	delete_sem(fWriteSem);

	if (fBuffers != NULL) {
		for (uint32 i = 0; i < fBufferCount; i++) {
			if (fBuffers[i] == NULL)
				break;

			free(fBuffers[i]);
		}

		free(fBuffers);
	}

	free(fReadBytes);
}


status_t
AdaptiveBuffering::Init()
{
	fReadBytes = (size_t*)malloc(fBufferCount * sizeof(size_t));
	if (fReadBytes == NULL)
		return B_NO_MEMORY;

	fBuffers = (uint8**)malloc(fBufferCount * sizeof(uint8*));
	if (fBuffers == NULL)
		return B_NO_MEMORY;

	for (uint32 i = 0; i < fBufferCount; i++) {
		fBuffers[i] = (uint8*)malloc(fMaxBufferSize);
		if (fBuffers[i] == NULL)
			return B_NO_MEMORY;
	}

	fReadSem = create_sem(0, "reader");
	if (fReadSem < B_OK)
		return fReadSem;

	fWriteSem = create_sem(fBufferCount - 1, "writer");
	if (fWriteSem < B_OK)
		return fWriteSem;

	fFinishedSem = create_sem(0, "finished");
	if (fFinishedSem < B_OK)
		return fFinishedSem;

	fWriterThread = spawn_thread(&_Writer, "buffer reader", B_LOW_PRIORITY,
		this);
	if (fWriterThread < B_OK)
		return fWriterThread;

	return resume_thread(fWriterThread);
}


status_t
AdaptiveBuffering::Read(uint8* /*buffer*/, size_t* _length)
{
	*_length = 0;
	return B_OK;
}


status_t
AdaptiveBuffering::Write(uint8* /*buffer*/, size_t /*length*/)
{
	return B_OK;
}


status_t
AdaptiveBuffering::Run()
{
	fReadIndex = 0;
	fWriteIndex = 0;
	fReadCount = 0;
	fWriteCount = 0;
	fWriteStatus = B_OK;
	fWriteTime = 0;

	while (fWriteStatus >= B_OK) {
		bigtime_t start = system_time();
		int32 index = fReadIndex;

		TRACE("%ld. read index %lu, buffer size %lu\n", fReadCount, index,
			fCurrentBufferSize);

		fReadBytes[index] = fCurrentBufferSize;
		status_t status = Read(fBuffers[index], &fReadBytes[index]);
		if (status < B_OK)
			return status;

		TRACE("%ld. read -> %lu bytes\n", fReadCount, fReadBytes[index]);

		fReadCount++;
		fReadIndex = (index + 1) % fBufferCount;
		if (fReadBytes[index] == 0)
			fFinished = true;
		release_sem(fReadSem);

		while (acquire_sem(fWriteSem) == B_INTERRUPTED)
			;

		if (fFinished)
			break;

		bigtime_t readTime = system_time() - start;
		uint32 writeTime = fWriteTime;
		if (writeTime) {
			if (writeTime > readTime) {
				fCurrentBufferSize = fCurrentBufferSize * 8/9;
				fCurrentBufferSize &= ~65535;
			} else {
				fCurrentBufferSize = fCurrentBufferSize * 9/8;
				fCurrentBufferSize = (fCurrentBufferSize + 65535) & ~65535;

				if (fCurrentBufferSize > fMaxBufferSize)
					fCurrentBufferSize = fMaxBufferSize;
			}
		}
	}

	while (acquire_sem(fFinishedSem) == B_INTERRUPTED)
		;

	return fWriteStatus;
}


void
AdaptiveBuffering::_QuitWriter()
{
	if (fWriterThread >= B_OK) {
		fQuit = true;
		release_sem(fReadSem);

		status_t status;
		wait_for_thread(fWriterThread, &status);

		fWriterThread = -1;
	}
}


status_t
AdaptiveBuffering::_Writer()
{
	while (true) {
		while (acquire_sem(fReadSem) == B_INTERRUPTED)
			;
		if (fQuit)
			break;

		bigtime_t start = system_time();

		TRACE("%ld. write index %lu, %p, bytes %lu\n", fWriteCount, fWriteIndex,
			fBuffers[fWriteIndex], fReadBytes[fWriteIndex]);

		fWriteStatus = Write(fBuffers[fWriteIndex], fReadBytes[fWriteIndex]);

		TRACE("%ld. write done\n", fWriteCount);

		fWriteIndex = (fWriteIndex + 1) % fBufferCount;
		fWriteTime = uint32(system_time() - start);
		fWriteCount++;

		release_sem(fWriteSem);

		if (fWriteStatus < B_OK)
			return fWriteStatus;
		if (fFinished && fWriteCount == fReadCount)
			release_sem(fFinishedSem);
	}

	return B_OK;
}


/*static*/ status_t
AdaptiveBuffering::_Writer(void* self)
{
	return ((AdaptiveBuffering*)self)->_Writer();
}

