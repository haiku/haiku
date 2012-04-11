/*
 * Copyright 2012, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *    Alexander von Gluck, kallisti5@unixzen.com
 */


#include "ringqueue.h"

#include <stdlib.h>
#include <string.h>


#define TRACE_RENDER_QUEUE
#ifdef TRACE_RENDER_QUEUE
extern "C" void _sPrintf(const char* format, ...);
#   define TRACE(x...) _sPrintf("radeon_hd: " x)
#else
#   define TRACE(x...) ;
#endif

#define ERROR(x...) _sPrintf("radeon_hd: " x)


static const char* queueName[RADEON_QUEUE_MAX] = {
    "GFX",
    "CP1",
    "CP2"
};


static int
compute_order(unsigned long size)
{
	int	order;
	unsigned long tmp;
	for (order = 0, tmp = size; tmp >>= 1; ++order);
		if (size & ~(1 << order))
			++order;
		return order;
}


RingQueue::RingQueue(size_t sizeBytes, uint32 queueType)
	:
	_queueType(queueType),
	_readPtr(0),
	_writePtr(0)
{
	TRACE("%s: Requested %d bytes for %s RingQueue.\n", __func__, sizeBytes,
		queueName[_queueType]);

	size_t renderQueueSize = compute_order(sizeBytes / 8);
	_size = (1 << (renderQueueSize + 1)) * 4;
	_writeBytesAvail = _size;
	_alignMask = 16 - 1;

	TRACE("%s: Allocating %d bytes for %s RingQueue.\n", __func__, _size,
		queueName[_queueType]);

	// Allocate buffer memory
	_data = (unsigned char*)malloc(_size);
	// Clear buffer
	memset(_data, 0, _size);
}


RingQueue::~RingQueue()
{
	TRACE("%s: Closing %s RingQueue.\n", __func__, queueName[_queueType]);
	free(_data);
}


status_t
RingQueue::Empty()
{
	TRACE("%s: Clearing %s RingQueue\n", __func__, queueName[_queueType]);
	// Clear buffer
	memset(_data, 0, _size);

	// Reset counters
	_readPtr = 0;
	_writePtr = 0;
	_writeBytesAvail = _size;
	return B_OK;
}


size_t
RingQueue::Read(unsigned char* dataPtr, size_t bytes)
{
	// If there is no data or nothing to read, return 0 bytes
	if (dataPtr == 0 || bytes <= 0 || _writeBytesAvail == _size)
		return 0;

	size_t readBytesAvail = _size - _writeBytesAvail;

	// Set a high threshold of total available bytes available.
	if (bytes > readBytesAvail)
		bytes = readBytesAvail;

	// Keep track of position and pull needed data
	if (bytes > _size - _readPtr) {
		size_t len = _size - _readPtr;
		memcpy(dataPtr, _data + _readPtr, len);
		memcpy(dataPtr + len, _data, bytes - len);
	} else {
		memcpy(dataPtr, _data + _readPtr, bytes);
	}

	_readPtr = (_readPtr + bytes) % _size;
	_writeBytesAvail += bytes;

	return bytes;
}


size_t
RingQueue::Write(unsigned char* dataPtr, size_t bytes)
{
	// If there is no data, or no room available, 0 bytes written.
	if (dataPtr == 0 || bytes <= 0 || _writeBytesAvail == 0)
		return 0;

	// Set a high threshold of the number of bytes available.
	if (bytes > _writeBytesAvail)
		bytes = _writeBytesAvail;

	// Keep track of position and push needed data
	if (bytes > _size - _writePtr) {
		size_t len = _size - _writePtr;
		memcpy(_data + _writePtr, dataPtr, len);
		memcpy(_data, dataPtr + len, bytes - len);
	} else
		memcpy(_data + _writePtr, dataPtr, bytes);

	_writePtr = (_writePtr + bytes) % _size;
	_writeBytesAvail -= bytes;

	return bytes;
}
