//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//---------------------------------------------------------------------

#ifndef _UDF_CS0_STRING_H
#define _UDF_CS0_STRING_H

#include <stdio.h>

#include "kernel_cpp.h"

#include "Array.h"
#include "UdfDebug.h"

namespace Udf {

/*! \brief String class that takes as input CS0 unicode strings,
	which it converts to UTF8 upon construction.
	
	For CS0 info, see: ECMA-167 1/7.2.2 (not very helpful), UDF-2.01 2.1.1
*/
class CS0String {
public:
	CS0String();
	CS0String(const char *cs0, uint32 length);
	template <uint32 length>	
		CS0String(const array<char, length> &cs0);		
	~CS0String();
	
	void SetTo(const char *cs0, uint32 length);
	template <uint32 length>
		void SetTo(const array<char, length> &cs0);
		
	template <uint32 length>
		CS0String& operator=(const array<char, length> &cs0); 
	
	const char* String() const { return fUtf8String; }
	uint32 Length() const { return fUtf8String ? strlen(fUtf8String) : 0; }
	
private:
	void _Clear();

	char *fUtf8String;
};

void unicode_to_utf8(uint32 c, char **out);

template <uint32 length>
CS0String::CS0String(const array<char, length> &cs0)
	: fUtf8String(NULL)
{
	DEBUG_INIT(CF_HELPER | CF_HIGH_VOLUME, "CS0String");	

	SetTo(cs0);
}

template <uint32 length>
void
CS0String::SetTo(const array<char, length> &cs0)
{
	SetTo(reinterpret_cast<const char*>(cs0.data), length);
}

template <uint32 length>
CS0String&
CS0String::operator=(const array<char, length> &cs0)
{
	SetTo(cs0);
	return *this;
}


};	// namespace UDF



#endif	// _UDF_CS0_STRING_H
