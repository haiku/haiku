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
	\version \$Id: EbmlBinary.cpp 1112 2005-03-28 09:55:50Z mosu $
	\author Steve Lhomme     <robux4 @ users.sf.net>
	\author Julien Coloos	<suiryc @ users.sf.net>
*/
#include <cassert>

#include "ebml/EbmlBinary.h"

START_LIBEBML_NAMESPACE

EbmlBinary::EbmlBinary()
 :EbmlElement(0, false), Data(NULL)
{}

EbmlBinary::EbmlBinary(const EbmlBinary & ElementToClone)
 :EbmlElement(ElementToClone)
{
	if (ElementToClone.Data == NULL)
		Data = NULL;
	else {
		Data = (binary *)malloc(Size * sizeof(binary));
		assert(Data != NULL);
		memcpy(Data, ElementToClone.Data, Size);
	}
}

EbmlBinary::~EbmlBinary(void) {
	if(Data)
		free(Data);
}

uint32 EbmlBinary::RenderData(IOCallback & output, bool bForceRender, bool bKeepIntact)
{
	output.writeFully(Data,Size);

	return Size;
}
	
/*!
	\note no Default binary value handled
*/
uint64 EbmlBinary::UpdateSize(bool bKeepIntact, bool bForceRender)
{
	return Size;
}

uint64 EbmlBinary::ReadData(IOCallback & input, ScopeMode ReadFully)
{
	if (Data != NULL)
		free(Data);
	
	if (ReadFully == SCOPE_NO_DATA)
	{
		Data = NULL;
		return Size;
	}

	Data = (binary *)malloc(Size * sizeof(binary));
	assert(Data != NULL);
	bValueIsSet = true;
	return input.read(Data, Size);
}

bool EbmlBinary::operator==(const EbmlBinary & ElementToCompare) const
{
	return ((Size == ElementToCompare.Size) && !memcmp(Data, ElementToCompare.Data, Size));
}

END_LIBEBML_NAMESPACE
