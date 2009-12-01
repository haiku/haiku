/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "ChunkCache.h"

#include <new>
#include <stdlib.h>
#include <string.h>

#include <Debug.h>


chunk_buffer::chunk_buffer()
	:
	buffer(NULL),
	size(0),
	capacity(0)
{
}


chunk_buffer::~chunk_buffer()
{
	rtm_free(buffer);
}


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

	fUnusedChunks.MoveFrom(&fChunks);

	release_sem(fWaitSem);
}


bool
ChunkCache::SpaceLeft() const
{
	ASSERT(IsLocked());

	return sizeof(chunk_buffer) + 2048 < rtm_available(fRealTimePool);
}


chunk_buffer*
ChunkCache::NextChunk()
{
	ASSERT(IsLocked());

	chunk_buffer* chunk = fChunks.RemoveHead();
	if (chunk != NULL) {
		fInFlightChunks.Add(chunk);
		release_sem(fWaitSem);
	}

	return chunk;
}


/*!	Moves the specified chunk from the in-flight list to the unused list.
	This means the chunk data can be overwritten again.
*/
void
ChunkCache::RecycleChunk(chunk_buffer* chunk)
{
	ASSERT(IsLocked());

	fInFlightChunks.Remove(chunk);
	fUnusedChunks.Add(chunk);
}


bool
ChunkCache::ReadNextChunk(Reader* reader, void* cookie)
{
	ASSERT(IsLocked());

	// retrieve chunk buffer
	chunk_buffer* chunk = fUnusedChunks.RemoveHead();
	if (chunk == NULL) {
		// allocate a new one
		chunk = (chunk_buffer*)rtm_alloc(fRealTimePool, sizeof(chunk_buffer));
		if (chunk == NULL)
			return false;

		new(chunk) chunk_buffer;
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
				return false;
			}
		}

		memcpy(chunk->buffer, buffer, bufferSize);
		chunk->size = bufferSize;
	}

	fChunks.Add(chunk);
	return chunk->status == B_OK;
}
