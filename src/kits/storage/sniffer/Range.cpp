//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file Range.cpp
	MIME sniffer range implementation
*/

#include <sniffer/Range.h>
#include <sniffer/Parser.h>
#include <stdio.h>

using namespace Sniffer;

Range::Range(int32 start, int32 end)
	: fStart(-1)
	, fEnd(-1)
{
	SetTo(start, end);
}

int32
Range::Start() const {
	return fStart;
}

int32
Range::End() const {
	return fEnd;
}
	
void
Range::SetTo(int32 start, int32 end) {
	if (start > end) {
		char start_str[32];
		char end_str[32];
		sprintf(start_str, "%ld", start);
		sprintf(end_str, "%ld", end);
		throw new Err(std::string("Sniffer Parser Error -- Invalid range: [") + start_str + ":" + end_str + "]", -1);
	} else {
		fStart = start;
		fEnd = end;
	}
}
