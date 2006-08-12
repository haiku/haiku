/*
 * Copyright 2003-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels S. Reedijk
 */

#include "usb_p.h"


Transfer::Transfer(Pipe *pipe, bool synchronous)
	:	fPipe(pipe),
		fData(NULL),
		fDataLength(0),
		fActualLength(NULL),
		fOwnActualLength(0),
		fStatus(B_NO_INIT),
		fCallback(NULL),
		fCallbackCookie(NULL),
		fSem(-1),
		fHostPrivate(NULL),
		fRequestData(NULL)
{
	fActualLength = &fOwnActualLength;
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
	fRequestData = data;
}


void
Transfer::SetData(uint8 *data, size_t dataLength)
{
	fData = data;
	fDataLength = dataLength;
}


void
Transfer::SetActualLength(size_t *actualLength)
{
	fActualLength = actualLength;
}


void
Transfer::SetCallback(usb_callback_func callback, void *cookie)
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

	// Call the callback function ...
	if (fCallback) {
		fCallback(fCallbackCookie, fStatus, fData, *fActualLength);
		return;
	}

	// ... or release the sem
	if (fSem > B_OK) {
		release_sem(fSem);
		return;
	}
}
