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

using namespace BPrivate::Storage::Sniffer;

PatternList::PatternList(Range range)
	: DisjList()
	, fRange(range)
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

/*! \brief Sniffs the given data stream, searching for a match with
	any of the list's patterns.
*/
bool
PatternList::Sniff(BPositionIO *data) const {
	if (InitCheck() != B_OK)
		return false;
	else {
		bool result = false;
		std::vector<Pattern*>::const_iterator i;
		for (i = fList.begin(); i != fList.end(); i++) {
			if (*i)
				result |= (*i)->Sniff(fRange, data, fCaseInsensitive);
		}
		return result;
	}
}
	
void
PatternList::Add(Pattern *pattern) {
	if (pattern)
		fList.push_back(pattern);
}



