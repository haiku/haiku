//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file sniffer/RPattern.h
	Mime Sniffer RPattern declarations
*/
#ifndef _SNIFFER_R_PATTERN_H
#define _SNIFFER_R_PATTERN_H

#include <sniffer/Range.h>

class BPositionIO;

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
private:
	Range fRange;
	Pattern *fPattern;
};

}

#endif	// _SNIFFER_R_PATTERN_H