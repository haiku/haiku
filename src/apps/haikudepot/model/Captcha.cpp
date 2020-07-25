/*
 * Copyright 2019-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "Captcha.h"

#include <DataIO.h>

#include "Logger.h"

// These are keys that are used to store this object's data into a BMessage
// instance.

#define KEY_TOKEN			"token"
#define KEY_PNG_IMAGE_DATA	"pngImageData"


Captcha::Captcha(BMessage* from)
	:
	fToken(""),
	fPngImageData(NULL)
{
	if (from->FindString(KEY_TOKEN, &fToken) != B_OK) {
		HDERROR("expected key [%s] in the message data when creating a "
			"captcha", KEY_TOKEN);
	}

	const void* data;
	ssize_t len;

	if (from->FindData(KEY_PNG_IMAGE_DATA, B_ANY_TYPE, &data, &len) != B_OK)
		HDERROR("expected key [%s] in the message data", KEY_PNG_IMAGE_DATA);
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
		result = into->AddString(KEY_TOKEN, fToken);
	if (result == B_OK && fPngImageData != NULL) {
		result = into->AddData(KEY_PNG_IMAGE_DATA, B_ANY_TYPE,
			fPngImageData->Buffer(), fPngImageData->BufferLength());
	}
	return result;
}