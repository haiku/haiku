/*
 * Copyright (c) 2004, Marcus Overhagen
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _CHUNK_CACHE_H
#define _CHUNK_CACHE_H

#include <MediaDefs.h>

namespace BPrivate {
namespace media {


struct chunk_info
{
	chunk_info *	next;
	void *			buffer;
	int32			sizeUsed;
	int32			sizeMax;
	media_header	mediaHeader;
	status_t		err;
};


class ChunkCache
{
public:
					ChunkCache();
					~ChunkCache();

	void			MakeEmpty();
	bool			NeedsRefill();
					
	status_t		GetNextChunk(void **chunkBuffer, int32 *chunkSize, media_header *mediaHeader);
	void			PutNextChunk(void *chunkBuffer, int32 chunkSize, const media_header &mediaHeader, status_t err);
	
private:
	enum { CHUNK_COUNT = 5 };
	
	chunk_info *	fNextPut;
	chunk_info *	fNextGet;
	chunk_info		fChunkInfos[CHUNK_COUNT];
	
	sem_id			fGetWaitSem;
	int32			fEmptyChunkCount;
	int32			fReadyChunkCount;
	int32			fNeedsRefill;
	
	BLocker *		fLocker;
};


}; // namespace media
}; // namespace BPrivate

using namespace BPrivate::media;

#endif
