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

#include <new>

#include "Err.h"
#include "Pattern.h"
#include "Data.h"

using namespace BPrivate::Storage::Sniffer;


Pattern::Pattern(const std::string& string, const std::string& mask)
	:
	fCStatus(B_NO_INIT),
	fErrorMessage(NULL)
{
	SetTo(string, mask);
}


Pattern::Pattern(const std::string& string)
	:
	fCStatus(B_NO_INIT),
	fErrorMessage(NULL)
{
	// Build a mask with all bits turned on of the
	// appropriate length
	std::string mask = "";
	for (uint i = 0; i < string.length(); i++)
		mask += (char)0xFF;
	SetTo(string, mask);
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
	else
		return new(std::nothrow) Err(*fErrorMessage);
}


void
dumpStr(const std::string& string, const char* label = NULL)
{
	if (label)
		printf("%s: ", label);
	for (uint i = 0; i < string.length(); i++)
		printf("%x ", string[i]);
	printf("\n");
}


status_t
Pattern::SetTo(const std::string& string, const std::string& mask)
{
	fString = string;
	if (fString.length() == 0) {
		SetStatus(B_BAD_VALUE, "Sniffer pattern error: illegal empty pattern");
	} else {
		fMask = mask;
//		dumpStr(string, "data");
//		dumpStr(mask, "mask");
		if (fString.length() != fMask.length())
			SetStatus(B_BAD_VALUE, "Sniffer pattern error: pattern and mask lengths do not match");
		else
			SetStatus(B_OK);
	}
	return fCStatus;
}

/*! \brief Looks for a pattern match in the given data stream, starting from
	each offset withing the given range. Returns true is a match is found,
	false if not.
*/
bool
Pattern::Sniff(Range range, const Data& data, bool caseInsensitive) const
{
	int32 start = range.Start();
	int32 end = range.End();
	if ((size_t)end >= data.length)
		end = data.length - 1; // Don't bother searching beyond the end of the stream
	for (int i = start; i <= end; i++) {
		if (Sniff(i, data, caseInsensitive))
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
		result = fString.length();
	return result;
}


bool
Pattern::Sniff(off_t start, const Data& data, bool caseInsensitive) const
{
	off_t len = fString.length();
	// \todo If there are fewer bytes left in the data stream
	// from the given position than the length of our data
	// string, should we just return false (which is what we're
	// doing now), or should we compare as many bytes as we
	// can and return true if those match?
	if ((data.length - start) < (size_t)len)
		return false;

	const uint8* string = (const uint8*)fString.data();
	const uint8* mask = (const uint8*)fMask.data();

	bool result = true;
	if (caseInsensitive) {
		for (int i = 0; i < len; i++) {
			char secondChar;
			if ('A' <= fString[i] && fString[i] <= 'Z') {
				// Also check lowercase
				secondChar = 'a' + (fString[i] - 'A');
			} else if ('a' <= fString[i] && fString[i] <= 'z') {
				// Also check uppercase
				secondChar = 'A' + (fString[i] - 'a');
			} else {
				secondChar = fString[i];
					// Check the same char twice as punishment for
					// doing a case insensitive search ;-)
			}
			if (((string[i] & mask[i]) != (data.buffer[start + i] & mask[i]))
				&& ((secondChar & mask[i]) != (data.buffer[start + i] & mask[i]))) {
				result = false;
				break;
			}
		}
	} else {
		for (int i = 0; i < len; i++) {
			if ((string[i] & mask[i]) != (data.buffer[start + i] & mask[i])) {
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
