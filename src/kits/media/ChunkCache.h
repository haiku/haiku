#ifndef _CHUNK_CACHE_H
#define _CHUNK_CACHE_H

#include <MediaDefs.h>

namespace BPrivate {
namespace media {


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

};

}; // namespace media
}; // namespace BPrivate

using namespace BPrivate::media;

#endif
