/***********************************************************************
 * Copyright (c) 2002 Marcus Overhagen. All Rights Reserved.
 * This file may be used under the terms of the OpenBeOS License.
 *
 * A cache for BBuffers to be received by 
 * BBufferConsumer::BufferReceived()
 ***********************************************************************/
#ifndef _BUFFER_ID_CACHE_H_
#define _BUFFER_ID_CACHE_H_

class _buffer_id_cache
{
public:
	_buffer_id_cache();
	~_buffer_id_cache();
	
	BBuffer *GetBuffer(media_buffer_id id);

private:
	enum { MAX_CACHED_BUFFER = 17 };
	struct _buffer_id_info
	{
		BBuffer *buffer;
		media_buffer_id id;
		int32 last_used;
	};
	_buffer_id_info info[MAX_CACHED_BUFFER];
	int32 in_use;
	int32 used;
	int32 stat_missed;
	int32 stat_hit;
};

#endif
