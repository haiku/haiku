/****************************************************************************
** libebml : parse EBML files, see http://embl.sourceforge.net/
**
** <file/class description>
**
** Copyright (C) 2002-2005 Steve Lhomme.  All rights reserved.
**
** This file is part of libebml.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License as published by the Free Software Foundation; either
** version 2.1 of the License, or (at your option) any later version.
** 
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
** 
** You should have received a copy of the GNU Lesser General Public
** License along with this library; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
**
** See http://www.matroska.org/license/lgpl/ for LGPL licensing information.
**
** Contact license@matroska.org if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

/*!
	\file
	\version \$Id: EbmlUnicodeString.cpp 1079 2005-03-03 13:18:14Z robux4 $
	\author Steve Lhomme     <robux4 @ users.sf.net>
	\author Jory Stone       <jcsston @ toughguy.net>
*/

#include <cassert>

#if __GNUC__ == 2 && ! defined ( __OpenBSD__ )
#include <wchar.h>
#endif

#include "ebml/EbmlUnicodeString.h"

START_LIBEBML_NAMESPACE

// ===================== UTFstring class ===================

UTFstring::UTFstring()
	:_Length(0)
	,_Data(NULL)
{}

UTFstring::UTFstring(const wchar_t * _aBuf)
	:_Length(0)
	,_Data(NULL)
{
	*this = _aBuf;
}

UTFstring::~UTFstring()
{
	delete [] _Data;
}

UTFstring::UTFstring(const UTFstring & _aBuf)
	:_Length(0)
	,_Data(NULL)
{
	*this = _aBuf.c_str();
}

UTFstring & UTFstring::operator=(const UTFstring & _aBuf)
{
	*this = _aBuf.c_str();
	return *this;
}

UTFstring & UTFstring::operator=(const wchar_t * _aBuf)
{
	delete [] _Data;
	if (_aBuf == NULL) {
		_Data = new wchar_t[1];
		_Data[0] = 0;
		UpdateFromUCS2();
		return *this;
	}

	size_t aLen;
	for (aLen=0; _aBuf[aLen] != 0; aLen++);
	_Length = aLen;
	_Data = new wchar_t[_Length+1];
	for (aLen=0; _aBuf[aLen] != 0; aLen++) {
		_Data[aLen] = _aBuf[aLen];
	}
	_Data[aLen] = 0;
	UpdateFromUCS2();
	return *this;
}

UTFstring & UTFstring::operator=(wchar_t _aChar)
{
	delete [] _Data;
	_Data = new wchar_t[2];
	_Length = 1;
	_Data[0] = _aChar;
	_Data[1] = 0;
	UpdateFromUCS2();
	return *this;
}

bool UTFstring::operator==(const UTFstring& _aStr) const
{
	if ((_Data == NULL) && (_aStr._Data == NULL))
		return true;
	if ((_Data == NULL) || (_aStr._Data == NULL))
		return false;
	return wcscmp_internal(_Data, _aStr._Data);
}

void UTFstring::SetUTF8(const std::string & _aStr)
{
	UTF8string = _aStr;
	UpdateFromUTF8();
}

/*!
	\see RFC 2279
*/
void UTFstring::UpdateFromUTF8()
{
	delete [] _Data;
	// find the size of the final UCS-2 string
	size_t i;
	for (_Length=0, i=0; i<UTF8string.length(); _Length++) {
		if ((UTF8string[i] & 0x80) == 0) {
			i++;
		} else if ((UTF8string[i] & 0x20) == 0) {
			i += 2;
		} else if ((UTF8string[i] & 0x10) == 0) {
			i += 3;
		}
	}
	_Data = new wchar_t[_Length+1];
	size_t j;
	for (j=0, i=0; i<UTF8string.length(); j++) {
		if ((UTF8string[i] & 0x80) == 0) {
			_Data[j] = UTF8string[i];
			i++;
		} else if ((UTF8string[i] & 0x20) == 0) {
			_Data[j] = ((UTF8string[i] & 0x1F) << 6) + (UTF8string[i+1] & 0x3F);
			i += 2;
		} else if ((UTF8string[i] & 0x10) == 0) {
			_Data[j] = ((UTF8string[i] & 0x0F) << 12) + ((UTF8string[i+1] & 0x3F) << 6) + (UTF8string[i+2] & 0x3F);
			i += 3;
		}
	}
	_Data[j] = 0;
}

