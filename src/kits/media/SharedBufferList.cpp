/***********************************************************************
 * Copyright (c) 2002 Marcus Overhagen. All Rights Reserved.
 * This file may be used under the terms of the OpenBeOS License.
 *
 * Used for BBufferGroup and BBuffer management across teams
 ***********************************************************************/
#include <Buffer.h>
#include "SharedBufferList.h"
#include "debug.h"


status_t
_shared_buffer_list::Init()
{
	CALLED();
	locker_atom = 0;
	locker_sem = create_sem(0,"shared buffer list lock");
	if (locker_sem < B_OK)
		return (status_t) locker_sem;

	for (int i = 0; i < MAX_BUFFER; i++) {
		info[i].id = -1;
		info[i].buffer = 0;
		info[i].reclaim_sem = 0;
		info[i].reclaimed = false;
	}
	return B_OK;
}

_shared_buffer_list *
_shared_buffer_list::Clone(area_id id)
{
	CALLED();
	// if id == -1, we are in the media_server team, 
	// and create the initial list, else we clone it

	_shared_buffer_list *adr;
	status_t status;

	if (id == -1) {
		size_t size = ((sizeof(_shared_buffer_list)) + (B_PAGE_SIZE - 1)) & ~(B_PAGE_SIZE - 1);
		status = create_area("shared buffer list",(void **)&adr,B_ANY_KERNEL_ADDRESS,size,B_LAZY_LOCK,B_READ_AREA | B_WRITE_AREA);
		if (status >= B_OK) {
			status = adr->Init();
			if (status != B_OK)
				delete_area(area_for(adr));
		}
	} else {
		status = clone_area("shared buffer list clone",(void **)&adr,B_ANY_KERNEL_ADDRESS,B_READ_AREA | B_WRITE_AREA,id);
		//TRACE("cloned area, id = 0x%08lx, ptr = 0x%08x\n",status,(int)adr);
	}
	
	return (status < B_OK) ? NULL : adr;
}

void
_shared_buffer_list::Unmap()
{
	// unmap the memory used by this struct
	// XXX is this save?
	area_id id;
	id = area_for(this);
	if (id >= B_OK)
		delete_area(id);
}

void
_shared_buffer_list::Terminate(sem_id group_reclaim_sem)
{
	CALLED();

	// delete all BBuffers of this group, then unmap from memory

	if (Lock() != B_OK) { // better not try to access the list unlocked
		// but at least try to unmap the memory
		Unmap();
		return;
	}

	for (int32 i = 0; i < buffercount; i++) {
		if (info[i].reclaim_sem == group_reclaim_sem) {
			// delete the associated buffer
			delete info[i].buffer;
			// decrement buffer count by one
			buffercount--;
			// fill the gap in the list with the last entry
			if (buffercount > 0) {
				info[i] = info[buffercount];
				i--; // make sure we check this entry again
			}
		}
	}
	
	Unlock();
	
	Unmap();
}

status_t
_shared_buffer_list::Lock()
{ 
	if (atomic_add(&locker_atom, 1) > 0) {
		status_t status;
		while (B_INTERRUPTED == (status = acquire_sem(locker_sem)))
			;
		return status; // will only return != B_OK if the media_server crashed or quit
	}
	return B_OK;
}

status_t
_shared_buffer_list::Unlock()
{ 
	if (atomic_add(&locker_atom, -1) > 1)
		return release_sem(locker_sem); // will only return != B_OK if the media_server crashed or quit
	return B_OK;
}

status_t
_shared_buffer_list::AddBuffer(sem_id group_reclaim_sem, BBuffer *buffer)
{
	CALLED();
	
	if (buffer == NULL)
		return B_BAD_VALUE;

	if (Lock() != B_OK)
		return B_ERROR;
	
	if (buffercount == MAX_BUFFER) {
		Unlock();
		debugger("we are doomed");
		return B_ERROR;
	}

	info[buffercount].id = buffer->ID();
	info[buffercount].buffer = buffer;
	info[buffercount].reclaim_sem = group_reclaim_sem;
	info[buffercount].reclaimed = true;
	buffercount++;

	status_t status1 = release_sem_etc(group_reclaim_sem,1,B_DO_NOT_RESCHEDULE);
	status_t status2 = Unlock();

	return (status1 == B_OK && status2 == B_OK) ? B_OK : B_ERROR;
}

