/* 
 * Copyright 2004, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Decoder.h>

#include <MediaFormats.h>

#include <stdio.h>
#include <string.h>


namespace BCodecKit {


BDecoder::BDecoder()
	:
	fChunkProvider(NULL),
	fMediaPlugin(NULL)
{
}


BDecoder::~BDecoder()
{
	delete fChunkProvider;
}

	
status_t
BDecoder::GetNextChunk(const void **chunkBuffer, size_t *chunkSize,
					  media_header *mediaHeader)
{
	return fChunkProvider->GetNextChunk(chunkBuffer, chunkSize, mediaHeader);
}


void
BDecoder::SetChunkProvider(BChunkProvider *provider)
{
	delete fChunkProvider;
	fChunkProvider = provider;
}


status_t
BDecoder::Perform(perform_code code, void* _data)
{
	return B_OK;
}


void BDecoder::_ReservedDecoder1() {}
void BDecoder::_ReservedDecoder2() {}
void BDecoder::_ReservedDecoder3() {}
void BDecoder::_ReservedDecoder4() {}
void BDecoder::_ReservedDecoder5() {}


BDecoderPlugin::BDecoderPlugin()
{
}


BChunkProvider::BChunkProvider()
{
}


BChunkProvider::~BChunkProvider()
{
}


} // namespace BCodecKit
