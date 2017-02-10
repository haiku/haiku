//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//---------------------------------------------------------------------
/*!
	\file RPatternList.cpp
	MIME sniffer rpattern list implementation
*/

#include <sniffer/Err.h>
#include <sniffer/RPattern.h>
#include <sniffer/RPatternList.h>
#include <DataIO.h>
#include <stdio.h>

using namespace BPrivate::Storage::Sniffer;

RPatternList::RPatternList()
	: DisjList()
{
}

RPatternList::~RPatternList() {
	// Clear our rpattern list
	std::vector<RPattern*>::iterator i;
	for (i = fList.begin(); i != fList.end(); i++)
		delete *i;
}

status_t
RPatternList::InitCheck() const {
	return B_OK;
}

Err*
RPatternList::GetErr() const {
	return NULL;
}

/*! Sniffs the given data stream, searching for a match
	with any of the list's patterns. Each pattern is searched
	over its own specified range.
*/
bool
RPatternList::Sniff(BPositionIO *data) const {
	if (InitCheck() != B_OK)
		return false;
	else {
		bool result = false;
		std::vector<RPattern*>::const_iterator i;
		for (i = fList.begin(); i != fList.end(); i++) {
			if (*i)
				result |= (*i)->Sniff(data, fCaseInsensitive);
		}
		return result;
	}
}

/*! \brief Returns the number of bytes needed to perform a complete sniff, or an error
	code if something goes wrong.
*/
ssize_t
RPatternList::BytesNeeded() const
{
	ssize_t result = InitCheck();
	
	// Tally up the BytesNeeded() values for all the RPatterns and return the largest.
	if (result == B_OK) {
		result = 0; // Just to be safe...
		std::vector<RPattern*>::const_iterator i;
		for (i = fList.begin(); i != fList.end(); i++) {
			if (*i) {
				ssize_t bytes = (*i)->BytesNeeded();
				if (bytes >= 0) {
					if (bytes > result)
						result = bytes;
				} else {
					result = bytes;
					break;
				}
			}
		}
	}	
	return result;
}
	
void
RPatternList::Add(RPattern *rpattern) {
	if (rpattern)
		fList.push_back(rpattern);
}



