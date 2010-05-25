/*
 * Copyright (C) 2010 David McPaul
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */
 
#ifndef __BUFFER_MIXER__
#define __BUFFER_MIXER__

#include <media/Buffer.h>
#include <map>

class BufferMixer {
public:
	BufferMixer();
	~BufferMixer();
		
	bool isBufferAvailable();
	BBuffer *GetOutputBuffer();
	void AddBuffer(int32 id, BBuffer *buffer, bool isPrimary);
	void RemoveBuffer(int32 id);
	void Merge(BBuffer *input, BBuffer *output);

private:
	std::map<int32, BBuffer *> groupedBuffers;
};

#endif	//__BUFFER_MIXER__
