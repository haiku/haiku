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
	\version \$Id: KaxCuesData.cpp 1265 2007-01-14 17:20:35Z mosu $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/
#include <cassert>

#include "matroska/KaxCuesData.h"
#include "matroska/KaxContexts.h"
#include "matroska/KaxBlock.h"
#include "matroska/KaxBlockData.h"
#include "matroska/KaxCluster.h"
#include "matroska/KaxSegment.h"

START_LIBMATROSKA_NAMESPACE

EbmlSemantic KaxCuePoint_ContextList[2] =
{
	EbmlSemantic(true,  true,  KaxCueTime::ClassInfos),
	EbmlSemantic(true,  false, KaxCueTrackPositions::ClassInfos),
};

#if MATROSKA_VERSION == 1
EbmlSemantic KaxCueTrackPositions_ContextList[3] =
#else // MATROSKA_VERSION
EbmlSemantic KaxCueTrackPositions_ContextList[5] =
#endif // MATROSKA_VERSION
{
	EbmlSemantic(true,  true,  KaxCueTrack::ClassInfos),
	EbmlSemantic(true,  true,  KaxCueClusterPosition::ClassInfos),
	EbmlSemantic(false, true,  KaxCueBlockNumber::ClassInfos),
#if MATROSKA_VERSION >= 2
	EbmlSemantic(false, true,  KaxCueCodecState::ClassInfos),
	EbmlSemantic(false, false, KaxCueReference::ClassInfos),
#endif // MATROSKA_VERSION
};

#if MATROSKA_VERSION >= 2
EbmlSemantic KaxCueReference_ContextList[4] =
{
	EbmlSemantic(true,  true,  KaxCueRefTime::ClassInfos),
	EbmlSemantic(true,  true,  KaxCueRefCluster::ClassInfos),
	EbmlSemantic(false, true,  KaxCueRefNumber::ClassInfos),
	EbmlSemantic(false, true,  KaxCueRefCodecState::ClassInfos),
};
#endif // MATROSKA_VERSION

EbmlId KaxCuePoint_TheId          (0xBB, 1);
EbmlId KaxCueTime_TheId           (0xB3, 1);
EbmlId KaxCueTrackPositions_TheId (0xB7, 1);
EbmlId KaxCueTrack_TheId          (0xF7, 1);
EbmlId KaxCueClusterPosition_TheId(0xF1, 1);
EbmlId KaxCueBlockNumber_TheId    (0x5378, 2);
#if MATROSKA_VERSION >= 2
EbmlId KaxCueCodecState_TheId     (0xEA, 1);
EbmlId KaxCueReference_TheId      (0xDB, 1);
EbmlId KaxCueRefTime_TheId        (0x96, 1);
EbmlId KaxCueRefCluster_TheId     (0x97, 1);
EbmlId KaxCueRefNumber_TheId      (0x535F, 2);
EbmlId KaxCueRefCodecState_TheId  (0xEB, 1);
#endif // MATROSKA_VERSION

