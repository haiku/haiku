/****************************************************************************
** libebml : parse EBML files, see http://embl.sourceforge.net/
**
** <file/class description>
**
** Copyright (C) 2002-2004 Steve Lhomme.  All rights reserved.
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
	\version \$Id: EbmlId.h 936 2004-11-10 20:46:28Z mosu $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/
#ifndef LIBEBML_ID_H
#define LIBEBML_ID_H

#include "EbmlTypes.h"

START_LIBEBML_NAMESPACE

/*!
	\class EbmlId
*/
class EBML_DLL_API EbmlId {
	public:
		uint32 Value;
		unsigned int Length;

		EbmlId(const binary aValue[4], const unsigned int aLength)
			:Length(aLength)
		{
			Value = 0;
			unsigned int i;
			for (i=0; i<aLength; i++) {
				Value <<= 8;
				Value += aValue[i];
			}
		}

		EbmlId(const uint32 aValue, const unsigned int aLength)
			:Value(aValue), Length(aLength) {}

		inline bool operator==(const EbmlId & TestId) const
		{
			return ((TestId.Length == Length) && (TestId.Value == Value));
		}
		inline bool operator!=(const EbmlId & TestId) const
		{
			return !(*this == TestId);
		}

		inline void Fill(binary * Buffer) const {
			unsigned int i;
			for (i = 0; i<Length; i++) {
				Buffer[i] = (Value >> (8*(Length-i-1))) & 0xFF;
			}
		}
};

END_LIBEBML_NAMESPACE

#endif // LIBEBML_ID_H
