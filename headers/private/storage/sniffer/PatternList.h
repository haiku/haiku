//----------------------------------------------------------------------
//  This software is part of the Haiku distribution and is covered
//  by the MIT License.
//---------------------------------------------------------------------
/*!
	\file sniffer/PatternList.h
	MIME sniffer pattern list declarations
*/
#ifndef _SNIFFER_PATTERN_LIST_H
#define _SNIFFER_PATTERN_LIST_H

#include <sniffer/DisjList.h>
#include <sniffer/Range.h>
#include <vector>

class BPositionIO;

namespace BPrivate {
namespace Storage {
namespace Sniffer {

class Err;
class Pattern;

/*! \brief A list of patterns, one of which must match for the list to match, all
	of which are to be searched over the same range.
*/
class PatternList : public DisjList {
public:
	PatternList(Range range);
	virtual ~PatternList();

	status_t InitCheck() const;
	Err* GetErr() const;
	
	virtual bool Sniff(BPositionIO *data) const;
	virtual ssize_t BytesNeeded() const;
	
	void Add(Pattern *pattern);
private:
	std::vector<Pattern*> fList;
	Range fRange;
};

};	// namespace Sniffer
};	// namespace Storage
};	// namespace BPrivate

#endif	// _SNIFFER_PATTERN_LIST_H


