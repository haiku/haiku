/*
 * Copyright (c) 2002, 2003 Marcus Overhagen <Marcus@Overhagen.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files or portions
 * thereof (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice
 *    in the  binary, as well as this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided with
 *    the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <BufferGroup.h>
#include <Buffer.h>
#include "debug.h"
#include "SharedBufferList.h"
#include "DataExchange.h"

/*************************************************************
 * private BBufferGroup
 *************************************************************/

status_t				
BBufferGroup::InitBufferGroup()
{
	CALLED();
	
	// some defaults
	fBufferList = 0;
	fReclaimSem = B_ERROR;
	fInitError = B_ERROR;
	fRequestError = B_ERROR;
	fBufferCount = 0;

	// create the reclaim semaphore
	fReclaimSem = create_sem(0,"buffer reclaim sem");
	if (fReclaimSem < B_OK) {
		ERROR("BBufferGroup::InitBufferGroup: couldn't create fReclaimSem\n");
		fInitError = (status_t)fReclaimSem;
		return fInitError;
	}

	// ask media_server to get the area_id of the shared buffer list
	server_get_shared_buffer_area_request area_request;
	server_get_shared_buffer_area_reply area_reply;
	if (QueryServer(SERVER_GET_SHARED_BUFFER_AREA, &area_request, sizeof(area_request), &area_reply, sizeof(area_reply)) != B_OK) {
		ERROR("BBufferGroup::InitBufferGroup: SERVER_GET_SHARED_BUFFER_AREA failed\n");
		fInitError = B_ERROR;
		return fInitError;
	}
	ASSERT(area_reply.area > 0);

	fBufferList = _shared_buffer_list::Clone(area_reply.area);
	if (fBufferList == NULL) {
		ERROR("BBufferGroup::InitBufferGroup: _shared_buffer_list::Clone failed\n");
		fInitError = B_ERROR;
		return fInitError;
	}

	fInitError = B_OK;
	return fInitError;
}


/*************************************************************
 * public BBufferGroup
 *************************************************************/

BBufferGroup::BBufferGroup(size_t size,
						   int32 count,
						   uint32 placement,
						   uint32 lock)
{
	CALLED();
	if (InitBufferGroup() != B_OK)
		return;
		
	// This one is easy. We need to create "count" BBuffers,
	// each one "size" bytes large. They all go into one 
	// area, with "placement" and "lock" attributes.
	// The BBuffers created will clone the area, and
	// then we delete our area. This way BBuffers are
	// independent from the BBufferGroup

	void *start_addr;
	area_id buffer_area;
	size_t area_size;
	buffer_clone_info bci;
	BBuffer *buffer;

	// don't allow all placement parameter values
	if (placement != B_ANY_ADDRESS && placement != B_ANY_KERNEL_ADDRESS) {
		ERROR("BBufferGroup: placement != B_ANY_ADDRESS && placement != B_ANY_KERNEL_ADDRESS (0x%08lx)\n",placement);
		placement = B_ANY_ADDRESS;
	}
	
	// first we roundup for a better placement in memory
	int allocsize = (size + 63) & ~63;

	// now we create the area
	area_size = ((allocsize * count) + (B_PAGE_SIZE - 1)) & ~(B_PAGE_SIZE - 1);

	buffer_area = create_area("some buffers area", &start_addr,placement,area_size,lock,B_READ_AREA | B_WRITE_AREA);
	if (buffer_area < B_OK) {
		ERROR("BBufferGroup: failed to allocate %ld bytes area\n",area_size);
		fInitError = (status_t)buffer_area;
		return;
	}
	
	fBufferCount = count;

	for (int32 i = 0; i < count; i++) {	
		bci.area = buffer_area;
		bci.offset = i * allocsize;
		bci.size = size;
		buffer = new BBuffer(bci);
		if (0 == buffer->Data()) {
			// BBuffer::Data() will return 0 if an error occured
			ERROR("BBufferGroup: error while creating buffer\n");
			delete buffer;
			fInitError = B_ERROR;
			break;
		}
		if (B_OK != fBufferList->AddBuffer(fReclaimSem,buffer)) {
			ERROR("BBufferGroup: error when adding buffer\n");
			delete buffer;
			fInitError = B_ERROR;
			break;
		}
	}

	delete_area(buffer_area);
}

/* explicit */
BBufferGroup::BBufferGroup()
{
	CALLED();
	if (InitBufferGroup() != B_OK)
		return;
		
	// this one simply creates an empty BBufferGroup
}


BBufferGroup::BBufferGroup(int32 count,
						   const media_buffer_id *buffers)
{
	CALLED();
	if (InitBufferGroup() != B_OK)
		return;
		
	// XXX we need to make sure that a media_buffer_id is only added once to each group

	// this one creates "BBuffer"s from "media_buffer_id"s passed 
	// by the application.

	fBufferCount = count;

	buffer_clone_info bci;
	BBuffer *buffer;
	for (int32 i = 0; i < count; i++) {	
		bci.buffer = buffers[i];
		buffer = new BBuffer(bci);
		if (0 == buffer->Data()) {
			// BBuffer::Data() will return 0 if an error occured
			ERROR("BBufferGroup(2): error while creating buffer\n");
			delete buffer;
			fInitError = B_ERROR;
			break;
		}
		if (B_OK != fBufferList->AddBuffer(fReclaimSem,buffer)) {
			ERROR("BBufferGroup(2): error when adding buffer\n");
			delete buffer;
			fInitError = B_ERROR;
			break;
		}
	}
}


BBufferGroup::~BBufferGroup()
{
	CALLED();
	if (fBufferList)
		fBufferList->Terminate(fReclaimSem);
	if (fReclaimSem >= B_OK)
		delete_sem(fReclaimSem);
}


