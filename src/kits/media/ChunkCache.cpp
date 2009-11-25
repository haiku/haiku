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
	free(buffer);
}


// #pragma mark -


ChunkCache::ChunkCache(sem_id waitSem, size_t maxBytes)
	:
	BLocker("media chunk cache"),
	fWaitSem(waitSem),
	fMaxBytes(maxBytes),
	fBytes(0)
{
}


ChunkCache::~ChunkCache()
{
	while (chunk_buffer* chunk = fChunks.RemoveHead())
		delete chunk;

	while (chunk_buffer* chunk = fUnusedChunks.RemoveHead())
		delete chunk;

	while (chunk_buffer* chunk = fInFlightChunks.RemoveHead())
		delete chunk;
}


void
ChunkCache::MakeEmpty()
{
	ASSERT(IsLocked());

	fUnusedChunks.MoveFrom(&fChunks);
	fBytes = 0;

	release_sem(fWaitSem);
}


bool
ChunkCache::SpaceLeft() const
{
	ASSERT(IsLocked());

	return fBytes < fMaxBytes;
}


chunk_buffer*
ChunkCache::NextChunk()
{
	ASSERT(IsLocked());

	chunk_buffer* chunk = fChunks.RemoveHead();
	if (chunk != NULL) {
		fBytes -= chunk->capacity;
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
		chunk = new(std::nothrow) chunk_buffer;
		if (chunk == NULL)
			return false;

	}

	const void* buffer;
	size_t bufferSize;
	chunk->status = reader->GetNextChunk(cookie, &buffer, &bufferSize,
		&chunk->header);
	if (chunk->status == B_OK) {
		if (chunk->capacity < bufferSize) {
			// adapt buffer size
			free(chunk->buffer);
			chunk->capacity = (bufferSize + 2047) & ~2047;
			chunk->buffer = malloc(chunk->capacity);
			if (chunk->buffer == NULL) {
				delete chunk;
				return false;
			}
		}

		memcpy(chunk->buffer, buffer, bufferSize);
		chunk->size = bufferSize;
		fBytes += chunk->capacity;
	}

	fChunks.Add(chunk);
	return chunk->status == B_OK;
}
