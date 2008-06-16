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
	\version \$Id: EbmlDate.cpp 1079 2005-03-03 13:18:14Z robux4 $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/
#include <cassert>

#include "ebml/EbmlDate.h"

START_LIBEBML_NAMESPACE

const uint64 EbmlDate::UnixEpochDelay = 978307200; // 2001/01/01 00:00:00 UTC

EbmlDate::EbmlDate(const EbmlDate & ElementToClone)
:EbmlElement(ElementToClone)
{
	myDate = ElementToClone.myDate;
}

uint64 EbmlDate::ReadData(IOCallback & input, ScopeMode ReadFully)
{
	if (ReadFully != SCOPE_NO_DATA)
	{
		if (Size != 0) {
			assert(Size == 8);
			binary Buffer[8];
			input.readFully(Buffer, Size);
		
			big_int64 b64;
			b64.Eval(Buffer);
			
			myDate = b64;
			bValueIsSet = true;
		}
	}
	
	return Size;
}

uint32 EbmlDate::RenderData(IOCallback & output, bool bForceRender, bool bKeepIntact)
{
	if (Size != 0) {
		assert(Size == 8);
		big_int64 b64(myDate);
		
		output.writeFully(&b64.endian(),Size);
	}

	return Size;
}

END_LIBEBML_NAMESPACE
