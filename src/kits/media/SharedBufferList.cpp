/*
 * Copyright 2009-2012, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2002, Marcus Overhagen. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


/*!	Used for BBufferGroup and BBuffer management across teams.
	Created in the media server, cloned into each BBufferGroup (visible in
	all address spaces).
*/

// TODO: don't use a simple list!


#include <SharedBufferList.h>

#include <string.h>

#include <Autolock.h>
#include <Buffer.h>
#include <Locker.h>

#include <MediaDebug.h>
#include <DataExchange.h>


static BPrivate::SharedBufferList* sList = NULL;
static area_id sArea = -1;
static int32 sRefCount = 0;
static BLocker sLocker("shared buffer list");


namespace BPrivate {


/*static*/ area_id
SharedBufferList::Create(SharedBufferList** _list)
{
	CALLED();

	size_t size = (sizeof(SharedBufferList) + (B_PAGE_SIZE - 1))
		& ~(B_PAGE_SIZE - 1);
	SharedBufferList* list;

	area_id area = create_area("shared buffer list", (void**)&list,
		B_ANY_ADDRESS, size, B_LAZY_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (area < 0)
		return area;

	status_t status = list->_Init();
	if (status != B_OK) {
		delete_area(area);
		return status;
	}

	return area;
}


/*static*/ SharedBufferList*
SharedBufferList::Get()
{
	CALLED();

	BAutolock _(sLocker);

	if (atomic_add(&sRefCount, 1) > 0 && sList != NULL)
		return sList;

	// ask media_server to get the area_id of the shared buffer list
	server_get_shared_buffer_area_request areaRequest;
	server_get_shared_buffer_area_reply areaReply;
	if (QueryServer(SERVER_GET_SHARED_BUFFER_AREA, &areaRequest,
			sizeof(areaRequest), &areaReply, sizeof(areaReply)) != B_OK) {
		ERROR("SharedBufferList::Get() SERVER_GET_SHARED_BUFFER_AREA failed\n");
		return NULL;
	}

	sArea = clone_area("shared buffer list clone", (void**)&sList,
		B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, areaReply.area);
	if (sArea < 0) {
		ERROR("SharedBufferList::Get() clone area %" B_PRId32 ": %s\n",
			areaReply.area, strerror(sArea));
		return NULL;
	}

	return sList;
}


/*static*/ void
SharedBufferList::Invalidate()
{
	delete_area(sArea);
	sList = NULL;
}


void
SharedBufferList::Put()
{
	CALLED();
	BAutolock _(sLocker);

	if (atomic_add(&sRefCount, -1) == 1)
		Invalidate();
}


/*!	Deletes all BBuffers of the group specified by \a groupReclaimSem, then
	unmaps the list from memory.
*/
void
SharedBufferList::DeleteGroupAndPut(sem_id groupReclaimSem)
{
	CALLED();

	if (Lock() == B_OK) {
		for (int32 i = 0; i < fCount; i++) {
			if (fInfos[i].reclaim_sem == groupReclaimSem) {
				// delete the associated buffer
				delete fInfos[i].buffer;

				// Decrement buffer count by one, and fill the gap
				// in the list with its last entry
				fCount--;
				if (fCount > 0)
					fInfos[i--] = fInfos[fCount];
			}
		}

		Unlock();
	}

	Put();
}


status_t
SharedBufferList::Lock()
{
	if (atomic_add(&fAtom, 1) > 0) {
		status_t status;
		do {
			status = acquire_sem(fSemaphore);
		} while (status == B_INTERRUPTED);

		return status;
	}
	return B_OK;
}


status_t
SharedBufferList::Unlock()
{
	if (atomic_add(&fAtom, -1) > 1)
		return release_sem(fSemaphore);

	return B_OK;
}


status_t
SharedBufferList::AddBuffer(sem_id groupReclaimSem,
	const buffer_clone_info& info, BBuffer** _buffer)
{
	status_t status = Lock();
	if (status != B_OK)
		return status;

	// Check if the id exists
	status = CheckID(groupReclaimSem, info.buffer);
	if (status != B_OK) {
		Unlock();
		return status;
	}
	BBuffer* buffer = new(std::nothrow) BBuffer(info);
	if (buffer == NULL) {
		Unlock();
		return B_NO_MEMORY;
	}

	if (buffer->Data() == NULL) {
		// BBuffer::Data() will return NULL if an error occured
		ERROR("BBufferGroup: error while creating buffer\n");
		delete buffer;
		Unlock();
		return B_ERROR;
	}

	status = AddBuffer(groupReclaimSem, buffer);
	if (status != B_OK) {
		delete buffer;
		Unlock();
		return status;
	} else if (_buffer != NULL)
		*_buffer = buffer;

	return Unlock();
}


status_t
SharedBufferList::AddBuffer(sem_id groupReclaimSem, BBuffer* buffer)
{
	CALLED();

	if (buffer == NULL)
		return B_BAD_VALUE;

	if (fCount == kMaxBuffers) {
		return B_MEDIA_TOO_MANY_BUFFERS;
	}

	fInfos[fCount].id = buffer->ID();
	fInfos[fCount].buffer = buffer;
	fInfos[fCount].reclaim_sem = groupReclaimSem;
	fInfos[fCount].reclaimed = true;
	fCount++;

	return release_sem_etc(groupReclaimSem, 1, B_DO_NOT_RESCHEDULE);
}


status_t
SharedBufferList::CheckID(sem_id groupSem, media_buffer_id id) const
{
	CALLED();

	if (id == 0)
		return B_OK;
	if (id < 0)
		return B_BAD_VALUE;

	for (int32 i = 0; i < fCount; i++) {
		if (fInfos[i].id == id
			&& fInfos[i].reclaim_sem == groupSem) {
			return B_ERROR;
		}
	}
	return B_OK;
}


status_t
SharedBufferList::RequestBuffer(sem_id groupReclaimSem, int32 buffersInGroup,
	size_t size, media_buffer_id wantID, BBuffer** _buffer, bigtime_t timeout)
{
	CALLED();
	// We always search for a buffer from the group indicated by groupReclaimSem
	// first.
	// If "size" != 0, we search for a buffer that is "size" bytes or larger.
	// If "wantID" != 0, we search for a buffer with this ID.
	// If "*_buffer" != NULL, we search for a buffer at this address.
	//
	// If we found a buffer, we also need to mark it in all other groups as
	// requested and also once need to acquire the reclaim_sem of the other
	// groups

	uint32 acquireFlags;

	if (timeout <= 0) {
		timeout = 0;
		acquireFlags = B_RELATIVE_TIMEOUT;
	} else if (timeout == B_INFINITE_TIMEOUT) {
		acquireFlags = B_RELATIVE_TIMEOUT;
	} else {
		timeout += system_time();
		acquireFlags = B_ABSOLUTE_TIMEOUT;
	}

	// With each itaration we request one more buffer, since we need to skip
	// the buffers that don't fit the request
	int32 count = 1;

	do {
		status_t status;
		do {
			status = acquire_sem_etc(groupReclaimSem, count, acquireFlags,
				timeout);
		} while (status == B_INTERRUPTED);

		if (status != B_OK)
			return status;

		// try to exit savely if the lock fails
		status = Lock();
		if (status != B_OK) {
			ERROR("SharedBufferList:: RequestBuffer: Lock failed: %s\n",
				strerror(status));
			release_sem_etc(groupReclaimSem, count, 0);
			return B_ERROR;
		}

		for (int32 i = 0; i < fCount; i++) {
			// We need a BBuffer from the group, and it must be marked as
			// reclaimed
			if (fInfos[i].reclaim_sem == groupReclaimSem
				&& fInfos[i].reclaimed) {
				if ((size != 0 && size <= fInfos[i].buffer->SizeAvailable())
					|| (*_buffer != 0 && fInfos[i].buffer == *_buffer)
					|| (wantID != 0 && fInfos[i].id == wantID)) {
				   	// we found a buffer
					fInfos[i].reclaimed = false;
					*_buffer = fInfos[i].buffer;

					// if we requested more than one buffer, release the rest
					if (count > 1) {
						release_sem_etc(groupReclaimSem, count - 1,
							B_DO_NOT_RESCHEDULE);
					}

					// And mark all buffers with the same ID as requested in
					// all other buffer groups
					_RequestBufferInOtherGroups(groupReclaimSem,
						fInfos[i].buffer->ID());

					Unlock();
					return B_OK;
				}
			}
		}

		release_sem_etc(groupReclaimSem, count, B_DO_NOT_RESCHEDULE);
		if (Unlock() != B_OK) {
			ERROR("SharedBufferList:: RequestBuffer: unlock failed\n");
			return B_ERROR;
		}
		// prepare to request one more buffer next time
		count++;
	} while (count <= buffersInGroup);

	ERROR("SharedBufferList:: RequestBuffer: no buffer found\n");
	return B_ERROR;
}


status_t
SharedBufferList::RecycleBuffer(BBuffer* buffer)
{
	CALLED();

	media_buffer_id id = buffer->ID();

	if (Lock() != B_OK)
		return B_ERROR;

	int32 reclaimedCount = 0;

	for (int32 i = 0; i < fCount; i++) {
		// find the buffer id, and reclaim it in all groups it belongs to
		if (fInfos[i].id == id) {
			reclaimedCount++;
			if (fInfos[i].reclaimed) {
				ERROR("SharedBufferList::RecycleBuffer, BBuffer %p, id = %"
					B_PRId32 " already reclaimed\n", buffer, id);
				DEBUG_ONLY(debugger("buffer already reclaimed"));
				continue;
			}
			fInfos[i].reclaimed = true;
			release_sem_etc(fInfos[i].reclaim_sem, 1, B_DO_NOT_RESCHEDULE);
		}
	}

	if (Unlock() != B_OK)
		return B_ERROR;

	if (reclaimedCount == 0) {
		ERROR("shared_buffer_list::RecycleBuffer, BBuffer %p, id = %" B_PRId32
			" NOT reclaimed\n", buffer, id);
		return B_ERROR;
	}

	return B_OK;
}


/*!	Returns exactly \a bufferCount buffers from the group specified via its
	\a groupReclaimSem if successful.
*/
status_t
SharedBufferList::GetBufferList(sem_id groupReclaimSem, int32 bufferCount,
	BBuffer** buffers)
{
	CALLED();

	if (Lock() != B_OK)
		return B_ERROR;

	int32 found = 0;

	for (int32 i = 0; i < fCount; i++)
		if (fInfos[i].reclaim_sem == groupReclaimSem) {
			buffers[found++] = fInfos[i].buffer;
			if (found == bufferCount)
				break;
		}

	if (Unlock() != B_OK)
		return B_ERROR;

	return found == bufferCount ? B_OK : B_ERROR;
}


status_t
SharedBufferList::_Init()
{
	CALLED();

	fSemaphore = create_sem(0, "shared buffer list lock");
	if (fSemaphore < 0)
		return fSemaphore;

	fAtom = 0;

	for (int32 i = 0; i < kMaxBuffers; i++) {
		fInfos[i].id = -1;
	}
	fCount = 0;

	return B_OK;
}


/*!	Used by RequestBuffer, call this one with the list locked!
*/
void
SharedBufferList::_RequestBufferInOtherGroups(sem_id groupReclaimSem,
	media_buffer_id id)
{
	for (int32 i = 0; i < fCount; i++) {
		// find buffers with same id, but belonging to other groups
		if (fInfos[i].id == id && fInfos[i].reclaim_sem != groupReclaimSem) {
			// and mark them as requested
			// TODO: this can deadlock if BBuffers with same media_buffer_id
			// exist in more than one BBufferGroup, and RequestBuffer()
			// is called on both groups (which should not be done).
			status_t status;
			do {
				status = acquire_sem(fInfos[i].reclaim_sem);
			} while (status == B_INTERRUPTED);

			// try to skip entries that belong to crashed teams
			if (status != B_OK)
				continue;

			if (fInfos[i].reclaimed == false) {
				ERROR("SharedBufferList:: RequestBufferInOtherGroups BBuffer "
					"%p, id = %" B_PRId32 " not reclaimed while requesting\n",
					fInfos[i].buffer, id);
				continue;
			}

			fInfos[i].reclaimed = false;
		}
	}
}


}	// namespace BPrivate
