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
	\version \$Id: EbmlHead.cpp 1096 2005-03-17 09:14:52Z robux4 $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/
#include "ebml/EbmlHead.h"
#include "ebml/EbmlSubHead.h"
#include "ebml/EbmlContexts.h"

START_LIBEBML_NAMESPACE

const EbmlSemantic EbmlHead_ContextList[] =
{
	EbmlSemantic(true, true, EVersion::ClassInfos),        ///< EBMLVersion
	EbmlSemantic(true, true, EReadVersion::ClassInfos),    ///< EBMLReadVersion
	EbmlSemantic(true, true, EMaxIdLength::ClassInfos),    ///< EBMLMaxIdLength
	EbmlSemantic(true, true, EMaxSizeLength::ClassInfos),  ///< EBMLMaxSizeLength
	EbmlSemantic(true, true, EDocType::ClassInfos),        ///< DocType
	EbmlSemantic(true, true, EDocTypeVersion::ClassInfos), ///< DocTypeVersion
	EbmlSemantic(true, true, EDocTypeReadVersion::ClassInfos), ///< DocTypeReadVersion
};

const EbmlSemanticContext EbmlHead_Context = EbmlSemanticContext(countof(EbmlHead_ContextList), EbmlHead_ContextList, NULL, *GetEbmlGlobal_Context, &EbmlHead::ClassInfos);

EbmlId EbmlHead_TheId(0x1A45DFA3, 4);
const EbmlCallbacks EbmlHead::ClassInfos(EbmlHead::Create, EbmlHead_TheId, "EBMLHead\0ratamapaga", EbmlHead_Context);

EbmlHead::EbmlHead()
 :EbmlMaster(EbmlHead_Context)
{}

END_LIBEBML_NAMESPACE
