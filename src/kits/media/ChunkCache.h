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
