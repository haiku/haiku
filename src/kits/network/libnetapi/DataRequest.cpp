/*
 * Copyright 2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */


#include "DataRequest.h"

#include <HttpAuthentication.h>
#include <mail_encoding.h>
#include <stdio.h>


BDataRequest::BDataRequest(const BUrl& url, BUrlProtocolListener* listener,
		BUrlContext* context)
	: BUrlRequest(url, listener, context, "data URL parser", "data"),
	fResult()
{
	fResult.SetContentType("text/plain");
}


const BUrlResult&
BDataRequest::Result() const
{
	return fResult;
}


status_t
BDataRequest::_ProtocolLoop()
{
	BString mimeType;
	BString charset;
	const char* payload;
	ssize_t length;
	bool isBase64 = false;

	fUrl.UrlDecode(true);
	BString data = fUrl.Path();
	int separatorPosition = data.FindFirst(',');

	if (fListener != NULL)
		fListener->ConnectionOpened(this);

	if (separatorPosition >= 0) {
		BString meta = data;
		meta.Truncate(separatorPosition);
		data.Remove(0, separatorPosition + 1);

		int pos = 0;
		while(meta.Length() > 0)
		{
			// Extract next parameter
			pos = meta.FindFirst(';', pos);

			BString parameter = meta;
			if(pos >= 0) {
				parameter.Truncate(pos);
				meta.Remove(0, pos+1);
			} else
				meta.Truncate(0);

			// Interpret the parameter
			if(parameter == "base64") {
				isBase64 = true;
			} else if(parameter.FindFirst("charset=") == 0) {
				charset = parameter;
			} else {
				// Must be the MIME type
				mimeType = parameter;
			}
		}

		if (charset.Length() > 0)
			mimeType << ";" << charset;
		fResult.SetContentType(mimeType);
	}

	if (isBase64) {
		char* buffer = new char[data.Length() * 3 / 4];
		payload = buffer;
			// payload must be a const char* so we can assign data.String() to
			// it below, but decode_64 modifies buffer.
		length = decode_base64(buffer, data.String(), data.Length());

		// There may be some padding at the end of the base64 stream. This
		// prevents us from computing the exact length we should get, so allow
		// for some error margin.
		if(length > data.Length() * 3 / 4
			|| length < data.Length() * 3 / 4 - 3) {
			delete[] buffer;
			return B_BAD_DATA;
		}
	} else {
		payload = data.String();
		length = data.Length();
	}

	fResult.SetLength(length);

	if (fListener != NULL) {
		fListener->DownloadProgress(this, length, length);
		if (length > 0)
			fListener->DataReceived(this, payload, 0, length);
	}

	if (isBase64)
		delete[] payload;

	return B_OK;
}
