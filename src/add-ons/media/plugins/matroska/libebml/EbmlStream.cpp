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
	\version \$Id: EbmlStream.cpp 639 2004-07-09 20:59:14Z mosu $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/
#include "ebml/EbmlStream.h"

START_LIBEBML_NAMESPACE

EbmlStream::EbmlStream(IOCallback & DataStream)
 :Stream(DataStream)
{}

EbmlStream::~EbmlStream()
{}

EbmlElement * EbmlStream::FindNextID(const EbmlCallbacks & ClassInfos, const uint64 MaxDataSize)
{
	return EbmlElement::FindNextID(Stream, ClassInfos, MaxDataSize);
}

EbmlElement * EbmlStream::FindNextElement(const EbmlSemanticContext & Context, int & UpperLevel, const uint64 MaxDataSize, bool AllowDummyElt, unsigned int MaxLowerLevel)
{
	return EbmlElement::FindNextElement(Stream, Context, UpperLevel, MaxDataSize, AllowDummyElt, MaxLowerLevel);
}

END_LIBEBML_NAMESPACE