status_t
BBufferGroup::InitCheck()
{
	CALLED();
	return fInitError;
}


status_t
BBufferGroup::AddBuffer(const buffer_clone_info &info,
						BBuffer **out_buffer)
{
	CALLED();
	if (fInitError != B_OK)
		return B_NO_INIT;

	// XXX we need to make sure that a media_buffer_id is only added once to each group

	BBuffer *buffer;	
	buffer = new BBuffer(info);
	if (0 == buffer->Data()) {
		// BBuffer::Data() will return 0 if an error occured
		ERROR("BBufferGroup::AddBuffer: error while creating buffer\n");
		delete buffer;
		return B_ERROR;
	}
	if (B_OK != fBufferList->AddBuffer(fReclaimSem,buffer)) {
		ERROR("BBufferGroup::AddBuffer: error when adding buffer\n");
		delete buffer;
		fInitError = B_ERROR;
		return B_ERROR;
	}
	atomic_add(&fBufferCount,1);
	
	if (out_buffer != 0)
		*out_buffer = buffer;

	return B_OK;
}


BBuffer *
BBufferGroup::RequestBuffer(size_t size,
							bigtime_t timeout)
{
	CALLED();
	if (fInitError != B_OK)
		return NULL;

	if (size <= 0)
		return NULL;
		
	BBuffer *buffer;
	status_t status;

	buffer = NULL;
	status = fBufferList->RequestBuffer(fReclaimSem, fBufferCount, size, 0, &buffer, timeout);
	fRequestError = status;
		
	return (status == B_OK) ? buffer : NULL;
}


status_t
BBufferGroup::RequestBuffer(BBuffer *buffer,
							bigtime_t timeout)
{
	CALLED();
	if (fInitError != B_OK)
		return B_NO_INIT;
	
	if (buffer == NULL)
		return B_BAD_VALUE;

	status_t status;
	status = fBufferList->RequestBuffer(fReclaimSem, fBufferCount, 0, 0, &buffer, timeout);
	fRequestError = status;
		
	return status;
}


status_t
BBufferGroup::RequestError()
{
	CALLED();
	if (fInitError != B_OK)
		return B_NO_INIT;

	return fRequestError;
}


status_t
BBufferGroup::CountBuffers(int32 *out_count)
{
	CALLED();
	if (fInitError != B_OK)
		return B_NO_INIT;

	*out_count = fBufferCount;
	return B_OK;
}


status_t
BBufferGroup::GetBufferList(int32 buf_count,
							BBuffer **out_buffers)
{
	CALLED();
	if (fInitError != B_OK)
		return B_NO_INIT;
		
	if (buf_count <= 0 || buf_count > fBufferCount)
		return B_BAD_VALUE;

	return fBufferList->GetBufferList(fReclaimSem,buf_count,out_buffers);
}


status_t
BBufferGroup::WaitForBuffers()
{
	CALLED();
	if (fInitError != B_OK)
		return B_NO_INIT;

	// XXX this function is not really useful anyway, and will 
	// XXX not work exactly as documented, but it is close enough

	if (fBufferCount < 0)
		return B_BAD_VALUE;
	if (fBufferCount == 0)
		return B_OK;

	// we need to wait until at least one buffer belonging to this group is reclaimed.
	// this has happened when can aquire "fReclaimSem"

	status_t status;
	while (B_INTERRUPTED == (status = acquire_sem(fReclaimSem)))
		;
	
	if (status != B_OK) // some error happened
		return status;
		
	// we need to release the "fReclaimSem" now, else we would block requesting of new buffers

	return release_sem(fReclaimSem);
}


status_t
BBufferGroup::ReclaimAllBuffers()
{
	CALLED();
	if (fInitError != B_OK)
		return B_NO_INIT;

	// because additional BBuffers might get added to this group betweeen acquire and release		
	int32 count = fBufferCount;

	if (count < 0)
		return B_BAD_VALUE;
	if (count == 0)
		return B_OK;

	// we need to wait until all BBuffers belonging to this group are reclaimed.
	// this has happened when the "fReclaimSem" can be aquired "fBufferCount" times

	status_t status;
	while (B_INTERRUPTED == (status = acquire_sem_etc(fReclaimSem, count, 0, 0)))
		;
	
	if (status != B_OK) // some error happened
		return status;
		
	// we need to release the "fReclaimSem" now, else we would block requesting of new buffers

	return release_sem_etc(fReclaimSem, count, 0);
}

/*************************************************************
 * private BBufferGroup
 *************************************************************/

/* not implemented */
/*
BBufferGroup::BBufferGroup(const BBufferGroup &)
BBufferGroup & BBufferGroup::operator=(const BBufferGroup &)
*/

status_t
BBufferGroup::AddBuffersTo(BMessage * message, const char * name, bool needLock)
{
	CALLED();
	if (fInitError != B_OK)
		return B_NO_INIT;
	
	// BeOS R4 legacy API. Implemented as a wrapper around GetBufferList
	// "needLock" is ignored, GetBufferList will do locking

	if (message == NULL)
		return B_BAD_VALUE;

	if (name == NULL || strlen(name) == 0)
		return B_BAD_VALUE;
	
	BBuffer ** buffers;
	status_t status;
	int32 count;
	
	count = fBufferCount;
	buffers = new BBuffer * [count];
	
	if (B_OK != (status = GetBufferList(count,buffers)))
		goto end;
		
	for (int32 i = 0; i < count; i++)
		if (B_OK != (status = message->AddInt32(name,int32(buffers[i]->ID()))))
			goto end;
	
end:
	delete [] buffers;
	return status;
}