const EbmlSemanticContext KaxCuePoint_Context = EbmlSemanticContext(countof(KaxCuePoint_ContextList), KaxCuePoint_ContextList, &KaxCues_Context, *GetKaxGlobal_Context, &KaxCuePoint::ClassInfos);
const EbmlSemanticContext KaxCueTime_Context = EbmlSemanticContext(0, NULL, &KaxCuePoint_Context, *GetKaxGlobal_Context, &KaxCueTime::ClassInfos);
const EbmlSemanticContext KaxCueTrackPositions_Context = EbmlSemanticContext(countof(KaxCueTrackPositions_ContextList), KaxCueTrackPositions_ContextList, &KaxCuePoint_Context, *GetKaxGlobal_Context, &KaxCueTrackPositions::ClassInfos);
const EbmlSemanticContext KaxCueTrack_Context = EbmlSemanticContext(0, NULL, &KaxCueTrackPositions_Context, *GetKaxGlobal_Context, &KaxCueTrack::ClassInfos);
const EbmlSemanticContext KaxCueClusterPosition_Context = EbmlSemanticContext(0, NULL, &KaxCueTrackPositions_Context, *GetKaxGlobal_Context, &KaxCueClusterPosition::ClassInfos);
const EbmlSemanticContext KaxCueBlockNumber_Context = EbmlSemanticContext(0, NULL, &KaxCueTrackPositions_Context, *GetKaxGlobal_Context, &KaxCueBlockNumber::ClassInfos);
#if MATROSKA_VERSION >= 2
const EbmlSemanticContext KaxCueCodecState_Context = EbmlSemanticContext(0, NULL, &KaxCueTrackPositions_Context, *GetKaxGlobal_Context, &KaxCueCodecState::ClassInfos);
const EbmlSemanticContext KaxCueReference_Context = EbmlSemanticContext(countof(KaxCueReference_ContextList), KaxCueReference_ContextList, &KaxCueTrackPositions_Context, *GetKaxGlobal_Context, &KaxCueReference::ClassInfos);
const EbmlSemanticContext KaxCueRefTime_Context = EbmlSemanticContext(0, NULL, &KaxCueReference_Context, *GetKaxGlobal_Context, &KaxCueRefTime::ClassInfos);
const EbmlSemanticContext KaxCueRefCluster_Context = EbmlSemanticContext(0, NULL, &KaxCueRefTime_Context, *GetKaxGlobal_Context, &KaxCueRefCluster::ClassInfos);
const EbmlSemanticContext KaxCueRefNumber_Context = EbmlSemanticContext(0, NULL, &KaxCueRefTime_Context, *GetKaxGlobal_Context, &KaxCueRefNumber::ClassInfos);
const EbmlSemanticContext KaxCueRefCodecState_Context = EbmlSemanticContext(0, NULL, &KaxCueRefTime_Context, *GetKaxGlobal_Context, &KaxCueRefCodecState::ClassInfos);
#endif // MATROSKA_VERSION

const EbmlCallbacks KaxCuePoint::ClassInfos(KaxCuePoint::Create, KaxCuePoint_TheId, "CuePoint", KaxCuePoint_Context);
const EbmlCallbacks KaxCueTime::ClassInfos(KaxCueTime::Create, KaxCueTime_TheId, "CueTime", KaxCueTime_Context);
const EbmlCallbacks KaxCueTrackPositions::ClassInfos(KaxCueTrackPositions::Create, KaxCueTrackPositions_TheId, "CueTrackPositions", KaxCueTrackPositions_Context);
const EbmlCallbacks KaxCueTrack::ClassInfos(KaxCueTrack::Create, KaxCueTrack_TheId, "CueTrack", KaxCueTrack_Context);
const EbmlCallbacks KaxCueClusterPosition::ClassInfos(KaxCueClusterPosition::Create, KaxCueClusterPosition_TheId, "CueClusterPosition", KaxCueClusterPosition_Context);
const EbmlCallbacks KaxCueBlockNumber::ClassInfos(KaxCueBlockNumber::Create, KaxCueBlockNumber_TheId, "CueBlockNumber", KaxCueBlockNumber_Context);
#if MATROSKA_VERSION >= 2
const EbmlCallbacks KaxCueCodecState::ClassInfos(KaxCueCodecState::Create, KaxCueCodecState_TheId, "CueCodecState", KaxCueCodecState_Context);
const EbmlCallbacks KaxCueReference::ClassInfos(KaxCueReference::Create, KaxCueReference_TheId, "CueReference", KaxCueReference_Context);
const EbmlCallbacks KaxCueRefTime::ClassInfos(KaxCueRefTime::Create, KaxCueRefTime_TheId, "CueRefTime", KaxCueRefTime_Context);
const EbmlCallbacks KaxCueRefCluster::ClassInfos(KaxCueRefCluster::Create, KaxCueRefCluster_TheId, "CueRefCluster", KaxCueRefCluster_Context);
const EbmlCallbacks KaxCueRefNumber::ClassInfos(KaxCueRefNumber::Create, KaxCueRefNumber_TheId, "CueRefNumber", KaxCueRefNumber_Context);
const EbmlCallbacks KaxCueRefCodecState::ClassInfos(KaxCueRefCodecState::Create, KaxCueRefCodecState_TheId, "CueRefCodecState", KaxCueRefCodecState_Context);
#endif // MATROSKA_VERSION

KaxCuePoint::KaxCuePoint() 
 :EbmlMaster(KaxCuePoint_Context)
{}

KaxCueTrackPositions::KaxCueTrackPositions()
 :EbmlMaster(KaxCueTrackPositions_Context)
{}

#if MATROSKA_VERSION >= 2
KaxCueReference::KaxCueReference()
 :EbmlMaster(KaxCueReference_Context)
{}
#endif // MATROSKA_VERSION

