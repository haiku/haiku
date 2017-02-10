#ifndef BUFFER_POOL_H
#define BUFFER_POOL_H
/* BufferPool - a buffer pool for uncached file access
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the MIT License.
*/


#include <OS.h>


class BufferPool {
	public:
		BufferPool();
		~BufferPool();

		status_t InitCheck();

		status_t RequestBuffers(uint32 blockSize);
		status_t ReleaseBuffers();

		status_t GetBuffer(void **_buffer);
		status_t PutBuffer(void *buffer);

	private:
		sem_id	fLock, fFreeBuffers;
		void	**fFirstFree;
};

#endif	/* BUFFER_POOL_H */
