/*
 * Copyright 2023, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "ChecksumJsonEventListener.h"


ChecksumJsonEventListener::ChecksumJsonEventListener(int32 checksumLimit)
	:
	fChecksum(0),
	fChecksumLimit(checksumLimit),
	fError(B_OK),
	fCompleted(false)
{
}


ChecksumJsonEventListener::~ChecksumJsonEventListener()
{
}


bool
ChecksumJsonEventListener::Handle(const BJsonEvent& event)
{
	if (fCompleted || B_OK != fError)
		return false;

	switch (event.EventType()) {
		case B_JSON_NUMBER:
		{
			const char* content = event.Content();
			_ChecksumProcessCharacters(content, strlen(content));
			break;
		}
		case B_JSON_STRING:
		case B_JSON_OBJECT_NAME:
		{
			const char* content = event.Content();
			_ChecksumProcessCharacters(content, strlen(content));
			break;
		}
		default:
			break;
	}

	return true;
}


void
ChecksumJsonEventListener::HandleError(status_t status, int32 line, const char* message)
{
	fError = status;
}


void
ChecksumJsonEventListener::Complete()
{
	fCompleted = true;
}


uint32
ChecksumJsonEventListener::Checksum() const
{
	return fChecksum;
}


status_t
ChecksumJsonEventListener::Error() const
{
	return fError;
}


void
ChecksumJsonEventListener::_ChecksumProcessCharacters(const char* content, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		fChecksum = (fChecksum + static_cast<int32>(content[i])) % fChecksumLimit;
	}
}
