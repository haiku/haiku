/* 
 * Copyright 2004, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "DecoderPlugin.h"

#include <stdio.h>
#include <string.h>

#include <MediaFormats.h>


Decoder::Decoder()
	:
	fChunkProvider(NULL),
	fMediaPlugin(NULL)
{
}


Decoder::~Decoder()
{
	delete fChunkProvider;
}

	
status_t
Decoder::GetNextChunk(const void **chunkBuffer, size_t *chunkSize,
					  media_header *mediaHeader)
{
	return fChunkProvider->GetNextChunk(chunkBuffer, chunkSize, mediaHeader);
}


void
Decoder::SetChunkProvider(ChunkProvider *provider)
{
	delete fChunkProvider;
	fChunkProvider = provider;
}


status_t
Decoder::Perform(perform_code code, void* _data)
{
	return B_OK;
}

void Decoder::_ReservedDecoder1() {}
void Decoder::_ReservedDecoder2() {}
void Decoder::_ReservedDecoder3() {}
void Decoder::_ReservedDecoder4() {}
void Decoder::_ReservedDecoder5() {}

//	#pragma mark -


DecoderPlugin::DecoderPlugin()
{
}
