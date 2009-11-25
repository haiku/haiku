/*
 * Copyright 2004, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "ChunkCache.h"

#include <string.h>

#include <Autolock.h>

#include "debug.h"


ChunkCache::ChunkCache()
	:
	fLocker("media chunk cache locker")
{
	// fEmptyChunkCount must be one less than the real chunk count,
	// because the buffer returned by GetNextChunk must be preserved
	// until the next call of that function, and must not be overwritten.

	fEmptyChunkCount = CHUNK_COUNT - 1;
	fReadyChunkCount = 0;
	fNeedsRefill = 1;

	fGetWaitSem = create_sem(0, "media chunk cache sem");

	fNextPut = &fChunkInfos[0];
	fNextGet = &fChunkInfos[0];

	for (int i = 0; i < CHUNK_COUNT; i++) {
		fChunkInfos[i].next = i == CHUNK_COUNT - 1
			? &fChunkInfos[0] : &fChunkInfos[i + 1];
		fChunkInfos[i].buffer = NULL;
		fChunkInfos[i].sizeUsed = 0;
		fChunkInfos[i].sizeMax = 0;
		fChunkInfos[i].status = B_ERROR;
	}
}


ChunkCache::~ChunkCache()
{
	delete_sem(fGetWaitSem);

	for (int i = 0; i < CHUNK_COUNT; i++) {
		free(fChunkInfos[i].buffer);
	}
}


void
ChunkCache::MakeEmpty()
{
	BAutolock _(fLocker);

	fEmptyChunkCount = CHUNK_COUNT - 1;
	fReadyChunkCount = 0;
	fNextPut = &fChunkInfos[0];
	fNextGet = &fChunkInfos[0];
	atomic_or(&fNeedsRefill, 1);
}


bool
ChunkCache::NeedsRefill()
{
	return atomic_or(&fNeedsRefill, 0);
}


status_t
ChunkCache::GetNextChunk(const void** _chunkBuffer, size_t* _chunkSize,
	media_header* mediaHeader)
{
	uint8 retryCount = 0;

//	printf("ChunkCache::GetNextChunk: %p fEmptyChunkCount %ld, fReadyChunkCount %ld\n", fNextGet, fEmptyChunkCount, fReadyChunkCount);
retry:
	acquire_sem(fGetWaitSem);

	BAutolock locker(fLocker);
	if (fReadyChunkCount == 0) {
		locker.Unlock();

		printf("ChunkCache::GetNextChunk: %p retrying\n", fNextGet);
		// Limit to 5 retries
		retryCount++;
		if (retryCount > 4)
			return B_ERROR;

		goto retry;
	}

	fEmptyChunkCount++;
	fReadyChunkCount--;
	atomic_or(&fNeedsRefill, 1);

	locker.Unlock();

	*_chunkBuffer = fNextGet->buffer;
	*_chunkSize = fNextGet->sizeUsed;
	*mediaHeader = fNextGet->mediaHeader;
	status_t status = fNextGet->status;
	fNextGet = fNextGet->next;

	return status;
}


void
ChunkCache::PutNextChunk(const void* chunkBuffer, size_t chunkSize,
	const media_header& mediaHeader, status_t status)
{
//	printf("ChunkCache::PutNextChunk: %p fEmptyChunkCount %ld, fReadyChunkCount %ld\n", fNextPut, fEmptyChunkCount, fReadyChunkCount);

	if (status == B_OK) {
		if (fNextPut->sizeMax < chunkSize) {
//			printf("ChunkCache::PutNextChunk: %p resizing from %ld to %ld\n", fNextPut, fNextPut->sizeMax, chunkSize);
			free(fNextPut->buffer);
			fNextPut->buffer = malloc((chunkSize + 1024) & ~1023);
			fNextPut->sizeMax = chunkSize;
		}
		memcpy(fNextPut->buffer, chunkBuffer, chunkSize);
		fNextPut->sizeUsed = chunkSize;
	}

	fNextPut->mediaHeader = mediaHeader;
	fNextPut->status = status;

	fNextPut = fNextPut->next;

	fLocker.Lock();
	fEmptyChunkCount--;
	fReadyChunkCount++;
	if (fEmptyChunkCount == 0)
		atomic_and(&fNeedsRefill, 0);
	fLocker.Unlock();

	release_sem(fGetWaitSem);
}
