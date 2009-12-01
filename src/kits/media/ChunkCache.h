/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CHUNK_CACHE_H
#define _CHUNK_CACHE_H


#include <Locker.h>
#include <MediaDefs.h>
#include <RealtimeAlloc.h>

#include <kernel/util/DoublyLinkedList.h>

#include "ReaderPlugin.h"


namespace BPrivate {
namespace media {


struct chunk_buffer;
typedef DoublyLinkedList<chunk_buffer> ChunkList;

struct chunk_buffer : public DoublyLinkedListLinkImpl<chunk_buffer> {
	chunk_buffer();
	~chunk_buffer();

	void*			buffer;
	size_t			size;
	size_t			capacity;
	media_header	header;
	status_t		status;
};


class ChunkCache : public BLocker {
public:
								ChunkCache(sem_id waitSem, size_t maxBytes);
								~ChunkCache();

			status_t			InitCheck() const;

			void				MakeEmpty();
			bool				SpaceLeft() const;

			chunk_buffer*		NextChunk();
			void				RecycleChunk(chunk_buffer* chunk);
			bool				ReadNextChunk(Reader* reader, void* cookie);

private:
			rtm_pool*			fRealTimePool;
			sem_id				fWaitSem;
			size_t				fMaxBytes;
			ChunkList			fChunks;
			ChunkList			fUnusedChunks;
			ChunkList			fInFlightChunks;
};


}	// namespace media
}	// namespace BPrivate

using namespace BPrivate::media;

#endif	// _CHUNK_CACHE_H
