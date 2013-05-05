/*
 * Copyright 2004-2007, Ingo Weinhold, bonefish@users.sf.net. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include <new>
#include <string.h>

#include "HashString.h"

/*!
	\class HashString
	\brief A very simple string class.
*/

// constructor
HashString::HashString()
	: fLength(0),
	  fString(NULL)
{
}

// copy constructor
HashString::HashString(const HashString &string)
	: fLength(0),
	  fString(NULL)
{
	*this = string;
}

// constructor
HashString::HashString(const char *string, int32 length)
	: fLength(0),
	  fString(NULL)
{
	SetTo(string, length);
}

// destructor
HashString::~HashString()
{
	Unset();
}

// SetTo
bool
HashString::SetTo(const char *string, int32 maxLength)
{
	if (string) {
		if (maxLength > 0)
			maxLength = strnlen(string, maxLength);
		else if (maxLength < 0)
			maxLength = strlen(string);
	}
	return _SetTo(string, maxLength);
}

// Unset
void
HashString::Unset()
{
	if (fString) {
		delete[] fString;
		fString = NULL;
	}
	fLength = 0;
}

// Truncate
void
HashString::Truncate(int32 newLength)
{
	if (newLength < 0)
		newLength = 0;
	if (newLength < fLength) {
		char *string = fString;
		fString = NULL;
		if (!_SetTo(string, newLength)) {
			fString = string;
			fLength = newLength;
			fString[fLength] = '\0';
		} else
			delete[] string;
	}
}

// GetString
const char *
HashString::GetString() const
{
	if (fString)
		return fString;
	return "";
}

// =
HashString &
HashString::operator=(const HashString &string)
{
	if (&string != this)
		_SetTo(string.fString, string.fLength);
	return *this;
}

// ==
bool
HashString::operator==(const HashString &string) const
{
	return (fLength == string.fLength
			&& (fLength == 0 || !strcmp(fString, string.fString)));
}

// _SetTo
bool
HashString::_SetTo(const char *string, int32 length)
{
	bool result = true;
	Unset();
	if (string && length > 0) {
		fString = new(std::nothrow) char[length + 1];
		if (fString) {
			memcpy(fString, string, length);
			fString[length] = '\0';
			fLength = length;
		} else
			result = false;
	}
	return result;
}

