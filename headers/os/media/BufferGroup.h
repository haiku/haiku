/*
 * Copyright 2009, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BUFFER_GROUP_H
#define _BUFFER_GROUP_H


#include <MediaDefs.h>


class BBuffer;
namespace BPrivate {
	struct SharedBufferList;
}


class BBufferGroup {
public:
							BBufferGroup(size_t size, int32 count = 3,
								uint32 placement = B_ANY_ADDRESS,
								uint32 lock = B_FULL_LOCK);
	explicit				BBufferGroup();
							BBufferGroup(int32 count,
								const media_buffer_id* buffers);
							~BBufferGroup();

			status_t		InitCheck();

			status_t		AddBuffer(const buffer_clone_info& info,
								BBuffer** _buffer = NULL);

			BBuffer*		RequestBuffer(size_t size,
								bigtime_t timeout = B_INFINITE_TIMEOUT);
			status_t		RequestBuffer(BBuffer* buffer,
								bigtime_t timeout = B_INFINITE_TIMEOUT);

			status_t		RequestError();

			status_t		CountBuffers(int32* _count);
			status_t		GetBufferList(int32 bufferCount,
								BBuffer** _buffers);

			status_t		WaitForBuffers();
			status_t		ReclaimAllBuffers();

private:
			// deprecated BeOS R4 API
			status_t 		AddBuffersTo(BMessage* message, const char* name,
								bool needLock);

							BBufferGroup(const BBufferGroup& other);
			BBufferGroup&	operator=(const BBufferGroup& other);

			status_t		_Init();

private:
	friend struct BPrivate::SharedBufferList;

			status_t		fInitError;
			status_t		fRequestError;
			int32			fBufferCount;
			BPrivate::SharedBufferList* fBufferList;
			sem_id			fReclaimSem;

			uint32			_reserved[9];
};


#endif	// _BUFFER_GROUP_H
