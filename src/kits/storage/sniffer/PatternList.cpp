//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file PatternList.cpp
	MIME sniffer pattern list implementation
*/

#include <sniffer/Err.h>
#include <sniffer/Pattern.h>
#include <sniffer/PatternList.h>
#include <DataIO.h>
#include <stdio.h>

using namespace Sniffer;

PatternList::PatternList(Range range)
	: fRange(range)
{
}

PatternList::~PatternList() {
	// Clean up
	std::vector<Pattern*>::iterator i;
	for (i = fList.begin(); i != fList.end(); i++)
		delete *i;	
}

status_t
PatternList::InitCheck() const {
	return fRange.InitCheck();
}

Err*
PatternList::GetErr() const {
	return fRange.GetErr();
}

bool
PatternList::Sniff(BPositionIO *data) const {
	if (InitCheck() != B_OK)
		return false;
	else {
		bool result = false;
		std::vector<Pattern*>::const_iterator i;
		for (i = fList.begin(); i != fList.end(); i++) {
			if (*i)
				result |= (*i)->Sniff(fRange, data);
		}
		return result;
	}
}
	
void
PatternList::Add(Pattern *pattern) {
	if (pattern)
		fList.push_back(pattern);
}
