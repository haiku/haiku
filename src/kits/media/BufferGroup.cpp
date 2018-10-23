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

#include "MediaDebug.h"
#include "DataExchange.h"
#include "SharedBufferList.h"


BBufferGroup::BBufferGroup(size_t size, int32 count, uint32 placement,
	uint32 lock)
{
	CALLED();
	fInitError = _Init();
	if (fInitError != B_OK)
		return;

	// This one is easy. We need to create "count" BBuffers,
	// each one "size" bytes large. They all go into one
	// area, with "placement" and "lock" attributes.
	// The BBuffers created will clone the area, and
	// then we delete our area. This way BBuffers are
	// independent from the BBufferGroup

	// don't allow all placement parameter values
	if (placement != B_ANY_ADDRESS && placement != B_ANY_KERNEL_ADDRESS) {
		ERROR("BBufferGroup: placement != B_ANY_ADDRESS "
			"&& placement != B_ANY_KERNEL_ADDRESS (0x%#" B_PRIx32 ")\n",
			placement);
		placement = B_ANY_ADDRESS;
	}

	// first we roundup for a better placement in memory
	size_t allocSize = (size + 63) & ~63;

	// now we create the area
	size_t areaSize
		= ((allocSize * count) + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);

	void* startAddress;
	area_id bufferArea = create_area("some buffers area", &startAddress,
		placement, areaSize, lock, B_READ_AREA | B_WRITE_AREA);
	if (bufferArea < 0) {
		ERROR("BBufferGroup: failed to allocate %ld bytes area\n", areaSize);
		fInitError = (status_t)bufferArea;
		return;
	}

	buffer_clone_info info;

	for (int32 i = 0; i < count; i++) {
		info.area = bufferArea;
		info.offset = i * allocSize;
		info.size = size;

		fInitError = AddBuffer(info);
		if (fInitError != B_OK)
			break;
	}

	delete_area(bufferArea);
}


BBufferGroup::BBufferGroup()
{
	CALLED();
	fInitError = _Init();
	if (fInitError != B_OK)
		return;

	// this one simply creates an empty BBufferGroup
}


BBufferGroup::BBufferGroup(int32 count, const media_buffer_id* buffers)
{
	CALLED();
	fInitError = _Init();
	if (fInitError != B_OK)
		return;

	// This one creates "BBuffer"s from "media_buffer_id"s passed
	// by the application.

	buffer_clone_info info;

	for (int32 i = 0; i < count; i++) {
		info.buffer = buffers[i];

		fInitError = AddBuffer(info);
		if (fInitError != B_OK)
			break;
	}
}


BBufferGroup::~BBufferGroup()
{
	CALLED();
	if (fBufferList != NULL)
		fBufferList->DeleteGroupAndPut(fReclaimSem);

	delete_sem(fReclaimSem);
}


status_t
BBufferGroup::InitCheck()
{
	CALLED();
	return fInitError;
}


status_t
BBufferGroup::AddBuffer(const buffer_clone_info& info, BBuffer** _buffer)
{
	CALLED();
	if (fInitError != B_OK)
		return B_NO_INIT;

	status_t status = fBufferList->AddBuffer(fReclaimSem, info, _buffer);
	if (status != B_OK) {
		ERROR("BBufferGroup: error when adding buffer\n");
		return status;
	}
	atomic_add(&fBufferCount, 1);
	return B_OK;
}


BBuffer*
BBufferGroup::RequestBuffer(size_t size, bigtime_t timeout)
{
	CALLED();
	if (fInitError != B_OK)
		return NULL;

	if (size <= 0)
		return NULL;

	BBuffer *buffer = NULL;
	fRequestError = fBufferList->RequestBuffer(fReclaimSem, fBufferCount,
		size, 0, &buffer, timeout);

	return fRequestError == B_OK ? buffer : NULL;
}


