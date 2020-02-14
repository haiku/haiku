/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2002, Marcus Overhagen. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SHARED_BUFFER_LIST_H_
#define _SHARED_BUFFER_LIST_H_


#include <Buffer.h>


namespace BPrivate {


class SharedBufferList
{
public:
	static	area_id						Create(SharedBufferList** _list);
	static	SharedBufferList*			Get();
	static	void						Invalidate();

			void						Put();
			void 						DeleteGroupAndPut(
											sem_id groupReclaimSem);

			status_t					Lock();
			status_t					Unlock();

			status_t					AddBuffer(sem_id groupReclaimSem,
											const buffer_clone_info& info,
											BBuffer** buffer);
			status_t					RemoveBuffer(BBuffer* buffer);

			// Call AddBuffer and CheckID locked
			status_t					AddBuffer(sem_id groupReclaimSem,
											BBuffer* buffer);
			status_t					CheckID(sem_id groupSem,
											media_buffer_id id) const;

			status_t					RequestBuffer(sem_id groupReclaimSem,
											int32 buffersInGroup, size_t size,
											media_buffer_id wantID,
											BBuffer** _buffer,
											bigtime_t timeout);

			status_t					RecycleBuffer(BBuffer* buffer);

			status_t					GetBufferList(sem_id groupReclaimSem,
											int32 bufferCount,
											BBuffer** buffers);
private:
	struct _shared_buffer_info {
		media_buffer_id			id;
		BBuffer*				buffer;
		bool 					reclaimed;
		// The reclaim_sem belonging to the BBufferGroup of this BBuffer
		// is also used as a unique identifier of the group
		sem_id					reclaim_sem;
	};

	// 16 bytes per buffer, 8 pages in total (one entry less for the list)
	enum { kMaxBuffers = 2047 };

			status_t					_Init();
			void 						_RequestBufferInOtherGroups(
											sem_id groupReclaimSem,
											media_buffer_id id);

private:
			sem_id						fSemaphore;
			int32						fAtom;

			_shared_buffer_info			fInfos[kMaxBuffers];
			int32						fCount;
};


}	// namespace BPrivate


#endif	// _SHARED_BUFFER_LIST_H_
