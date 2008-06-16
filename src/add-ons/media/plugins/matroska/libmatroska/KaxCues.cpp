/****************************************************************************
** libmatroska : parse Matroska files, see http://www.matroska.org/
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
** See http://www.matroska.org/license/lgpl/ for LGPL licensing information.**
** Contact license@matroska.org if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

/*!
	\file
	\version \$Id: KaxCues.cpp 1265 2007-01-14 17:20:35Z mosu $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/
#include <cassert>

#include "matroska/KaxCues.h"
#include "matroska/KaxCuesData.h"
#include "matroska/KaxContexts.h"
#include "ebml/EbmlStream.h"

// sub elements
START_LIBMATROSKA_NAMESPACE

EbmlSemantic KaxCues_ContextList[1] = 
{
	EbmlSemantic(true,  false,  KaxCuePoint::ClassInfos),
};

const EbmlSemanticContext KaxCues_Context = EbmlSemanticContext(countof(KaxCues_ContextList), KaxCues_ContextList, &KaxSegment_Context, *GetKaxGlobal_Context, &KaxCues::ClassInfos);

EbmlId KaxCues_TheId(0x1C53BB6B, 4);
const EbmlCallbacks KaxCues::ClassInfos(KaxCues::Create, KaxCues_TheId, "Cues", KaxCues_Context);

KaxCues::KaxCues()
	:EbmlMaster(KaxCues_Context)
{}

KaxCues::~KaxCues()
{
	assert(myTempReferences.size() == 0); // otherwise that means you have added references and forgot to set the position
}

bool KaxCues::AddBlockGroup(const KaxBlockGroup & BlockRef)
{
	// Do not add the element if it's already present.
	std::vector<const KaxBlockBlob *>::iterator ListIdx;
	KaxBlockBlob &BlockReference = *(new KaxBlockBlob(BLOCK_BLOB_NO_SIMPLE));
	BlockReference.SetBlockGroup(*const_cast<KaxBlockGroup*>(&BlockRef));

	for (ListIdx = myTempReferences.begin(); ListIdx != myTempReferences.end(); ListIdx++)
		if (*ListIdx == &BlockReference)
			return true;

	myTempReferences.push_back(&BlockReference);
	return true;
}

bool KaxCues::AddBlockBlob(const KaxBlockBlob & BlockReference)
{
	// Do not add the element if it's already present.
	std::vector<const KaxBlockBlob *>::iterator ListIdx;

	for (ListIdx = myTempReferences.begin(); ListIdx != myTempReferences.end(); ListIdx++)
		if (*ListIdx == &BlockReference)
			return true;

	myTempReferences.push_back(&BlockReference);
	return true;
}

void KaxCues::PositionSet(const KaxBlockBlob & BlockReference)
{
	// look for the element in the temporary references
	std::vector<const KaxBlockBlob *>::iterator ListIdx;

	for (ListIdx = myTempReferences.begin(); ListIdx != myTempReferences.end(); ListIdx++) {
		if (*ListIdx == &BlockReference) {
			// found, now add the element to the entry list
			KaxCuePoint & NewPoint = AddNewChild<KaxCuePoint>(*this);
			NewPoint.PositionSet(BlockReference, GlobalTimecodeScale());
			myTempReferences.erase(ListIdx);
			break;
		}
	}
}

void KaxCues::PositionSet(const KaxBlockGroup & BlockRef)
{
	// look for the element in the temporary references
	std::vector<const KaxBlockBlob *>::iterator ListIdx;

	for (ListIdx = myTempReferences.begin(); ListIdx != myTempReferences.end(); ListIdx++) {
		const KaxInternalBlock &refTmp = **ListIdx;
		if (refTmp.GlobalTimecode() == BlockRef.GlobalTimecode() &&
			refTmp.TrackNum() == BlockRef.TrackNumber()) {
			// found, now add the element to the entry list
			KaxCuePoint & NewPoint = AddNewChild<KaxCuePoint>(*this);
			NewPoint.PositionSet(**ListIdx, GlobalTimecodeScale());
			myTempReferences.erase(ListIdx);
			break;
		}
	}
}

/*!
	\warning Assume that the list has been sorted (Sort())
*/
const KaxCuePoint * KaxCues::GetTimecodePoint(uint64 aTimecode) const
{
	uint64 TimecodeToLocate = aTimecode / GlobalTimecodeScale();
	const KaxCuePoint * aPointPrev = NULL;
	uint64 aPrevTime = 0;
	const KaxCuePoint * aPointNext = NULL;
	uint64 aNextTime = EBML_PRETTYLONGINT(0xFFFFFFFFFFFF);

	for (unsigned int i=0; i<ListSize(); i++)
	{
		if (EbmlId(*(*this)[i]) == KaxCuePoint::ClassInfos.GlobalId) {
			const KaxCuePoint *tmp = static_cast<const KaxCuePoint *>((*this)[i]);
			// check the tile
			const KaxCueTime *aTime = static_cast<const KaxCueTime *>(tmp->FindFirstElt(KaxCueTime::ClassInfos));
			if (aTime != NULL)
			{
				uint64 _Time = uint64(*aTime);
				if (_Time > aPrevTime && _Time < TimecodeToLocate) {
					aPrevTime = _Time;
					aPointPrev = tmp;
				}
				if (_Time < aNextTime && _Time > TimecodeToLocate) {
					aNextTime= _Time;
					aPointNext = tmp;
				}
			}
		}
	}

	return aPointPrev;
}

uint64 KaxCues::GetTimecodePosition(uint64 aTimecode) const
{
	const KaxCuePoint * aPoint = GetTimecodePoint(aTimecode);
	if (aPoint == NULL)
		return 0;

	const KaxCueTrackPositions * aTrack = aPoint->GetSeekPosition();
	if (aTrack == NULL)
		return 0;

	return aTrack->ClusterPosition();
}

END_LIBMATROSKA_NAMESPACE
