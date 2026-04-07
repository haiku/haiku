/*
 * Copyright 2002, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Dauwalder
 */

/*!
	\file RPattern.cpp
	MIME sniffer rpattern implementation
*/

#include "Err.h"
#include "RPattern.h"
#include "Range.h"
#include "Data.h"

using namespace BPrivate::Storage::Sniffer;


RPattern::RPattern(Range range, bool caseInsensitive, const std::string& string, std::string mask)
	:
	Pattern(caseInsensitive, string, mask),
	fRange(range)
{
}


status_t
RPattern::InitCheck() const
{
	status_t err = fRange.InitCheck();
	if (!err)
		err = Pattern::InitCheck();
	return err;
}


Err*
RPattern::GetErr() const
{
	if (fRange.InitCheck() != B_OK) {
		return fRange.GetErr();
	} else {
		if (Pattern::InitCheck() != B_OK)
			return Pattern::GetErr();
		else
			return NULL;
	}
}


RPattern::~RPattern()
{
}


//! Sniffs the given data stream over the object's range for the object's pattern
bool
RPattern::Sniff(const Data& data) const
{
	if (!data.buffer || InitCheck() != B_OK)
		return false;
	else
		return Pattern::Sniff(fRange, data);
}


/*! \brief Returns the number of bytes needed to perform a complete sniff, or an error
	code if something goes wrong.
*/
ssize_t
RPattern::BytesNeeded() const
{
	ssize_t result = InitCheck();
	if (result == B_OK)
		result = Pattern::BytesNeeded();
	if (result >= 0)
		result += fRange.End();
	return result;
}
