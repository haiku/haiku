//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
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

using namespace Sniffer;

RPattern::RPattern(Range range, Pattern *pattern)
	: fRange(range)
	, fPattern(pattern)
{
}

status_t
RPattern::InitCheck() const {
	if (fRange.InitCheck() != B_OK)
		return fRange.InitCheck();
	else if (fPattern) {
		if (fPattern->InitCheck() != B_OK)
			return fPattern->InitCheck();
		else
			return B_OK;
	} else
		return B_BAD_VALUE;
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
RPattern::Sniff(BPositionIO *data) const {
	if (!data || InitCheck() != B_OK)
		return false;
	else 
		return fPattern->Sniff(fRange, data);
}
