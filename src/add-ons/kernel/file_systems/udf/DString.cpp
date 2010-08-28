/*
 * Copyright 2003, Tyler Dauwalder, tyler@dauwalder.net.
 * Distributed under the terms of the MIT License.
 */


#include "DString.h"

#include <string.h>


/*! \brief Creates a useless, empty string object. */
DString::DString()
	:
	fLength(0),
	fString(NULL)
{
}


/*! \brief Create a new DString object that is a copy of \a ref. */
DString::DString(const DString &ref)
	:
	fLength(0),
	fString(NULL)
{
	SetTo(ref);
}


/*! \brief Creates a new DString \a fieldLength bytes long that contains
	at most the first \c (fieldLength-1) bytes of \a string.Cs0().
*/
DString::DString(const UdfString &string, uint8 fieldLength)
	:
	fLength(0),
	fString(NULL)
{
	SetTo(string, fieldLength);
}


/*! \brief Creates a new DString \a fieldLength bytes long that contains
	at most the first \c (fieldLength-1) bytes of the Cs0 representation
	of the NULL-terminated UTF8 string \a utf8.
*/
DString::DString(const char *utf8, uint8 fieldLength)
	:
	fLength(0),
	fString(NULL)
{
	SetTo(utf8, fieldLength);
}


DString::~DString()
{
	delete[] fString;
}


void
DString::SetTo(const DString &ref)
{
	_Clear();
	if (ref.Length() > 0) {
		fString = new(nothrow) uint8[ref.Length()];
		if (fString != NULL) {
			fLength = ref.Length();
			memcpy(fString, ref.String(), fLength);
		}
	}
}


/*! \brief Sets the DString be \a fieldLength bytes long and contain
	at most the first \c (fieldLength-1) bytes of \a string.Cs0().
*/
void
DString::SetTo(const UdfString &string, uint8 fieldLength)
{
	_Clear();
	if (fieldLength > 0) {
		// Allocate our string
		fString = new(nothrow) uint8[fieldLength];
		status_t error = fString ? B_OK : B_NO_MEMORY;
		if (!error) {
			// Figure out how many bytes to copy
			uint32 sourceLength = string.Cs0Length();
			if (sourceLength > 0) {
				uint8 destLength = sourceLength > uint8(fieldLength - 1)
					? uint8(fieldLength - 1) : uint8(sourceLength);
				// If the source string is 16-bit unicode, make sure any dangling
				// half-character at the end of the string is not copied
				if (string.Cs0()[1] == '\x10' && destLength > 0
					&& destLength % 2 == 0)
					destLength--;
				// Copy
				memcpy(fString, string.Cs0(), destLength);
				// Zero any characters between the end of the string and
				// the terminating string length character
				if (destLength < fieldLength - 1)
					memset(&fString[destLength], 0, fieldLength - 1 - destLength);
				// Write the string length to the last character in the field
				fString[fieldLength - 1] = destLength;
			} else {
				// Empty strings are to contain all zeros
				memset(fString, 0, fieldLength);
			}
		}
	}
}


/*! \brief Sets the DString be \a fieldLength bytes long and contain
	at most the first \c (fieldLength-1) bytes of the Cs0 representation
	of the NULL-terminated UTF8 string \a utf8.
*/
void
DString::SetTo(const char *utf8, uint8 fieldLength)
{
	UdfString string(utf8);
	SetTo(string, fieldLength);
}


void
DString::_Clear()
{
	DEBUG_INIT("DString");
	delete[] fString;
	fString = NULL;
	fLength = 0;
}
