/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2002, Marcus Overhagen. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BUFFER_CACHE_H_
#define _BUFFER_CACHE_H_


#include <map>

#include <MediaDefs.h>


class BBuffer;


namespace BPrivate {


class BufferCache {
public:
								BufferCache();
								~BufferCache();

			BBuffer*			GetBuffer(media_buffer_id id);

private:
	typedef std::map<media_buffer_id, BBuffer*> BufferMap;

			BufferMap			fMap;
};


}	// namespace BPrivate


#endif	// _BUFFER_CACHE_H_
