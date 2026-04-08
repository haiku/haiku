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


/*static*/ RPattern*
RPattern::Create(Range range,
	bool caseInsensitive, const std::string& string, std::string mask)
{
	RPattern* rpattern = (RPattern*)Pattern::_Create(sizeof(RPattern),
		offsetof(RPattern, fPattern), caseInsensitive, string, mask);
	if (rpattern == NULL)
		return NULL;

	rpattern->fRange = range;
	return rpattern;
}


status_t
RPattern::InitCheck() const
{
	status_t err = fRange.InitCheck();
	if (!err)
		err = fPattern.InitCheck();
	return err;
}


Err*
RPattern::GetErr() const
{
	if (fRange.InitCheck() != B_OK) {
		return fRange.GetErr();
	} else {
		if (fPattern.InitCheck() != B_OK)
			return fPattern.GetErr();
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
		return fPattern.Sniff(fRange, data);
}


/*! \brief Returns the number of bytes needed to perform a complete sniff, or an error
	code if something goes wrong.
*/
ssize_t
RPattern::BytesNeeded() const
{
	ssize_t result = InitCheck();
	if (result == B_OK)
		result = fPattern.BytesNeeded();
	if (result >= 0)
		result += fRange.End();
	return result;
}
