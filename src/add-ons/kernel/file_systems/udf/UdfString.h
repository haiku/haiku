//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//---------------------------------------------------------------------

#ifndef _UDF_STRING_H
#define _UDF_STRING_H

#include <stdio.h>

#include "Array.h"
#include "UdfDebug.h"

#include <new>
#include <string.h>


/*! \brief UdfString class that takes as input either a UTF8 string or a
	CS0 unicode string and then provides access to said string in both
	formats.
	
	For CS0 info, see: ECMA-167 1/7.2.2 (not very helpful), UDF-2.01 2.1.1
*/
class UdfString {
public:
						UdfString();
						UdfString(const char *utf8);
						UdfString(const char *cs0, uint32 length);

	template <uint32 length>	
						UdfString(const array<char, length> &dString);		
						~UdfString();
	
	void				SetTo(const char *utf8);
	void				SetTo(const char *cs0, uint32 length);
	template <uint32 length>
	void				SetTo(const array<char, length> &dString);
		
	template <uint32 length>
	UdfString&			operator=(const array<char, length> &dString); 
	
	const char			*Cs0() const { return fCs0String; }
	const char			*Utf8() const { return fUtf8String; }
	uint32				Cs0Length() const { return fCs0Length; }
	uint32				Utf8Length() const { return fUtf8String ? strlen(fUtf8String) : 0; }
	
private:
	void				_Clear();

	char				*fCs0String;
	uint32				fCs0Length;
	char				*fUtf8String;
};

/*! \brief Creates a new UdfString object from the given d-string.
*/
template <uint32 length>
UdfString::UdfString(const array<char, length> &dString)
	: fCs0String(NULL)
	, fUtf8String(NULL)
{
	TRACE(("UdfString::UdfString: dString.length(): %" B_PRIu32,
		dString.length()));
	SetTo(dString);
}

/*! \brief Assignment from a d-string.

	The last byte of a d-string specifies the data length of the
	enclosed Cs0 string.
*/
template <uint32 length>
void
UdfString::SetTo(const array<char, length> &dString)
{
	uint8 dataLength = dString.length() == 0
		? 0 : reinterpret_cast<const uint8 *>(dString.data)[dString.length() - 1];
	if (dataLength == 0 
		|| dataLength == 1 /* technically illegal, but... */) {
		SetTo(NULL);
	} else {
		if (dataLength > dString.length() - 1)
			dataLength = dString.length() - 1;
		SetTo(reinterpret_cast<const char *>(dString.data), dataLength);
	}
}

/*! \brief Assignment from a d-string.
*/
template <uint32 length>
UdfString&
UdfString::operator=(const array<char, length> &dString)
{
	SetTo(dString);
	return *this;
}

#endif	// _UDF_STRING_H
