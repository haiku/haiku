/*
 * Copyright 2002, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Dauwalder
 */

/*!
	\file Pattern.cpp
	MIME sniffer pattern implementation
*/

#include "Pattern.h"

#include <new>
#include <string.h>

#include "Err.h"
#include "Data.h"

using namespace BPrivate::Storage::Sniffer;


/*static*/ Pattern*
Pattern::Create(bool caseInsensitive, const std::string& string, std::string mask)
{
	return (Pattern*)_Create(sizeof(Pattern), 0, caseInsensitive, string, mask);
}


//! Creates an object with a Pattern stored at the specified offset.
/*static*/ void*
Pattern::_Create(size_t baseSize, size_t offset,
	bool caseInsensitive, const std::string& string, std::string mask)
{
	size_t size = baseSize;
	size += string.length();
	if (!mask.empty() || caseInsensitive)
		size += string.length();

	void* object = malloc(size);
	if (object == NULL)
		return object;

	new((uint8*)object + offset) Pattern(caseInsensitive, string, mask);
	return object;
}


Pattern::Pattern(bool caseInsensitive, const std::string& string, std::string mask)
	:
	fCStatus(B_NO_INIT),
	fErrorMessage(NULL),
	fCaseInsensitive(caseInsensitive)
{
	fStringLength = string.length();
	if (fStringLength == 0) {
		SetStatus(B_BAD_VALUE, "Sniffer pattern error: illegal empty pattern");
		return;
	}

	uint8* patternString = fData;
	uint8* patternMask = fData + fStringLength;
	memcpy(patternString, string.data(), fStringLength);

	if (mask.empty() && !caseInsensitive) {
		// No mask and not case insensitive: the whole string is "unmasked".
		fUnmaskedStartLength = fStringLength;
	} else if (caseInsensitive && mask.empty()) {
		// We need a mask in this case.
		memset(patternMask, 0xFF, fStringLength);

		// But if there's non-case-sensitive characters at the string's start,
		// we can still consider those "unmasked".
		fUnmaskedStartLength = 0;
		for (uint32 i = 0; i < fStringLength; i++) {
			if ('A' <= patternString[i] && patternString[i] <= 'Z')
				break;
			if ('a' <= patternString[i] && patternString[i] <= 'z')
				break;
			fUnmaskedStartLength++;
		}
	} else if (mask.length() != string.length()) {
		SetStatus(B_BAD_VALUE,
			"Sniffer pattern error: pattern and mask lengths do not match");
		return;
	} else {
		memcpy(patternMask, mask.data(), fStringLength);

		fUnmaskedStartLength = 0;
		for (uint32 i = 0; i < fStringLength; i++) {
			if (patternMask[i] != 0xFF)
				break;
			fUnmaskedStartLength++;
		}
	}

	fCStatus = B_OK;
}


Pattern::~Pattern()
{
	delete fErrorMessage;
}


status_t
Pattern::InitCheck() const
{
	return fCStatus;
}


Err*
Pattern::GetErr() const
{
	if (fCStatus == B_OK)
		return NULL;
	return new(std::nothrow) Err(*fErrorMessage);
}


/*! \brief Looks for a pattern match in the given data stream, starting from
	each offset withing the given range. Returns true is a match is found,
	false if not.
*/
bool
Pattern::Sniff(Range range, const Data& data) const
{
	int32 start = range.Start();
	int32 end = range.End();
	if ((size_t)end >= data.length)
		end = data.length - 1; // Don't bother searching beyond the end of the stream
	for (int i = start; i <= end; i++) {
		if (Sniff(i, data))
			return true;
	}
	return false;
}


/*! \brief Returns the number of bytes needed to perform a complete sniff, or an error
	code if something goes wrong.
*/
ssize_t
Pattern::BytesNeeded() const
{
	ssize_t result = InitCheck();
	if (result == B_OK)
		result = fStringLength;
	return result;
}


bool
Pattern::Sniff(off_t start, const Data& data) const
{
	int32 len = fStringLength;
	// \todo If there are fewer bytes left in the data stream
	// from the given position than the length of our data
	// string, should we just return false (which is what we're
	// doing now), or should we compare as many bytes as we
	// can and return true if those match?
	if ((data.length - start) < (size_t)len)
		return false;

	const uint8* string = fData;
	const uint8* buffer = data.buffer + start;

	// Compare the "unmasked" portion of the pattern.
	if (fUnmaskedStartLength != 0) {
		if (memcmp(string, buffer, fUnmaskedStartLength) != 0)
			return false;
		if (fUnmaskedStartLength == fStringLength)
			return true;
	}

	// Compare the remainder.
	string += fUnmaskedStartLength;
	buffer += fUnmaskedStartLength;
	len -= fUnmaskedStartLength;
	const uint8* mask = fData + fStringLength + fUnmaskedStartLength;

	bool result = true;
	if (fCaseInsensitive) {
		for (int32 i = 0; i < len; i++) {
			uint8 secondChar;
			if ('A' <= string[i] && string[i] <= 'Z') {
				// Also check lowercase
				secondChar = 'a' + (string[i] - 'A');
			} else if ('a' <= string[i] && string[i] <= 'z') {
				// Also check uppercase
				secondChar = 'A' + (string[i] - 'a');
			} else {
				secondChar = string[i];
					// Check the same char twice as punishment for
					// doing a case insensitive search ;-)
			}
			if (((string[i] & mask[i]) != (buffer[i] & mask[i]))
				&& ((secondChar & mask[i]) != (buffer[i] & mask[i]))) {
				result = false;
				break;
			}
		}
	} else {
		for (int32 i = 0; i < len; i++) {
			if ((string[i] & mask[i]) != (buffer[i] & mask[i])) {
				result = false;
				break;
			}
		}
	}
	return result;
}


void
Pattern::SetStatus(status_t status, const char* msg)
{
	fCStatus = status;
	if (status == B_OK) {
		SetErrorMessage(NULL);
	} else if (msg) {
		SetErrorMessage(msg);
	} else {
		SetErrorMessage(
			"Sniffer parser error: Pattern::SetStatus() -- NULL msg with non-B_OK status.\n"
			"(This is officially the most helpful error message you will ever receive ;-)");
	}
}


void
Pattern::SetErrorMessage(const char* msg)
{
	delete fErrorMessage;
	fErrorMessage = (msg) ? (new(std::nothrow) Err(msg, -1)) : (NULL);
}
