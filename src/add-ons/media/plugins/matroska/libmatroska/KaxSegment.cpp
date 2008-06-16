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
	\version \$Id: KaxSegment.cpp 1096 2005-03-17 09:14:52Z robux4 $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/
#include "matroska/KaxSegment.h"
#include "ebml/EbmlHead.h"

// sub elements
#include "matroska/KaxCluster.h"
#include "matroska/KaxSeekHead.h"
#include "matroska/KaxCues.h"
#include "matroska/KaxTracks.h"
#include "matroska/KaxInfo.h"
#include "matroska/KaxChapters.h"
#include "matroska/KaxAttachments.h"
#include "matroska/KaxTags.h"
#include "matroska/KaxContexts.h"

START_LIBMATROSKA_NAMESPACE

EbmlSemantic KaxMatroska_ContextList[2] =
{
	EbmlSemantic(true, true,  EbmlHead::ClassInfos),
	EbmlSemantic(true, false, KaxSegment::ClassInfos),
};

EbmlSemantic KaxSegment_ContextList[8] =
{
	EbmlSemantic(false, false, KaxCluster::ClassInfos),
	EbmlSemantic(false, false, KaxSeekHead::ClassInfos),
	EbmlSemantic(false, true,  KaxCues::ClassInfos),
	EbmlSemantic(false, false, KaxTracks::ClassInfos),
	EbmlSemantic(true,  true,  KaxInfo::ClassInfos),
	EbmlSemantic(false, true,  KaxChapters::ClassInfos),
	EbmlSemantic(false, true,  KaxAttachments::ClassInfos),
	EbmlSemantic(false, true,  KaxTags::ClassInfos),
};

const EbmlSemanticContext KaxMatroska_Context = EbmlSemanticContext(countof(KaxMatroska_ContextList), KaxMatroska_ContextList, NULL, *GetKaxGlobal_Context, NULL);
const EbmlSemanticContext KaxSegment_Context = EbmlSemanticContext(countof(KaxSegment_ContextList), KaxSegment_ContextList, NULL, *GetKaxGlobal_Context, &KaxSegment::ClassInfos);

EbmlId KaxSegment_TheId(0x18538067, 4);
const EbmlCallbacks KaxSegment::ClassInfos(KaxSegment::Create, KaxSegment_TheId, "Segment\0rotomopogo", KaxSegment_Context);

KaxSegment::KaxSegment()
	:EbmlMaster(KaxSegment_Context)
{
	SetSizeLength(5); // mandatory min size support (for easier updating) (2^(7*5)-2 = 32Go)
	SetSizeInfinite(); // by default a segment is big and the size is unknown in advance
}

KaxSegment::KaxSegment(const KaxSegment & ElementToClone)
 :EbmlMaster(ElementToClone)
{
	// update the parent of each children
	std::vector<EbmlElement *>::const_iterator Itr = ElementList.begin();
	while (Itr != ElementList.end())
	{
		if (EbmlId(**Itr) == KaxCluster::ClassInfos.GlobalId) {
			static_cast<KaxCluster *>(*Itr)->SetParent(*this);
		}
	}
}


uint64 KaxSegment::GetRelativePosition(uint64 aGlobalPosition) const
{
	return aGlobalPosition - GetElementPosition() - HeadSize();
}

uint64 KaxSegment::GetRelativePosition(const EbmlElement & Elt) const
{
	return GetRelativePosition(Elt.GetElementPosition());
}

uint64 KaxSegment::GetGlobalPosition(uint64 aRelativePosition) const
{
	return aRelativePosition + GetElementPosition() + HeadSize();
}

END_LIBMATROSKA_NAMESPACE
