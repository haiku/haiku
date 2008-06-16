/****************************************************************************
** libebml : parse EBML files, see http://embl.sourceforge.net/
**
** <file/class description>
**
** Copyright (C) 2002-2005 Steve Lhomme.  All rights reserved.
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
	\version \$Id: EbmlSInteger.cpp 1079 2005-03-03 13:18:14Z robux4 $
	\author Steve Lhomme     <robux4 @ users.sf.net>
	\author Moritz Bunkus <moritz @ bunkus.org>
*/
#include <cassert>

#include "ebml/EbmlSInteger.h"

START_LIBEBML_NAMESPACE

EbmlSInteger::EbmlSInteger()
 :EbmlElement(DEFAULT_INT_SIZE, false)
{}

EbmlSInteger::EbmlSInteger(const int64 aDefaultValue)
 :EbmlElement(DEFAULT_INT_SIZE, true), Value(aDefaultValue)
{
	DefaultIsSet = true;
}

EbmlSInteger::EbmlSInteger(const EbmlSInteger & ElementToClone)
 :EbmlElement(ElementToClone)
 ,Value(ElementToClone.Value)
 ,DefaultValue(ElementToClone.DefaultValue)
{
}

/*!
	\todo handle exception on errors
*/
uint32 EbmlSInteger::RenderData(IOCallback & output, bool bForceRender, bool bKeepIntact)
{
	binary FinalData[8]; // we don't handle more than 64 bits integers
	unsigned int i;
	
	if (SizeLength > 8)
		return 0; // integer bigger coded on more than 64 bits are not supported
	
	int64 TempValue = Value;
	for (i=0; i<Size;i++) {
		FinalData[Size-i-1] = binary(TempValue & 0xFF);
		TempValue >>= 8;
	}
	
	output.writeFully(FinalData,Size);

	return Size;
}

uint64 EbmlSInteger::UpdateSize(bool bKeepIntact, bool bForceRender)
{
	if (!bKeepIntact && IsDefaultValue())
		return 0;

	if (Value <= 0x7F && Value >= (-0x80)) {
		Size = 1;
	} else if (Value <= 0x7FFF && Value >= (-0x8000)) {
		Size = 2;
	} else if (Value <= 0x7FFFFF && Value >= (-0x800000)) {
		Size = 3;
	} else if (Value <= 0x7FFFFFFF && Value >= (-0x80000000)) {
		Size = 4;
	} else if (Value <= EBML_PRETTYLONGINT(0x7FFFFFFFFF) &&
		   Value >= EBML_PRETTYLONGINT(-0x8000000000)) {
		Size = 5;
	} else if (Value <= EBML_PRETTYLONGINT(0x7FFFFFFFFFFF) &&
		   Value >= EBML_PRETTYLONGINT(-0x800000000000)) {
		Size = 6;
	} else if (Value <= EBML_PRETTYLONGINT(0x7FFFFFFFFFFFFF) &&
		   Value >= EBML_PRETTYLONGINT(-0x80000000000000)) {
		Size = 7;
	} else {
		Size = 8;
	}

	if (DefaultSize > Size) {
		Size = DefaultSize;
	}

	return Size;
}

uint64 EbmlSInteger::ReadData(IOCallback & input, ScopeMode ReadFully)
{
	if (ReadFully != SCOPE_NO_DATA)
	{
		binary Buffer[8];
		input.readFully(Buffer, Size);
		
		if (Buffer[0] & 0x80)
			Value = -1; // this is a negative value
		else
			Value = 0; // this is a positive value
		
		for (unsigned int i=0; i<Size; i++)
		{
			Value <<= 8;
			Value |= Buffer[i];
		}
		bValueIsSet = true;
	}

	return Size;
}

END_LIBEBML_NAMESPACE
