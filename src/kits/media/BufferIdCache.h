/***********************************************************************
 * Copyright (c) 2002 Marcus Overhagen. All Rights Reserved.
 * This file may be used under the terms of the OpenBeOS License.
 *
 * A cache for BBuffers to be received by 
 * BBufferConsumer::BufferReceived()
 ***********************************************************************/
#ifndef _BUFFER_ID_CACHE_H_
#define _BUFFER_ID_CACHE_H_

#include "TMap.h"

class _buffer_id_cache
{
public:
	_buffer_id_cache();
	~_buffer_id_cache();
	
	BBuffer *GetBuffer(media_buffer_id id);

private:
	Map<media_buffer_id, BBuffer *> fMap;
};

#endif
