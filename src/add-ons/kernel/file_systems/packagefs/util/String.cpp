/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "String.h"


bool
String::SetToExactLength(const char* string, size_t length)
{
	StringData* data = StringPool::Get(string, length);
	if (data == NULL)
		return false;

	fData->ReleaseReference();
	fData = data;
	return true;
}


String&
String::operator=(const String& other)
{
	if (this == &other)
		return *this;

	fData->ReleaseReference();
	fData = other.fData;
	fData->AcquireReference();

	return *this;
}
