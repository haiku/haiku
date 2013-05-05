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


int
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
	fQueueType(queueType),
	fReadPtr(0),
	fWritePtr(0)
{
	TRACE("%s: Requested %d bytes for %s RingQueue.\n", __func__, sizeBytes,
		queueName[fQueueType]);

	size_t renderQueueSize = compute_order(sizeBytes / 8);
	fSize = (1 << (renderQueueSize + 1)) * 4;
	fWriteBytesAvail = fSize;
	fAlignMask = 16 - 1;

	TRACE("%s: Allocating %d bytes for %s RingQueue.\n", __func__, fSize,
		queueName[fQueueType]);

	// Allocate buffer memory
	fData = (unsigned char*)malloc(fSize);
	// Clear buffer
	memset(fData, 0, fSize);
}


RingQueue::~RingQueue()
{
	TRACE("%s: Closing %s RingQueue.\n", __func__, queueName[fQueueType]);
	free(fData);
}


status_t
RingQueue::Empty()
{
	TRACE("%s: Clearing %s RingQueue\n", __func__, queueName[fQueueType]);
	// Clear buffer
	memset(fData, 0, fSize);

	// Reset counters
	fReadPtr = 0;
	fWritePtr = 0;
	fWriteBytesAvail = fSize;
	return B_OK;
}


size_t
RingQueue::Read(unsigned char* dataPtr, size_t bytes)
{
	// If there is no data or nothing to read, return 0 bytes
	if (dataPtr == 0 || bytes <= 0 || fWriteBytesAvail == fSize)
		return 0;

	size_t readBytesAvail = fSize - fWriteBytesAvail;

	// Set a high threshold of total available bytes available.
	if (bytes > readBytesAvail)
		bytes = readBytesAvail;

	// Keep track of position and pull needed data
	if (bytes > fSize - fReadPtr) {
		size_t len = fSize - fReadPtr;
		memcpy(dataPtr, fData + fReadPtr, len);
		memcpy(dataPtr + len, fData, bytes - len);
	} else {
		memcpy(dataPtr, fData + fReadPtr, bytes);
	}

	fReadPtr = (fReadPtr + bytes) % fSize;
	fWriteBytesAvail += bytes;

	return bytes;
}


size_t
RingQueue::Write(unsigned char* dataPtr, size_t bytes)
{
	// If there is no data, or no room available, 0 bytes written.
	if (dataPtr == 0 || bytes <= 0 || fWriteBytesAvail == 0)
		return 0;

	// Set a high threshold of the number of bytes available.
	if (bytes > fWriteBytesAvail)
		bytes = fWriteBytesAvail;

	// Keep track of position and push needed data
	if (bytes > fSize - fWritePtr) {
		size_t len = fSize - fWritePtr;
		memcpy(fData + fWritePtr, dataPtr, len);
		memcpy(fData, dataPtr + len, bytes - len);
	} else
		memcpy(fData + fWritePtr, dataPtr, bytes);

	fWritePtr = (fWritePtr + bytes) % fSize;
	fWriteBytesAvail -= bytes;

	return bytes;
}
