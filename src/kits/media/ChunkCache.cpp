#include "ChunkCache.h"
#include "debug.h"


ChunkCache::ChunkCache()
{
}


ChunkCache::~ChunkCache()
{
}


void
ChunkCache::MakeEmpty()
{
}


bool
ChunkCache::NeedsRefill()
{
	return false;
}

					
status_t
ChunkCache::GetNextChunk(void **chunkBuffer, int32 *chunkSize, media_header *mediaHeader)
{
	return B_ERROR;
}


void
ChunkCache::PutNextChunk(void *chunkBuffer, int32 chunkSize, const media_header &mediaHeader, status_t err)
{
}