/*!
	\todo handle codec state checking
	\todo remove duplicate references (reference to 2 frames that each reference the same frame)
*/
void KaxCuePoint::PositionSet(const KaxBlockGroup & BlockReference, uint64 GlobalTimecodeScale)
{
	// fill me
	KaxCueTime & NewTime = GetChild<KaxCueTime>(*this);
	*static_cast<EbmlUInteger*>(&NewTime) = BlockReference.GlobalTimecode() / GlobalTimecodeScale;

	KaxCueTrackPositions & NewPositions = AddNewChild<KaxCueTrackPositions>(*this);
	KaxCueTrack & TheTrack = GetChild<KaxCueTrack>(NewPositions);
	*static_cast<EbmlUInteger*>(&TheTrack) = BlockReference.TrackNumber();
	
	KaxCueClusterPosition & TheClustPos = GetChild<KaxCueClusterPosition>(NewPositions);
	*static_cast<EbmlUInteger*>(&TheClustPos) = BlockReference.ClusterPosition();

#if MATROSKA_VERSION >= 2
	// handle reference use
	if (BlockReference.ReferenceCount() != 0)
	{
		unsigned int i;
		for (i=0; i<BlockReference.ReferenceCount(); i++) {
			KaxCueReference & NewRefs = AddNewChild<KaxCueReference>(NewPositions);
			NewRefs.AddReference(BlockReference.Reference(i).RefBlock(), GlobalTimecodeScale);
		}
	}

	KaxCodecState *CodecState = static_cast<KaxCodecState *>(BlockReference.FindFirstElt(KaxCodecState::ClassInfos));
	if (CodecState != NULL) {
		KaxCueCodecState &CueCodecState = AddNewChild<KaxCueCodecState>(NewPositions);
		*static_cast<EbmlUInteger*>(&CueCodecState) = BlockReference.GetParentCluster()->GetParentSegment()->GetRelativePosition(CodecState->GetElementPosition());
	}
#endif // MATROSKA_VERSION

	bValueIsSet = true;
}

void KaxCuePoint::PositionSet(const KaxBlockBlob & BlobReference, uint64 GlobalTimecodeScale)
{
	const KaxInternalBlock &BlockReference = BlobReference;

	// fill me
	KaxCueTime & NewTime = GetChild<KaxCueTime>(*this);
	*static_cast<EbmlUInteger*>(&NewTime) = BlockReference.GlobalTimecode() / GlobalTimecodeScale;

	KaxCueTrackPositions & NewPositions = AddNewChild<KaxCueTrackPositions>(*this);
	KaxCueTrack & TheTrack = GetChild<KaxCueTrack>(NewPositions);
	*static_cast<EbmlUInteger*>(&TheTrack) = BlockReference.TrackNum();
	
	KaxCueClusterPosition & TheClustPos = GetChild<KaxCueClusterPosition>(NewPositions);
	*static_cast<EbmlUInteger*>(&TheClustPos) = BlockReference.ClusterPosition();

#if 0 // MATROSKA_VERSION >= 2
	// handle reference use
	if (BlockReference.ReferenceCount() != 0)
	{
		unsigned int i;
		for (i=0; i<BlockReference.ReferenceCount(); i++) {
			KaxCueReference & NewRefs = AddNewChild<KaxCueReference>(NewPositions);
			NewRefs.AddReference(BlockReference.Reference(i).RefBlock(), GlobalTimecodeScale);
		}
	}
#endif // MATROSKA_VERSION

#if MATROSKA_VERSION >= 2
	if (!BlobReference.IsSimpleBlock()) {
		const KaxBlockGroup &BlockGroup = BlobReference;
		const KaxCodecState *CodecState = static_cast<KaxCodecState *>(BlockGroup.FindFirstElt(KaxCodecState::ClassInfos));
		if (CodecState != NULL) {
			KaxCueCodecState &CueCodecState = AddNewChild<KaxCueCodecState>(NewPositions);
			*static_cast<EbmlUInteger*>(&CueCodecState) = BlockGroup.GetParentCluster()->GetParentSegment()->GetRelativePosition(CodecState->GetElementPosition());
		}
	}
#endif // MATROSKA_VERSION

	bValueIsSet = true;
}

