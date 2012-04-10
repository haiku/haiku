/*
 * Copyright 2012, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *    Alexander von Gluck, kallisti5@unixzen.com
 */


#include "Accelerant.h"


// A basic ring buffer for passing render data into card.
class RenderQueue {
public:
							RenderQueue(size_t bytes);
							~RenderQueue();
			size_t			Read(unsigned char* data, size_t bytes);
			size_t			Write(unsigned char* data, size_t bytes);
			status_t		Empty();

			size_t			GetSize() {return _size;};
			size_t			GetWriteAvail() {return _writeBytesAvail;}
			size_t			GetReadAvail() {return _size - _writeBytesAvail;}
private:
			unsigned char*	_data;
			size_t			_size;
			size_t			_writeBytesAvail;
			int				_readPtr;
			int				_writePtr;

			uint32			_alignMask;
};
