/*
 * Copyright 2012, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *    Alexander von Gluck, kallisti5@unixzen.com
 */
#ifndef _RADEON_HD_RINGQUEUE_H
#define _RADEON_HD_RINGQUEUE_H


#include "Accelerant.h"


#define RADEON_QUEUE_MAX 3
// Basic r100+ graphic data ring 
#define RADEON_QUEUE_TYPE_GFX_INDEX 0
// Cayman+ have two compute command processor rings
#define CAYMAN_QUEUE_TYPE_CP1_INDEX 1
#define CAYMAN_QUEUE_TYPE_CP2_INDEX 2


// A basic ring buffer for passing render data into card.
// Data flows from the host to the GPU
class RingQueue {
public:
							RingQueue(size_t bytes, uint32 queueType);
							~RingQueue();
			size_t			Read(unsigned char* data, size_t bytes);
			size_t			Write(unsigned char* data, size_t bytes);
			status_t		Empty();

			size_t			GetSize() {return _size;};
			size_t			GetWriteAvail() {return _writeBytesAvail;}
			size_t			GetReadAvail() {return _size - _writeBytesAvail;}
private:
			uint32			_queueType;

			unsigned char*	_data;
			size_t			_size;
			size_t			_writeBytesAvail;
			int				_readPtr;
			int				_writePtr;

			uint32			_alignMask;
};


#endif /* _RADEON_HD_RINGQUEUE_H */
