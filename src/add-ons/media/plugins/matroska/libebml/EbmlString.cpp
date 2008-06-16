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
	\version \$Id: EbmlString.cpp 1079 2005-03-03 13:18:14Z robux4 $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/
#include <cassert>

#include "ebml/EbmlString.h"

START_LIBEBML_NAMESPACE

EbmlString::EbmlString()
 :EbmlElement(0, false)
{
	DefaultSize = 0;
/* done automatically	
	Size = Value.length();
	if (DefaultSize > Size)
		Size = DefaultSize;*/
}

EbmlString::EbmlString(const std::string & aDefaultValue)
 :EbmlElement(0, true), Value(aDefaultValue), DefaultValue(aDefaultValue)
{
	DefaultSize = 0;
	DefaultIsSet = true;
/* done automatically	
	Size = Value.length();
	if (DefaultSize > Size)
		Size = DefaultSize;*/
}

/*!
	\todo Cloning should be on the same exact type !
*/
EbmlString::EbmlString(const EbmlString & ElementToClone)
 :EbmlElement(ElementToClone)
 ,Value(ElementToClone.Value)
 ,DefaultValue(ElementToClone.DefaultValue)
{
}

/*!
	\todo handle exception on errors
*/
uint32 EbmlString::RenderData(IOCallback & output, bool bForceRender, bool bKeepIntact)
{
	uint32 Result;
	output.writeFully(Value.c_str(), Value.length());
	Result = Value.length();
	
	if (Result < DefaultSize) {
		// pad the rest with 0
		binary *Pad = new binary[DefaultSize - Result];
		if (Pad == NULL)
		{
			return Result;
		}
		memset(Pad, 0x00, DefaultSize - Result);
		output.writeFully(Pad, DefaultSize - Result);
		Result = DefaultSize;
		delete [] Pad;
	}
	
	return Result;
}

EbmlString & EbmlString::operator=(const std::string NewString)
{
	Value = NewString;
	bValueIsSet = true;
/* done automatically	
	Size = Value.length();
	if (DefaultSize > Size)
		Size = DefaultSize;*/
	return *this;
}

uint64 EbmlString::UpdateSize(bool bKeepIntact, bool bForceRender)
{
	if (!bKeepIntact && IsDefaultValue())
		return 0;

	if (Value.length() < DefaultSize) {
		Size = DefaultSize;
	} else {
		Size = Value.length();
	}
	return Size;
}

uint64 EbmlString::ReadData(IOCallback & input, ScopeMode ReadFully)
{
	if (ReadFully != SCOPE_NO_DATA)
	{
		if (Size == 0) {
			Value = "";
			bValueIsSet = true;
		} else {
			char *Buffer = new char[Size + 1];
			if (Buffer == NULL) {
				// unable to store the data, skip it
				input.setFilePointer(Size, seek_current);
			} else {
				input.readFully(Buffer, Size);
				if (Buffer[Size-1] != '\0') {
					Buffer[Size] = '\0';
				}
				Value = Buffer;
				delete [] Buffer;
				bValueIsSet = true;
			}
		}
	}

	return Size;
}

END_LIBEBML_NAMESPACE
