/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2002, Marcus Overhagen. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


//! A cache for BBuffers to be received by BBufferConsumer::BufferReceived().


#include "BufferCache.h"

#include <Buffer.h>

#include "debug.h"


namespace BPrivate {


BufferCache::BufferCache()
{
}


BufferCache::~BufferCache()
{
	for (BufferMap::iterator iterator = fMap.begin(); iterator != fMap.end();
			iterator++) {
		delete iterator->second;
	}
}


BBuffer*
BufferCache::GetBuffer(media_buffer_id id)
{
	BufferMap::iterator found = fMap.find(id);
	if (found != fMap.end())
		return found->second;

	buffer_clone_info info;
	info.buffer = id;
	BBuffer* buffer = new(std::nothrow) BBuffer(info);
	if (buffer == NULL)
		return NULL;

	try {
		fMap.insert(std::make_pair(id, buffer));
	} catch (std::bad_alloc& exception) {
		delete buffer;
		return NULL;
	}

	return buffer;
}


}	// namespace BPrivate
