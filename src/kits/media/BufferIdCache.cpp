/***********************************************************************
 * Copyright (c) 2002 Marcus Overhagen. All Rights Reserved.
 * This file may be used under the terms of the OpenBeOS License.
 *
 * A cache for BBuffers to be received by 
 * BBufferConsumer::BufferReceived()
 ***********************************************************************/

#include <Buffer.h>
#include "BufferIdCache.h"
#include "debug.h"

// XXX we are not allowed to delete BBuffer objects when they are not recycled

_buffer_id_cache::_buffer_id_cache()
{
}

_buffer_id_cache::~_buffer_id_cache()
{
	// XXX deleting buffers here is not save, too
/*
	fMap.Rewind();
	BBuffer **buffer;
	while (fMap.GetNext(&buffer)) {
		fMap.RemoveCurrent();
		delete *buffer;
	}
*/
}
	
BBuffer *
_buffer_id_cache::GetBuffer(media_buffer_id id)
{
	BBuffer **buffer;
	if (fMap.Get(id, &buffer))
		return *buffer;

	buffer_clone_info ci;
	ci.buffer = id;
	BBuffer *buf = new BBuffer(ci);

	fMap.Insert(id, buf);
	return buf;
}
