/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef STRING_H
#define STRING_H

#include <SupportDefs.h>
#include <string.h>
#include <new>


// string_hash
//
// from the Dragon Book: a slightly modified hashpjw()
static inline
uint32
string_hash(const char *name)
{
	uint32 h = 0;
	if (name) {
		for (; *name; name++) {
			uint32 g = h & 0xf0000000;
			if (g)
				h ^= g >> 24;
			h = (h << 4) + *name;
		}
	}
	return h;
}

#ifdef __cplusplus

// String
class String {
public:
	inline String();
	inline String(const String &string);
	inline String(const char *string, int32 length = -1);
	inline ~String();

	inline bool SetTo(const char *string, int32 maxLength = -1);
	inline void Unset();

	inline void Truncate(int32 newLength);

	inline const char *GetString() const;
	inline int32 GetLength() const	{ return fLength; }

	inline uint32 GetHashCode() const	{ return string_hash(GetString()); }

	inline String &operator=(const String &string);
	inline bool operator==(const String &string) const;
	inline bool operator!=(const String &string) const { return !(*this == string); }

private:
	inline bool _SetTo(const char *string, int32 length);

private:
	int32	fLength;
	char	*fString;
};

/*!
	\class String
	\brief A very simple string class.
*/

// constructor
String::String()
	: fLength(0),
	  fString(NULL)
{
}

// copy constructor
String::String(const String &string)
	: fLength(0),
	  fString(NULL)
{
	*this = string;
}

// constructor
String::String(const char *string, int32 length)
	: fLength(0),
	  fString(NULL)
{
	SetTo(string, length);
}

// destructor
String::~String()
{
	Unset();
}

// SetTo
bool
String::SetTo(const char *string, int32 maxLength)
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
String::Unset()
{
	if (fString) {
		delete[] fString;
		fString = NULL;
	}
	fLength = 0;
}

// Truncate
void
String::Truncate(int32 newLength)
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
String::GetString() const
{
	if (fString)
		return fString;
	return "";
}

// =
String &
String::operator=(const String &string)
{
	if (&string != this)
		_SetTo(string.fString, string.fLength);
	return *this;
}

// ==
bool
String::operator==(const String &string) const
{
	return (fLength == string.fLength
			&& (fLength == 0 || !strcmp(fString, string.fString)));
}

// _SetTo
bool
String::_SetTo(const char *string, int32 length)
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


#endif	// __cplusplus

#endif	// STRING_H
