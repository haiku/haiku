// String.cpp
//
// Copyright (c) 2003, Ingo Weinhold (bonefish@cs.tu-berlin.de)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can alternatively use *this file* under the terms of the the MIT
// license included in this package.

#include <new>
#include <string.h>

#include "String.h"

using std::nothrow;

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
		fString = new(nothrow) char[length + 1];
		if (fString) {
			memcpy(fString, string, length);
			fString[length] = '\0';
			fLength = length;
		} else
			result = false;
	}
	return result;
}