#if MATROSKA_VERSION >= 2
/*!
	\todo handle codec state checking
*/
void KaxCueReference::AddReference(const KaxBlockBlob & BlockReference, uint64 GlobalTimecodeScale)
{
	const KaxInternalBlock & theBlock = BlockReference;
	KaxCueRefTime & NewTime = GetChild<KaxCueRefTime>(*this);
	*static_cast<EbmlUInteger*>(&NewTime) = theBlock.GlobalTimecode() / GlobalTimecodeScale;

	KaxCueRefCluster & TheClustPos = GetChild<KaxCueRefCluster>(*this);
	*static_cast<EbmlUInteger*>(&TheClustPos) = theBlock.ClusterPosition();

#ifdef OLD
	// handle recursive reference use
	if (BlockReference.ReferenceCount() != 0)
	{
		unsigned int i;
		for (i=0; i<BlockReference.ReferenceCount(); i++) {
			AddReference(BlockReference.Reference(i).RefBlock());
		}
	}
#endif /* OLD */
}
#endif

bool KaxCuePoint::operator<(const EbmlElement & EltB) const
{
	assert(EbmlId(*this) == KaxCuePoint_TheId);
	assert(EbmlId(EltB) == KaxCuePoint_TheId);

	const KaxCuePoint & theEltB = *static_cast<const KaxCuePoint *>(&EltB);

	// compare timecode
	const KaxCueTime * TimeCodeA = static_cast<const KaxCueTime *>(FindElt(KaxCueTime::ClassInfos));
	if (TimeCodeA == NULL)
		return false;

	const KaxCueTime * TimeCodeB = static_cast<const KaxCueTime *>(theEltB.FindElt(KaxCueTime::ClassInfos));
	if (TimeCodeB == NULL)
		return false;

	if (*TimeCodeA < *TimeCodeB)
		return true;

	if (*TimeCodeB < *TimeCodeA)
		return false;

	// compare tracks (timecodes are equal)
	const KaxCueTrack * TrackA = static_cast<const KaxCueTrack *>(FindElt(KaxCueTrack::ClassInfos));
	if (TrackA == NULL)
		return false;

	const KaxCueTrack * TrackB = static_cast<const KaxCueTrack *>(theEltB.FindElt(KaxCueTrack::ClassInfos));
	if (TrackB == NULL)
		return false;

	if (*TrackA < *TrackB)
		return true;

	if (*TrackB < *TrackA)
		return false;

	return false;
}

bool KaxCuePoint::Timecode(uint64 & aTimecode, uint64 GlobalTimecodeScale) const
{
	const KaxCueTime *aTime = static_cast<const KaxCueTime *>(FindFirstElt(KaxCueTime::ClassInfos));
	if (aTime == NULL)
		return false;
	aTimecode = uint64(*aTime) * GlobalTimecodeScale;
	return true;
}

/*!
	\brief return the position of the Cluster to load
*/
const KaxCueTrackPositions * KaxCuePoint::GetSeekPosition() const
{
	const KaxCueTrackPositions * result = NULL;
	uint64 aPosition = EBML_PRETTYLONGINT(0xFFFFFFFFFFFFFFF);
	// find the position of the "earlier" Cluster
	const KaxCueTrackPositions *aPoss = static_cast<const KaxCueTrackPositions *>(FindFirstElt(KaxCueTrackPositions::ClassInfos));
	while (aPoss != NULL)
	{
		const KaxCueClusterPosition *aPos = static_cast<const KaxCueClusterPosition *>(aPoss->FindFirstElt(KaxCueClusterPosition::ClassInfos));
		if (aPos != NULL && uint64(*aPos) < aPosition) {
			aPosition = uint64(*aPos);
			result = aPoss;
		}
		
		aPoss = static_cast<const KaxCueTrackPositions *>(FindNextElt(*aPoss));
	}
	return result;
}

uint64 KaxCueTrackPositions::ClusterPosition() const
{
	const KaxCueClusterPosition *aPos = static_cast<const KaxCueClusterPosition *>(FindFirstElt(KaxCueClusterPosition::ClassInfos));
	if (aPos == NULL)
		return 0;

	return uint64(*aPos);
}

uint16 KaxCueTrackPositions::TrackNumber() const
{
	const KaxCueTrack *aTrack = static_cast<const KaxCueTrack *>(FindFirstElt(KaxCueTrack::ClassInfos));
	if (aTrack == NULL)
		return 0;

	return uint16(*aTrack);
}


END_LIBMATROSKA_NAMESPACE
