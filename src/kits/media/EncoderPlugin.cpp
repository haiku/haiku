/* 
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Copyright 2004, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "EncoderPlugin.h"

#include <stdio.h>
#include <string.h>

#include <MediaFormats.h>


Encoder::Encoder()
	:
	fChunkWriter(NULL),
	fMediaPlugin(NULL)
{
}


Encoder::~Encoder()
{
	delete fChunkWriter;
}

	
status_t
Encoder::WriteChunk(const void* chunkBuffer, size_t chunkSize,
	const media_header* mediaHeader)
{
	return fChunkWriter->WriteChunk(chunkBuffer, chunkSize, mediaHeader);
}


void
Encoder::SetChunkWriter(ChunkWriter* writer)
{
	delete fChunkWriter;
	fChunkWriter = writer;
}


status_t
Encoder::Perform(perform_code code, void* data)
{
	return B_OK;
}


void Encoder::_ReservedEncoder1() {}
void Encoder::_ReservedEncoder2() {}
void Encoder::_ReservedEncoder3() {}
void Encoder::_ReservedEncoder4() {}
void Encoder::_ReservedEncoder5() {}


//	#pragma mark -


EncoderPlugin::EncoderPlugin()
{
}
