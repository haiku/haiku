/* BufferPool - a buffer pool for uncached file access
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the OpenBeOS License.
*/


#include "BufferPool.h"
#include "cpp.h"

#include <stdlib.h>


const uint32 kNumBuffers = 8;


BufferPool::BufferPool()
	:
	fFirstFree(NULL)
{
	fLock = create_sem(1, "buffer lock");
	fFreeBuffers = create_sem(0, "free buffers");
}


BufferPool::~BufferPool()
{
	delete_sem(fLock);
	delete_sem(fFreeBuffers);
	
	void **buffer = fFirstFree;
	while (buffer != NULL) {
		void **nextBuffer = (void **)*buffer;
		free(buffer);
		buffer = nextBuffer;
	}
}


status_t 
BufferPool::InitCheck()
{
	if (fLock < B_OK
		|| fFreeBuffers < B_OK)
		return B_ERROR;

	return B_OK;
}


status_t 
BufferPool::RequestBuffers(uint32 blockSize)
{
	void **buffers[kNumBuffers];

	// allocate and connect buffers

	for (int32 i = 0; i < kNumBuffers; i++) {
		buffers[i] = (void **)malloc(blockSize);
		if (buffers[i] == NULL) {
			// free already allocated buffers
			for (;i-- > 0; i++)
				free(buffers[i]);
			return B_NO_MEMORY;
		}
		if (i > 0)
			*(buffers[i]) = buffers[i - 1];
	}

	// add the buffers to the free buffers queue

	status_t status = acquire_sem(fLock);
	if (status == B_OK) {
		*(buffers[0]) = fFirstFree;
		fFirstFree = buffers[kNumBuffers - 1];
		release_sem(fLock);
		release_sem_etc(fFreeBuffers, kNumBuffers, B_DO_NOT_RESCHEDULE);
	} else {
		for (int32 i = 0; i < kNumBuffers; i++)
			free(buffers[i]);
	}

	return status;
}


status_t 
BufferPool::ReleaseBuffers()
{
	status_t status = acquire_sem_etc(fFreeBuffers, kNumBuffers, 0, 0);
	if (status < B_OK)
		return status;

	status = acquire_sem(fLock);
	if (status < B_OK)
		return status;

	void **buffer = fFirstFree;
	for (int32 i = 0; i < kNumBuffers && buffer; i++) {
		void **nextBuffer = (void **)*buffer;

		free(buffer);
		buffer = nextBuffer;
	}
	fFirstFree = buffer;

	release_sem(fLock);
}


status_t
BufferPool::GetBuffer(void **_buffer)
{
	status_t status = acquire_sem(fFreeBuffers);
	if (status < B_OK)
		return status;

	if ((status = acquire_sem(fLock)) < B_OK) {
		release_sem(fFreeBuffers);
		return status;
	}

	void **buffer = fFirstFree;
	fFirstFree = (void **)*buffer;

	release_sem(fLock);

	*_buffer = (void *)buffer;
	return B_OK;
}


status_t 
BufferPool::PutBuffer(void *_buffer)
{
	void **buffer = (void **)_buffer;
	if (buffer == NULL)
		return B_BAD_VALUE;

	status_t status = acquire_sem(fLock);
	if (status < B_OK)
		return status;

	*buffer = fFirstFree;
	fFirstFree = buffer;

	release_sem(fLock);
	release_sem(fFreeBuffers);

	return B_OK;
}