status_t	
_shared_buffer_list::RequestBuffer(sem_id group_reclaim_sem, int32 buffers_in_group, size_t size, media_buffer_id wantID, BBuffer **buffer, bigtime_t timeout)
{
	CALLED();
	// we always search for a buffer from the group indicated by group_reclaim_sem first
	// if "size" != 0, we search for a buffer that is "size" bytes or larger
	// if "wantID" != 0, we search for a buffer with this id
	// if "*buffer" != NULL, we search for a buffer at this address
	// if we found a buffer, we also need to mark it in all other groups as requested
	// and also once need to acquire the reclaim_sem of the other groups
	
	status_t status;
	uint32 acquire_flags;
	int32 count;

	if (timeout <= 0) {
		timeout = 0;
		acquire_flags = B_RELATIVE_TIMEOUT;
	} else if (timeout != B_INFINITE_TIMEOUT) {
		timeout += system_time();
		acquire_flags = B_ABSOLUTE_TIMEOUT;
	} else {
	 	//timeout is B_INFINITE_TIMEOUT
		acquire_flags = B_RELATIVE_TIMEOUT;
	}
		
	// with each itaration we request one more buffer, since we need to skip the buffers that don't fit the request
	count = 1;
	
	do {
		while (B_INTERRUPTED == (status = acquire_sem_etc(group_reclaim_sem, count, acquire_flags, timeout)))
			;
		if (status != B_OK)
			return status;

		// try to exit savely if the lock fails			
		if (Lock() != B_OK) {
			release_sem_etc(group_reclaim_sem, count, 0);
			return B_ERROR;
		}
		
		for (int32 i = 0; i < buffercount; i++) {
			// we need a BBuffer from the group, and it must be marked as reclaimed
			if (info[i].reclaim_sem == group_reclaim_sem && info[i].reclaimed) {
				if (
					  (size != 0 && size <= info[i].buffer->SizeAvailable()) ||
					  (*buffer != 0 && info[i].buffer == *buffer) ||
					  (wantID != 0 && info[i].id == wantID)
				   ) {
				   	// we found a buffer
					info[i].reclaimed = false;
					*buffer = info[i].buffer;
					// if we requested more than one buffer, release the rest
					if (count > 1)
						release_sem_etc(group_reclaim_sem, count - 1, B_DO_NOT_RESCHEDULE);
					
					// and mark all buffers with the same ID as requested in all other buffer groups
					RequestBufferInOtherGroups(group_reclaim_sem, info[i].buffer->ID());

					Unlock();
					return B_OK;
				}
			}
		}

		release_sem_etc(group_reclaim_sem, count, B_DO_NOT_RESCHEDULE);
		if (Unlock() != B_OK)
			return B_ERROR;

		// prepare to request one more buffer next time
		count++;
	} while (count <= buffers_in_group);

	return B_ERROR;
}

void 
_shared_buffer_list::RequestBufferInOtherGroups(sem_id group_reclaim_sem, media_buffer_id id)
{
	for (int32 i = 0; i < buffercount; i++) {
		// find buffers with same id, but belonging to other groups
		if (info[i].id == id && info[i].reclaim_sem != group_reclaim_sem) {

			// and mark them as requested 
			// XXX this can deadlock if BBuffers with same media_buffer_id
			// XXX exist in more than one BBufferGroup, and RequestBuffer()
			// XXX is called on both groups (which should not be done).
			status_t status;
			while (B_INTERRUPTED == (status = acquire_sem(info[i].reclaim_sem)))
				;
			// try to skip entries that belong to crashed teams
			if (status != B_OK)
				continue;

			if (info[i].reclaimed == false) {
				TRACE("Error, BBuffer 0x%08x, id = 0x%08x not reclaimed while requesting\n",(int)info[i].buffer,(int)id);
				continue;
			}
			
			info[i].reclaimed = false;
		}
	}
}

status_t
_shared_buffer_list::RecycleBuffer(BBuffer *buffer)
{
	CALLED();
	
	int reclaimed_count;
	
	media_buffer_id id = buffer->ID();

	if (Lock() != B_OK)
		return B_ERROR;

	reclaimed_count = 0;	
	for (int32 i = 0; i < buffercount; i++) {
		// find the buffer id, and reclaim it in all groups it belongs to
		if (info[i].id == id) {
			reclaimed_count++;
			if (info[i].reclaimed) {
				TRACE("Error, BBuffer 0x%08x, id = 0x%08x already reclaimed\n",(int)buffer,(int)id);
				continue;
			}
			info[i].reclaimed = true;
			release_sem_etc(info[i].reclaim_sem, 1, B_DO_NOT_RESCHEDULE);
		}
	}
	if (Unlock() != B_OK)
		return B_ERROR;
	
	if (reclaimed_count == 0) {
		TRACE("Error, BBuffer 0x%08x, id = 0x%08x NOT reclaimed\n",(int)buffer,(int)id);
		return B_ERROR;
	}

	return B_OK;
}

status_t	
_shared_buffer_list::GetBufferList(sem_id group_reclaim_sem, int32 buf_count, BBuffer **out_buffers)
{
	CALLED();

	int32 found;
	
	found = 0;

	if (Lock() != B_OK)
		return B_ERROR;

	for (int32 i = 0; i < buffercount; i++)
		if (info[i].reclaim_sem == group_reclaim_sem) {
			out_buffers[found++] = info[i].buffer;
			if (found == buf_count)
				break;
		}
	
	if (Unlock() != B_OK)
		return B_ERROR;

	return (found == buf_count) ? B_OK : B_ERROR;
}

