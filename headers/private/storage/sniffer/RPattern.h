//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//---------------------------------------------------------------------
/*!
	\file sniffer/RPattern.h
	Mime Sniffer RPattern declarations
*/
#ifndef _SNIFFER_R_PATTERN_H
#define _SNIFFER_R_PATTERN_H

#include <sniffer/Range.h>

class BPositionIO;

namespace BPrivate {
namespace Storage {
namespace Sniffer {

class Err;
class Pattern;

//! A Pattern and a Range, bundled into one.
class RPattern {
public:
	RPattern(Range range, Pattern *pattern);
	~RPattern();	
	
	status_t InitCheck() const;
	Err* GetErr() const;
	
	bool Sniff(BPositionIO *data, bool caseInsensitive) const;
	ssize_t BytesNeeded() const;
private:
	Range fRange;
	Pattern *fPattern;
};

};	// namespace Sniffer
};	// namespace Storage
};	// namespace BPrivate

#endif	// _SNIFFER_R_PATTERN_H


