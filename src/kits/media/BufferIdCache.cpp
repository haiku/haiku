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

_buffer_id_cache::_buffer_id_cache() : 
	in_use(0),
	used(0),
	stat_missed(0),
	stat_hit(0)
{
	for (int i = 0; i < MAX_CACHED_BUFFER; i++) {
		info[i].buffer = 0;
		info[i].id = 0;
		info[i].last_used = 0;
	}
}

_buffer_id_cache::~_buffer_id_cache()
{
	for (int i = 0; i < MAX_CACHED_BUFFER; i++)
		if (info[i].buffer)
			delete info[i].buffer;
	TRACE("### _buffer_id_cache finished, %ld in_use, %ld used, %ld hit, %ld missed\n",in_use,used,stat_hit,stat_missed);
}
	
BBuffer *
_buffer_id_cache::GetBuffer(media_buffer_id id)
{
	if (id == 0)
		debugger("_buffer_id_cache::GetBuffer called with 0 id\n");
	
	used++;

	// try to find in cache		
	for (int i = 0; i < MAX_CACHED_BUFFER; i++) {
		if (info[i].id == id) {
			stat_hit++;
			info[i].last_used = used;
			return info[i].buffer;
		}
	}
	
	stat_missed++;

	// remove last recently used
	if (in_use == MAX_CACHED_BUFFER) {
		int32 min_last_used = used;
		int index = 0;
		for (int i = 0; i < MAX_CACHED_BUFFER; i++) {
			if (info[i].last_used < min_last_used) {
				min_last_used = info[i].last_used;
				index = i;
			}
		}
		delete info[index].buffer;
		info[index].buffer = NULL;
		in_use--;
	}
	
	BBuffer *buffer;
	buffer_clone_info ci;
	ci.buffer = id;
	buffer = new BBuffer(ci);

	// insert into cache
	for (int i = 0; i < MAX_CACHED_BUFFER; i++) {
		if (info[i].buffer == NULL) {
			info[i].buffer = buffer;
			info[i].id = id;
			info[i].last_used = used;
			in_use++;
			break;
		}
	}
	return buffer;	
}
