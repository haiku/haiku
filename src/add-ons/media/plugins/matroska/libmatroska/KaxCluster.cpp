/****************************************************************************
** libmatroska : parse Matroska files, see http://www.matroska.org/
**
** <file/class description>
**
** Copyright (C) 2002-2004 Steve Lhomme.  All rights reserved.
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
	\version \$Id: KaxCluster.cpp 640 2004-07-09 21:05:36Z mosu $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/
#include "matroska/KaxCluster.h"
#include "matroska/KaxClusterData.h"
#include "matroska/KaxBlock.h"
#include "matroska/KaxContexts.h"
#include "matroska/KaxSegment.h"

// sub elements
START_LIBMATROSKA_NAMESPACE

EbmlSemantic KaxCluster_ContextList[4] =
{
	EbmlSemantic(true,  true,  KaxClusterTimecode::ClassInfos),
	EbmlSemantic(false, true,  KaxClusterPrevSize::ClassInfos),
	EbmlSemantic(true,  false, KaxBlockGroup::ClassInfos),
	EbmlSemantic(false, true,  KaxClusterPosition::ClassInfos),
};

const EbmlSemanticContext KaxCluster_Context = EbmlSemanticContext(countof(KaxCluster_ContextList), KaxCluster_ContextList, &KaxSegment_Context, *GetKaxGlobal_Context, &KaxCluster::ClassInfos);

EbmlId KaxCluster_TheId(0x1F43B675, 4);
const EbmlCallbacks KaxCluster::ClassInfos(KaxCluster::Create, KaxCluster_TheId, "Cluster", KaxCluster_Context);

KaxCluster::KaxCluster()
	:EbmlMaster(KaxCluster_Context)
	,currentNewBlock(NULL)
	,ParentSegment(NULL)
	,bFirstFrameInside(false)
	,bPreviousTimecodeIsSet(false)
	,bTimecodeScaleIsSet(false)
{}

KaxCluster::KaxCluster(const KaxCluster & ElementToClone) 
 :EbmlMaster(ElementToClone)
{
	// update the parent of each children
	std::vector<EbmlElement *>::const_iterator Itr = ElementList.begin();
	while (Itr != ElementList.end())
	{
		if (EbmlId(**Itr) == KaxBlockGroup::ClassInfos.GlobalId) {
			static_cast<KaxBlockGroup   *>(*Itr)->SetParent(*this);
		} else if (EbmlId(**Itr) == KaxBlock::ClassInfos.GlobalId) {
			static_cast<KaxBlock        *>(*Itr)->SetParent(*this);
#if MATROSKA_VERSION >= 2
		} else if (EbmlId(**Itr) == KaxBlockVirtual::ClassInfos.GlobalId) {
			static_cast<KaxBlockVirtual *>(*Itr)->SetParent(*this);
#endif // MATROSKA_VERSION
		}
	}
}

bool KaxCluster::AddFrameInternal(const KaxTrackEntry & track, uint64 timecode, DataBuffer & buffer, KaxBlockGroup * & MyNewBlock, const KaxBlockGroup * PastBlock, const KaxBlockGroup * ForwBlock, LacingType lacing)
{
	if (!bFirstFrameInside) {
		bFirstFrameInside = true;
		MinTimecode = MaxTimecode = timecode;
	} else {
		if (timecode < MinTimecode)
			MinTimecode = timecode;
		if (timecode > MaxTimecode)
			MaxTimecode = timecode;
	}

	MyNewBlock = NULL;

	if (lacing == LACING_NONE || !track.LacingEnabled()) {
		currentNewBlock = NULL;
	}

	// force creation of a new block
	if (currentNewBlock == NULL || uint32(track.TrackNumber()) != uint32(currentNewBlock->TrackNumber()) || PastBlock != NULL || ForwBlock != NULL) {
		KaxBlockGroup & aNewBlock = GetNewBlock();
		MyNewBlock = currentNewBlock = &aNewBlock;
		currentNewBlock = &aNewBlock;
	}

	if (PastBlock != NULL) {
		if (ForwBlock != NULL) {
			if (currentNewBlock->AddFrame(track, timecode, buffer, *PastBlock, *ForwBlock, lacing)) {
				// more data are allowed in this Block
				return true;
			} else {
				currentNewBlock = NULL;
				return false;
			}
		} else {
			if (currentNewBlock->AddFrame(track, timecode, buffer, *PastBlock, lacing)) {
				// more data are allowed in this Block
				return true;
			} else {
				currentNewBlock = NULL;
				return false;
			}
		}
	} else {
		if (currentNewBlock->AddFrame(track, timecode, buffer, lacing)) {
			// more data are allowed in this Block
			return true;
		} else {
			currentNewBlock = NULL;
			return false;
		}
	}
}

bool KaxCluster::AddFrame(const KaxTrackEntry & track, uint64 timecode, DataBuffer & buffer, KaxBlockGroup * & MyNewBlock, LacingType lacing)
{
	return AddFrameInternal(track, timecode, buffer, MyNewBlock, NULL, NULL, lacing);
}

bool KaxCluster::AddFrame(const KaxTrackEntry & track, uint64 timecode, DataBuffer & buffer, KaxBlockGroup * & MyNewBlock, const KaxBlockGroup & PastBlock, LacingType lacing)
{
	return AddFrameInternal(track, timecode, buffer, MyNewBlock, &PastBlock, NULL, lacing);
}

bool KaxCluster::AddFrame(const KaxTrackEntry & track, uint64 timecode, DataBuffer & buffer, KaxBlockGroup * & MyNewBlock, const KaxBlockGroup & PastBlock, const KaxBlockGroup & ForwBlock, LacingType lacing)
{
	return AddFrameInternal(track, timecode, buffer, MyNewBlock, &PastBlock, &ForwBlock, lacing);
}

/*!
	\todo only put the Blocks written in the cue entries
*/
uint32 KaxCluster::Render(IOCallback & output, KaxCues & CueToUpdate, bool bSaveDefault)
{
	// update the Timecode of the Cluster before writing
	KaxClusterTimecode * Timecode = static_cast<KaxClusterTimecode *>(this->FindElt(KaxClusterTimecode::ClassInfos));
	*static_cast<EbmlUInteger *>(Timecode) = GlobalTimecode() / GlobalTimecodeScale();

	uint32 Result = EbmlMaster::Render(output, bSaveDefault);
	// For all Blocks add their position on the CueEntry
	size_t Index;
	
	for (Index = 0; Index < ElementList.size(); Index++) {
		if (EbmlId(*ElementList[Index]) == KaxBlockGroup::ClassInfos.GlobalId) {
			CueToUpdate.PositionSet(*static_cast<const KaxBlockGroup *>(ElementList[Index]));
		}
	}
	return Result;
}

