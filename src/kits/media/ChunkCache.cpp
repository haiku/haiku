#include <Locker.h>

#include "ChunkCache.h"
#include "debug.h"


ChunkCache::ChunkCache()
{
	fLocker = new BLocker("media chunk cache locker");

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
		fChunkInfos[i].next = (i == CHUNK_COUNT -1) ? &fChunkInfos[0] : &fChunkInfos[i + 1];
		fChunkInfos[i].buffer = NULL;
		fChunkInfos[i].sizeUsed = 0;
		fChunkInfos[i].sizeMax = 0;
		fChunkInfos[i].err = B_ERROR;
	}
}


ChunkCache::~ChunkCache()
{
	delete_sem(fGetWaitSem);
	delete fLocker;
	for (int i = 0; i < CHUNK_COUNT; i++) {
		free(fChunkInfos[i].buffer);
	}
}


void
ChunkCache::MakeEmpty()
{
	fLocker->Lock();
	fEmptyChunkCount = CHUNK_COUNT - 1;
	fReadyChunkCount = 0;
	atomic_or(&fNeedsRefill, 1);
	fLocker->Unlock();
}


bool
ChunkCache::NeedsRefill()
{
	return atomic_or(&fNeedsRefill, 0);
}

					
status_t
ChunkCache::GetNextChunk(void **chunkBuffer, int32 *chunkSize, media_header *mediaHeader)
{
//	printf("ChunkCache::GetNextChunk: %p fEmptyChunkCount %ld, fReadyChunkCount %ld\n", fNextGet, fEmptyChunkCount, fReadyChunkCount);
retry:
	acquire_sem(fGetWaitSem);

	fLocker->Lock();
	if (fReadyChunkCount == 0) {
		fLocker->Unlock();
		printf("ChunkCache::GetNextChunk: %p retrying\n", fNextGet);
		goto retry;
	}
	fEmptyChunkCount++;
	fReadyChunkCount--;
	atomic_or(&fNeedsRefill, 1);
	fLocker->Unlock();
	
	*chunkBuffer = fNextGet->buffer;
	*chunkSize = fNextGet->sizeUsed;
	*mediaHeader = fNextGet->mediaHeader;
	status_t err = fNextGet->err;
	fNextGet = fNextGet->next;

	return err;
}


void
ChunkCache::PutNextChunk(void *chunkBuffer, int32 chunkSize, const media_header &mediaHeader, status_t err)
{
//	printf("ChunkCache::PutNextChunk: %p fEmptyChunkCount %ld, fReadyChunkCount %ld\n", fNextPut, fEmptyChunkCount, fReadyChunkCount);

	if (err == B_OK) {
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
	fNextPut->err = err;

	fNextPut = fNextPut->next;

	fLocker->Lock();
	fEmptyChunkCount--;
	fReadyChunkCount++;
	if (fEmptyChunkCount == 0)
		atomic_and(&fNeedsRefill, 0);
	fLocker->Unlock();

	release_sem(fGetWaitSem);
}
