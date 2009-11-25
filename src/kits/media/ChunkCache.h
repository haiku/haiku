/*
 * Copyright 2004, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CHUNK_CACHE_H
#define _CHUNK_CACHE_H


#include <Locker.h>
#include <MediaDefs.h>


namespace BPrivate {
namespace media {


struct chunk_info {
	chunk_info*		next;
	void*			buffer;
	size_t			sizeUsed;
	size_t			sizeMax;
	media_header	mediaHeader;
	status_t		status;
};


class ChunkCache {
public:
								ChunkCache();
								~ChunkCache();

			void				MakeEmpty();
			bool				NeedsRefill();

			status_t			GetNextChunk(const void** _chunkBuffer,
									size_t* _chunkSize,
									media_header* mediaHeader);
			void				PutNextChunk(const void* chunkBuffer,
									size_t chunkSize,
									const media_header& mediaHeader,
									status_t status);

private:
	enum { CHUNK_COUNT = 5 };

			chunk_info*		fNextPut;
			chunk_info*		fNextGet;
			chunk_info		fChunkInfos[CHUNK_COUNT];

			sem_id			fGetWaitSem;
			int32			fEmptyChunkCount;
			int32			fReadyChunkCount;
			int32			fNeedsRefill;

			BLocker			fLocker;
};


} // namespace media
} // namespace BPrivate

using namespace BPrivate::media;

#endif	// _CHUNK_CACHE_H
