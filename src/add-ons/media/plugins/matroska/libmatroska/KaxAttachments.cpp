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
	\version \$Id: KaxAttachments.cpp 640 2004-07-09 21:05:36Z mosu $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/
#include "matroska/KaxAttachments.h"
#include "matroska/KaxAttached.h"
#include "matroska/KaxContexts.h"

using namespace LIBEBML_NAMESPACE;

// sub elements
START_LIBMATROSKA_NAMESPACE

EbmlSemantic KaxAttachments_ContextList[1] =
{
	EbmlSemantic(true, false, KaxAttached::ClassInfos),        ///< EBMLVersion
};

const EbmlSemanticContext KaxAttachments_Context = EbmlSemanticContext(countof(KaxAttachments_ContextList), KaxAttachments_ContextList, &KaxSegment_Context, *GetKaxGlobal_Context, &KaxAttachments::ClassInfos);

EbmlId KaxAttachments_TheId(0x1941A469, 4);
const EbmlCallbacks KaxAttachments::ClassInfos(KaxAttachments::Create, KaxAttachments_TheId, "Attachments", KaxAttachments_Context);

KaxAttachments::KaxAttachments()
 :EbmlMaster(KaxAttachments_Context)
{
	SetSizeLength(2); // mandatory min size support (for easier updating) (2^(7*2)-2 = 16Ko)
}

END_LIBMATROSKA_NAMESPACE
