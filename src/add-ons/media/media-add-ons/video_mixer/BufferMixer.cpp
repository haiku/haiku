/*
 * Copyright (C) 2010 David McPaul
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */

// This class merges buffers together
// Merge is called everytime a primary buffer needs to be passed downstream
// This should allow different framerates to be handled by buffering slower 
// buffer producers and discarding buffers from faster producers
// TODO ColorConversion

#include "BufferMixer.h"

BufferMixer::BufferMixer() {
}

BufferMixer::~BufferMixer() {
}

bool
BufferMixer::isBufferAvailable() {
	return groupedBuffers[0] != NULL;
}

// Should only be called after checking with isBufferAvailable
BBuffer *
BufferMixer::GetOutputBuffer() {
	// Do the merging of all buffers in the groupedBuffers map
	// into the primary buffer and return that buffer.
	// The primary buffer is removed;
	
	BBuffer *outputBuffer = groupedBuffers[0];
	groupedBuffers[0] = NULL;
	
	std::map<int32, BBuffer*>::iterator each;
	
	for (each=groupedBuffers.begin(); each != groupedBuffers.end(); each++) {
		if (each->second != outputBuffer) {
			if (each->second != NULL) {
				Merge(each->second, outputBuffer);
			}
		}
	}
	
	return outputBuffer;
}

#define ALPHABLEND(source, destination, alpha) (((destination) * (256 - (alpha)) + (source) * (alpha)) >> 8)

void
BufferMixer::Merge(BBuffer *input, BBuffer *output) {
	// Currently only deals with RGBA32
	
	uint8 *source = (uint8 *)input->Data();
	uint8 *destination = (uint8 *)output->Data();
	uint32 size = input->Header()->size_used / 4;
	uint8 alpha = 0;
	uint8 c1, c2, c3;

	for (uint32 i=0; i<size; i++) {
		c1    = *source++;
		c2    = *source++;
		c3    = *source++;
		alpha = *source++;
		destination[0] = ALPHABLEND(c1, destination[0], alpha);
		destination[1] = ALPHABLEND(c2, destination[1], alpha);
		destination[2] = ALPHABLEND(c3, destination[2], alpha);
		destination[3] = 0x00;
		destination += 4;
	}
}

void
BufferMixer::AddBuffer(int32 id, BBuffer *buffer, bool isPrimary) {
	BBuffer *oldBuffer;
	
	if (isPrimary) {
		oldBuffer = groupedBuffers[0];
		groupedBuffers[0] = buffer;
	} else {
		oldBuffer = groupedBuffers[id];
		groupedBuffers[id] = buffer;
	}

	if (oldBuffer != NULL) {
		oldBuffer->Recycle();
	}
}

void 
BufferMixer::RemoveBuffer(int32 id) {
	BBuffer *oldBuffer;

	if (uint32(id) < groupedBuffers.size()) {
		oldBuffer = groupedBuffers[id];
		groupedBuffers[id] = NULL;
	
		if (oldBuffer != NULL) {
			oldBuffer->Recycle();
		}
	}	
}
