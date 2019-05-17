/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CHUNK_CACHE_H
#define _CHUNK_CACHE_H


#include <Locker.h>
#include <MediaDefs.h>
#include <RealtimeAlloc.h>
#include <queue>
#include <deque>

#include "ReaderPlugin.h"


namespace BPrivate {
namespace media {

// Limit to 10 entries, we might want to instead limit to a length of time
#define CACHE_MAX_ENTRIES 10

struct chunk_buffer {
	void*			buffer;
	size_t			size;
	size_t			capacity;
	media_header	header;
	status_t		status;
};

typedef std::queue<chunk_buffer*> ChunkQueue;
typedef std::deque<chunk_buffer*> ChunkList;

class ChunkCache : public BLocker {
public:
								ChunkCache(sem_id waitSem, size_t maxBytes);
								~ChunkCache();

			status_t			InitCheck() const;

			void				MakeEmpty();
			bool				SpaceLeft() const;

			chunk_buffer*		NextChunk(Reader* reader, void* cookie);
			void				RecycleChunk(chunk_buffer* chunk);
			bool				ReadNextChunk(Reader* reader, void* cookie);

private:
			rtm_pool*			fRealTimePool;
			sem_id				fWaitSem;
			size_t				fMaxBytes;
			ChunkQueue			fChunkCache;
			ChunkList			fUnusedChunks;
};


}	// namespace media
}	// namespace BPrivate

using namespace BPrivate::media;

#endif	// _CHUNK_CACHE_H