status_t
BBufferGroup::RequestBuffer(BBuffer* buffer, bigtime_t timeout)
{
	CALLED();
	if (fInitError != B_OK)
		return B_NO_INIT;

	if (buffer == NULL)
		return B_BAD_VALUE;

	fRequestError = fBufferList->RequestBuffer(fReclaimSem, fBufferCount, 0, 0,
		&buffer, timeout);

	return fRequestError;
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
BBufferGroup::CountBuffers(int32* _count)
{
	CALLED();
	if (fInitError != B_OK)
		return B_NO_INIT;

	*_count = fBufferCount;
	return B_OK;
}


status_t
BBufferGroup::GetBufferList(int32 bufferCount, BBuffer** _buffers)
{
	CALLED();
	if (fInitError != B_OK)
		return B_NO_INIT;

	if (bufferCount <= 0 || bufferCount > fBufferCount)
		return B_BAD_VALUE;

	return fBufferList->GetBufferList(fReclaimSem, bufferCount, _buffers);
}


status_t
BBufferGroup::WaitForBuffers()
{
	CALLED();
	if (fInitError != B_OK)
		return B_NO_INIT;

	// TODO: this function is not really useful anyway, and will
	// not work exactly as documented, but it is close enough

	if (fBufferCount < 0)
		return B_BAD_VALUE;
	if (fBufferCount == 0)
		return B_OK;

	// We need to wait until at least one buffer belonging to this group is
	// reclaimed.
	// This has happened when can aquire "fReclaimSem"

	status_t status;
	while ((status = acquire_sem(fReclaimSem)) == B_INTERRUPTED)
		;
	if (status != B_OK)
		return status;

	// we need to release the "fReclaimSem" now, else we would block
	// requesting of new buffers

	return release_sem(fReclaimSem);
}


status_t
BBufferGroup::ReclaimAllBuffers()
{
	CALLED();
	if (fInitError != B_OK)
		return B_NO_INIT;

	// because additional BBuffers might get added to this group betweeen
	// acquire and release
	int32 count = fBufferCount;

	if (count < 0)
		return B_BAD_VALUE;
	if (count == 0)
		return B_OK;

	// we need to wait until all BBuffers belonging to this group are reclaimed.
	// this has happened when the "fReclaimSem" can be aquired "fBufferCount"
	// times

	status_t status = B_ERROR;
	do {
		status = acquire_sem_etc(fReclaimSem, count, B_RELATIVE_TIMEOUT, 0);
	} while (status == B_INTERRUPTED);

	if (status != B_OK)
		return status;

	// we need to release the "fReclaimSem" now, else we would block
	// requesting of new buffers

	return release_sem_etc(fReclaimSem, count, 0);
}


//	#pragma mark - deprecated BeOS R4 API


status_t
BBufferGroup::AddBuffersTo(BMessage* message, const char* name, bool needLock)
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

	BBuffer* buffers[fBufferCount];
	status_t status = GetBufferList(fBufferCount, buffers);
	if (status != B_OK)
		return status;

	for (int32 i = 0; i < fBufferCount; i++) {
		status = message->AddInt32(name, int32(buffers[i]->ID()));
		if (status != B_OK)
			return status;
	}

	return B_OK;
}


//	#pragma mark - private methods


/* not implemented */
//BBufferGroup::BBufferGroup(const BBufferGroup &)
//BBufferGroup & BBufferGroup::operator=(const BBufferGroup &)


status_t
BBufferGroup::_Init()
{
	CALLED();

	// some defaults in case we drop out early
	fBufferList = NULL;
	fRequestError = B_ERROR;
	fBufferCount = 0;

	// Create the reclaim semaphore
	// This is also used as a system wide unique identifier for this group
	fReclaimSem = create_sem(0, "buffer reclaim sem");
	if (fReclaimSem < 0) {
		ERROR("BBufferGroup::InitBufferGroup: couldn't create fReclaimSem\n");
		return (status_t)fReclaimSem;
	}

	fBufferList = BPrivate::SharedBufferList::Get();
	if (fBufferList == NULL) {
		ERROR("BBufferGroup::InitBufferGroup: SharedBufferList::Get() "
			"failed\n");
		return B_ERROR;
	}

	return B_OK;
}

