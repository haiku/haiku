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
	\version \$Id: KaxSeekHead.cpp 640 2004-07-09 21:05:36Z mosu $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/
#include "matroska/KaxSeekHead.h"
#include "matroska/KaxContexts.h"
#include "matroska/KaxSegment.h"
#include "matroska/KaxCues.h"

using namespace LIBEBML_NAMESPACE;

// sub elements
START_LIBMATROSKA_NAMESPACE

EbmlSemantic KaxSeekHead_ContextList[1] = 
{
	EbmlSemantic(true,  false,  KaxSeek::ClassInfos),
};

EbmlSemantic KaxSeek_ContextList[2] = 
{
	EbmlSemantic(true,  true,  KaxSeekID::ClassInfos),
	EbmlSemantic(true,  true,  KaxSeekPosition::ClassInfos),
};

const EbmlSemanticContext KaxSeekHead_Context = EbmlSemanticContext(countof(KaxSeekHead_ContextList), KaxSeekHead_ContextList, &KaxSegment_Context, *GetKaxGlobal_Context, &KaxSeekHead::ClassInfos);
const EbmlSemanticContext KaxSeek_Context = EbmlSemanticContext(countof(KaxSeek_ContextList), KaxSeek_ContextList, &KaxSeekHead_Context, *GetKaxGlobal_Context, &KaxSeek::ClassInfos);
const EbmlSemanticContext KaxSeekID_Context = EbmlSemanticContext(0, NULL, &KaxSeek_Context, *GetKaxGlobal_Context, &KaxSeekID::ClassInfos);
const EbmlSemanticContext KaxSeekPosition_Context = EbmlSemanticContext(0, NULL, &KaxSeek_Context, *GetKaxGlobal_Context, &KaxSeekPosition::ClassInfos);

EbmlId KaxSeekHead_TheId    (0x114D9B74, 4);
EbmlId KaxSeek_TheId        (0x4DBB, 2);
EbmlId KaxSeekID_TheId      (0x53AB, 2);
EbmlId KaxSeekPosition_TheId(0x53AC, 2);

const EbmlCallbacks KaxSeekHead::ClassInfos(KaxSeekHead::Create, KaxSeekHead_TheId, "SeekHeader", KaxSeekHead_Context);
const EbmlCallbacks KaxSeek::ClassInfos(KaxSeek::Create, KaxSeek_TheId, "SeekPoint", KaxSeek_Context);
const EbmlCallbacks KaxSeekID::ClassInfos(KaxSeekID::Create, KaxSeekID_TheId, "SeekID", KaxSeekID_Context);
const EbmlCallbacks KaxSeekPosition::ClassInfos(KaxSeekPosition::Create, KaxSeekPosition_TheId, "SeekPosition", KaxSeekPosition_Context);

KaxSeekHead::KaxSeekHead()
	:EbmlMaster(KaxSeekHead_Context)
{}

KaxSeek::KaxSeek()
	:EbmlMaster(KaxSeek_Context)
{}

/*!
	\todo verify that the element is not already in the list
*/
void KaxSeekHead::IndexThis(const EbmlElement & aElt, const KaxSegment & ParentSegment)
{
	// create a new point
	KaxSeek & aNewPoint = AddNewChild<KaxSeek>(*this);

	// add the informations to this element
	KaxSeekPosition & aNewPos = GetChild<KaxSeekPosition>(aNewPoint);
	*static_cast<EbmlUInteger *>(&aNewPos) = ParentSegment.GetRelativePosition(aElt);

	KaxSeekID & aNewID = GetChild<KaxSeekID>(aNewPoint);
	binary ID[4];
	for (int i=aElt.Generic().GlobalId.Length; i>0; i--) {
		ID[4-i] = (aElt.Generic().GlobalId.Value >> 8*(i-1)) & 0xFF;
	}
	aNewID.CopyBuffer(ID, aElt.Generic().GlobalId.Length);
}

KaxSeek * KaxSeekHead::FindFirstOf(const EbmlCallbacks & Callbacks) const
{
	// parse all the Entries and find the first to match the type
	KaxSeek * aElt = static_cast<KaxSeek *>(FindFirstElt(KaxSeek::ClassInfos));
	while (aElt != NULL)
	{
		KaxSeekID * aId = NULL;
		for (unsigned int i = 0; i<aElt->ListSize(); i++) {
			if (EbmlId(*(*aElt)[i]) == KaxSeekID::ClassInfos.GlobalId) {
				aId = static_cast<KaxSeekID*>((*aElt)[i]);
				EbmlId aEbmlId(aId->GetBuffer(), aId->GetSize());
				if (aEbmlId == Callbacks.GlobalId)
				{
					return aElt;
				}
				break;
			}
		}
		aElt = static_cast<KaxSeek *>(FindNextElt(*aElt));
	}

	return NULL;
}

KaxSeek * KaxSeekHead::FindNextOf(const KaxSeek &aPrev) const
{
	unsigned int iIndex;
	KaxSeek *tmp;
	
	// look for the previous in the list
	for (iIndex = 0; iIndex<ElementList.size(); iIndex++)
	{
		if (ElementList[iIndex] == static_cast<const EbmlElement*>(&aPrev))
			break;
	}

	if (iIndex <ElementList.size()) {
		iIndex++;
		for (; iIndex<ElementList.size(); iIndex++)
		{
			if (EbmlId(*(ElementList[iIndex])) == KaxSeek::ClassInfos.GlobalId)
			{
				tmp = static_cast<KaxSeek *>(ElementList[iIndex]);
				if (tmp->IsEbmlId(aPrev))
					return tmp;
			}
		}
	}

	return NULL;
}

int64 KaxSeek::Location() const
{
	KaxSeekPosition *aPos = static_cast<KaxSeekPosition*>(FindFirstElt(KaxSeekPosition::ClassInfos));
	if (aPos == NULL)
		return 0;
	return uint64(*aPos);
}

bool KaxSeek::IsEbmlId(const EbmlId & aId) const
{
	KaxSeekID *_Id = static_cast<KaxSeekID*>(FindFirstElt(KaxSeekID::ClassInfos));
	if (_Id == NULL)
		return false;
	EbmlId aEbmlId(_Id->GetBuffer(), _Id->GetSize());
	return (aId == aEbmlId);
}

bool KaxSeek::IsEbmlId(const KaxSeek & aPoint) const
{
	KaxSeekID *_IdA = static_cast<KaxSeekID*>(FindFirstElt(KaxSeekID::ClassInfos));
	if (_IdA == NULL)
		return false;
	KaxSeekID *_IdB = static_cast<KaxSeekID*>(aPoint.FindFirstElt(KaxSeekID::ClassInfos));
	if (_IdB == NULL)
		return false;
	EbmlId aEbmlIdA(_IdA->GetBuffer(), _IdA->GetSize());
	EbmlId aEbmlIdB(_IdB->GetBuffer(), _IdB->GetSize());
	return (aEbmlIdA == aEbmlIdB);
}

END_LIBMATROSKA_NAMESPACE
