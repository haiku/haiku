/*
 * Copyright 2019, Ryan Leavengood.
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2002, Marcus Overhagen. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BUFFER_CACHE_H_
#define _BUFFER_CACHE_H_


#include <HashMap.h>
#include <MediaDefs.h>


class BBuffer;


namespace BPrivate {


struct buffer_cache_entry {
	BBuffer*	buffer;
	port_id		port;
};


class BufferCache {
public:
								BufferCache();
								~BufferCache();

			BBuffer*			GetBuffer(media_buffer_id id, port_id port);

			void				FlushCacheForPort(port_id port);

private:
	typedef HashMap<HashKey32<media_buffer_id>, buffer_cache_entry> BufferMap;

			BufferMap			fMap;
};


}	// namespace BPrivate


#endif	// _BUFFER_CACHE_H_
