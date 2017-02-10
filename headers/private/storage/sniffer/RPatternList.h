//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//---------------------------------------------------------------------
/*!
	\file sniffer/RPatternList.h
	MIME sniffer rpattern list declarations
*/
#ifndef _SNIFFER_R_PATTERN_LIST_H
#define _SNIFFER_R_PATTERN_LIST_H

#include <sniffer/DisjList.h>
#include <vector>

class BPositionIO;

namespace BPrivate {
namespace Storage {
namespace Sniffer {

class Err;	
class RPattern;

//! A list of patterns, each of which is to be searched over its own specified range.
class RPatternList : public DisjList {
public:
	RPatternList();
	virtual ~RPatternList();
	
	status_t InitCheck() const;
	Err* GetErr() const;
	
	virtual bool Sniff(BPositionIO *data) const;
	virtual ssize_t BytesNeeded() const;
	void Add(RPattern *rpattern);
private:
	std::vector<RPattern*> fList;
};

};	// namespace Sniffer
};	// namespace Storage
};	// namespace BPrivate

#endif	// _SNIFFER_R_PATTERN_LIST_H


