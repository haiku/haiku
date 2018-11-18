/*
 * Copyright 2018, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <MetaData.h>
#include <String.h>

#include <stdio.h>
#include <stdlib.h>


// TODO: probably we can do better
const char*
key_to_string(uint32 key)
{
	char buf[sizeof(char) * sizeof(uint32) * 4 + 1];
	if (buf) {
		sprintf(buf, "%" B_PRId32, key);
	}
	BString ret(buf);
	ret.Prepend("codec:metadata:");
	return ret.String();
}


BMetaData::BMetaData()
	:
	fMessage(NULL)
{
	fMessage = new BMessage();
}


BMetaData::~BMetaData()
{
	delete fMessage;
}


status_t
BMetaData::SetString(uint32 key, const BString& value)
{
	return fMessage->AddString(key_to_string(key), value);
}


status_t
BMetaData::SetBool(uint32 key, bool value)
{
	return fMessage->AddBool(key_to_string(key), value);
}


status_t
BMetaData::SetUInt32(uint32 key, uint32 value)
{
	return fMessage->AddUInt32(key_to_string(key), value);
}


status_t
BMetaData::FindString(uint32 key, BString* value) const
{
	return fMessage->FindString(key_to_string(key), value);
}


status_t
BMetaData::FindBool(uint32 key, bool* value) const
{
	return fMessage->FindBool(key_to_string(key), value);
}


status_t
BMetaData::FindUInt32(uint32 key, uint32* value) const
{
	return fMessage->FindUInt32(key_to_string(key), value);
}


status_t
BMetaData::RemoveValue(uint32 key)
{
	return fMessage->RemoveName(key_to_string(key));
}


// Clean up all keys
void
BMetaData::Reset()
{
	delete fMessage;
	fMessage = new BMessage();
}
