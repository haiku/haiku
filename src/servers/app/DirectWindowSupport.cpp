/*
 * Copyright 2008-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stefano Ceccherini <stefano.ceccherini@gmail.com>
 *		Axel Dörfler, axeld@pinc-software.de
 */

#include "DirectWindowSupport.h"

#include <string.h>

DirectWindowData::DirectWindowData()
	:
	buffer_info(NULL),
	full_screen(false),
	fSem(-1),
	fAcknowledgeSem(-1),
	fBufferArea(-1),
	fTransition(0)
{
	fBufferArea = create_area("direct area", (void**)&buffer_info,
		B_ANY_ADDRESS, DIRECT_BUFFER_INFO_AREA_SIZE,
		B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);

	memset(buffer_info, 0, DIRECT_BUFFER_INFO_AREA_SIZE);
	buffer_info->buffer_state = B_DIRECT_STOP;
	fSem = create_sem(0, "direct sem");
	fAcknowledgeSem = create_sem(0, "direct sem ack");
}


DirectWindowData::~DirectWindowData()
{
	// this should make the client die in case it's still running
	buffer_info->bits = NULL;
	buffer_info->bytes_per_row = 0;

	delete_area(fBufferArea);
	delete_sem(fSem);
	delete_sem(fAcknowledgeSem);
}


status_t
DirectWindowData::InitCheck() const
{
	if (fBufferArea < B_OK)
		return fBufferArea;
	if (fSem < B_OK)
		return fSem;
	if (fAcknowledgeSem < B_OK)
		return fAcknowledgeSem;

	return B_OK;
}


status_t
DirectWindowData::GetSyncData(direct_window_sync_data& data) const
{
	data.area = fBufferArea;
	data.disable_sem = fSem;
	data.disable_sem_ack = fAcknowledgeSem;

	return B_OK;
}


status_t
DirectWindowData::SyncronizeWithClient()
{
	// Releasing this semaphore causes the client to call
	// BDirectWindow::DirectConnected()
	status_t status = release_sem(fSem);
	if (status != B_OK)
		return status;

	// Wait with a timeout of half a second until the client exits
	// from its DirectConnected() implementation
	do {
		status = acquire_sem_etc(fAcknowledgeSem, 1, B_TIMEOUT, 500000);
	} while (status == B_INTERRUPTED);

	return status;
}


bool
DirectWindowData::SetState(const direct_buffer_state& bufferState,
	const direct_driver_state& driverState)
{
	BufferState inputState(bufferState);
	BufferState currentState(buffer_info->buffer_state);
	
	bool handle = false;
		
	if (inputState.Action() == B_DIRECT_STOP)
		handle = _HandleStop(bufferState);
	else if (inputState.Action() == B_DIRECT_START)
		handle = _HandleStart(bufferState);
	else if (inputState.Action() == B_DIRECT_MODIFY)
		handle = _HandleModify(bufferState);
		
	if (driverState != -1)
		buffer_info->driver_state = driverState;

	return handle;
}


bool
DirectWindowData::_HandleStop(const direct_buffer_state& state)
{
	buffer_info->buffer_state = B_DIRECT_STOP;
	if (fTransition-- >= 1)
		return true;
	return false;
}


bool
DirectWindowData::_HandleStart(const direct_buffer_state& state)
{
	buffer_info->buffer_state = (direct_buffer_state)
		(BufferState(buffer_info->buffer_state).Reason() | state);
	if (fTransition++ >= 0)
		return true;
	
	return false;
}


bool
DirectWindowData::_HandleModify(const direct_buffer_state& state)
{
	buffer_info->buffer_state = state;
	if (fTransition > 0)
		return true;
	
	return false;
}
