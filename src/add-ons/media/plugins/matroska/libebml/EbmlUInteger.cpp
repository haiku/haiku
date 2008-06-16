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
	\version \$Id: EbmlUInteger.cpp 1079 2005-03-03 13:18:14Z robux4 $
	\author Steve Lhomme     <robux4 @ users.sf.net>
	\author Moritz Bunkus <moritz @ bunkus.org>
*/
#include <cassert>

#include "ebml/EbmlUInteger.h"

START_LIBEBML_NAMESPACE

EbmlUInteger::EbmlUInteger()
 :EbmlElement(DEFAULT_UINT_SIZE, false)
{}

EbmlUInteger::EbmlUInteger(const uint64 aDefaultValue)
 :EbmlElement(DEFAULT_UINT_SIZE, true), Value(aDefaultValue), DefaultValue(aDefaultValue)
{
	DefaultIsSet = true;
}

EbmlUInteger::EbmlUInteger(const EbmlUInteger & ElementToClone)
 :EbmlElement(ElementToClone)
 ,Value(ElementToClone.Value)
 ,DefaultValue(ElementToClone.DefaultValue)
{
}

/*!
	\todo handle exception on errors
*/
uint32 EbmlUInteger::RenderData(IOCallback & output, bool bForceRender, bool bKeepIntact)
{
	binary FinalData[8]; // we don't handle more than 64 bits integers
	
	if (SizeLength > 8)
		return 0; // integer bigger coded on more than 64 bits are not supported
	
	uint64 TempValue = Value;
	for (unsigned int i=0; i<Size;i++) {
		FinalData[Size-i-1] = TempValue & 0xFF;
		TempValue >>= 8;
	}
	
	output.writeFully(FinalData,Size);

	return Size;
}

uint64 EbmlUInteger::UpdateSize(bool bKeepIntact, bool bForceRender)
{
	if (!bKeepIntact && IsDefaultValue())
		return 0;

	if (Value <= 0xFF) {
		Size = 1;
	} else if (Value <= 0xFFFF) {
		Size = 2;
	} else if (Value <= 0xFFFFFF) {
		Size = 3;
	} else if (Value <= 0xFFFFFFFF) {
		Size = 4;
	} else if (Value <= EBML_PRETTYLONGINT(0xFFFFFFFFFF)) {
		Size = 5;
	} else if (Value <= EBML_PRETTYLONGINT(0xFFFFFFFFFFFF)) {
		Size = 6;
	} else if (Value <= EBML_PRETTYLONGINT(0xFFFFFFFFFFFFFF)) {
		Size = 7;
	} else {
		Size = 8;
	}

	if (DefaultSize > Size) {
		Size = DefaultSize;
	}

	return Size;
}

uint64 EbmlUInteger::ReadData(IOCallback & input, ScopeMode ReadFully)
{
	if (ReadFully != SCOPE_NO_DATA)
	{
		binary Buffer[8];
		input.readFully(Buffer, Size);
		Value = 0;
		
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
