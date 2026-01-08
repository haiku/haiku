/*
 * Copyright 2019-2026, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "Captcha.h"

#include <DataIO.h>

#include "Logger.h"

// These are keys that are used to store this object's data into a BMessage
// instance.

static const char* const kKeyToken = "token";
static const char* const kKeyPngImageData = "png_image_data";


Captcha::Captcha(BMessage* from)
	:
	fToken(""),
	fPngImageData(NULL)
{
	if (from->FindString(kKeyToken, &fToken) != B_OK)
		HDERROR("expected key [%s] in the message data when creating a captcha", kKeyToken);

	const void* data;
	ssize_t len;

	if (from->FindData(kKeyPngImageData, B_ANY_TYPE, &data, &len) != B_OK)
		HDERROR("expected key [%s] in the message data", kKeyPngImageData);
	else
		SetPngImageData(data, len);
}


Captcha::Captcha()
	:
	fToken(""),
	fPngImageData(NULL)
{
}


Captcha::~Captcha()
{
	delete fPngImageData;
}


const BString&
Captcha::Token() const
{
	return fToken;
}


BPositionIO*
Captcha::PngImageData() const
{
	return fPngImageData;
}


void
Captcha::SetToken(const BString& value)
{
	fToken = value;
}


void
Captcha::SetPngImageData(const void* data, size_t len)
{
	if (fPngImageData != NULL)
		delete fPngImageData;
	fPngImageData = NULL;
	if (data != NULL) {
		fPngImageData = new BMallocIO();
		fPngImageData->Write(data, len);
	}
}


status_t
Captcha::Archive(BMessage* into, bool deep) const
{
	status_t result = B_OK;
	if (result == B_OK && into == NULL)
		result = B_ERROR;
	if (result == B_OK)
		result = into->AddString(kKeyToken, fToken);
	if (result == B_OK && fPngImageData != NULL) {
		result = into->AddData(kKeyPngImageData, B_ANY_TYPE, fPngImageData->Buffer(),
			fPngImageData->BufferLength());
	}
	return result;
}