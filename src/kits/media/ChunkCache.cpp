/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "ChunkCache.h"

#include <new>
#include <stdlib.h>
#include <string.h>

#include "MediaDebug.h"

// #pragma mark -


ChunkCache::ChunkCache(sem_id waitSem, size_t maxBytes)
	:
	BLocker("media chunk cache"),
	fWaitSem(waitSem)
{
	rtm_create_pool(&fRealTimePool, maxBytes, "media chunk cache");
	fMaxBytes = rtm_available(fRealTimePool);
}


ChunkCache::~ChunkCache()
{
	rtm_delete_pool(fRealTimePool);
}


status_t
ChunkCache::InitCheck() const
{
	if (fRealTimePool == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


void
ChunkCache::MakeEmpty()
{
	ASSERT(IsLocked());

	while (!fChunkCache.empty()) {
		RecycleChunk(fChunkCache.front());
		fChunkCache.pop();
	}
	
	release_sem(fWaitSem);
}


bool
ChunkCache::SpaceLeft() const
{
	ASSERT(IsLocked());

	if (fChunkCache.size() >= CACHE_MAX_ENTRIES) {
		return false;
	}

	// If there is no more memory we are likely to fail soon after
	return sizeof(chunk_buffer) + 2048 < rtm_available(fRealTimePool);
}


chunk_buffer*
ChunkCache::NextChunk(Reader* reader, void* cookie)
{
	ASSERT(IsLocked());

	chunk_buffer* chunk = NULL;

	if (fChunkCache.empty()) {
		TRACE("ChunkCache is empty, going direct to reader\n");
		if (ReadNextChunk(reader, cookie)) {
			return NextChunk(reader, cookie);
		}
	} else {
		chunk = fChunkCache.front();
		fChunkCache.pop();
		
		release_sem(fWaitSem);
	}

	return chunk;
}


/*	Moves the specified chunk to the unused list.
	This means the chunk data can be overwritten again.
*/
void
ChunkCache::RecycleChunk(chunk_buffer* chunk)
{
	ASSERT(IsLocked());

	rtm_free(chunk->buffer);
	chunk->capacity = 0;
	chunk->size = 0;
	chunk->buffer = NULL;
	fUnusedChunks.push_back(chunk);
}


bool
ChunkCache::ReadNextChunk(Reader* reader, void* cookie)
{
	ASSERT(IsLocked());

	// retrieve chunk buffer
	chunk_buffer* chunk = NULL;
	if (fUnusedChunks.empty()) {
		// allocate a new one
		chunk = (chunk_buffer*)rtm_alloc(fRealTimePool, sizeof(chunk_buffer));
		if (chunk == NULL) {
			ERROR("RTM Pool empty allocating chunk buffer structure");
			return false;
		}
		
		chunk->size = 0;
		chunk->capacity = 0;
		chunk->buffer = NULL;

	} else {
		chunk = fUnusedChunks.front();
		fUnusedChunks.pop_front();
	}

	const void* buffer;
	size_t bufferSize;
	chunk->status = reader->GetNextChunk(cookie, &buffer, &bufferSize,
		&chunk->header);
	if (chunk->status == B_OK) {
		if (chunk->capacity < bufferSize) {
			// adapt buffer size
			rtm_free(chunk->buffer);
			chunk->capacity = (bufferSize + 2047) & ~2047;
			chunk->buffer = rtm_alloc(fRealTimePool, chunk->capacity);
			if (chunk->buffer == NULL) {
				rtm_free(chunk);
				ERROR("RTM Pool empty allocating chunk buffer\n");
				return false;
			}
		}

		memcpy(chunk->buffer, buffer, bufferSize);
		chunk->size = bufferSize;
	}

	fChunkCache.push(chunk);
	return chunk->status == B_OK;
}
