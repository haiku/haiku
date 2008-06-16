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
	\version \$Id: EbmlBinary.h 1298 2008-02-21 22:14:18Z mosu $
	\author Steve Lhomme     <robux4 @ users.sf.net>
	\author Julien Coloos	<suiryc @ users.sf.net>
*/
#ifndef LIBEBML_BINARY_H
#define LIBEBML_BINARY_H

#include <string>
#include <cstring>

#include "EbmlTypes.h"
#include "EbmlElement.h"

// ----- Added 10/15/2003 by jcsston from Zen -----
#if defined (__BORLANDC__) //Maybe other compilers?
  #include <mem.h>
#endif //__BORLANDC__
// ------------------------------------------------

START_LIBEBML_NAMESPACE

/*!
    \class EbmlBinary
    \brief Handle all operations on an EBML element that contains "unknown" binary data

	\todo handle fix sized elements (like UID of CodecID)
*/
class EBML_DLL_API EbmlBinary : public EbmlElement {
	public:
		EbmlBinary();
		EbmlBinary(const EbmlBinary & ElementToClone);
		virtual ~EbmlBinary(void);
	
		uint32 RenderData(IOCallback & output, bool bForceRender, bool bKeepIntact = false);
		uint64 ReadData(IOCallback & input, ScopeMode ReadFully = SCOPE_ALL_DATA);
		uint64 UpdateSize(bool bKeepIntact = false, bool bForceRender = false);
	
		void SetBuffer(const binary *Buffer, const uint32 BufferSize) {
			Data = (binary *) Buffer;
			Size = BufferSize;
			bValueIsSet = true;
		}

		binary *GetBuffer() const {return Data;}
		
		void CopyBuffer(const binary *Buffer, const uint32 BufferSize) {
			if (Data != NULL)
				free(Data);
			Data = (binary *)malloc(BufferSize * sizeof(binary));
			memcpy(Data, Buffer, BufferSize);
			Size = BufferSize;
			bValueIsSet = true;
		}
		
		uint64 GetSize() const {return Size;}
		operator const binary &() const {return *Data;}
	
		bool IsDefaultValue() const {
			return false;
		}

		bool operator==(const EbmlBinary & ElementToCompare) const;

	protected:
		binary *Data; // the binary data inside the element
};

END_LIBEBML_NAMESPACE

#endif // LIBEBML_BINARY_H
