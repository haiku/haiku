/****************************************************************************
** libmatroska : parse Matroska files, see http://www.matroska.org/
**
** <file/class description>
**
** Copyright (C) 2002-2004 Steve Lhomme.  All rights reserved.
**
** This file is part of libmatroska.
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
** See http://www.matroska.org/license/lgpl/ for LGPL licensing information.**
** Contact license@matroska.org if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

/*!
	\file
	\version \$Id: KaxTags.cpp 711 2004-08-10 12:58:47Z robux4 $
	\author Steve Lhomme     <robux4 @ users.sf.net>
	\author Jory Stone       <jcsston @ toughguy.net>
*/
#include "matroska/KaxTags.h"
#include "matroska/KaxTag.h"
#include "matroska/KaxContexts.h"

using namespace LIBEBML_NAMESPACE;

// sub elements
START_LIBMATROSKA_NAMESPACE

EbmlSemantic KaxTags_ContextList[1] =
{
	EbmlSemantic(true, false, KaxTag::ClassInfos),
};

const EbmlSemanticContext KaxTags_Context = EbmlSemanticContext(countof(KaxTags_ContextList), KaxTags_ContextList, &KaxSegment_Context, *GetKaxGlobal_Context, &KaxTags::ClassInfos);

EbmlId KaxTags_TheId(0x1254C367, 4);

const EbmlCallbacks KaxTags::ClassInfos(KaxTags::Create, KaxTags_TheId, "Tags", KaxTags_Context);

KaxTags::KaxTags()
	:EbmlMaster(KaxTags_Context)
{}

END_LIBMATROSKA_NAMESPACE
