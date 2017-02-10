//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//---------------------------------------------------------------------

#ifndef _D_STRING_H
#define _D_STRING_H

#include "kernel_cpp.h"
#include "UdfString.h"
#include "UdfDebug.h"

namespace Udf {

/*! \brief Fixed-length d-string class that takes a Udf::String as input
	and provides a properly formatted ECMA-167 d-string of the given
	field length as ouput.
	
	For d-string info, see: ECMA-167 1/7.2.12, UDF-2.50 2.1.3
*/
class DString {
public:
	DString();
	DString(const DString &ref);
	DString(const Udf::String &string, uint8 fieldLength);
	DString(const char *utf8, uint8 fieldLength);

	void SetTo(const DString &ref);
	void SetTo(const Udf::String &string, uint8 fieldLength);
	void SetTo(const char *utf8, uint8 fieldLength);

	const uint8* String() const { return fString; }
	uint8 Length() const { return fLength; }	
private:
	void _Clear();

	uint8 *fString;
	uint8 fLength;
};

};	// namespace UDF



#endif	// _D_STRING_H
