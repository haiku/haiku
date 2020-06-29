/*
 * Copyright 2022 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MEMORY_RING_IO_H
#define _MEMORY_RING_IO_H


#include <pthread.h>

#include <DataIO.h>
#include <Locker.h>


class BMemoryRingIO : public BDataIO {
public:
								BMemoryRingIO(size_t size);
	virtual						~BMemoryRingIO();

			status_t			InitCheck() const; 

	virtual	ssize_t				Read(void* buffer, size_t size);
	virtual	ssize_t				Write(const void* buffer, size_t size);

			status_t			SetSize(size_t size);
			void				Clear();

			size_t				BytesAvailable();
			size_t				SpaceAvailable();
			size_t				BufferSize();

			status_t			WaitForRead(
									bigtime_t timeout = B_INFINITE_TIMEOUT);
			status_t			WaitForWrite(
									bigtime_t timeout = B_INFINITE_TIMEOUT);

			void				SetWriteDisabled(bool disabled);
			bool				WriteDisabled();

private:
			template<typename Condition>
			status_t			_WaitForCondition(bigtime_t timeout);
private:
			pthread_mutex_t		fLock;
			pthread_cond_t		fEvent;

			uint8*				fBuffer;

			size_t				fBufferSize;
			size_t				fWriteAtNext;
			size_t				fReadAtNext;

			bool				fBufferFull;
			bool				fWriteDisabled;

			uint32				_reserved[4];
};


#endif	// _MEMORY_RING_IO_H
