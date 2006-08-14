/*
 * Copyright 2003-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels S. Reedijk
 */

#include "usb_p.h"


Transfer::Transfer(Pipe *pipe)
	:	fPipe(pipe),
		fData(NULL),
		fDataLength(0),
		fCallback(NULL),
		fCallbackCookie(NULL),
		fRequestData(NULL)
{
}


Transfer::~Transfer()
{
	// we take ownership of the request data
	if (fRequestData)
		delete fRequestData;
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
Transfer::SetCallback(usb_callback_func callback, void *cookie)
{
	fCallback = callback;
	fCallbackCookie = cookie;
}


void
Transfer::Finished(uint32 status, size_t actualLength)
{
	if (fCallback)
		fCallback(fCallbackCookie, status, fData, actualLength);
}
