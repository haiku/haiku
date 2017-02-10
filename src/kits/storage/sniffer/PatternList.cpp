//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
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
	
/*! \brief Returns the number of bytes needed to perform a complete sniff, or an error
	code if something goes wrong.
*/
ssize_t
PatternList::BytesNeeded() const
{
	ssize_t result = InitCheck();

	// Find the number of bytes needed to sniff any of our
	// patterns from a single location in a data stream
	if (result == B_OK) {
		result = 0;	// I realize it already *is* zero if it == B_OK, but just in case that changes...	
		std::vector<Pattern*>::const_iterator i;
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

	// Now add on the number of bytes needed to get to the
	// furthest allowed starting point
	if (result >= 0)
		result += fRange.End();

	return result;	
}

void
PatternList::Add(Pattern *pattern) {
	if (pattern)
		fList.push_back(pattern);
}



