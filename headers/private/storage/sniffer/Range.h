//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//---------------------------------------------------------------------
/*!
	\file sniffer/Range.h
	MIME sniffer range declarations
*/
#ifndef _SNIFFER_RANGE_H
#define _SNIFFER_RANGE_H

#include <SupportDefs.h>

namespace BPrivate {
namespace Storage {
namespace Sniffer {

class Err;

//! A range of byte offsets from which to check a pattern against a data stream.
class Range {
public:
	Range(int32 start, int32 end);
	
	status_t InitCheck() const;
	Err* GetErr() const;

	int32 Start() const;
	int32 End() const;
	
	void SetTo(int32 start, int32 end);
private:
	int32 fStart;
	int32 fEnd;
	status_t fCStatus;
};

};	// namespace Sniffer
};	// namespace Storage
};	// namespace BPrivate

#endif	// _SNIFFER_RANGE_H


