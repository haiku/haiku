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
	fTimeout = 0;
	fStatus = B_ERROR;
	fSem = -1;
	fHostPrivate = NULL;

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
Transfer::SetCallbackFunction(usb_callback_func callback)
{
	fCallback = callback;
}


void
Transfer::SetHostPrivate(hostcontroller_priv *priv)
{
	fHostPrivate = priv;
}


void
Transfer::WaitForFinish()
{
	if (fSem > B_OK)
		acquire_sem(fSem);
	// ToDo: and otherwise?
}


void
Transfer::TransferDone()
{
	if (fSem > B_OK)
		release_sem(fSem);
}


void Transfer::Finish()
{
	// If we are synchronous, release a sem
	if (fSem > B_OK)
		release_sem(fSem);
}