/*!
	\todo automatically choose valid timecode for the Cluster based on the previous cluster timecode (must be incremental)
*/
uint64 KaxCluster::GlobalTimecode() const
{
	assert(bPreviousTimecodeIsSet);
	uint64 result = MinTimecode;

	if (result < PreviousTimecode)
		result = PreviousTimecode + 1;
	
	return result;
}

/*!
	\brief retrieve the relative 
	\todo !!! We need a way to know the TimecodeScale
*/
int16 KaxCluster::GetBlockLocalTimecode(uint64 aGlobalTimecode) const
{
	int64 TimecodeDelay = (int64(aGlobalTimecode) - int64(GlobalTimecode())) / int64(GlobalTimecodeScale());
	assert(TimecodeDelay >= int16(0x8000) && TimecodeDelay <= int16(0x7FFF));
	return int16(TimecodeDelay);
}

uint64 KaxCluster::GetBlockGlobalTimecode(int16 GlobalSavedTimecode)
{
	if (!bFirstFrameInside) {
		KaxClusterTimecode * Timecode = static_cast<KaxClusterTimecode *>(this->FindElt(KaxClusterTimecode::ClassInfos));
		assert (bFirstFrameInside); // use the InitTimecode() hack for now
		MinTimecode = MaxTimecode = PreviousTimecode = *static_cast<EbmlUInteger *>(Timecode);
		bFirstFrameInside = true;
		bPreviousTimecodeIsSet = true;
	}
	return int64(GlobalSavedTimecode * GlobalTimecodeScale()) + GlobalTimecode();
}

KaxBlockGroup & KaxCluster::GetNewBlock()
{
	KaxBlockGroup & MyBlock = AddNewChild<KaxBlockGroup>(*this);
	MyBlock.SetParent(*this);
	return MyBlock;
}

void KaxCluster::ReleaseFrames()
{
	size_t Index;
	
	for (Index = 0; Index < ElementList.size(); Index++) {
		if (EbmlId(*ElementList[Index]) == KaxBlockGroup::ClassInfos.GlobalId) {
			static_cast<KaxBlockGroup*>(ElementList[Index])->ReleaseFrames();
		}
	}
}

uint64 KaxCluster::GetPosition() const
{
	assert(ParentSegment != NULL);
	return ParentSegment->GetRelativePosition(*this);
}

END_LIBMATROSKA_NAMESPACE
