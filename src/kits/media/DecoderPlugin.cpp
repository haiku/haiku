/* 
** Copyright 2004, Marcus Overhagen. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "DecoderPlugin.h"

#include <MediaFormats.h>
#include <stdio.h>
#include <string.h>


Decoder::Decoder()
{
	fChunkProvider = 0;
}


Decoder::~Decoder()
{
	delete fChunkProvider;
}

	
status_t
Decoder::GetNextChunk(void **chunkBuffer, int32 *chunkSize,
					  media_header *mediaHeader)
{
	return fChunkProvider->GetNextChunk(chunkBuffer, chunkSize, mediaHeader);
}


void
Decoder::Setup(ChunkProvider *provider)
{
	delete fChunkProvider;
	fChunkProvider = provider;
}


//	#pragma mark -


DecoderPlugin::DecoderPlugin()
{
}

