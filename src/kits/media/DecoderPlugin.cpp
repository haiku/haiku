#include "DecoderPlugin.h"

Decoder::Decoder()
{
}

Decoder::~Decoder()
{
}
	
status_t
Decoder::GetNextChunk(void **chunkBuffer, int32 *chunkSize,
					  media_header *mediaHeader)
{
	return fReader->ReadChunk((char **)chunkBuffer, chunkSize, mediaHeader);
}

void
Decoder::Setup(BMediaTrack *reader)
{
	fReader = reader;
}
