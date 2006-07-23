/*
 * Copyright 2003-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels S. Reedijk
 */

#include "usb_p.h"


Transfer::Transfer(Pipe *pipe, bool synchronous)
{
	fPipe = pipe;
	fBuffer = NULL;
	fBufferLength = 0;
	fActualLength = NULL;
	fStatus = B_NO_INIT;
	fSem = -1;
	fHostPrivate = NULL;
	fCallback = NULL;

	if (synchronous) {
		fSem = create_sem(0, "USB Transfer");
		set_sem_owner(fSem, B_SYSTEM_TEAM);
	}
}


Transfer::~Transfer()
{
	if (fSem >= B_OK)
		delete_sem(fSem);
}


void
Transfer::SetRequestData(usb_request_data *data)
{
	fRequest = data;
}


void
Transfer::SetBuffer(uint8 *buffer)
{
	fBuffer = buffer;
}


void
Transfer::SetBufferLength(size_t length)
{
	fBufferLength = length;
}


void
Transfer::SetActualLength(size_t *actualLength)
{
	fActualLength = actualLength;
}


void
Transfer::SetCallbackFunction(usb_callback_func callback, void *cookie)
{
	fCallback = callback;
	fCallbackCookie = cookie;
}


void
Transfer::SetHostPrivate(hostcontroller_priv *priv)
{
	fHostPrivate = priv;
}


status_t
Transfer::WaitForFinish()
{
	if (fSem < B_OK)
		return fStatus;

	status_t result = B_OK;
	result = acquire_sem(fSem);

	if (result < B_OK)
		return result;

	return fStatus;
}


void
Transfer::Finished(status_t result)
{
	fStatus = result;

	// If we are synchronous, release the sem
	if (fSem > B_OK)
		release_sem(fSem);

	// ToDo: implement callback correctly
	if (fCallback)
		fCallback(fCallbackCookie, fStatus, fBuffer, 0);
}
