//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//---------------------------------------------------------------------

#ifndef _UDF_CS0_STRING_H
#define _UDF_CS0_STRING_H

#include <stdio.h>

#include "cpp.h"

#include "Array.h"
#include "UdfDebug.h"

//#include "SupportDefs.h"

namespace Udf {

/*! \brief String class that takes as input CS0 unicode strings,
	which it converts to UTF8 upon construction.
	
	For CS0 info, see: ECMA-167 1/7.2.2 (not very helpful), UDF-2.01 2.1.1
*/
class CS0String {
public:
	CS0String();
	template <uint32 length>	
		CS0String(array<char, length> &cs0);		
	~CS0String();
	
	template <uint32 length>
		void SetTo(array<char, length> &cs0);
		
	template <uint32 length>
		CS0String& operator=(array<char, length> &cs0); 
	
	const char* String() const { return fUtf8String; }
	
private:
	void _Clear();

	char *fUtf8String;
};

void unicode_to_utf8(uint32 c, char **out);

template <uint32 length>
CS0String::CS0String(array<char, length> &cs0)
	: fUtf8String(NULL)
{
	DEBUG_INIT(CF_HELPER | CF_HIGH_VOLUME, "CS0String");	

	SetTo(cs0);
}

template <uint32 length>
void
CS0String::SetTo(array<char, length> &cs0)
{
	DEBUG_INIT(CF_HELPER | CF_HIGH_VOLUME, "CS0String");	

	_Clear();

	// The first byte of the CS0 string is the compression ID.
	// - 8: 1 byte characters
	// - 16: 2 byte, big endian characters
	// - 254: "CS0 expansion is empty and unique", 1 byte characters
	// - 255: "CS0 expansion is empty and unique", 2 byte, big endian characters
	PRINT(("compression ID == %d\n", cs0.data[0]));
	switch (cs0.data[0]) {
		case 8:			
		case 254:
		{
			uint8 *inputString = reinterpret_cast<uint8*>(&(cs0.data[1]));
			int32 maxLength = length-1;				// Max length of input string in uint8 characters
			int32 allocationLength = maxLength*2+1;	// Need at most 2 utf8 chars per uint8 char
			fUtf8String = new char[allocationLength];	
			if (fUtf8String) {
				char *outputString = fUtf8String;
	
				for (int32 i = 0; i < maxLength && inputString[i]; i++) {
					unicode_to_utf8(inputString[i], &outputString);
				}
				outputString[0] = 0;
			} else {
				PRINT(("new fUtf8String[%ld] allocation failed\n", allocationLength));
			}
				
			break;
		}

		case 16:
		case 255:
		{
			uint16 *inputString = reinterpret_cast<uint16*>(&(cs0.data[1]));
			int32 maxLength = (length-1) / 2;		// Max length of input string in uint16 characters
			int32 allocationLength = maxLength*3+1;	// Need at most 3 utf8 chars per uint16 char
			fUtf8String = new char[allocationLength];
			if (fUtf8String) {
				char *outputString = fUtf8String;

				for (int32 i = 0; i < maxLength && inputString[i]; i++) {
					unicode_to_utf8(B_BENDIAN_TO_HOST_INT16(inputString[i]), &outputString);
				}
				outputString[0] = 0;
			} else {
				PRINT(("new fUtf8String[%ld] allocation failed\n", allocationLength));
			}
			
			break;
		}
		
		default:
			PRINT(("invalid compression id\n"));
	}
}

template <uint32 length>
CS0String&
CS0String::operator=(array<char, length> &cs0)
{
	SetTo(cs0);
	return *this;
}


};	// namespace UDF



#endif	// _UDF_CS0_STRING_H
