//----------------------------------------------------------------------
//  This software is part of the Haiku distribution and is covered
//  by the MIT License.
//---------------------------------------------------------------------
/*!
	\file RPattern.cpp
	MIME sniffer rpattern implementation
*/

#include <sniffer/Err.h>
#include <sniffer/Pattern.h>
#include <sniffer/Range.h>
#include <sniffer/RPattern.h>
#include <DataIO.h>

using namespace BPrivate::Storage::Sniffer;

RPattern::RPattern(Range range, Pattern *pattern)
	: fRange(range)
	, fPattern(pattern)
{
}

status_t
RPattern::InitCheck() const {
	status_t err = fRange.InitCheck();
	if (!err)
		err = fPattern ? B_OK : B_BAD_VALUE;
	if (!err)
		err = fPattern->InitCheck();
	return err;
}

Err*
RPattern::GetErr() const {
	if (fRange.InitCheck() != B_OK)
		return fRange.GetErr();
	else if (fPattern) {
		if (fPattern->InitCheck() != B_OK)
			return fPattern->GetErr();
		else
			return NULL;
	} else
		return new Err("Sniffer parser error: RPattern::RPattern() -- NULL pattern parameter", -1);
}

RPattern::~RPattern() {
	delete fPattern;
}

//! Sniffs the given data stream over the object's range for the object's pattern
bool
RPattern::Sniff(BPositionIO *data, bool caseInsensitive) const {
	if (!data || InitCheck() != B_OK)
		return false;
	else 
		return fPattern->Sniff(fRange, data, caseInsensitive);
}

/*! \brief Returns the number of bytes needed to perform a complete sniff, or an error
	code if something goes wrong.
*/
ssize_t
RPattern::BytesNeeded() const
{
	ssize_t result = InitCheck();
	if (result == B_OK)
		result = fPattern->BytesNeeded();
	if (result >= 0)
		result += fRange.End();
	return result;	
}


