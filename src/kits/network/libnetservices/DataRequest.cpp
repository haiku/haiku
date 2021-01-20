/*
 * Copyright 2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */


#include "DataRequest.h"

#include <AutoDeleter.h>
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

	// The RFC has examples where some characters are URL-Encoded.
	fUrl.UrlDecode(true);

	// The RFC says this uses a nonstandard scheme, so the path, query and
	// fragment are a bit nonsensical. It would be nice to handle them, but
	// some software (eg. WebKit) relies on data URIs with embedded "#" char
	// in the data...
	BString data = fUrl.UrlString();
	data.Remove(0, 5); // remove "data:"

	int separatorPosition = data.FindFirst(',');

	if (fListener != NULL)
		fListener->ConnectionOpened(this);

	if (separatorPosition >= 0) {
		BString meta = data;
		meta.Truncate(separatorPosition);
		data.Remove(0, separatorPosition + 1);

		int pos = 0;
		while (meta.Length() > 0) {
			// Extract next parameter
			pos = meta.FindFirst(';', pos);

			BString parameter = meta;
			if (pos >= 0) {
				parameter.Truncate(pos);
				meta.Remove(0, pos+1);
			} else
				meta.Truncate(0);

			// Interpret the parameter
			if (parameter == "base64") {
				isBase64 = true;
			} else if (parameter.FindFirst("charset=") == 0) {
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

	ArrayDeleter<char> buffer;
	if (isBase64) {
		// Check that the base64 data is properly padded (we process characters
		// by groups of 4 and there must not be stray chars at the end as
		// Base64 specifies padding.
		if (data.Length() & 3)
			return B_BAD_DATA;

		buffer.SetTo(new char[data.Length() * 3 / 4]);
		payload = buffer.Get();
			// payload must be a const char* so we can assign data.String() to
			// it below, but decode_64 modifies buffer.
		length = decode_base64(buffer.Get(), data.String(), data.Length());

		// There may be some padding at the end of the base64 stream. This
		// prevents us from computing the exact length we should get, so allow
		// for some error margin.
		if (length > data.Length() * 3 / 4
			|| length < data.Length() * 3 / 4 - 3) {
			return B_BAD_DATA;
		}
	} else {
		payload = data.String();
		length = data.Length();
	}

	fResult.SetLength(length);

	if (fListener != NULL) {
		fListener->HeadersReceived(this, fResult);
		if (length > 0) {
			fListener->DataReceived(this, payload, 0, length);
			fListener->DownloadProgress(this, length, length);
		}
	}

	return B_OK;
}
