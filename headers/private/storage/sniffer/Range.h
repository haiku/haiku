//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file sniffer/Range.h
	MIME sniffer range declarations
*/
#ifndef _sk_sniffer_range_h_
#define _sk_sniffer_range_h_

#include <SupportDefs.h>

namespace Sniffer {

class Range {
public:
	Range(int32 start, int32 end);

	int32 Start() const;
	int32 End() const;
	
	void SetTo(int32 start, int32 end);
private:
	int32 fStart;
	int32 fEnd;
};

}

#endif	// _sk_sniffer_range_h_