// String.cpp

#include <new>
#include <string.h>

#include "String.h"

// strnlen
size_t
strnlen(const char *str, size_t maxLen)
{
	if (str) {
		size_t origMaxLen = maxLen;
		while (maxLen > 0 && *str != '\0') {
			maxLen--;
			str++;
		}
		return origMaxLen - maxLen;
	}
	return 0;
}


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