void UTFstring::UpdateFromUCS2()
{
	// find the size of the final UTF-8 string
	size_t i,Size=0;
	for (i=0; i<_Length; i++)
	{
		if (_Data[i] < 0x80) {
			Size++;
		} else if (_Data[i] < 0x800) {
			Size += 2;
		} else if (_Data[i] < 0x10000) {
			Size += 3;
		}
	}
	std::string::value_type *tmpStr = new std::string::value_type[Size+1];
	for (i=0, Size=0; i<_Length; i++)
	{
		if (_Data[i] < 0x80) {
			tmpStr[Size++] = _Data[i];
		} else if (_Data[i] < 0x800) {
			tmpStr[Size++] = 0xC0 | (_Data[i] >> 6);
			tmpStr[Size++] = 0x80 | (_Data[i] & 0x3F);
		} else if (_Data[i] < 0x10000) {
			tmpStr[Size++] = 0xE0 | (_Data[i] >> 12);
			tmpStr[Size++] = 0x80 | ((_Data[i] >> 6) & 0x3F);
			tmpStr[Size++] = 0x80 | (_Data[i] & 0x3F);
		}
	}
	tmpStr[Size] = 0;
	UTF8string = tmpStr; // implicit conversion
	delete [] tmpStr;

}

bool UTFstring::wcscmp_internal(const wchar_t *str1, const wchar_t *str2)
{
	size_t Index=0;
	while (str1[Index] == str2[Index] && str1[Index] != 0) {
		Index++;
	}
	return (str1[Index] == str2[Index]);
}

// ===================== EbmlUnicodeString class ===================

EbmlUnicodeString::EbmlUnicodeString()
:EbmlElement(0, false)
{
	DefaultSize = 0;
}

EbmlUnicodeString::EbmlUnicodeString(const UTFstring & aDefaultValue)
:EbmlElement(0, true), Value(aDefaultValue), DefaultValue(aDefaultValue)
{
	DefaultSize = 0;
	DefaultIsSet = true;
}

EbmlUnicodeString::EbmlUnicodeString(const EbmlUnicodeString & ElementToClone)
 :EbmlElement(ElementToClone)
 ,Value(ElementToClone.Value)
 ,DefaultValue(ElementToClone.DefaultValue)
{
}

/*!
\note limited to UCS-2
\todo handle exception on errors
*/
uint32 EbmlUnicodeString::RenderData(IOCallback & output, bool bForceRender, bool bKeepIntact)
{
	uint32 Result = Value.GetUTF8().length();

	if (Result != 0) {
		output.writeFully(Value.GetUTF8().c_str(), Result);
	}

	if (Result < DefaultSize) {
		// pad the rest with 0
		binary *Pad = new binary[DefaultSize - Result];
		if (Pad != NULL) {
			memset(Pad, 0x00, DefaultSize - Result);
			output.writeFully(Pad, DefaultSize - Result);

			Result = DefaultSize;
			delete [] Pad;
		}
	}

	return Result;
}

EbmlUnicodeString & EbmlUnicodeString::operator=(const UTFstring & NewString)
{
	Value = NewString;
	bValueIsSet = true;
	return *this;
}

/*!
\note limited to UCS-2
*/
uint64 EbmlUnicodeString::UpdateSize(bool bKeepIntact, bool bForceRender)
{
	if (!bKeepIntact && IsDefaultValue())
		return 0;

	Size = Value.GetUTF8().length();
	if (Size < DefaultSize)
		Size = DefaultSize;

	return Size;
}

/*!
	\note limited to UCS-2
*/
uint64 EbmlUnicodeString::ReadData(IOCallback & input, ScopeMode ReadFully)
{	
	if (ReadFully != SCOPE_NO_DATA)
	{
		if (Size == 0) {
			Value = UTFstring::value_type(0);
			bValueIsSet = true;
		} else {
			char *Buffer = new char[Size+1];
			if (Buffer == NULL) {
				// impossible to read, skip it
				input.setFilePointer(Size, seek_current);
			} else {
				input.readFully(Buffer, Size);
				if (Buffer[Size-1] != 0) {
					Buffer[Size] = 0;
				}

				Value.SetUTF8(Buffer); // implicit conversion to std::string
				delete [] Buffer;
				bValueIsSet = true;
			}
		}
	}

	return Size;
}

END_LIBEBML_NAMESPACE
